#include "hpil.h"

#include "message.c"
#include "send.h"

void ilInit() {
  DDRB |= ILOP | ILON; // output pins
  PCMSK = ILIP | ILIN;
  DIDR0 = ~PCMSK; // dWire bit can't be masked if in use!
}

#define PULSE_CYCLES (MF_CPU / 1000000)  // 1 us
#define CALL_CYCLES 3
#define RET_CYCLES  5

void toggle() {
  __builtin_avr_delay_cycles(PULSE_CYCLES - CALL_CYCLES - RET_CYCLES - 1);
  PINB = ILOP | ILON;
}

void idle() {
  __builtin_avr_delay_cycles(PULSE_CYCLES - CALL_CYCLES - 1);
  PORTB &= ~(ILOP | ILON);   // PORTB.OUTCLR = ILOP | ILON
  __builtin_avr_delay_cycles(2 * PULSE_CYCLES - 2 * RET_CYCLES - 1);
}

void dataBit() {
  toggle();
  idle();
}

void one() {
  PINB = ILOP;
  dataBit();
}

void zero() {
  PINB = ILON;
  dataBit();
}

void sync() {
  toggle();
  toggle();
  toggle();
  idle();
}

void oneSync() {
  PINB = ILOP;
  sync();
}

void zeroSync() {
  PINB = ILON;
  sync();
}

void sendFrame(Frame cmd, uint8_t param = 0) {
  // if (param & ~cmd.paramMask) while(1);  // error check

  // GIFR = _BV(PCIF); // clear receive pin change flag

  uint16_t sendBits = (uint16_t)cmd.frameControl << 8 | cmd.frameData | (param & cmd.paramMask);
	cmd.frameControl & CMD ? oneSync() : zeroSync();
  for (uint8_t i = 2 + 8; i--; ) {
		sendBits & 0x200 ? one() : zero();  // MSB first
    sendBits <<= 1;
    // if (GIFR) break;  // early reply to RDY class -- start recvFrame ASAP ??
  }
}


// ILIN ILIP
//  P1   P0
//   1    1  Hi
//   1    0  Never
//   0    1  Idle
//   0    0  Lo

typedef enum {Lo, Idle, Never, Hi} State;

inline State state() {
  return (State)(PINB & (ILIP | ILIN));  // DIDR0 doesn't mask dWire mode PB5!!
}

XferFrame recvFrame() {
  static union {
    XferFrame recvdFrame;
    uint16_t data;
  };

  uint8_t bits = 3 + 8;
  data = 0xFFC0; // indicate error in case times out
  uint8_t dataBit;
  uint32_t frameTimeout = MF_CPU / 16 / 2; // ~500ms reading wait  TODO adjust
  while ((dataBit = state()) == Idle && --frameTimeout);

  while (1) {
    while (state() != Idle || state() != Idle);  // filter transitions thru Idle -- can get stuck here with no/bad connection  TODO
    // Idle -- end of bit
    data |= dataBit & 1;
    if (!--bits)
      return recvdFrame;

    data <<= 1;
    uint8_t bitTimeout = 0;
    while ((dataBit = state()) == Idle && ++bitTimeout);  // first Lo/Hi after Idle is dataBit
  }
}

bool waitForReceiveActive() { // for early overlapped responses to RDY frameControl bits only
  uint16_t rfcTimeout = 1;
  while (!GIFR && ++rfcTimeout); // wait for any activity, including previous
  if (!rfcTimeout) {
    send("Rcv timeout\n");
    return false;
  }
  return true;
}

void waitForIdle() {
  while (1) { // wait for no activity for ~ a bit time
    GIFR = _BV(PCIF); // clear receive pin change flag
    uint8_t bitTimeout = MF_CPU / 1000000 * 2 / 4; // 2us, > 4 cycles
    while (!GIFR && --bitTimeout);
    if (!bitTimeout)
      return;
  }
}

void ilCmd(Frame cmd, uint8_t param) {
  if (cmd.frameControl == CMD
  || (cmd.frameControl == RDY && param == SDA.frameData)) {
    // ? other cases needing RFC first? TODO
    GIFR = _BV(PCIF); // clear receive pin change flag
    sendFrame(RFC); // check if device is Ready For Command
    // response may be overlapped with sent frame!

    if (waitForReceiveActive()) { // in case noise coupled from transmit to receive --> waitForResponse AFTER frame sent
      GIFR = _BV(PCIF); // clear receive pin change flag
      waitForReceiveActive();
    }
  }

  sendFrame(cmd, param);

  switch (cmd.frameControl) {
    case RDY : // early, overlapped response based on just the 3 frameControl bits
      if (cmd.frameData != SDA.frameData)  // SDA replaced by response data -- may overlap sent SDA frame!!  TODO
        waitForReceiveActive();
      return;
    default : break;
  }

  XferFrame echo = recvFrame();  // echo often long delayed processing before echo

#if 0 // check looped echo
  // LAD 4 20 times out
  if (cmd.frameControl == ENDcc) return; // ??

  if (echo.frameControl & (SRQ | EOI)) return; // TODO: handle SRQ, EOI

  if (echo.frameControl != cmd.frameControl
  ||  echo.frameData != (cmd.frameData | (param & cmd.paramMask))) {
    send("Echo!:\n"); // AAD increments
    send(' '); sendHex((uint8_t)cmd.frameControl); sendHex(cmd.frameData); send('\n');
    send(' '); sendHex((uint8_t)echo.frameControl); sendHex(echo.frameData);
    send('\n');
  }
#endif
}

void ilSendStr(const char* str, uint8_t addr) {
  ilCmd(UNL);
  ilCmd(LAD, addr); // echo times out if already addressed?
  ilCmd(NOP);

  do ilCmd(DAB, *str); while (*++str); // synch via echo
  ilCmd(END, '\n'); // echoed with ETO
}


char* ilGetData(uint8_t addr) {
  static char dataBuf[MAX_RESPONSE_LEN + 1]; // to hold at least 14 character reading + CRLF + NUL
  XferFrame recvdFrame;
  uint16_t dataBufIdx;
  do {
    dataBufIdx = 0;
    ilCmd(TAD, addr);
    ilCmd(SDA);  // replaced with data
    do {
      recvdFrame = recvFrame();
      dataBuf[dataBufIdx++] = recvdFrame.frameData;
      switch (recvdFrame.frameControl) {
        case DABcc :
        case DAB_SRQ :
          sendFrame(DAB, recvdFrame.frameData); break; // echoed data requests next byte

        case RDY : --dataBufIdx; // response to RFC
        case IDY_SRQ :
          break; // asynch SRQ ? only after EAR ?? --> retry ? TODO

        case ENDcc :
        case END_SRQ :
          sendFrame(ETO);
        // TODO : handle other cases
        default :
          if (dataBufIdx < 12) { // for readings, less initial character
            send("Short!: "); send(dataBufIdx); send((uint8_t)recvdFrame.frameControl); send('\n');
            // normally Fc DAB_EOI (2)
          }
          dataBuf[dataBufIdx] = 0;
          return dataBuf;
      }
    } while (dataBufIdx < MAX_RESPONSE_LEN && recvdFrame.frameControl != IDY_SRQ);
  } while (recvdFrame.frameControl == IDY_SRQ); // ?
  
  dataBuf[dataBufIdx] = 0;
  return dataBuf;
}

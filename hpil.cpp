#include "hpil.h"

#include "message.c"

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

  uint16_t sendBits = (uint16_t)cmd.frameControl << 8 | cmd.frameData | (param & cmd.paramMask);
	cmd.frameControl & CMD ? oneSync() : zeroSync();
  for (uint8_t i = 2 + 8; i--; ) {
		sendBits & 0x200 ? one() : zero();  // MSB first
    sendBits <<= 1;
  }
}


//ILIN ILIP
// P1   P0
//  1    1  Hi
//  1    0  Never
//  0    1  Idle
//  0    0  Lo

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
    while (state() != Idle || state() != Idle);  // filter transitions thru Idle
    // Idle -- end of bit
    data |= dataBit & 1;
    if (!--bits)
      return recvdFrame;

    data <<= 1;
    uint8_t bitTimeout = 0;
    while ((dataBit = state()) == Idle && ++bitTimeout);  // first Lo/Hi is dataBit
  }
}

void waitForResponse() { // for early overlapped response to RDY frameControl bits only
  GIFR = _BV(PCIF); // clear pin change flag
  uint16_t rfcTimeout = 1;
  while (!GIFR && ++rfcTimeout);

  if (rfcTimeout)
    while (1) { // wait for idle
      GIFR = _BV(PCIF); // clear pin change flag
      uint8_t bitTimeout = MF_CPU / 1000000 * 2 / 4; // 2us, > 4 cycles
      while (!GIFR && --bitTimeout);
      if (!bitTimeout)
        return;
    }
}

void ilCmd(Frame cmd, uint8_t param) {
  sendFrame(cmd, param);

  switch (cmd.frameControl) {
    case RDY :  // early, overlapped response
      if (cmd.frameData != SDA.frameData)
         waitForResponse(); // ??
      return;
    default : break;
  }

  recvFrame();  // often long delayed processing before echo
  // TODO: handle EOI, SRQ Bits

  sendFrame(RFC);
  waitForResponse();
}

void ilSendStr(const char* str, uint8_t addr) {
  ilCmd(LAD, addr);
  do ilCmd(DAB, *str); while (*++str);
  ilCmd(END, '\n');
}

char dataBuf[32];

char* ilGetData() {
  uint8_t dataBufIdx = 0;
  ilCmd(SDA);  // replaced with data
  do {
    XferFrame recvdFrame = recvFrame();
    switch (recvdFrame.frameControl) {
      case DABcc :
        dataBuf[dataBufIdx++] = recvdFrame.frameData;
        sendFrame(DAB, recvdFrame.frameData); break; // echoed data requests next byte
      case ENDcc : return dataBuf;
      default : break;
    }
  } while (dataBufIdx < sizeof(dataBuf));
  return dataBuf;
}

// HP_IL

// Device Output:
// 100 Ohm matching / 1.5V attenuation network for 5V 23/27 Ohm ATtiny85 outputs:

//  HP-IL In ------------------v- 59 Ohm -- ILOP
//                             |
//                          240 Ohm                ATtiny
//                             |
//  HP-IL In ------------------^- 59 Ohm -- ILON
// (Ref: left)

// Device Input:
//                       ILP  ILN  (~90 Ohms DC)
//                        |     |
//  Vcc -- 100 Ohms -->|---     ---- Gnd
//                  |     |
//                 P0    P1
// (Ref: right)

// ATtiny thresholds typ. 2.35V +/- 100 mV hysteresis

#include <avr/io.h>
#include <avr/interrupt.h>
#include "send.h"  // Add library: ..\..\Common\Common\Debug\libCommon.a
#include "config.h"
#include "message.c"


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

void sendFrame(Frame cmd, uint8_t param = 0){
  // if (param & ~cmd.paramBits) while(1);  // error check

  uint8_t data = cmd.frameData | (param & cmd.paramBits);  // mask
	cmd.frameControl & CMD ? oneSync() : zeroSync();
	cmd.frameControl & ENDcc ? one() : zero();
  cmd.frameControl & SRQ ? one() : zero();
  for (int8_t i = 8; i--; ) {
		data & 0x80 ? one() : zero();
    data <<= 1;
  }
}

uint8_t dataBuf[256];
uint8_t dataBufIdx;

// P1  P0
//  1   1  Hi
//  0   1  Idle
//  0   0  Lo

inline uint8_t state() {
  return PINB & (ILIH | ILIL);  // DIDR0 won't mask dWire PB5!!
}

const uint8_t Idle = 1;

uint8_t lastCmd, lastParam;

// Alternative: sample mid bit ?

void recvFrame() {
  uint8_t bits = 3 + 8;
  uint8_t data;

  uint16_t frameTimeout = 1;
  while (state() == Idle && ++frameTimeout);
  if (!frameTimeout) return;

#if 0  // TEST sampling
   uint8_t* pDataBuf = dataBuf;
   uint8_t i = 0;
   while (++i)
     *pDataBuf++ = PINB;
   return;
#endif

  while (1) {
    uint8_t dataBit;
    while (state() != Idle || state() != Idle);
    data |= dataBit & 1;
    if (!--bits) {
      dataBuf[dataBufIdx++] = data;
      return;
    }
    data <<= 1;
    uint8_t bitTimeout = 0;
    while ((dataBit = state()) == Idle && ++bitTimeout);  // 2us
  }
}

void waitForResponse() {
  // often gets an early overlapped response to RDY frameControl
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

void cmd(Frame cmd, uint8_t param = 0) {
  lastCmd = cmd.frameData;
  lastParam = param;
  sendFrame(cmd, param);

  if (cmd.frameData == SDA.frameData) return;

  if (cmd.frameControl == RDY) {  // early, overlapped response
    waitForResponse();
    return;
  }

  recvFrame();  // often long delayed processing before echo

  if (cmd.frameControl == DABcc) return; // echoed data syncs

  sendFrame(RFC);
  waitForResponse();

}

void sendStr(const char* str) {
  do cmd(DAB, *str); while (*++str);
  cmd(END, '\n');
}


int main(void) {
  DDRB = ILOP | ILON; // output pins
  PCMSK = ILIH | ILIL;
  DIDR0 = ~PCMSK;

  cmd(REN);
  cmd(DCL);
  cmd(AAD, 1);
  cmd(LAD, 1);

#if 0
  sendStr("F1T1");

  cmd(TAD, 1);
  cmd(SDA);
  while (1) {
    recvFrame();
    sendFrame(DAB, data);

    // handle ETO --> new SDA
  }
#endif

  // 3468
  while (1)
    sendStr("D2" __TIME__); // no lower case

  // sendStr("D1F4");

#if 0
  // "B2"  binary cal constant out
  // "Wn" - read SRAM byte (1024 nibbles)
  cmd(DAB, 'W');
  cmd(END, 1);
  cmd(TAD, 1);
  cmd(SDA);
#endif

  // listen

  cmd(GTL);

}


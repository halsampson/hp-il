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

  uint16_t sendBits = (uint16_t)cmd.frameControl << 8 | cmd.frameData | (param & cmd.paramBits);  // mask
	cmd.frameControl & CMD ? oneSync() : zeroSync();
  for (uint8_t i = 2 + 8; i--; ) {
		sendBits & 0x200 ? one() : zero();  // MSB first
    sendBits <<= 1;
  }
}

XferFrame dataBuf[128];
uint8_t dataBufIdx;

// P1  P0
//  1   1  Hi
//  1   0  Never
//  0   1  Idle
//  0   0  Lo

typedef enum {Lo, Idle, Never, Hi} State;

inline State state() {
  return (State)(PINB & (ILIH | ILIL));  // DIDR0 doesn't mask dWire mode PB5!!
}

static union {
  XferFrame recvdFrame;
  uint16_t data;
};


void recvFrame() {
  uint8_t bits = 3 + 8;
  data = 0;
  uint8_t dataBit;
  uint32_t frameTimeout = MF_CPU / 10;
  while ((dataBit = state()) == Idle && --frameTimeout);

  while (1) {
    while (state() != Idle || state() != Idle);  // filter transitions thru Idle
    // Idle -- end of bit
    data |= dataBit & 1;
    if (!--bits) {
      dataBuf[dataBufIdx++ & 0x3F] = recvdFrame;
      return;
    }
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

void cmd(Frame cmd, uint8_t param = 0) {
  sendFrame(cmd, param);

  switch (cmd.frameControl) {
    case RDY :  // early, overlapped response
      if (cmd.frameData != SDA.frameData)
         waitForResponse(); // ??
      return;
    default : break;
  }

  recvFrame();  // often long delayed processing before echo
  // TODO chk EOI, SRQ Bits

  sendFrame(RFC);
  waitForResponse();
}

void sendStr(const char* str) {
  do cmd(DAB, *str); while (*++str);
  cmd(END, '\n');
}

void getData() {
  cmd(SDA);
  while (1) {
    recvFrame();
    switch (recvdFrame.frameControl) {
      case DABcc : sendFrame(DAB, recvdFrame.frameData); break; // echoed data requests next byte
      case ENDcc : return;
      default : break;
    }
  }
}

void getReadings() {
  sendStr("F1T1");

  cmd(TAD, 1);
  dataBufIdx = 0;
  do {
    getData();
  } while (1 || dataBufIdx);
}

void displayVersion() {
 sendStr("D2" __TIME__); // no lower case
 __builtin_avr_delay_cycles(MF_CPU);
 sendStr("D1");
}

void dumpCalibrationSRAM() {
  // "B2"  binary cal constant out (at each calibration step)  -- see bottom of unit
  // "Wn" - read SRAM byte (1024 nibbles?) ??

  dataBufIdx = 0;
  for (uint16_t sramAddr = 0; sramAddr <= 127; sramAddr++) {  // TODO: 255
    cmd(LAD, 1);
    cmd(DAB, 'W');  // ???
    cmd(DAB, sramAddr);
    cmd(END, '\n');

    cmd(TAD, 1);
    getData();
    //dataBufIdx -= 3;
  }
}

int main(void) {
  DDRB = ILOP | ILON; // output pins
  PCMSK = ILIH | ILIL;
  DIDR0 = ~PCMSK;

  cmd(REN);
  cmd(DCL);
  cmd(AAD, 1);
  cmd(LAD, 1);

  displayVersion();

#if 1
  getReadings();
#elif 1
  dumpCalibrationSRAM();
#elif 1
  while (1)
    displayVersion();
#endif

  cmd(GTL);
}

// TODO: add serial connection

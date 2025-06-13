// HP_IL

// 100 Ohm matching / 1.5V attenuation network for 5V 23/27 Ohm ATtiny85 outputs:

//  HP-IL In ------------------v- 59 Ohm -- ILOP
//                             |
//                          240 Ohm                ATtiny
//                             |
//  HP-IL In ------------------^- 59 Ohm -- ILON
// (Ref: left)

// Output is ~3.5 Vp-p, no load;
// ILIP --> AIN1::Vbg (1.1V) and PINB (2.5V)
// Read ACO and PINB
//       1       1     Hi
//       1       0     Idle
//       0       0     Lo
// other end to (1.1 + 2.5) / 2 = 1.8V
// 100 Ohm termination OK?

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

void cmd(Frame cmd, uint8_t param = 0) {
  sendFrame(cmd, param);

  sendFrame(RFC);
  // wait for ready response
  for (uint8_t bits = 8; bits--;) {  // past sync
    if (PINB & ILIP) // Hi
      while (PINB & ILIP);
    else if (!(ACSR & _BV(ACO))) // Lo  -- ? how fast is comparator -- no spec!!
      while (!(ACSR & _BV(ACO)));
    else ++bits; // idle - timeout
  }
}

void sendStr(const char* str) {
  do cmd(DAB, *str); while (*++str);
  cmd(END, '\n');
}


void done() {
  DDRB |= LED;
  while (1) {
    PINB = LED;
    __builtin_avr_delay_cycles(MF_CPU / 4);
  }
}

int main(void) {
  DDRB = ILOP | ILON;
  PORTB |= ILIN;  // ~32K pullup for 1.8V; external pull down: 32K * (5 - 1.8)/1.8 ~ 56K

  ACSR = ACBG | ACI;

  cmd(REN);
  cmd(AAD, 1);
  cmd(LAD, 1);
  cmd(DCL);

  // 3468
  while (1)
    sendStr("D2" __TIME__); // no lower case

  done();

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

  done();

}


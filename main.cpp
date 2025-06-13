// HP_IL

// 100 Ohm matching / 1.5V attenuation network for 5V 23/27 Ohm ATtiny85 outputs:

//  HP-IL In ------------------v- 59 Ohm -- ILOP
//                             |
//                          240 Ohm                ATtiny
//                             |
//  HP-IL In ------------------^- 59 Ohm -- ILON
// (Ref: left)


//

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
  __builtin_avr_delay_cycles(2 * PULSE_CYCLES - RET_CYCLES - 1);
}

void data() {
  toggle();
  idle();
}

void one() {
  PINB = ILOP;
  data();
}

void zero() {
  PINB = ILON;
  data();
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
  for (int8_t i = 7; i >= 0; i--)
		data & (1 << i) ? one() : zero();
}

void cmd(Frame cmd, uint8_t param = 0) {
  sendFrame(cmd, param);
#if 1
  __builtin_avr_delay_cycles(MF_CPU / 50);
#else
   sendFrame(RFC);
  // TODO: wait for looped response  *****
#endif
}

void sendStr(const char* str) {
  do cmd(DAB, *str); while (*++str);
  cmd(EOT);

  // ? END last character
}

int main(void) {
  DDRB = LED | ILOP | ILON;

  cmd(REN);
  cmd(AAD, 1);
  cmd(LAD, 1);

  // 3468
  sendStr("D2" "TEST " __TIME__); // no lower case
  // how terminate command??

  // "Wn" - read SRAM byte (1024 nibbles)

  while (1) {
    PINB = LED;
    __builtin_avr_delay_cycles(MF_CPU / 4);
  }


  return 0;

  cmd(DDT);
  cmd(DAB, 'W'); // read SRAM
  cmd(END, 0);

  cmd(SDA);

  return 0;

}


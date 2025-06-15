#pragma once

#define MF_CPU     16000000

#define ILIP       _BV(PB0) // diode anode    (AIN0)
#define ILIN       _BV(PB1) // diode cathode  (AIN1)
// #define LED        _BV(PB1) // LED, 1K Ohm to Gnd   *** remove 1K Ohm R5

#define TxDbit     2
// #define RxDbit     2
#define SerialIO   _BV(PB2) // ADC1

#define ILOP      _BV(PB3) // 3.6V Zener to Gnd  1.5K Ohm to Vcc
#define ILON      _BV(PB4) // ADC2 3.6V Zener to Gnd

#define dWire     _BV(PB5) // ADC0 < 2V resets
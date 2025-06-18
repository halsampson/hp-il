#pragma once

#define MF_CPU     16000000

#define ILIP       _BV(PB0) // diode anode and 100 Ohm to Vcc
#define ILIN       _BV(PB1) // diode cathode and HP-IL Out Signal Plus
//                             HP-IL Out Ref to Gnd
// #define LED        _BV(PB1) // LED, 1K Ohm to Gnd

#define TxDbit     2
// #define RxDbit     2
#define SerialIO   _BV(PB2) // ADC1

#define ILOP      _BV(PB3) // 3.6V Zener to Gnd  1.5K Ohm to Vcc
#define ILON      _BV(PB4) // ADC2 3.6V Zener to Gnd

#define dWire     _BV(PB5) // ADC0 < 2V resets
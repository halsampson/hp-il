// Super simple HP_IL interface

// Using DigiSpark ATtiny 85, but easily ported to other MCUs

/*
100 Ohm matching / 1.5V attenuation network for 5V 23/27 Ohm ATtiny85 outputs:

HP-IL In <-----------------v-- 59 Ohm ----< ILOP
                           |
                        240 Ohm
                           |
HP-IL In <-----------------^-- 59 Ohm ----< ILON
 (Ref: left)

                             /------------< Vcc
                             |
                          100 Ohm
                             |
                             >------------> ILIP
                             |
                    Si diode v
                            ___
                             |
HP-IL Out >----------------- ^------------> ILIN


HP-IL Out >------------------------------<> Gnd
  (Ref: right)

ATtiny thresholds typ. 2.35V +/- 100 mV hysteresis @ Vcc = 5V
*/

#pragma once

#include <avr/io.h>
#include "config.h"

enum ControlCode {DABcc, DAB_SRQ, ENDcc, END_SRQ,  CMD, RDY, IDYcc, IDY_SRQ};

#define SRQ 1
#define EOI 2

typedef struct { // in big endian transfer order: 3 frame control bits first
  uint8_t frameData;
  ControlCode frameControl;
} XferFrame;

typedef struct {
  ControlCode frameControl;
  uint8_t frameData;
  uint8_t paramMask;
} Frame;

#include "message.c"

#define MAX_RESPONSE_LEN 256 // for "B2" calibration data

void ilInit();
void ilCmd(Frame cmd, uint8_t param = 0);
void ilSendStr(const char* str, uint8_t addr = 1);
char* ilGetData(uint8_t addr = 1);

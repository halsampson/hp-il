// Bit-banging Serial port I/O for ATtiny85, ...

#pragma once
#include <avr/io.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#define BAUD_RATE 2000000

#include "config.h"

// PORTB pins:
#define TxD     _BV(TxDbit)

#define SAVED_OSCCAL_ADDR 0 // 0..511 in EEPROM
void sendInit();

const bool txdInvert = false;

void send(char ch);
void send(const char* s);

#ifndef __cplusplus // __flash support only in C, but overloading only CPP
  #define msgLINE   msg##__LINE__
  #define sendc(msg) __flash const char msgLINE[] = "msg"; send(msgLINE);
#endif

extern bool punctuation;

void sendDec(int16_t n);
void sendHex(uint16_t i);
void sendHex(uint32_t i);
void send(int32_t i);
void send(int i);
void send(uint16_t i);
void send(uint32_t i);

#ifdef RxDbit
  char getCh();  // sleeps
  extern volatile char receivedCh;
  void enableRx();
#endif




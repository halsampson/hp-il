#include "send.h"

inline void txStop() {  // init only
  if (txdInvert)
    PORTB &= ~TxD; // stop bit
  else
    PORTB |= TxD; // stop bit
  DDRB |= TxD;
}

void sendInit() {
#if MF_CPU != 16000000  
  uint8_t savedOSCCAL = eeprom_read_byte(SAVED_OSCCAL_ADDR);
  if (savedOSCCAL != 0xFF) {
    OSCCAL = savedOSCCAL;
    __builtin_avr_delay_cycles(MF_CPU / 1000);  // oscillator settle?
  }
#endif  
  txStop();
}

const int BAUD_CYCLES = ((MF_CPU + BAUD_RATE / 2)/ BAUD_RATE);
bool punctuation;

void send(char ch) {
#ifdef RxDbit
  if (RxDbit == TxDbit) {
    GIMSK &= ~_BV(PCIE);  // disable Rx
    DDRB |= TxD;
  }
#endif

  switch (ch) {
    case ' ' :
    case '\r' :
    case '\n' :
      punctuation = true;
  }

  int16_t out = (uint16_t)ch << 2 | 0x401; // stop  data  start  prev stop
  out = out ^ (out >> 1); // calculate toggles from previous bits
  out <<= TxDbit; // align with TxD pin
  char bits = 1 + 8 + 1;  // start data stop
  do {
    PINB = out & TxD; // toggle if (out & TxD)
    out >>= 1;
    __builtin_avr_delay_cycles(BAUD_CYCLES - 8);  // check loop time optimized
  } while (--bits);

#ifdef RxDbit
  if (RxDbit == TxDbit) {
    DDRB &= ~TxD;  // just pullup
  }
#endif
}

void send(const char* s) {
  if (!(PORTB & TxD)) // TODO: better test of need for OSCCAL setting
    sendInit();
  if (!punctuation)
    send(' '); // no separator: provide space
  punctuation = false;
  while (*s) send(*s++);
}

void sendHex(uint16_t i) {
  char intStr[4+1]; // max 4 digits + trailing null
  char* p = intStr + sizeof(intStr) - 1; // at end
  *p = 0;
  do {
    uint8_t n = i & 0xF;
    *--p = n <= 9 ? n + '0' : n + 'A' - 10;
    i >>= 4;
  } while (i);
  send(p);
}

void sendHex(uint8_t i) {
  sendHex((uint16_t)i);
}

void sendDigit(uint8_t digit) {
  send((char)(digit + '0'));
}

void sendDec(int16_t n) { // 208 bytes
  if (n < 0) {
    n = -n;
    send('-');
  }

  // https://homepage.cs.uiowa.edu/~dwjones/bcd/decimal.html
  uint8_t d1 = (n >> 4) & 0xF;
  uint8_t d2 = (n >> 8) & 0xF;
  uint8_t d3 = (n >> 12) & 0xF;
  uint8_t d0 = 6 * (d3 + d2 + d1) + (n & 0xF);
  uint8_t q = (d0 * (uint16_t)205) >> 11;  // q = d0 / 10 = d0 * 205 / 2048
  d0 -= 10 * q;
  d1 = q + 9 * d3 + 5 * d2 + d1;
  if (d1) {
    q = (d1 * (uint16_t)205) >> 11;
    d1 -= 10 * q;
    d2 = q + 2 * d2;
    if (d2 || d3) {
      q = (d2 * 26) >> 8; // q = d2 / 10 = d2 * 26 / 256
      d2 -= 10 * q;
      d3 = q + 4 * d3;
      if (d3) {
        uint8_t d4 = (d3 * 26) >> 8;
        d3 -= 10 * d4;
        if (d4)
          sendDigit(d4);
        sendDigit(d3);
      }
      sendDigit(d2);
    }
    sendDigit(d1);
  }
  sendDigit(d0);
}


void sendHex(uint32_t i) {
  char buf[9];
  send(ltoa(i, buf, 16));
}

void send(int32_t i) {
  char buf[12];
  send(ltoa(i, buf, 10));
}

void send(int i) {
  char buf[7];
  send(itoa(i, buf, 10));
}

void send(uint16_t i) {
  char buf[6];
  send(utoa(i, buf, 10));
}

void send(uint32_t i) {
  char buf[11];
  send(ultoa(i, buf, 10));
}

void send(uint8_t i) {
  sendDec(i);
}

#ifdef RxDbit

// receive
volatile char receivedCh;

ISR(PCINT0_vect, ISR_NAKED) { // PCINT synch 2 clocks;  4 clocks push?; 2 clock int vector jump = 6 or 8 cycles?
  union {
    struct {uint8_t recvd, accum;};
    uint16_t all; // non-zero indicates char received
  } serialIn;
  // PCINT delayed 3 cycles; 7 cycle interupt latency
#if BAUD_CYCLES >= 12
  __builtin_avr_delay_cycles(BAUD_CYCLES * 3 / 2 -3 -7 -8); // sample middle of LSB after start bit
#else
  __builtin_avr_delay_cycles(2);  // ??
#endif

  uint8_t bits = 8;
  do { // LSB first
    serialIn.accum |= PINB & _BV(RxDbit);
    serialIn.all >>= 1;
#if BAUD_CYCLES > 8  // extra MOV in -Og (9 cycles)
    __builtin_avr_delay_cycles(BAUD_CYCLES - 8);  // check loop time 8 with -O2
#endif
  } while (--bits);
  serialIn.all >>= RxDbit;
  receivedCh = serialIn.recvd;
  GIMSK &= ~_BV(PCIE); // avoid another interrupt from stop bit edge
  reti(); // if ISR_NAKED

  // TODO: why does LS nibble get extra 1 bits??
}

void enableRx() {
  PCMSK = _BV(RxDbit);
  GIFR |= _BV(PCIF);
  GIMSK |= _BV(PCIE);
}

char getCh() {
  enableRx();
  asm("SLEEP");
  return receivedCh;
}

#endif


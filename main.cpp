// Tests of HP-IL interface commands

#include "hpil.h"
#include "send.h"
#include "stdio.h"

char* getReading() {
  return ilGetData();
}

char timeStr[] = "D2 " __TIME__;

void incrSeconds() {
  char* pTime = timeStr + 3 + 8 - 1;
  while (1) {
    if (++*pTime <= '9') break;
    *pTime-- -= 10;
    if (++*pTime <= '5') break;
    *pTime-- -= 6;
    if (*pTime <= '0') break; // space, ... off left end
    --pTime;  // over colon
  } // to 59:59:59 hours
}

void dumpCalibrationSRAM() {
  send("Calibration data:\n");

  ilSendStr("B2"); // binary Calibration constants out -- see bottom of HP 3468
  const char* calData = ilGetData();

  // string for "B3" restore
  for (uint16_t p = 0; p < MAX_RESPONSE_LEN; ++p) {
    if (!calData[p]) break; // end of string
    send(calData[p]);
    if (p % 16 == 15) send('\n');
  }
  send('\n');

  // slightly more readable: offset, gain?
  for (uint16_t p = 0; p < MAX_RESPONSE_LEN; ++p) {
    if (!calData[p]) break; // end of string
    punctuation = true;
    sendHex((uint16_t)(calData[p] - 64)); // nibbles
    if (p % 16 == 6) send(' ');
    if (p % 16 == 15) send('\n');
  }
  send('\n');

/*
3468B Calibration data:
 @@@@@@@@@@@@@@@@
 IIIIIHFOADKBNJLD
 IIIIIIHOAE@NOBEH
 @@@@@@EO@OOKONOH
 @@@@@@@O@@C@OCOO
 @@@AGBFOADLOK@M@
 @@@@EID@ACAACIEC
 @@@@@EG@ADD@JIDG
 @@@@@@F@ACNLOIMG
 @@@@@@@@ACMDODM@
 IIIIIII@ACMAOHM@
 IIIIIIG@AMKDOKD@
 @@@@@AFOCB@CNDNK
 @@@@@@@@@@@@@@@@
 @@@@@@@@@@@@@@@@
 @@@@@@@@@@@@@@@@

 0000000 000000000
 9999986 F14B2EAC4
 9999998 F150EF258
 0000005 F0FFBFEF8
 0000000 F0030F3FF
 0001726 F14CFB0D0
 0000594 013113953
 0000057 01440A947
 0000006 013ECF9D7
 0000000 013D4F4D0
 9999999 013D1F8D0
 9999997 01DB4FB40
 0000016 F3203E4EB
 0000000 000000000
 0000000 000000000
 0000000 000000000
*/
  // 3478A calibration data decoding:
  // See https://tomverbeure.github.io/2022/12/02/HP3478A-Multimeter-Calibration-Data-Backup-and-Battery-Replacement.html
}

uint32_t isqrt(uint32_t num) {
  uint32_t res = 0;
  uint32_t bit = 1L << (sizeof(num) * 8 - 2);
  while (bit > num) bit >>= 2;  // bit starts at highest 4 ^ N <= num
  while (bit) {
    if (num >= res + bit) {
      num -= res + bit;
      res = (res >> 1) + bit;
    } else res >>= 1;
    bit >>= 2;
  }
  return res;
}

#define THERM_BETA    3950
#define ZERO_KELVIN   273.15f
#define THERM_T0      (25 + ZERO_KELVIN)
#define R0            10000  // Ohms

const uint8_t NumAvg = 16;

int32_t milliDegreesC(uint32_t sumReadings) {
  #define ROOT_X_SHIFT 15
  #define LOG_X_SHIFT (2 * ROOT_X_SHIFT)
  #define LOG_X_SCALE (1LL << LOG_X_SHIFT)

  // x = LOG_X_SCALE * R / R0
  uint32_t x = LOG_X_SCALE / NumAvg * sumReadings / 10 / R0;  // near LOG_X_SCALE (* 2 or / 2)

  // Borchardts approximation: very good near 1.0; 0.83% error at 10 or 0.1
  // ln(x) ~ 6 * (x - 1) / (x + 1 + 4 * sqrtfn(x));
  int64_t  num = (int64_t)6 * (x - LOG_X_SCALE);
  int64_t denom = x + LOG_X_SCALE + ((int64_t)isqrt(x) << (2 + ROOT_X_SHIFT));

  // 1/T = 1/T0 + ln(R/R0) / THERM_BETA
  #define SCALE (1LL << (64 - (3 + LOG_X_SHIFT)))
  return 1000 * SCALE /
      (SCALE / THERM_T0 + SCALE * num / THERM_BETA / denom)
   - (int32_t)(1000 * ZERO_KELVIN);
}

void showTemperature() {
  // Readings near " 0.99999E+4"
  // 30K Ohms ~ 1C  3K ~ 55C so always E+4  6 digits

  uint32_t sumReadings = 0;
  uint8_t i = NumAvg;
  while (1) {
    char* reading = getReading();
    if (reading[0] != ' ' || reading[2] != '.') {
      send(i); send("!\n");
      continue;
    }
    reading[2] = reading[1]; // make 6 digit integer
    sumReadings += atol(reading + 2);
    if (!--i) break;
  }

  int32_t milliDegC = milliDegreesC(sumReadings);
  static int32_t last_milliDegC = 25 * 1000;
  int deltaT = milliDegC - last_milliDegC;
  last_milliDegC = milliDegC;

  char tempStr[20];
  sprintf(tempStr, "D2%d.%03d%+dC\n", (int)(milliDegC / 1000), (int)(milliDegC % 1000), deltaT);
  ilSendStr(tempStr);
  send(tempStr+2);
}

int main(void) {
  ilInit();

  ilCmd(REN);
  ilCmd(DCL);
  ilCmd(AAD, 1);

  ilSendStr(timeStr);  // display compile time as version
  send(timeStr); send('\n');

  if (0) dumpCalibrationSRAM();

  // ilSendStr("F1T1"); // read Volts

  if (1) {
    ilSendStr("F3R3T1"); // 2-wire 30K Ohm range, internal trigger
    while (1)
      showTemperature();
  }

  timeStr[10] += 2;
  while (1) {
    incrSeconds();
    ilSendStr(timeStr);
    send(getReading());  // 2 readings per sec @ 5 1/2 digits -- poor clock
    __builtin_avr_delay_cycles(MF_CPU / 2); // adjust
  }

  ilCmd(GTL);
}



// Tests of HP-IL interface commands

// #pragma GCC poison float double   // float imported with atol, ... !!!

#include "hpil.h"
#include "send.h"
#include "stdio.h"


char* getReading() {
  return ilGetData();
}

char timeStr[] = "D2" __TIME__;

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

void dumpCalibrationSRAM(bool format = false) {
  send("Calibration data:\n");

  ilSendStr("B2"); // binary Calibration constants out -- see bottom of HP 3468
  while (1) {  // string for "B3" restore
    char* calData = ilGetData();
    if (*calData < '@') break;
    for (uint16_t p = 0; p < MAX_RESPONSE_LEN; ++p) {
      if (!calData[p]) break;
      if (format) {
        punctuation = true;
        sendHex((uint16_t)(calData[p] - 64)); // nibbles
        if (p % 16 == 6) send(' ');
      } else send(calData[p]);
      if (p % 16 == 15) send('\n');
    }
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

#define RESOLUTION    1000   // milliDegrees

#define ROOT_X_SHIFT 15
#define LOG_X_SHIFT (2 * ROOT_X_SHIFT)
#define LOG_X_SCALE (1L << LOG_X_SHIFT)

#define LOG_NUM_READINGS 7  // ~ 2 readings per sec --> ~ 1 minute report interval
const uint8_t NumAvg = (1 << LOG_NUM_READINGS);

#define SCALE_SHIFT (64 - (3 + LOG_X_SHIFT))
const uint32_t ScaledT0 = (uint32_t)((1UL << SCALE_SHIFT) / THERM_T0);
const int32_t DegreesKtoC = (int32_t)(RESOLUTION * ZERO_KELVIN);

int32_t milliDegreesC(uint32_t sumReadings) {
  // scaledRratio = LOG_X_SCALE * R / R0
  uint32_t scaledRratio = ((uint64_t)sumReadings << (LOG_X_SHIFT - LOG_NUM_READINGS -1 -4)) / (10L * R0 / 32); // near LOG_X_SCALE (* 2 or / 2)

  // Borchardts approximation: very good near 1.0; 0.83% error at 10 or 0.1
  // ln(scaledRratio) ~ 6 * (scaledRratio - 1) / (scaledRratio + 1 + 4 * sqrtfn(scaledRratio));
  int64_t num = (int64_t)6 * ((int64_t)scaledRratio - LOG_X_SCALE);
  int64_t denom = scaledRratio + LOG_X_SCALE + ((int64_t)isqrt(scaledRratio) << (2 + ROOT_X_SHIFT));

  // 1/T = 1/T0 + ln(R/R0) / THERM_BETA
  return ((int64_t)RESOLUTION << SCALE_SHIFT) /
      (ScaledT0 + (num << SCALE_SHIFT) / THERM_BETA / denom)
   - DegreesKtoC;
}


void showTemperature() {
  // Resistor readings near " 0.99999E+4" = 10K Ohms
  //   30K Ohms ~ 1C .. 3K ~ 55C so always E+4  6 digits

  static uint64_t scaledR;
  static uint32_t scaledRratio, sqrtRatio;
  static int64_t num, denom;
  static int64_t tempRes;
  static int32_t milliDegC, lastMilliDeg;
  static char tempStr[20] = "D2";

  uint32_t sumReadings = 0;
  uint8_t i = 0;
  while (1) {
    char* reading = ilGetData();
    if (reading[2] == '.') {
      reading[2] = reading[1]; // make 6 digit integer
      reading[1] = '0';
    } else if (reading[1] == '.')  // occasional dropped leading space (resistance "sign") -- SDA/DAB overlap?
      reading[1] = reading[0];
    else {
      send("Rcv err: ");
      send(reading);
      send('\n');
      continue;
    }
    sumReadings += atol(reading + 1);

    if (++i == NumAvg) {
      scaledR = (uint64_t)sumReadings << (LOG_X_SHIFT - LOG_NUM_READINGS -1 -4);
      return;
    }

    if (scaledR) switch(i) { // calculations distributed between readings
      case 2 : scaledRratio = scaledR / (10L * R0 / 32); break;
      case 3 : num = 6 * ((int64_t)scaledRratio - LOG_X_SCALE); break;
      case 4 : sqrtRatio = isqrt(scaledRratio); break;
      case 5 : denom = (int64_t)scaledRratio + LOG_X_SCALE + ((int64_t)sqrtRatio << (2 + ROOT_X_SHIFT)); break;

      case 6 : tempRes = num << SCALE_SHIFT; break;
      case 7 : tempRes /= THERM_BETA; break;
      case 8 : tempRes /= denom; break;
      case 9 : tempRes += ScaledT0; break;
      case 10 : milliDegC = ((int64_t)RESOLUTION << SCALE_SHIFT) / tempRes; break;
      case 11 : milliDegC -= DegreesKtoC; break;

      case 12 : sprintf(tempStr + 2, "%d.%03d%+dC\n", (int)(milliDegC / RESOLUTION), (int)(abs(milliDegC) % RESOLUTION), (int)(milliDegC - lastMilliDeg)); break;
          // sprintf uses ~2000 bytes even with -u _printf_float   (atol also brings in float~!)
          // --> could output integer and insert decimal point
      case 13 : send(tempStr+2); break;
      case 14 : ilSendStr(tempStr); break;
      case 15 : lastMilliDeg = milliDegC; break;
    }
  }
}

int main(void) {
  __builtin_avr_delay_cycles(MF_CPU * 2); // allow for USB enumeration -- longer down tree
  send('\n');
  send(timeStr+2); send('\n');

  ilInit();
  ilCmd(DCL);
  ilCmd(REN);
  ilCmd(AAD, 1);

  ilSendStr("M00F3R3T1"); // mask all SRQs off; 2-wire 30K Ohm range, internal trigger
  __builtin_avr_delay_cycles(MF_CPU / 2);

  ilSendStr(timeStr);  // display compile time as version

  if (0) dumpCalibrationSRAM();

  if (1) {
    ilSendStr("F3R3T1"); // 2-wire 30K Ohm range, internal trigger
    while (1)
      showTemperature();
  }

  // ilSendStr("F1T1"); // read Volts

  timeStr[10] += 2;
  while (1) {
    incrSeconds();
    ilSendStr(timeStr);
    send(getReading());  // 2 readings per sec @ 5 1/2 digits -- poor clock
    __builtin_avr_delay_cycles(MF_CPU / 2); // adjust
  }

  ilCmd(GTL);
}

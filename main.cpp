// Tests of HP-IL interface commands

// #pragma GCC poison float double   // float imported with atol, ... !!!

#include "hpil.h"
#include "send.h"
#include "stdio.h"
#include <avr/io.h>


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

void sendRawCalData() {
  ilSendStr("B2"); // binary Calibration constants out -- see bottom of HP 3468
  while (1) {  // string for "B3" restore
    char* calData = ilGetData();
    uint8_t p = MAX_RESPONSE_LEN;  // 16
    do {
      if (*calData < '@') {
        send('\n');
        return;
      }
      send(*calData++);
    } while (--p);
    send('\n');
  }
}

const char* RangeStr[16] = {  // AVR Harvard __flash only in C
  "",
  "300mV",
  "3V",
  "30V",
  "300V",
  "ACV",
  "300R",
  "3KR",
  "30KR",
  "300KR",
  "3MR",
  "30MR",
  "3A",
  "", "", ""
};

typedef union {
  struct {
    uint8_t offset[7]; // BCD
    uint8_t gain[5];   // BCD signed nibbles -8..+7
    uint8_t columnParity[2];
    uint8_t rowParity[2];
  };
  uint8_t columnRow[2][6];
  char rawData[16];
} CalData;  // only LS nibbles are data


inline void checkParity(CalData* calData) {
  // Parity, SBEC: https://www.hpmuseum.org/forum/thread-8061-post-190013.html#pid190013

  // check column parity C D
  for (uint8_t column = 0; column < 2; ++column) {
    uint8_t parity = 0xF; // odd
    for (uint8_t row = 0; row < 6; ++row)
      parity ^= calData->columnRow[column][row];
    parity ^= calData->columnParity[column];
    if (parity & 0xF) { // should be 0
      send("col parity! ");
      sendHex((uint8_t)(parity & 0xF));
    }
  }

  // check row parity E F
  const uint8_t RowParity[16] = {0,1,1,0,1,0,0,1, 1,0,0,1, 0,1,1,0};
  uint8_t rowParity = 0;
  for (uint8_t row = 0; row < 6; ++row) {
    rowParity |= RowParity[calData->columnRow[0][row]] ^ RowParity[calData->columnRow[1][row]] ^ 1;
    rowParity <<= 1;
  }
  rowParity |= RowParity[calData->columnParity[0]] ^ RowParity[calData->columnParity[1]] ^ 1;
  rowParity <<= 1;
  rowParity |= RowParity[calData->rowParity[0]] ^ RowParity[calData->rowParity[1] & 0xC] ^ 1;

  if (rowParity >> 4   != calData->rowParity[0]
  || (rowParity & 0xF) != calData->rowParity[1]) {
    send("row parity! "); sendHex(rowParity);
  }
  // row/col intersection could correct a single bit error
}

inline uint32_t getGain(CalData* calData) { // 0.911112 .. 1.077777
  // Gain signed BCD nibbles: https://www.eevblog.com/forum/repair/hp-3478a-how-to-readwrite-cal-sram/msg1966463/#msg1966463
  uint32_t gain = 10;
  for (uint8_t gp = 0; gp < 5; ++gp) {
    gain *= 10;
    int8_t gainDigit = calData->gain[gp];
    if (gainDigit >= 8) gainDigit -= 16;
    gain += gainDigit;
  }
  if (gain < 1000000) send(' ');
  return gain;
}

inline int32_t getOffset(CalData* calData) {
  for (uint8_t p = sizeof(calData->offset); p--;)
    calData->offset[p] += '0'; // convert to BCD ASCII
  calData->gain[0] = 0; // terminate offset string - LAST!!
  int32_t offset = atol(calData->rawData);
  if (offset > 5000000)
    offset -= 10000000;
  return offset;
}

void dumpCalibrationSRAM() {
  send("Calibration data:\n");
  sendRawCalData();

  // interpret calibration data:
  ilSendStr("B2"); // binary Calibration constants out -- see bottom of HP 3468
  for (uint8_t idx = 0; idx < 16; ++idx) {
    CalData* calData = (CalData*)ilGetData();
    if (!RangeStr[idx][0]) continue;
    send(RangeStr[idx]); send ('\t');

    for (uint8_t p = 16; p--;)
      calData->rawData[p] &= 0xF; // leave just data nibbles

    checkParity(calData);
    send(getGain(calData)); send('\t');
    send(getOffset(calData)); // LAST (sets gain[0] = 0)
    send('\n');
  }

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

Range      Gain Offset
300mV    991352 -14
3V       991498 -2
30V      989885 5
300V     990030 0
ACV      991359 1726
300R    1001311 594
3KR     1001440 57
30KR    1001276 6
300KR   1001274 0
3MR     1001271 -1
30MR    1000654 -3
3A       993203 16

*/
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

  dumpCalibrationSRAM();

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

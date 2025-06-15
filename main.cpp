// Tests of HP-IL interface commands

#include "hpil.h"
#include "send.h"

const char* getReading() {
  ilSendStr("T1");
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
    if (*pTime <= '0') break;
    --pTime;  // over colon
  } // to 59:59:59 hours
}

void dumpCalibrationSRAM() {
  send("Calibration data:\n");

  ilSendStr("B2"); // binary Calibration constants out -- see bottom of HP 3468
  const char* calData = ilGetData();
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
 0000000 00000000
*/
  // 3478A calibration data decoding:
  // See https://tomverbeure.github.io/2022/12/02/HP3478A-Multimeter-Calibration-Data-Backup-and-Battery-Replacement.html
  // "Wn" - read SRAM byte (1024 nibbles ?)

#if 0
  send("\nTrying W command\n");
  for (uint16_t sramAddr = 0; sramAddr <= 255; sramAddr++) {
#if 0
    ilCmd(LAD, 1);
    ilCmd(DAB, 'W');
    ilCmd(DAB, sramAddr);
    ilCmd(END, '\n');
#elif 0
    char readSRAM[8] = "W";
    itoa(sramAddr, readSRAM + 1,10);
    ilSendStr(readSRAM);
    // no response
#endif
    send(ilGetData()[0]);
  }
#endif

}

int main(void) {
  ilInit();

  ilCmd(REN);
  ilCmd(DCL);
  ilCmd(AAD, 1);

  ilSendStr(timeStr);  // display compile time as version

  dumpCalibrationSRAM();

  ilSendStr("F1T1"); // read Volts

  timeStr[10] += 2;
  while (1) {
    incrSeconds();
    ilSendStr(timeStr);
    send(getReading());  // 2 readings per sec @ 5 1/2 digits -- poor clock
    __builtin_avr_delay_cycles(MF_CPU / 2); // adjust
  }

  ilCmd(GTL);
}



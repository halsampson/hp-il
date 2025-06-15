// Tests of HP-IL

#include "hpil.h"
#include "send.h"

const char* getReading() {
  ilSendStr("D1F1T1");

  ilCmd(TAD, 1);
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

void displayVersion() {
  ilSendStr(timeStr); // no lower case
  __builtin_avr_delay_cycles(MF_CPU - MF_CPU / 19); // adjust
  incrSeconds();
}

void dumpCalibrationSRAM() { // TODO: fix
  // "B2"  binary cal constant out (at each calibration Cn step?)  -- see bottom of unit
  // "Wn" - read SRAM byte (1024 nibbles?) ??

  for (uint16_t sramAddr = 0; sramAddr <= 127; sramAddr++) {  // TODO: 255+ ???
    ilCmd(LAD, 1);
    ilCmd(DAB, 'W');  // ??? TODO getting readings!!
    ilCmd(DAB, sramAddr);
    ilCmd(END, '\n');

    ilCmd(TAD, 1);
    ilGetData();
    //dataBufIdx -= 3;
  }
}

int main(void) {
  ilInit();

  ilCmd(REN);
  ilCmd(DCL);
  ilCmd(AAD, 1);

  timeStr[10] += 2;
  displayVersion();

#if 1
  send(getReading());
  timeStr[10] += 6;
#elif 1
  dumpCalibrationSRAM();
#endif

  while (1) {
    displayVersion();
    send(getReading());
  }    

  ilCmd(GTL);
}



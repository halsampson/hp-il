// Tests of HP-IL

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

void dumpCalibrationSRAM() { // TODO: figure out
  // "B2"  binary cal constant out (at each calibration Cn step?)  -- see bottom of unit
  // "Wn" - read SRAM byte (1024 nibbles?) ?? 3478

  ilSendStr("B2");
  for (uint8_t sramAddr = 0; sramAddr <= 15; sramAddr++) {
    send(ilGetData()); send('\n');
  }
  /*
@@@@@@@@@@@@@@@@@
IIIIIHFOADKBNJLDD
IIIIIIHOAE@NOBEHH
@@@@@@EO@OOKONOHH
@@@@@@@O@@C@OCOOO
@@@AGBFOADLOK@M@@
@@@@EID@ACAACIECC
@@@@@EG@ADD@JIDGG
@@@@@@F@ACNLOIMGG
@@@@@@@@ACMDODM@@
IIIIIII@ACMAOHM@@
IIIIIIG@AMKDOKD@@
@@@@@AFOCB@CNDNKK
@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@
  */

  // 3478A calibration data decoding:
  // See https://tomverbeure.github.io/2022/12/02/HP3478A-Multimeter-Calibration-Data-Backup-and-Battery-Replacement.html

#if 0
  send("\nTrying W\n");
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

  ilSendStr("F1T1"); // read Volts

  ilSendStr(timeStr);  // compile time = version

#if 1
  dumpCalibrationSRAM();  // not yet working
#endif

  timeStr[10] += 5;

  while (1) {
    incrSeconds();
    ilSendStr(timeStr);
    send(getReading());  // 2 readings per sec @ 5 1/2 digits
    __builtin_avr_delay_cycles(MF_CPU / 2);
  }

  ilCmd(GTL);
}



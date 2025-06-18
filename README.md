## Super simple HP-IL interface

Tested communicating with HP 3468B DMM: can set up, read measurements, and read calibration SRAM data (using "B2" command).

Tested using ATtiny85 DigiSpark, but can be easily ported to other MCUs.

HP-IL cable interface circuit consists of 3 impedance matching resistors and a level-shifting diode to decode the 3 level pulses using 2 digital input pins (ILIP and ILIN) with ~200mV hysteresis.  ILOP and ILON are ~25 Ohm drive 5V digital output pins.

Works OK without 240 Ohm resistor on short (1 m) cable. 62 Ohm output resistors also worked OK.

The ATtiny input side is less tolerant. A 97.6 Ohm resistor to Vcc worked best with a short, low DC resistance cable. Choose pullup resistor so that idle ILIP voltage is 0.47 * Vcc (mid Vin for ATTiny) + half Si diode drop (~0.72V / 2 for 1N4148).

```
HP-IL In <-----------------v-- 59 Ohm ----< ILOP
                           |
                        (240 Ohm)
                           |
HP-IL In <-----------------^-- 59 Ohm ----< ILON
 (Ref: left)

                                                       ATtiny MCU

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
```

Device input connections used standard cheap female sockets laying around: https://www.te.com/usa-en/product-166291-1.datasheet.pdf (spread split end a bit to fit over device side cylindrical female sockets; crimp to cable)  

Device outputs specs Cannon 031-9542-xxx female sockets ($1.70 on Mouser) but there must be alternatives with a close i.d and o.d. (tight fit). Can't find dimensioned drawings for 031-9542-xxx.

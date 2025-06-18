## Super simple HP-IL interface

Tested communicating with HP 3468B DMM: can set up, read measurements, and read calibration SRAM data (using "B2" command).

Tested using ATtiny85 DigiSpark, but can be easily ported to other MCUs.

HP-IL cable interface circuit consists of 3 impedance matching resistors and a level-shifting diode to decode the 3 level pulses using 2 digital input pins (ILIP and ILIN) with ~200mV hysteresis.  ILOP and ILON are ~25 Ohm drive 5V digital output pins.

Works OK without 240 Ohm resistor on short (1 m) cable. 62 Ohm output resistors also worked OK.

The ATtiny input side is less tolerant. A 97.6 Ohm diode pullup resistor worked better with a short, low DC resistance cable. Choose pullup resistor so that idle ILIP voltage is 0.47 * Vcc (mid Vin for ATTiny) + half Si diode drop (~0.72V / 2 for 1N4148).

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


Input connection pins used: https://www.te.com/en/product-166291-1.html (crimp to cable)

These female pins can also be split and filed down to use on the HP-IL Output pins, but there must be better fits!

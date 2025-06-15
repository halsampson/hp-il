## Super simple HP-IL interface

Using DigiSpark ATtiny 85, but easily ported to other MCUs

```
100 Ohm matching / 1.5V attenuation network for 5V 23/27 Ohm ATtiny85 outputs:

HP-IL In <-----------------v-- 59 Ohm ----< ILOP
                           |
                        240 Ohm
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

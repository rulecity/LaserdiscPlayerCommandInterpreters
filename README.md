# Laserdisc Player Command Interpreters
A repository to hold interpreters for the command sets of various laserdisc players.

# To build

## As part of Dexter's firmware
https://github.com/RulecityLLC/DexterBuilder

## Cross-compile for AVR
```
mkdir build.avr

cd build.avr

CC=avr-gcc cmake -DCMAKE_C_FLAGS="-mmcu=atmega644p -Wall -gdwarf-2 -std=gnu99 -DREV3 -DF_CPU=18432000UL -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums" -DCMAKE_BUILD_TYPE=Release ..

make

make install
```

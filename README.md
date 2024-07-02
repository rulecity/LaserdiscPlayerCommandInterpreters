 Laserdisc Player Command Interpreters
A repository to hold interpreters for the command sets of various laserdisc players.

# To build

## Cross-compile for AVR
`mkdir build.avr`

`cd build.avr`

`CC=avr-gcc cmake -DCMAKE_C_FLAGS="-mmcu=atmega644p -Wall -gdwarf-2 -std=gnu99 -DREV3 -DF_CPU=18432000UL -O3 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums" -DCMAKE_BUILD_TYPE=Release ..`

`make`

`make install`

## Native compile with mutation testing
`mkdir build.mutation`

`cd build.mutation`

Create a file called mull.yml that contains the following:

mutators:
  - cxx_arithmetic
  - cxx_comparison
excludePaths:
  - .*gtest.*
  - .*gmock.*
  - .*tests.*

`CC=clang-17 CXX=clang++-17 cmake -DCMAKE_C_FLAGS="-O0 -fpass-plugin=/usr/lib/mull-ir-frontend-17 -g -grecord-command-line" -DCMAKE_CXX_FLAGS="-O0 -fpass-plugin=/usr/lib/mull-ir-frontend-17 -g -grecord-command-line" -DBUILD_TESTING=ON ..`

`make`

`mull-runner-17 -ld-search-path /lib/x86_64-linux-gnu tests/test_ldp_in`

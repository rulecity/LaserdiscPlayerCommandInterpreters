# Laserdisc Player Command Interpreters
A repository to hold interpreters for the command sets of various laserdisc players.

# To build

## As part of Dexter's firmware
https://github.com/RulecityLLC/DexterBuilder

## To run the unit tests locally
Start Docker container using a command like this (you'd need to modify the /c/projects to point to your local file system) :
```
docker run --rm -v /c/projects/LaserdiscPlayerCommandInterpreters:/ldp-in -ti mpownby/dexter-builder sh -c "cd /ldp-in && /bin/bash"
```

Then once the container is running, use these commands:
```
mkdir build
cd build

CC=clang-17 CXX=clang++-17 cmake .. -DCMAKE_C_FLAGS="-O0 -fpass-plugin=/usr/lib/mull-ir-frontend-17 -g -grecord-command-line" -DBUILD_TESTING=ON

make
```

After building, to run the unit tests:
```
tests/test_ldp_in
```

To run the mutation tests (all tests must pass before doing this or you will get invalid results):
```
/usr/bin/mull-runner-17 --ld-search-path /lib/x86_64-linux-gnu tests/test_ldp_in
```

## Cross-compile for AVR
```
mkdir build.avr

cd build.avr

CC=avr-gcc cmake -DCMAKE_C_FLAGS="-mmcu=atmega644p -Wall -gdwarf-2 -std=gnu99 -DREV3 -DF_CPU=18432000UL -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums" -DCMAKE_BUILD_TYPE=Release ..

make

make install
```

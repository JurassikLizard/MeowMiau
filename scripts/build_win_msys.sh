#!/bin/bash
cmake -S . -B build-win -DPLATFORM=Desktop -G "MSYS Makefiles"
cmake --build build-win
./build-win/meowmiau.exe
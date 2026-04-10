#!/bin/bash
cmake -S . -B build-win -DPLATFORM=Desktop -G "MSYS Makefiles"
cmake --build build-win
cd build-win/
./meowmiau.exe
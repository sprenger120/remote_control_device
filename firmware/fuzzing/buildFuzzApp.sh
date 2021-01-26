#!/bin/bash

# run fuzzing compile test
python2 canfestival/objdictgen/objdictgen.py objectDictionary/RemoteControlDevice.od objectDictionary/generatedOD/RemoteControlDevice.c
export CC=/usr/local/bin/afl-gcc
export CXX=/usr/local/bin/afl-g++
cmake -S . -B build/
if make -C build -j 8; then
build/fuzzapp ./goodTestData.data
fi
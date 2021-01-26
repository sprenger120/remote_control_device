#!/bin/bash


# run firmware tests
#rm -Rf test/build/*
#rm -Rf canfestival/test/src/testOD/generated/*
python2 canopen_stack_canfestival/objdictgen/objdictgen.py objectDictionary/RemoteControlDevice.od objectDictionary/generatedOD/RemoteControlDevice.c
cmake -S test/ -B test/build/
if make -C test/build -j 8; then
test/build/testapp
fi
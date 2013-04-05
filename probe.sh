#!/bin/bash


cd matmul-16_host/Release/

sudo LD_LIBRARY_PATH=/opt/adapteva/esdk/tools/host/armv7l/lib ./e-probe.elf 0

cd ../../


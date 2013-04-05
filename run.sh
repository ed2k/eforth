#!/bin/bash


cd host/Release/

sudo LD_LIBRARY_PATH=/opt/adapteva/esdk/tools/host/armv7l/lib ./matmul-16.elf ../../matmul-16/Cores/matmul-16.srec

cd ../../


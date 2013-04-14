#!/bin/bash

set -e

ESDK=${EPIPHANY_HOME}
ELIBS=${ESDK}/tools/host.armv7l/lib:${LD_LIBRARY_PATH}
EHDF=${EPIPHANY_HDF}

cd host/Release/

sudo -E LD_LIBRARY_PATH=${ELIBS} EPIPHANY_HDF=${EHDF} ./matmul-16.elf ../../matmul-16/Cores/matmul-16.srec

cd ../../


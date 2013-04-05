#!/bin/bash

set -e

if [[ "`arch`" == "x86_64" ]]; then
	echo "Setting path to ARM tools."
	GNU_PATH=${HOME}'/CodeSourcery/Sourcery_CodeBench_Lite_for_ARM_GNU_Linux/bin/arm-none-linux-gnueabi-'
else
	GNU_PATH=''
fi

Proj='matmul-16'
Config='Release'
# If building via Eclipse IDE, then assign 'no'. For command line build, assign 'yes'
BUILD_DEVICE='yes'
BUILD_HOST='yes'

MK_CLEAN='yes'
MK_ALL='yes'

#System_ID=3
#COREID='0x808'
#STATIC_LINK='-static'


if [[ "${BUILD_DEVICE}" == "yes" ]]; then
	echo "=== Building device programs ==="

	#for f in {${Proj}_commonlib/${Config},${Proj}.core.*/${Config},${Proj}}; do
	for f in {e_commonlib/${Config},core.32_08/${Config},${Proj}}; do
		pushd $f >& /dev/null
		if [[ "${MK_CLEAN}" == "yes" ]]; then
			echo "*** Cleaning $f"
			make clean
		fi
		if [[ "${MK_ALL}" == "yes" ]]; then
			echo "*** Building $f"
			make --warn-undefined-variables BuildConfig=${Config} all
		fi
		popd >& /dev/null
	done
fi


INC_PATH=${EPIPHANY_HOME}/tools/host/`arch`/include
LIB_PATH=${EPIPHANY_HOME}/tools/host/`arch`/lib

if [[ "${BUILD_HOST}" == "yes" ]]; then
	echo "=== Building host programs ==="

	mkdir -p ./host/${Config}

	if [[ "${MK_CLEAN}" == "yes" ]]; then
		echo "*** Cleaning host programs"
		rm -f ./host/${Config}/matmul-16.elf
		rm -f ./host/${Config}/e-probe.elf
	fi

	if [[ "${MK_ALL}" == "yes" ]]; then
		echo "*** Building host program"
                cd host/src
		make 
		echo "*** Building probe program"

	fi
fi


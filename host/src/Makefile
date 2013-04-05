Config = Release
GNU_PATH='/usr/bin/arm-linux-gnueabi-'


INC_PATH=${EPIPHANY_HOME}/tools/host/armv7l/include
LIB_PATH=${EPIPHANY_HOME}/tools/host/armv7l/lib


HOST_ELF=../${Config}/matmul-16.elf
HOST_PROBE=../${Config}/e-probe.elf


all:${HOST_ELF} ${HOST_PROBE}

${HOST_ELF}: matmul_host.c
	${GNU_PATH}gcc ${STATIC_LINK} \
                        -Ofast \
                        -Wall \
                        -g0 -std=c99 \
                        -ffast-math \
                        -falign-loops=8 \
                        -funroll-loops \
                        -D__HOST__ \
                        -Dasm=__asm__ \
                        -Drestrict= \
                        -I../../include \
                        -I${INC_PATH} \
                        -L${LIB_PATH} \
                        -Wl,-R'/home/linaro/Adapteva/work/e-host' \
                        -o "${HOST_ELF}" \
                        "matmul_host.c" \
                        -le-loader \
                        -le-host

${HOST_PROBE}: e-probe.c e-probe-offsets.c
	${GNU_PATH}gcc ${STATIC_LINK} \
                        -Ofast \
                        -Wall \
                        -g0 -std=c99 \
                        -D__HOST__ \
                        -Dasm=__asm__ \
                        -Drestrict= \
                        -I../../include \
                        -I${INC_PATH} \
                        -L${LIB_PATH} \
                        -Wl,-R'/home/linaro/Adapteva/work/e-host' \
                        -o "${HOST_PROBE}" \
                        "e-probe.c" \
                        -le-host


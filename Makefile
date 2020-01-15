TOP_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))


DIR :=
DIR += private/libnxdeinterlace
#DIR += private/libnxv4l2
DIR += src/librearcam
DIR += apps/NxQuickRearCam

CROSS_COMPILE ?= aarch64-linux-gnu-

all:
	@for dir in $(DIR); do	\
	make CROSS_COMPILE=$(CROSS_COMPILE) -C $$dir || exit $?;	\
	make -C $$dir install;	\
	done

clean:
	@for dir in $(DIR); do	\
	make -C $$dir clean || exit $?;	\
	done
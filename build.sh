#!/bin/bash

function usage()
{
	echo "Usage: `basename $0 ` [ -a -t -c -p -r]"
	echo "  -a : app build"
	echo "  -t : all build"
	echo "  -c : build clean"
	echo "  -p : build private lib"
	echo "  -r : build nxrearcam lib"
}

function build_clean()
{
	rm $SCRIPT_PATH/NxQuickRearCam
	rm $SCRIPT_PATH/lib/linux/libnxrearcam.a
	cd $SCRIPT_PATH/src/librearcam
	make clean

	cd $SCRIPT_PATH/apps/NxQuickRearCam
	make clean

	rm $SCRIPT_PATH/lib/linux/libnxv4l2.a
	rm $SCRIPT_PATH/lib/linux/libnxdeinterlace.a

	cd $SCRIPT_PATH/private/libnxdeinterlace
	make clean

	cd $SCRIPT_PATH/private/libnxv4l2
	make clean


	cd $SCRIPT_PATH/
}

function build_private()
{
	rm $SCRIPT_PATH/lib/linux/libnxv4l2.a
	rm $SCRIPT_PATH/lib/linux/libnxdeinterlace.a

	cd $SCRIPT_PATH/private/libnxdeinterlace
	make clean
	make -j 8
	make install

	cd $SCRIPT_PATH/private/libnxv4l2
	make clean
	make -j 8
	make install

	cd $SCRIPT_PATH/
}

function build_rearcam()
{
	rm $SCRIPT_PATH/lib/linux/libnxrearcam.a
	cd $SCRIPT_PATH/src/librearcam
	make clean
	make -j 8
	make install

	cd $SCRIPT_PATH/
}

function build_application()
{
	rm $SCRIPT_PATH/NxQuickRearCam
	cd $SCRIPT_PATH/apps/NxQuickRearCam
	make clean
	make -j 8
	mv NxQuickRearCam $SCRIPT_PATH/NxQuickRearCam

	cd $SCRIPT_PATH/
}


cd `dirname $0`
SCRIPT_PATH=`pwd`

while getopts 'atcprh' opt
do
	case $opt in
		a ) BUILD_APP="true";;
		t ) BUILD_ALL="true";;
		c ) BUILD_CLEAN="true" ;;
		p ) BUILD_PRIVATE="true";;
		r ) BUILD_REARCAM="true";;
		h ) usage; exit 1 ;;
		- ) break ;;
		* ) echo "invalid options $1"; usage;exit 1 ;;
	esac
done

if [ "$BUILD_ALL" == "true" ]; then
	build_private
	build_rearcam
	build_application
fi

if [ "$BUILD_CLEAN" == "true" ]; then
	build_clean
fi

if [ "$BUILD_PRIVATE" == "true" ]; then
	build_private
fi

if [ "$BUILD_REARCAM" == "true" ]; then
	build_rearcam
fi

if [ "$BUILD_APP" == "true" ]; then
	build_application
fi



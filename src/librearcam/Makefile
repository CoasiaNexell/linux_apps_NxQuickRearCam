########################################################################
# Get Linux Build Enviornment:

export TOPDIR	:=$(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi | sed s,.*users,/home/users,)


#########################################################################
#																		#
#								Includes								#
#																		#
#########################################################################
INCLUDE		+=  \
			-I. \
			-I$(TOPDIR)/../../src/common \
			-I$(TOPDIR)/../../include \
			-I$(TOPDIR)/../../include/drm \

#########################################################################
#																		#
#								Sources									#
#																		#
#########################################################################

CXXOBJS		:=	NX_CV4l2Camera.o \
				NX_CV4l2VipFilter.o \
				NX_CFlipFilter.o \
				NX_CDeinterlaceFilter.o \
				NX_CVideoRenderFilter.o \

# BackGear Detection
CXXOBJS		+=	NX_CGpioControl.o \
				NX_CBackgearDetect.o \
# Utils
CXXOBJS		+=	../common/NX_DbgMsg.o \
				../common/NX_ThreadUtils.o \

# Manager
CXXOBJS		+=	NX_CRearCamManager.o \

CXXOBJS		+= nxp_video_alloc_ion.o \

COBJS		:= nxp_video_alloc_drm.o \

#########################################################################
#																		#
#								Target									#
#																		#
#########################################################################
# Target
TARGET		:= libnxquickrearcam.a


#########################################################################
#																		#
#								Build									#
#																		#
#########################################################################
# Build option

CFLAGS	+= -O3 -fPIC -Wall -static
#CFLAGS 	+= -DTHUNDER_DEINTERLACE_TEST
OBJS := $(CXXOBJS) $(COBJS)


$(TARGET): depend $(OBJS)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJS)

all: $(TARGET)

install:
	cp -a $(TARGET) ../../lib/linux


.PHONY: clean

clean :
	rm -rf $(OBJS) $(TARGET)

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(CXXOBJS:.o=.cpp) $(COBJS:.o=.c)
INCS := $(INCLUDE)
depend dep:
	$(CC) -M $(CFLAGS) $(INCS) $(SRCS) > .depend

%.o: %.c
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CFLAGS) $(INCS) -c -o $@ $<

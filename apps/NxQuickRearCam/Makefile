 ######################################################################
# Build options
export TOPDIR	:=$(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi | sed s,.*users,/home/users,)

CPPFLAGS = -Wall

#########################################################################
#																		#
#								Includes								#
#																		#
#########################################################################
#	Local Application Includes

INCLUDES		?= -I./ -I$(SDKTARGETSYSROOT)/usr/include/drm
INCLUDES		+= -I/$(SDKTARGETSYSROOT)/usr/include/nexell \
			-I/$(SDKTARGETSYSROOT)/usr/include/libdrm \
			-I$(SDKTARGETSYSROOT)/usr/include \
			-I$(TOPDIR)/../../src/common \
			-I$(TOPDIR)/../../include \
			-I$(TOPDIR)/../../include/drm \
			
#########################################################################
#																		#
#								Sources									#
#																		#
#########################################################################

OBJS		:=	NX_DbgMsg.o \
				NX_QuickRearCam.o \
				NX_CRGBDraw.o \
				NX_CCommand.o \
				$(TOPDIR)/../../src/common/NX_ThreadUtils.o \

#########################################################################
#																		#
#								Library									#
#																		#
#########################################################################

LDFLAGS += \
	-L$(TOPDIR)/../../lib/linux \
	-L$(SDKTARGETSYSROOT)/usr/lib \

LIBS += -lrt -lpthread 
LIBS += -lnxquickrearcam -lnxv4l2 -ldrm 
LIBS += -lnxdeinterlace -ltsdeinterlacer

#COBJS  = 
#COBJS += $(wildcard ../../lib/linux/deinterlacer.a)
#########################################################################
#																		#
#								Target									#
#																		#
#########################################################################
# Target
TARGET		:= NxQuickRearCam


######################################################################
# Build

CFLAGS		+= -fPIC
CPPFLAGS	+= -fPIC -Wall
CPPFLAGS 	+= -O3 $(INCLUDES) $(LDFLAGS) $(LIBS) -static -pthread 


SRCS = $(OBJS:.o=.cpp) 

$(TARGET): $(OBJS) $(COBJS)
	$(CXX) -o $@ $(CPPFLAGS) $(INCLUDES) $(OBJS) $(LIBS) $(COBJS)


all: $(TARGET)


.PHONY: clean

clean :
	rm -rf $(OBJS) $(TARGET) 
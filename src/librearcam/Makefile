########################################################################
# Get Linux Build Enviornment:

export TOPDIR	:=$(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi | sed s,.*users,/home/users,)



#########################################################################
#																		#
#								Includes								#
#																		#
#########################################################################
#	Local Application Includes

INCLUDES		?= -I./ -I$(SDKTARGETSYSROOT)/usr/include/drm
INCLUDES		+= -I/$(SDKTARGETSYSROOT)/usr/include/nexell \
			-I/$(SDKTARGETSYSROOT)/usr/include/libdrm \
			-I$(TOPDIR) \
			-I$(TOPDIR)/../../src/common \
			-I$(TOPDIR)/../../include \


#########################################################################
#																		#
#								Sources									#
#																		#
#########################################################################

OBJS		:=	NX_CV4l2Camera.o \
				NX_CV4l2VipFilter.o \
				NX_CDeinterlaceFilter.o \
				NX_CVideoRenderFilter.o \
				nxp_video_alloc_drm.o \
				nxp_video_alloc_ion.o \
#				parking_line.o \


# BackGear Detection
OBJS		+=	NX_CGpioControl.o \
				NX_CBackgearDetect.o \
# Utils
OBJS		+=	../common/NX_DbgMsg.o \

# Manager
OBJS		+=	NX_CRearCamManager.o \



#########################################################################
#																		#
#								Target									#
#																		#
#########################################################################
# Target
TARGET		:= libnxrearcam.a

######################################################################
# Build

CFLAGS		+= -fPIC
CPPFLAGS	+= -fPIC -Wall
CPPFLAGS 	+= -O3 $(INCLUDES) $(LDFLAGS) $(LIBS) -static -pthread
CPPFLAGS 	+= -DTHUNDER_DEINTERLACE_TEST

SRCS = $(OBJS:.o=.cpp)


$(TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJS)

all: $(TARGET)

install:
	cp -a $(TARGET) ../../lib/linux


.PHONY: clean

clean :
	rm -rf $(OBJS) $(TARGET)

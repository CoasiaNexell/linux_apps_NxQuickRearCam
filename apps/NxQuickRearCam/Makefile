 ######################################################################
# Build options
export TOPDIR	:=$(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi | sed s,.*users,/home/users,)

#########################################################################
#																		#
#								Includes								#
#																		#
#########################################################################
INCLUDE	+= \
			-I. \
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
LIBS += -lrt -lpthread
LIBS += -L$(TOPDIR)/../../lib/linux -lnxquickrearcam -lnxdeinterlace -ltsdeinterlacer
LIBS += -lnx_v4l2 -ldrm



#########################################################################
#																		#
#								Target									#
#																		#
#########################################################################
# Target
TARGET		:= NxQuickRearCam


#########################################################################
#																		#
#								Build									#
#																		#
#########################################################################
# Build option

CFLAGS	+= -O3 -fPIC -Wall



all: $(TARGET)

$(TARGET): $(OBJS) $(COBJS)
	$(CXX) -o $@ $(CFLAGS) $(INCLUDE) $(LDFLAGS) $(OBJS) $(LIBS) $(COBJS)

install:

clean :
	rm -rf $(TARGET) *.o .depend

distclean: clean

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS = $(OBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	$(CC) -M $(CFLAGS) $(INCS) $(SRCS) > .depend

%.o: %.c
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CFLAGS) $(INCS) -c -o $@ $<
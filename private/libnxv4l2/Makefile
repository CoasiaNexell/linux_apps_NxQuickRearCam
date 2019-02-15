########################################################################
# Get Linux Build Enviornment:

export TOPDIR	:=$(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi | sed s,.*users,/home/users,)



#########################################################################
#																		#
#								Includes								#
#																		#
#########################################################################
#	Local Application Includes

INCLUDES		?= \
			-I$(TOPDIR) \
			-I$(TOPDIR)/../../src/common \
			-I$(TOPDIR)/../../include \


#########################################################################
#																		#
#								Sources									#
#																		#
#########################################################################

OBJS		:=	nx-v4l2.o \

# Utils
OBJS		+=	../../src/common/NX_DbgMsg.o \



#########################################################################
#																		#
#								Library									#
#																		#
#########################################################################
LDFLAGS += \
	-L$(TOPDIR)/../../lib \
#LIBRARY 	?= -lnx_video_api -lnx_v4l2  -ldrm
#LIBS += -lrt -lc -lstdc++ -s
#LIBS += -ldrm -lnx_video_api -lnxv4l2 -ldrm


#########################################################################
#																		#
#								Target									#
#																		#
#########################################################################
# Target
TARGET		:= libnxv4l2.a

######################################################################
# Build

CFLAGS		+= -fPIC
CPPFLAGS	+= -fPIC -Wall
CPPFLAGS 	+= -O3 $(INCLUDES) $(LDFLAGS) $(LIBS)


SRCS = $(OBJS:.o=.cpp)


all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJS)


install:
	cp -a $(TARGET) ../../lib/linux/


.PHONY: clean

clean :
	rm -rf $(OBJS) $(TARGET)
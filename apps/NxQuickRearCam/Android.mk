LOCAL_PATH		:= $(call my-dir)

NX_INC_TOP		:= $(LOCAL_PATH)/../../include
NX_LIB_TOP		:= $(LOCAL_PATH)/../../lib/android


include $(CLEAR_VARS)

#########################################################################
#																		#
#								Includes								#
#																		#
#########################################################################
LOCAL_C_INCLUDES +=	\
	external/libdrm \
	hardware/nexell/s5pxx18/gralloc	\
	$(NX_INC_TOP) \
	$(NX_INC_TOP)/drm \
	$(LOCAL_PATH)

#########################################################################
#																		#
#								Sources									#
#																		#
#########################################################################
LOCAL_SRC_FILES		+=		\
	NX_DbgMsg.cpp \
	NX_CCommand.cpp \
	NX_CRGBDraw.cpp \
	NX_QuickRearCam.cpp \

#########################################################################
#																		#
#								Library									#
#																		#
#########################################################################
LOCAL_STATIC_LIBRARIES += liblog libdrm

LOCAL_STATIC_LIBRARIES += libnxrearcam libnxdeinterlace libnx_v4l2

#LOCAL_LDLIBS = -L$(NX_LIB_TOP) -ldeinterlacer_static
LOCAL_LDLIBS = $(NX_LIB_TOP)/libdeinterlacer_static.a


#########################################################################
#																		#
#								Target									#
#																		#
#########################################################################
LOCAL_MODULE		:= NxQuickRearCam
LOCAL_MODULE_TAGS	:= optional
LOCAL_CFLAGS 		:= -DANDROID
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)
LOCAL_POST_INSTALL_CMD := $(hide) mkdir -p $(TARGET_ROOT_OUT)/sbin

include $(BUILD_EXECUTABLE)
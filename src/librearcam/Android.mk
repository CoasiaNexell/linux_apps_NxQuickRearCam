LOCAL_PATH		:= $(call my-dir)

include $(CLEAR_VARS)

#########################################################################
#																		#
#								Sources									#
#																		#
#########################################################################
LOCAL_C_INCLUDES        += \
	external/libdrm \
	hardware/nexell/s5pxx18/gralloc	\
	device/nexell/library/nx-video-api/src \
	device/nexell/library/nx-renderer/include \
	$(LOCAL_PATH)/../common \
	$(LOCAL_PATH)/../../include \
	$(LOCAL_PATH)/../../include/drm \
	$(LOCAL_PATH)

#########################################################################
#																		#
#								Includes								#
#																		#
#########################################################################
LOCAL_SRC_FILES			:=  \
	NX_CV4l2Camera.cpp \
	NX_CV4l2VipFilter.cpp \
	NX_CDeinterlaceFilter.cpp \
	NX_CVideoRenderFilter.cpp \
	nxp_video_alloc_drm.c \

#BackGear Detection
LOCAL_SRC_FILES			+= \
	NX_CGpioControl.cpp \
	NX_CBackgearDetect.cpp \

#Utils
LOCAL_SRC_FILES			+= \
	../common/NX_DbgMsg.cpp \

# Manager
LOCAL_SRC_FILES			+= \
	NX_CRearCamManager.cpp \

#########################################################################
#																		#
#								Build optios							#
#																		#
#########################################################################
LOCAL_CFLAGS	+= -DTHUNDER_DEINTERLACE_TEST

#########################################################################
#																		#
#								Target									#
#																		#
#########################################################################
LOCAL_MODULE		:= libnxrearcam
LOCAL_MODULE_PATH	:= $(LOCAL_PATH)

LOCAL_MODULE_TAGS	:= optional

include $(BUILD_STATIC_LIBRARY)
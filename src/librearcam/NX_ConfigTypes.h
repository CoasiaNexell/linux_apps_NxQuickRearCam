//------------------------------------------------------------------------------
//
//	Copyright (C) 2016 Nexell Co. All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module		:
//	File		:
//	Description	:
//	Author		:
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#ifndef __NX_CONFIGTYPES_H__
#define __NX_CONFIGTYPES_H__

#ifdef __cplusplus

//
//	Format Definition
//
enum {
	NX_PIN_NONE,
	NX_PIN_INPUT,
	NX_PIN_OUTPUT,
};

enum {
	NX_TYPE_NONE,
	NX_TYPE_VIDEO,
	NX_TYPE_AUDIO,
	NX_TYPE_TEXT,
	NX_TYPE_STREAM,
};

enum {
	NX_FORMAT_NONE,
	NX_FORMAT_RGB,
	NX_FORMAT_YUV,
	NX_FORMAT_H264,
	NX_FORMAT_MJPEG,
	NX_FORMAT_PCM,
	NX_FORMAT_AAC,
	NX_FORMAT_MP3,
	NX_FORMAT_TEXT,
	NX_FORMAT_MUXED
};

enum {
	NX_CONTAINER_NONE,
	NX_CONTAINER_MP4,
	NX_CONTAINER_TS,
};

enum {
	NX_CODEC_NONE,
	NX_CODEC_H264,
	NX_CODEC_MP3,
	NX_CODEC_AAC,
};

typedef struct _NX_PIN_INFO {
	int32_t			iIndex;
	int32_t			iDirection;
} NX_PIN_INFO;

typedef struct _NX_FILTER_INFO {
	char*			pFilterId;
	char*			pFilterName;
	int32_t			iInPinNum;
	int32_t 		iOutPinNum;
} NX_FILTER_INFO;


typedef struct _NX_MEDIA_INFO {
	//	Common Config
	int32_t			iMediaType;
	int32_t			iCodec;
	int32_t			iBitrate;
	int32_t			iContainerType;

	//	Video Config

	//int32_t			iPort;			// for UVC Port Select. ( 0, 1, 2, ... )
	//int32_t			iMux;			// for VIP Port Select. ( 0, 1, 2, ... )
	//int32_t			iModule;
	//int32_t			iSensor;		// for VIP Port Select. ( 0, 1 --> NX_VIP_SENSOR0, NX_VIP_SENSOR1 )
	//int32_t			bUseMipi;		// for VIP Port Select. ( 0, 1 )
	//int32_t			iClipper;		// for VIP Port Select. ( 0, 1 --> NX_VIP_CLIPPER0, NX_VIP_CLIPPER1 )
	//int32_t			iWidth;
	//int32_t			iHeight;
	//int32_t			iFrameRate;
	//float			fEdgeParam;

	int32_t		iModule;
	int32_t		iSensorId;
	int32_t		iBusFormat;
	int32_t		bInterlace;

	int32_t		iWidth;			//	Camera Input Width
	int32_t		iHeight;		//	Camera Input Height

	int32_t		iFpsNum;		//	Frame per seconds's Numerate value
	int32_t		iFpsDen;		//	Frame per seconds's Denominate value

	int32_t 	iNumPlane;		//	Input image's plane number

	int32_t		iCropX;			//	Cliper x
	int32_t		iCropY;			//	Cliper y
	int32_t		iCropWidth;		//	Cliper width
	int32_t		iCropHeight;	//	Cliper height

	int32_t		iOutWidth;		//	Decimator width
	int32_t		iOutHeight;		//	Decimator height

	float			fEdgeParam;



	//	Audio Config
	int32_t			iChannel;
	int32_t			iSampleRate;
	int32_t			iSampleFormat;
	int32_t			iFrameSize;

	//	Internal Config
	int32_t			iFormat;
	int8_t*			pSeqData;
	int32_t			iSeqSize;

} NX_MEDIA_INFO;


//
//	For EventMessage
//
enum {
	//
	//	Application Notify. ( Event, Error .. )
	//
	NOTIFY_CREATE_FILE				= 0x1001,
	NOTIFY_FILEWRITE_DONE			= 0x1002,
	NOTIFY_CAPTURE_DONE				= 0x1003,

	NOTIFY_ERR_VIDEO_INPUT			= 0xA001,
	NOTIFY_ERR_VIDEO_ENCDOE			= 0xA002,
	NOTIFY_ERR_FILE_OPEN			= 0xA003,
	NOTIFY_ERR_FILE_WRITE			= 0xA004,

	//
	//	Internal Message
	//
	NOTIFY_INTERNAL					= 0xF000,
	NOTIFY_VIDEO_FRAME_READY		= 0xF001,
};

#endif	// __cplusplus

#endif	// __NX_CONFIGTYPES_H__

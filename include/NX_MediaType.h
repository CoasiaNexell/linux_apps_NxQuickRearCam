#ifndef _NX_MediaType_h__
#define _NX_MediaType_h__

#ifdef _WIN32
#include <windows.h>
#define NXFILEHANDLE	HANDLE
#	define _PACKED_
#endif

#ifdef _LINUX
#include <stdlib.h>  // malloc
#include <string.h>  // memset
#	define NXFILEHANDLE	int
#	define _PACKED_		__attribute__((packed))
#endif

//-----------------------------------------------------------------------------
//	Media architecture's result value
#ifndef NX_HANDLE
#define NX_HANDLE		void*
#endif
#ifndef NX_RESULT
#define NX_RESULT		S32
#endif


#ifndef __NX_TYPE_H__
#define __NX_TYPE_H__

//------------------------------------------------------------------------------
// basic data types
//------------------------------------------------------------------------------
/// @brief 8bit signed integer(s.7) value
typedef char    		S8;
/// @brief 16bit signed integer(s.15) value
typedef short   		S16;
/// @brief 32bit signed integer(s.31) value
typedef int   			S32;
/// @brief 64bit signed integer(s.63) value
typedef long long		S64;

/// @brief 8bit unsigned integer value
typedef unsigned char	U8;
/// @brief 16bit unsigned integer value
typedef unsigned short	U16;
/// @brief 32bit unsigned integer value
typedef unsigned int	U32;
/// @brief 64bit unsigned integer value
typedef unsigned long long	U64;

//------------------------------------------------------------------------------
// boolean type
//------------------------------------------------------------------------------
/// @brief boolean type
typedef int		CBOOL;
/// @brief		true (boolean type)
///	@remarks	this is not 1 but true
#define CTRUE	(0==0)
/// @brief		false (boolean type)
///	@remarks	this is not 0 but true
#define CFALSE	(0!=0)

//------------------------------------------------------------------------------
// NULL
//------------------------------------------------------------------------------
// @brief	null pointer
#	define CNULL 0
#endif  // __NX_TYPE_H__

#endif	//	_NX_MediaType_h__

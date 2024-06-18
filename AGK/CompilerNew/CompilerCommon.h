// Common Header
#ifndef _H_COMMON_
#define _H_COMMON_

// Platform : Windows
#ifdef _WINDOWS
	
	
	// disable deprecation warning
	#pragma warning(disable:4995)
	#pragma warning(disable:4996)

	#include <windows.h>
	#include <stdio.h>
	#include <math.h>
	#include <cassert>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <wininet.h>
	#include <time.h>

	#ifndef INT64
		#define INT64 __int64
	#endif

	#ifdef DeleteFile
		#undef DeleteFile
	#endif

	#define INTPTR uintptr_t
#endif

// Platform : Mac
#ifdef IDE_MAC

	#import <Cocoa/Cocoa.h>

	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <string.h>
	#include <math.h>
	#include <sys/time.h>
	#include <mach/mach_time.h>
	#include <mach-o/dyld.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>	
	#include <sys/param.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
	#include <sys/stat.h>

	#define BYTE unsigned char

	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#define STDCALL
	#define MAX_PATH 1024

	// these are used in interpreter, can replace them if need be
	#define DWORD UINT
	#define LONGLONG UINT
	#define LPSTR char*

	#ifndef INT64
		#define INT64 long
	#endif

	#define INTPTR long

#endif

#ifdef IDE_LINUX
	#define BYTE unsigned char
	
	#include <ctype.h>
	#include <unistd.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <assert.h>
	#include <math.h>
	#include <time.h>

	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
	#include <sys/stat.h>
	#include <utime.h>
	#include <fcntl.h>
	#include <pthread.h>
	#include <stdarg.h>
	#include <stdint.h>

	#define MAX_PATH 512
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#define STDCALL

	// these are used in interpreter, can replace them if need be
	#define DWORD UINT
	#define LONGLONG UINT
	#define LPSTR char*

	#ifndef INT64
		#define INT64 long
	#endif

    #ifndef UINT_PTR
        #define UINT_PTR uintptr_t
    #endif
#endif

#ifdef IDE_PI
	#include <ctype.h>
	#include <unistd.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <assert.h>
	#include <math.h>
	#include <time.h>

	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
	#include <sys/stat.h>
	#include <utime.h>
	#include <fcntl.h>
	#include <pthread.h>
	#include <stdarg.h>
	#include <stdint.h>

	#ifndef _EXPORT_
		#define _EXPORT_
	#endif

	#define MAX_PATH 512
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#define STDCALL

	// these are used in interpreter, can replace them if need be
	#define DWORD UINT
	#define LONGLONG UINT
	#define LPSTR char*

	#ifndef INT64
		#define INT64 long
	#endif

    #ifndef UINT_PTR
        #define UINT_PTR uintptr_t
    #endif
#endif

#ifdef AGK_IOS
	#define _AGK_ERROR_CHECK

    #import <Foundation/Foundation.h>

    #include <stdio.h>
    #include <stdlib.h>
	#include <string.h>
	#include <stdarg.h>
	#include <math.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
	#include <sys/stat.h>
	#include <utime.h>

	#ifndef _EXPORT_
		#define _EXPORT_ 
	#endif

	#define STDCALL

	#define MAX_PATH 1024

	#ifndef INT64
		#define INT64 __int64_t
	#endif

    #ifndef UINT_PTR
        #define UINT_PTR uintptr_t
    #endif
#endif

#ifdef AGK_ANDROID
	#include <stdio.h>
	#include <string.h>
	#include <jni.h>
	#include <stdlib.h>
	#include <math.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
	#include <sys/stat.h>
	#include <utime.h>
	#include <fcntl.h>
	#include <unistd.h>
	
	#ifndef _EXPORT_
		#define _EXPORT_
	#endif

	#define MAX_PATH 512
	#define STDCALL

	#define INT64 int64_t
    #ifndef UINT_PTR
        #define UINT_PTR uintptr_t
    #endif

	#ifndef __ARM_ARCH__
	#  if defined __ARM_ARCH_8__   || defined __ARM_ARCH_8A__ || \
		  defined __ARM_ARCH_8R__  || defined __ARM_ARCH_8M__ || defined __ARM_ARCH_ISA_A64
	#    define __ARM_ARCH__ 8
	#
	#  elif defined __ARM_ARCH_7__   || defined __ARM_ARCH_7A__ || \
		  defined __ARM_ARCH_7R__  || defined __ARM_ARCH_7M__
	#    define __ARM_ARCH__ 7
	#
	#  elif defined __ARM_ARCH_6__   || defined __ARM_ARCH_6J__ || \
		  defined __ARM_ARCH_6K__  || defined __ARM_ARCH_6Z__ || \
		  defined __ARM_ARCH_6KZ__ || defined __ARM_ARCH_6T2__
	#    define __ARM_ARCH__ 6
	#
	#  elif defined __ARM_ARCH_5__ || defined __ARM_ARCH_5T__ || \
			defined __ARM_ARCH_5TE__ || defined __ARM_ARCH_5TEJ__
	#    define __ARM_ARCH__ 5
	#  else 
	#    define __ARM_ARCH__ 4
	#  endif
	#endif
#endif

#ifndef UINT
    #define UINT unsigned int
#endif

namespace AGK_Compiler
{
	void Error( const char *msg, int lineNum=0, const char *includeFile=0 );
	void Warning( const char *msg, int lineNum=0, const char *includeFile=0 );
}

#endif // _H_COMMON_

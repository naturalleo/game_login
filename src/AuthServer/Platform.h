/*
 * Platform.h
 *
 *  Created on: 2013-1-23
 *      Author: root
 */
#pragma once
#define WINDOWS32 0x00000032
#define WINDOWS64 0x00000064
#define LINUX32	0x00010032
#define LINUX64 0x00010064

#ifdef WIN32
#define PLATFORM WINDOWS32
#define WINDOWS
#endif
#ifdef WIN64
#define PLATFORM WINDOWS64
#define WINDOWS
#endif

#if PLATFORM == WINDOWS32 || PLATFORM == WINDOWS64
#pragma comment(lib, "Winmm.lib")
#include <WinSock2.h>
#include <Windows.h>
#define WINDOWS
typedef int _w64 int64;
#define UI64FMTD "%UI64"
// TLS
#define __thread __declspec(thread)
#endif

///linux
#ifndef PLATFORM
#define PLATFORM LINUX64
#endif

#if PLATFORM == LINUX64
typedef long int INT_PTR, *PINT_PTR;
typedef unsigned long int UINT_PTR;
#elif PLATFORM == LINUX32
typedef int INT_PTR, *PINT_PTR;
typedef unsigned int UINT_PTR;
#endif

#if PLATFORM == LINUX64 || PLATFORM == LINUX32
#define LINUX
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <memory.h>
#include <wchar.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <iconv.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#define INVALID_HANDLE_VALUE 0
#define MAXWORD 0xffff
#define FALSE   0
#define TRUE    1

#ifndef NULL
#define NULL 0
#endif

#define CONST const
//typedef double LONGLONG;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef long long __int64;
typedef int LONG;
typedef unsigned int DWORD;
typedef DWORD DWORD_PTR;
typedef size_t SIZE_T;
typedef long int int64;
typedef long int INT64;
typedef unsigned long ULONG_PTR;
typedef unsigned long ULONG;
typedef void* LPVOID, PVOID;
typedef void VOID;
typedef const void* LPCVOID, PCVOID;
typedef void* HANDLE;
typedef unsigned short WORD;
typedef unsigned short  *PWORD;
typedef const char *LPCTSTR, *LPCSTR;
typedef char *LPTSTR, *LPSTR;
typedef char TCHAR;
typedef char CHAR;
typedef char BYTE;
typedef wchar_t *LPWSTR;
typedef int BOOL;
typedef int INT;
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDRIN, * PSOCKADDRIN;
typedef int MMRESULT;
typedef unsigned int UINT;
typedef long long *PINT64;
typedef int *PINT;
typedef int INT32;
typedef double DOUBLE;
typedef struct sockaddr_in SOCKADDR_IN, *PSOCKADDR_IN;
typedef void *(*LPTHREAD_START_ROUTINE)(void*);

#define __FUNCTION__ __func__
#define timeBeginPeriod
///线程相关
typedef struct RTL_CRITICAL_SECTION
{
	pthread_mutexattr_t attr;
	pthread_mutex_t mtx;
}CRITICAL_SECTION, *LPCRITICAL_SECTION;
#define InitializeCriticalSection(T) {pthread_mutexattr_init(&(T)->attr);\
                                     pthread_mutexattr_settype(&(T)->attr, PTHREAD_MUTEX_RECURSIVE);\
                                     pthread_mutex_init(&(T)->mtx, &(T)->attr);}
#define DeleteCriticalSection(T) pthread_mutex_destroy(&(T)->mtx)
#define EnterCriticalSection(T) pthread_mutex_lock(&(T)->mtx)
#define TryEnterCriticalSection(T) !pthread_mutex_trylock(&(T)->mtx)
#define LeaveCriticalSection(T) pthread_mutex_unlock(&(T)->mtx)
#define InterlockedCompareExchange(Destination, newvalue, oper) __sync_val_compare_and_swap(Destination, oper, newvalue)
#define InterlockedExchangeAdd(V, P) __sync_fetch_and_add(V, P)
#define InterlockedIncrement(T) __sync_fetch_and_add(T, 1)
#define InterlockedDecrement(T) __sync_sub_and_fetch(T, 1)
#define GetCurrentThreadId() syscall(SYS_gettid)
#define WaitForSingleObject(P, T,C) waitpid(P,T,C)
#define	CloseThread(ht)		 { if ( ht ) {waitpid( ht, 0, 0 ); pthread_cancel( ht ); (ht) = NULL;} }
#define ExitThread(T) pthread_exit(T)
#define DebugBreak() assert(false);

#define UI64FMTD "%lld"
#define ERROR_SUCCESS                    0L
#define _tcsncicmp(T, P, C) strncasecmp(T, P, C)
#define __try try
#define __finally catch(...)
#define MAX_PATH          260
#define WINAPI
#define INVALID_SOCKET  (int)(~0)
#define SOCKET_ERROR            (-1)
#define ZeroMemory(Destination, Length) memset(Destination, 0, Length)
#define WSAGetLastError() errno
#define GetLastError() errno
//#define ioctlsocket(socket, mode, p) ioctl(socket, mode, p)
#define OUT
#define IN
#define __in_opt
//#define __in gnc 出错
#define MAXUINT     ((UINT)~((UINT)0))
#define MAXINT      ((INT)(MAXUINT >> 1))
#define MININT      ((INT)~MAXINT)
#define __out_opt
#define WSAEWOULDBLOCK	EAGAIN
#define closesocket(socket) {::shutdown(socket, SHUT_RDWR); ::close(socket);}
#define _vsntprintf vsnprintf
#define _vsnprintf vsnprintf
#define _vsnwprintf vswprintf
#define _sntprintf snprintf
#define _tcslen(STR) strlen(STR)
#define _tcscpy strcpy
#define _tcscmp strcmp
#define _tcschr strchr
#define _tcsncpy strncpy
#define _stricmp strcasecmp
#define  strnicmp strncasecmp
#define INFINITE 0
#define  _tcsnicmp strncasecmp
#define  stricmp strcasecmp
#define sprintf_s snprintf
#define _T(T) T
#define _tprintf printf
#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))
/*
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define min(a,b)  (((a) < (b)) ? (a) : (b))*/

#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

#if PLATFORM == LINUX32
#define EXTEN_CALL __attribute__((stdcall))
#define CALLBACK __attribute__((pascal))
#endif

#if PLATFORM == LINUX64
#define EXTEN_CALL
#define CALLBACK 0xFFFFFFFF
#define __cdecl
#endif

///精度秒
inline void GetLocalTime(SYSTEMTIME *pSysTime)
{
	if(!pSysTime)
	{
		return;
	}
	time_t t = time(0);
	struct tm _tm;
	localtime_r(&t, &_tm);
	pSysTime->wYear	 	= _tm.tm_year + 1900;
	pSysTime->wMonth 	= _tm.tm_mon + 1;
	pSysTime->wDay	 	= _tm.tm_mday;
	pSysTime->wHour	 	= _tm.tm_hour;
	pSysTime->wMinute	= _tm.tm_min;
	pSysTime->wSecond	= _tm.tm_sec;
	pSysTime->wMilliseconds = 0;
}

inline size_t CovertCode(char* pSrc, size_t srcLen, char* pDestBuf, size_t destSize, char* pTo, char* pFrom)
{
	if(!pDestBuf || !destSize || !pSrc || !srcLen || !pTo || !pFrom)
		{
			return -1;
		}
		iconv_t ct = iconv_open(pTo, pFrom);
		size_t c = iconv(ct, (char**)(&pSrc), &srcLen, (char**)(&pDestBuf), (&destSize));
		iconv_close(ct);
		return c;
}
inline DWORD timeGetTime()
{
	struct timeval ti;
	gettimeofday(&ti, 0);
	return ti.tv_sec * 1000 + ti.tv_usec / 1000;
}

inline unsigned long GetTickCount()
{
	 struct timespec ts;
	 clock_gettime(CLOCK_MONOTONIC, &ts);
	 return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

inline char *itoa(int v, char *buf, int i = 10)
{
	sprintf(buf, "%d", v);
	return buf;
}

template <class _Ty, class _Pr>
inline const _Ty& max(const _Ty& ty, const _Pr& pr)
{
	return ty >= pr ? ty : (_Ty)(pr);
}


#endif


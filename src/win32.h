/* This file is part of MCF Gthread.
 * See LICENSE.TXT for licensing information.
 * Copyleft 2022, LH_Mouse. All wrongs reserved.  */

#ifndef __MCFGTHREAD_WIN32_H_
#define __MCFGTHREAD_WIN32_H_

#include "fwd.h"

/* Request Windows 7.  */
#ifndef _WIN32_WINNT
#  define _WIN32_WINNT  0x0601
#endif

#if _WIN32_WINNT < 0x0601
#  error Please define `_WIN32_WINNT` to at least Windows 7.
#endif

#define WIN32_LEAN_AND_MEAN  1
#define NOMINMAX  1
#define NOCOMM 1
#include <windows.h>
#include <ntstatus.h>
#include <winternl.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __MCF_WIN32_EXTERN_INLINE
#  define __MCF_WIN32_EXTERN_INLINE  __MCF_GNU_INLINE
#endif

/* Add some attributes to existent functions.  */
DWORD __stdcall
GetCurrentThreadId(void)
  __attribute__((__dllimport__, __nothrow__, __const__));

HANDLE __stdcall
GetCurrentThread(void)
  __attribute__((__dllimport__, __nothrow__, __const__));

DWORD __stdcall
GetCurrentProcessId(void)
  __attribute__((__dllimport__, __nothrow__, __const__));

HANDLE __stdcall
GetCurrentProcess(void)
  __attribute__((__dllimport__, __nothrow__, __const__));

DWORD __stdcall
GetLastError(void)
  __attribute__((__dllimport__, __nothrow__, __pure__));

VOID __stdcall
SetLastError(DWORD __code)
  __attribute__((__dllimport__, __nothrow__));

LPVOID __stdcall
TlsGetValue(DWORD __index)
  __attribute__((__dllimport__, __nothrow__, __pure__));

INT __stdcall
TlsSetValue(DWORD __index, LPVOID __value)
  __attribute__((__dllimport__, __nothrow__));

/* Declare some NTDLL functions that are not available here.  */
NTSTATUS __stdcall
NtWaitForKeyedEvent(HANDLE __event, const void* __key, BOOLEAN __alertable, LARGE_INTEGER* __timeout)
  __attribute__((__dllimport__, __nothrow__));

NTSTATUS __stdcall
NtReleaseKeyedEvent(HANDLE __event, const void* __key, BOOLEAN __alertable, LARGE_INTEGER* __timeout)
  __attribute__((__dllimport__, __nothrow__));

NTSTATUS __stdcall
NtDelayExecution(BOOLEAN __alertable, LARGE_INTEGER* __timeout)
  __attribute__((__dllimport__, __nothrow__));

NTSTATUS __stdcall
NtWaitForSingleObject(HANDLE __object, BOOLEAN __alertable, LARGE_INTEGER* __timeout)
  __attribute__((__dllimport__, __nothrow__));

BOOLEAN __stdcall
RtlDllShutdownInProgress(void)
  __attribute__((__dllimport__, __nothrow__));

/* Declare helper functions here.  */
EXCEPTION_DISPOSITION __cdecl
__MCF_seh_top(EXCEPTION_RECORD* __record, void* __frame, CONTEXT* __ctx, void* __disp_ctx)
  __attribute__((__nothrow__));

#ifdef __i386__  /* SEH is stack-based  */

typedef struct __MCF_i386_seh_node __MCF_i386_seh_node;

struct __MCF_i386_seh_node
  {
    DWORD __next;  /* `__MCF_i386_seh_node*`  */
    DWORD __filter;  /* `typeof(__MCF_seh_top)*`  */
  };

__MCF_ALWAYS_INLINE
void
__MCF_i386_seh_install(__MCF_i386_seh_node* __seh_node) __MCF_NOEXCEPT
  {
    __seh_node->__next = __readfsdword(0U);
    __seh_node->__filter = (DWORD) __MCF_seh_top;
    __writefsdword(0U, (DWORD) __seh_node);
  }

__MCF_ALWAYS_INLINE
void
__MCF_i386_seh_cleanup(__MCF_i386_seh_node* __seh_node) __MCF_NOEXCEPT
  {
    __writefsdword(0U, __seh_node->__next);
  }

#  define __MCF_SEH_DEFINE_TERMINATE_FILTER  \
    __MCF_i386_seh_node __MCF_PPCAT(__MCF_seh_node_, __LINE__)  \
           __MCF_USE_DTOR(__MCF_i386_seh_cleanup);  \
    __MCF_i386_seh_install(  \
           &(__MCF_PPCAT(__MCF_seh_node_, __LINE__)))  /* no semicolon  */

#else  /* SEH is stack-based  ^/v  SEH is table-based  */

#  define __MCF_SEH_DEFINE_TERMINATE_FILTER  \
    __asm__ volatile (".seh_handler __MCF_seh_top, @except;")  /* no semicolon  */

#endif  /* SEH is table-based  */

LARGE_INTEGER*
__MCF_initialize_timeout(LARGE_INTEGER* __li, const int64_t* __int64_opt)
  __attribute__((__nothrow__));

__MCF_WIN32_EXTERN_INLINE
LARGE_INTEGER*
__MCF_initialize_timeout(LARGE_INTEGER* __li, const int64_t* __int64_opt)
  {
    if(__int64_opt == NULL)
      return NULL;  /* wait infinitely  */

    if(*__int64_opt > 910692730085477)
      return NULL;  /* overflowed; assume infinity  */

    if(*__int64_opt < -922337203685477)
      return NULL;  /* overflowed; assume infinity  */

    /* If `*__int64_opt` is positive, it denotes the number of milliseconds
     * since 1970-01-01T00:00:00Z, and has to be converted into the number of
     * 100 nanoseconds since the 1601-01-01T00:00:00Z.  */
    int64_t __relative = *__int64_opt >> 63;
    __li->QuadPart = ((~__relative & 11644473600000) + *__int64_opt) * 10000;
    return __li;
  }

size_t
__MCF_batch_release_common(const void* __key, size_t __count)
  __attribute__((__nothrow__));

__MCF_WIN32_EXTERN_INLINE
size_t
__MCF_batch_release_common(const void* __key, size_t __count)
  {
    size_t __k;

    if(__count == 0)
      return 0;

    /* A call to `ExitProcess()` terminates all the other threads, even
     * if they are waiting. Don't release the keyed event in this case,
     * as it blocks the calling thread infinitely if there is no thread
     * to wake up. See <https://github.com/lhmouse/mcfgthread/issues/21>.  */
    if(RtlDllShutdownInProgress())
      return 0;

    for(__k = 0;  __k != __count;  ++__k) {
      /* Release a thread. This operation shall block until the target
       * thread has received the notification.  */
      NTSTATUS __status = NtReleaseKeyedEvent(NULL, __key, FALSE, NULL);
      __MCFGTHREAD_ASSERT(NT_SUCCESS(__status));
    }

    /* Return the number of threads that have been woken.  */
    return __count;
  }

/* Undefine macros that redirect to standard functions.
 * This ensures we call the ones from KERNEL32.  */
#undef RtlCopyMemory
#undef RtlMoveMemory
#undef RtlFillMemory
#undef RtlZeroMemory
#undef RtlCompareMemory
#undef RtlEqualMemory

void __stdcall
RtlMoveMemory(void* __dst, const void* __src, SIZE_T __size)
  __attribute__((__dllimport__, __nothrow__));

void __stdcall
RtlFillMemory(void* __dst, SIZE_T __size, int __val)
  __attribute__((__dllimport__, __nothrow__));

void __stdcall
RtlZeroMemory(void* __dst, SIZE_T __size)
  __attribute__((__dllimport__, __nothrow__));

SIZE_T __stdcall
RtlCompareMemory(const void* __s1, const void* __s2, SIZE_T __size)
  __attribute__((__dllimport__, __pure__, __nothrow__));

/* Copy a block of memory, like `memcpy()`.  */
void* __cdecl
__MCF_mcopy(void* __restrict__ __dst, const void* __restrict__ __src, size_t __size) __MCF_NOEXCEPT;

__MCF_WIN32_EXTERN_INLINE
void* __cdecl
__MCF_mcopy(void* __restrict__ __dst, const void* __restrict__ __src, size_t __size) __MCF_NOEXCEPT
  {
    __MCFGTHREAD_ASSERT(__size <= (uintptr_t) __dst - (uintptr_t) __src);
#if defined(__i386__) || defined(__amd64__)
    typedef char __mem[];
    uintptr_t __di, __si, __cx;
    __asm__ (
      "rep movsb;"
      : "=m"(*(__mem*)__dst), "=D"(__di), "=S"(__si), "=c"(__cx)
      : "m"(*(const __mem*)__src), "D"(__dst), "S"(__src), "c"(__size)
    );
#else
    /* Call the generic but slower version in NTDLL.  */
    RtlMoveMemory(__dst, __src, __size);
#endif
    return __dst;
  }

/* Copy a block of potentially overlapped memory, like `memmove()`.  */
void* __cdecl
__MCF_mmove(void* __dst, const void* __src, size_t __size) __MCF_NOEXCEPT;

__MCF_WIN32_EXTERN_INLINE
void* __cdecl
__MCF_mmove(void* __dst, const void* __src, size_t __size) __MCF_NOEXCEPT
  {
#if defined(__i386__) || defined(__amd64__)
    typedef char __mem[];
    uintptr_t __di, __si, __cx;
    if(__size <= (uintptr_t) __dst - (uintptr_t) __src)
      __asm__ (
        "rep movsb;"  /* go forward  */
        : "=m"(*(__mem*)__dst), "=D"(__di), "=S"(__si), "=c"(__cx)
        : "m"(*(const __mem*)__src), "D"(__dst), "S"(__src), "c"(__size)
      );
    else
      __asm__ (
        "std;"
        "rep movsb;"  /* go backward  */
        "cld;"
        : "=m"(*(__mem*)__dst), "=D"(__di), "=S"(__si), "=c"(__cx)
        : "m"(*(const __mem*)__src), "D"((char*) __dst + __size - 1),
          "S"((const char*) __src + __size - 1), "c"(__size)
      );
#else
    /* Call the generic but slower version in NTDLL.  */
    RtlMoveMemory(__dst, __src, __size);
#endif
    return __dst;
  }

/* Fill a block of memory with the given byte, like `memset()`.  */
void* __cdecl
__MCF_mfill(void* __dst, int __val, size_t __size) __MCF_NOEXCEPT;

__MCF_WIN32_EXTERN_INLINE
void* __cdecl
__MCF_mfill(void* __dst, int __val, size_t __size) __MCF_NOEXCEPT
  {
#if defined(__i386__) || defined(__amd64__)
    typedef char __mem[];
    uintptr_t __di, __cx;
    __asm__ (
      "rep stosb;"
      : "=m"(*(__mem*)__dst), "=D"(__di), "=c"(__cx)
      : "D"(__dst), "a"(__val), "c"(__size)
    );
#else
    /* Call the generic but slower version in NTDLL.  */
    RtlFillMemory(__dst, __size, __val);
#endif
    return __dst;
  }

/* Fill a block of memory with zeroes, like `bzero()`.  */
void* __cdecl
__MCF_mzero(void* __dst, size_t __size) __MCF_NOEXCEPT;

__MCF_WIN32_EXTERN_INLINE
void* __cdecl
__MCF_mzero(void* __dst, size_t __size) __MCF_NOEXCEPT
  {
#if defined(__i386__) || defined(__amd64__)
    typedef char __mem[];
    uintptr_t __di, __cx;
    __asm__ (
      "rep stosb;"
      : "=m"(*(__mem*)__dst), "=D"(__di), "=c"(__cx)
      : "D"(__dst), "a"(0), "c"(__size)
    );
#else
    /* Call the generic but slower version in NTDLL.  */
    RtlZeroMemory(__dst, __size);
#endif
    return __dst;
  }

/* Check whether two blocks of memory compare equal, like `memcmp() == 0`.
 * The result is a boolean value.  */
uint8_t __cdecl
__MCF_mequal(const void* __src, const void* __cmp, size_t __size) __MCF_NOEXCEPT
  __attribute__((__pure__));

__MCF_WIN32_EXTERN_INLINE
uint8_t __cdecl
__MCF_mequal(const void* __src, const void* __cmp, size_t __size) __MCF_NOEXCEPT
  {
    uint8_t __result;
#if defined(__i386__) || defined(__amd64__)
    typedef char __mem[];
    uintptr_t __si, __di, __cx;
    __asm__ (
      "xorl %%eax, %%eax;"
      "repz cmpsb;"
#  ifdef __GCC_ASM_FLAG_OUTPUTS__
      : "=@ccz"(__result), "=S"(__si), "=D"(__di), "=c"(__cx)
      : "m"(*(const __mem*)__src), "m"(*(const __mem*)__cmp),
        "S"(__src), "D"(__cmp), "c"(__size)
      : "ax"
#  else  /* __GCC_ASM_FLAG_OUTPUTS__  */
      "setzb %%al;"
      : "=a"(__result), "=S"(__si), "=D"(__di), "=c"(__cx)
      : "m"(*(const __mem*)__src), "m"(*(const __mem*)__cmp),
        "S"(__src), "D"(__cmp), "c"(__size)
      : "cc"
#  endif  /* __GCC_ASM_FLAG_OUTPUTS__  */
    );
#else
    /* Call the generic but slower version in NTDLL.  */
    SIZE_T __n = RtlCompareMemory(__src, __cmp, __size);
    __result = __n == __size;
#endif
    return __result;
  }

/* Compare two blocks of memory, like `memcmp()`.  */
int __cdecl
__MCF_mcomp(const void* __src, const void* __cmp, size_t __size) __MCF_NOEXCEPT
  __attribute__((__pure__));

__MCF_WIN32_EXTERN_INLINE
int __cdecl
__MCF_mcomp(const void* __src, const void* __cmp, size_t __size) __MCF_NOEXCEPT
  {
    int __result;
#if defined(__i386__) || defined(__amd64__)
    typedef char __mem[];
    uintptr_t __si, __di, __cx;
    __asm__ (
      "xorl %%eax, %%eax;"
      "repz cmpsb;"
      "setnzb %%al;"
      "sbbl %%ecx, %%ecx;"
      "orl %%ecx, %%eax;"
      : "=a"(__result), "=S"(__si), "=D"(__di), "=c"(__cx)
      : "m"(*(const __mem*)__src), "m"(*(const __mem*)__cmp),
        "S"(__src), "D"(__cmp), "c"(__size)
      : "cc"
    );
#else
    /* Call the generic but slower version in NTDLL.  */
    SIZE_T __n = RtlCompareMemory(__src, __cmp, __size);
    if(__n != __size) {
      __result = *((const uint8_t*)__src + __n) - *((const uint8_t*)__cmp + __n);
      __MCFGTHREAD_ASSERT(__result != 0);
    }
    else
      __result = 0;
#endif
    return __result;
  }

/* Allocate a block of zeroed memory, like `calloc()`.  */
void*
__MCF_malloc_0(size_t __size) __MCF_NOEXCEPT
  __attribute__((__warn_unused_result__, __malloc__, __alloc_size__(1)));

__MCF_WIN32_EXTERN_INLINE
void*
__MCF_malloc_0(size_t __size) __MCF_NOEXCEPT
  {
    void* __ptr = HeapAlloc(__MCF_crt_heap, HEAP_ZERO_MEMORY, __size);
    return __ptr;
  }

/* Re-allocate a block of memory, like `realloc()`. If the existent
 * block should be extended, vacuum bytes are filled with zeroes.  */
void*
__MCF_mrealloc_0(void** __restrict__ __pptr, size_t __size) __MCF_NOEXCEPT
  __attribute__((__warn_unused_result__, __alloc_size__(2)));

__MCF_WIN32_EXTERN_INLINE
void*
__MCF_mrealloc_0(void** __restrict__ __pptr, size_t __size) __MCF_NOEXCEPT
  {
    void* __ptr = HeapReAlloc(__MCF_crt_heap, HEAP_ZERO_MEMORY, *__pptr, __size);
    return !__ptr ? NULL : (*__pptr = __ptr);
  }

/* Allocate a copy of a block of memory, like `malloc()` followed by
 * `memcpy()`.  */
void*
__MCF_malloc_copy(const void* __data, size_t __size) __MCF_NOEXCEPT
  __attribute__((__warn_unused_result__, __alloc_size__(2)));

__MCF_WIN32_EXTERN_INLINE
void*
__MCF_malloc_copy(const void* __data, size_t __size) __MCF_NOEXCEPT
  {
    void* __ptr = HeapAlloc(__MCF_crt_heap, 0, __size);
    return !__ptr ? NULL : __MCF_mcopy(__ptr, __data, __size);
  }

/* Get the size of an allocated block, like `malloc_usable_size()`.  */
size_t
__MCF_msize(const void* __ptr) __MCF_NOEXCEPT
  __attribute__((__pure__));

__MCF_WIN32_EXTERN_INLINE
size_t
__MCF_msize(const void* __ptr) __MCF_NOEXCEPT
  {
    size_t __size = HeapSize(__MCF_crt_heap, 0, __ptr);
    __MCFGTHREAD_ASSERT(__size != (size_t)-1);
    return __size;
  }

/* Free a block of memory, like `free()`.  */
void
__MCF_mfree(void* __ptr) __MCF_NOEXCEPT;

__MCF_WIN32_EXTERN_INLINE
void
__MCF_mfree(void* __ptr) __MCF_NOEXCEPT
  {
    if(!__ptr)
      return;

#ifdef __MCF_DEBUG
    __MCF_mfill(__ptr, 0xFE, HeapSize(__MCF_crt_heap, 0, __ptr));
#endif
    int __succ = HeapFree(__MCF_crt_heap, 0, __ptr);
    __MCFGTHREAD_ASSERT(__succ);
  }

#ifdef __cplusplus
}
#endif

#endif  /* __MCFGTHREAD_WIN32_H_  */

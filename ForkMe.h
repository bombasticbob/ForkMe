//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//              _____             _     __  __           _                  //
//             |  ___|___   _ __ | | __|  \/  |  ___    | |__               //
//             | |_  / _ \ | '__|| |/ /| |\/| | / _ \   | '_ \              //
//             |  _|| (_) || |   |   < | |  | ||  __/ _ | | | |             //
//             |_|   \___/ |_|   |_|\_\|_|  |_| \___|(_)|_| |_|             //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//          Copyright (c) 2019 by S.F.T. Inc. - All rights reserved         //
//  Use, copying, and distribution of this software are licensed according  //
//    to the GPLv2, LGPLv2, or BSD license, as appropriate (see COPYING)    //
//                                                                          //
//                                                                          //
//  Pretty much all of this code is derived from the X11workbenchProject,   //
//  which has been licensed in a similar manner.                            //
//                                                                          //
//  repo:  github.com/bombasticbob/ForkMe                                   //
//  see also: github.com/bombasticbob/X11workbenchProject                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////



#ifndef __FORKME_H_INCLUDED__
#define __FORKME_H_INCLUDED__

/** \defgroup ForkMe The 'Fork Me' utilities
  *
  * Functions and definitions associated with the 'Fork Me' process control utilities
**/

#include <stdarg.h> /* stdarg to make sure I have va_list and other important stuff */
#include <stdint.h> /* for standard integer types like uint32_t */
#include <pthread.h> /* make sure this is included for POSIX */


/** \ingroup ForkMe
  * @{
**/

/** \brief file handle abstraction
**/
#define WB_FILE_HANDLE int

/** \brief process ID abastraction
**/
#define WB_PROCESS_ID unsigned int

/** \brief invalid file handle abstraction
**/
#define WB_INVALID_FILE_HANDLE -1

/** \brief 'invalid handle' abstraction
**/
#define INVALID_HANDLE_VALUE -1


#if defined(__GNUC__) || defined(__DOXYGEN__)
// ======================================================================================
// this section is used for doxygen documentation
// Additionally, 'clang' compilers also define '__GNUC__' and will use this section


/** \brief optimization for code branching when condition is 'unlikely'.  use within conditionals
 **
 ** \code
    if(WB_UNLIKELY(x == 5))
    {
      // branch optimized to bypass this code when condition is FALSE
    }
 ** \endcode
**/
#define WB_UNLIKELY(x) (__builtin_expect (!!(x), 0))
/** \brief optimization for code branching when condition is 'likely'.  use within conditionals
 **
 ** \code
    if(WB_LIKELY(x == 5))
    {
      // branch optimized to execute this code when condition is TRUE
    }
 ** \endcode
**/
#define WB_LIKELY(x) (__builtin_expect (!!(x), 1))

#else

#define WB_UNLIKELY(x) (x)
#define WB_LIKELY(x) (x)

#endif // defined(__GNUC__) || defined(__DOXYGEN__)


/**
  * \brief 'STRINGIZE' macro, make a string out of the parameter AFTER substitution
**/
#define WB_STRINGIZE(X) WB_STRINGIZE_INTERNAL(X)
/**
  * \brief helper for 'STRINGIZE' macro
**/
#define WB_STRINGIZE_INTERNAL(X) #X

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// TYPEDEFS

/** \brief Platform abstract 32-bit integer
  *
  * This definition identifies the data type for a 32-bit integer
**/
typedef int WB_INT32;

/** \brief Platform abstract unsigned 32-bit integer
  *
  * This definition identifies the data type for an unsigned 32-bit integer
**/
typedef unsigned int WB_UINT32;

/** \brief Platform abstract 64-bit integer
  *
  * This definition identifies the data type for a 64-bit integer
**/
typedef long long WB_INT64;

/** \brief Platform abstract unsigned 64-bit integer
  *
  * This definition identifies the data type for an unsigned 64-bit integer
**/
typedef unsigned long long WB_UINT64;

#if defined(__LP64__) || defined(__DOXYGEN__)
/** \brief Platform abstract integer pointer
  *
  * This definition identifies the data type for an unsigned integer that's the size of a pointer
**/
typedef WB_UINT64 WB_UINTPTR;
#elif __SIZEOF_POINTER__ == 4 /* 4-byte pointer */
typedef WB_UINT32 WB_UINTPTR;
#else // assume long long pointer
typedef WB_UINT64 WB_UINTPTR;
#endif // __LP64__

/** \brief MODULE HANDLE equivalent
  *
  * This 'typedef' refers to a MODULE
**/
typedef void * WB_MODULE;

/** \brief THREAD HANDLE equivalent
  *
  * This 'typedef' refers to a THREAD
**/
typedef pthread_t WB_THREAD;

/** \brief PROC ADDRESS equivalent
  *
  * This 'typedef' refers to a PROC ADDRESS as exported from a shared library
**/
typedef void (* WB_PROCADDRESS)(void);

/** \brief THREAD LOCAL STORAGE 'key' equivalent
  *
  * This 'typedef' refers to a THREAD LOCAL STORAGE key, identifying a storage slot
**/
typedef pthread_key_t   WB_THREAD_KEY;

/** \brief CONDITION HANDLE equivalent (similar to an 'event')
  *
  * This 'typedef' refers to a CONDITION, a triggerable synchronization resource
**/
typedef WB_UINT32 WB_COND; // defined as 'WB_UINT32' because of pthread_cond problems under Linux
//typedef pthread_cond_t  WB_COND;

/** \brief MUTEX HANDLE equivalent
  *
  * This 'typedef' refers to a MUTEX, a lockable synchronization object
**/
typedef pthread_mutex_t WB_MUTEX;

/** \brief MODULE HANDLE equivalent
  *
  * This 'typedef' refers to a MODULE
**/
typedef void * WB_MODULE;

/** \brief THREAD HANDLE equivalent
  *
  * This 'typedef' refers to a THREAD
**/
typedef pthread_t WB_THREAD;

/** \brief PROC ADDRESS equivalent
  *
  * This 'typedef' refers to a PROC ADDRESS as exported from a shared library
**/
typedef void (* WB_PROCADDRESS)(void);

/** \brief THREAD LOCAL STORAGE 'key' equivalent
  *
  * This 'typedef' refers to a THREAD LOCAL STORAGE key, identifying a storage slot
**/
typedef pthread_key_t   WB_THREAD_KEY;

/** \brief CONDITION HANDLE equivalent (similar to an 'event')
  *
  * This 'typedef' refers to a CONDITION, a triggerable synchronization resource
**/
typedef WB_UINT32 WB_COND; // defined as 'WB_UINT32' because of pthread_cond problems under Linux
//typedef pthread_cond_t  WB_COND;

/** \brief MUTEX HANDLE equivalent
  *
  * This 'typedef' refers to a MUTEX, a lockable synchronization object
**/
typedef pthread_mutex_t WB_MUTEX;


typedef char * WB_PSTR;         ///< pointer to char string - a convenience typedef
typedef const char * WB_PCSTR;  ///< pointer to const char string - a convenience typedef


/** \brief Get Time Index
  *
  * returns 'time index' associated with current 'UNIX time' in microseconds
**/
WB_UINT64 WBGetTimeIndex(void);


/** \brief efficient delay
  *
  * efficient delay in microseconds
**/
void WBDelay(uint32_t uiDelay);  // approximate delay for specified period (in microseconds).  may be interruptible


/** \brief copy string
  *
  * make malloc'd copy of string
**/
char *WBCopyString(const char *pSrc);

/** \brief copy string
  *
  * make malloc'd copy of string up to 'n' chars
**/
char *WBCopyStringN(const char *pSrc, unsigned int nMaxChars);

/** \brief concatenate strings
  *
  * make malloc'd concatenation of 2 strings
**/
void WBCatString(char **ppDest, const char *pSrc);

/** \brief concatenate strings
  *
  * make malloc'd concatenation of 2 strings, up to 'n characters within 2nd string
**/
void WBCatStringN(char **ppDest, const char *pSrc, unsigned int nMaxChars);



/** \brief make directory using flags
  *
  * make directory using flags, returns zero on success
**/
int WBMkDir(const char *szFileName, int flags);

/** \brief search path for file
  *
  * searches PATH environment var for specified file name, returns malloc'd string of canonical name
**/
char * WBSearchPath(const char *szFileName);

/** \brief create temporary file with 'szExt' file extension (does not register it)
**/
char * WBTempFile0(const char *szExt);

/** \brief create temporary file with 'szExt' file extension
**/
char * WBTempFile(const char *szExt);


/** \brief Run an application asynchronously
  *
  * \param szAppName A const pointer to a character string containing the path to the application
  * \returns A valid process ID or process handle, depending upon the operating system.  On error it returns WB_INVALID_FILE_HANDLE
  *
  * Use this function to spawn an asynchronous process.  The function returns an invalid process ID
  * or process handle on error.  If the process ID is an allocated resource, the caller must free it.
  * Each additional parameter passed to this function is a parameter that is to be passed to the program.
  * The final parameter in the list must be NULL, so any call to this function will need to have at
  * least 2 parameters.
  *
  * Header File:  platform_helper.h
**/
WB_PROCESS_ID WBRunAsync(const char *szAppName, ...);

/** \brief Check on a running process, and return its state, and optionally the exit code
  *
  * \param idProcess A WB_PROCESS_ID for the running process
  * \param pExitCode An optional pointer to a WB_INT32 to retrieve the exit code (may be NULL)
  * \return A positive value if the process is still running, zero if the process has terminated, negative on error.
  *
  * Call this function to determine if an asynchronous process is still running, and possibly retrieve
  * the exit code if it is no longer running.  On POSIX systems, the exit code is generally a single byte
  * (other values may be reserved).  On Windows, it's officially an unsigned 32-bit integer.
  *
  * If a process is suspended, killed, or exits due to a system error, this function will not differentiate
  * between normal or abnormal termination, or between running or suspended.  If you need this kind of
  * complex application status, use 'waitpid()' (POSIX) or GetExitCodeProcess() (Windows).
  *
  * This function does not perform any explicit wait states when checking a process's state.  If you must block
  * your thread's execution in order to wait for a process to exit, you should call WBSleep() (or its equivalent)
  * within the loop to avoid 'maxing out' the CPU utilization, which would very likely use up more electricity
  * than a more efficient wait state.
  *
  * See Also:  WBRunAsync(), WBRunAsyncPipe(), WBRunAsyncPipeV()
**/
int WBGetProcessState(WB_PROCESS_ID idProcess, WB_INT32 *pExitCode);

/** \brief Run an application synchronously, returning 'stdout' output in a character buffer.
  *
  * \param szAppName A const pointer to a character string containing the path to the application
  * \returns A WBAlloc() pointer to a buffer containing the 'stdout' output from the application, or NULL on error.
  *
  * Use this function to run an external process and capture its output.  This function will ignore the
  * error return code from the program, so if this information is necessary, you should write a different
  * function (based on this one) using 'WBRunAsync' and a wait loop, etc. that checks the application's
  * return value on exit.
  * Each additional parameter passed to this function is a parameter that is to be passed to the program.
  * The final parameter in the list must be NULL, so any call to this function will need to have at
  * least 2 parameters.
  * On error this function returns a NULL value.  Any non-NULL value must be 'free'd by the caller using WBFree().
  *
  * Header File:  platform_helper.h
**/
char * WBRunResult(const char *szAppName, ...);

/** \brief Run an application synchronously, supplying an input buffer for 'stdin', and returning 'stdout' output in a character buffer.
  *
  * \param szStdInBuf A const pointer to 0-byte terminated string/buffer containing the input for piped data.
  * \param szAppName A const pointer to a character string containing the path to the application
  * \returns A WBAlloc() pointer to a buffer containing the 'stdout' output from the application.
  *
  * Use this function to run an external process, providing a buffer that contains data to be sent to the
  * applications 'stdin', and in the process, capture its output.  This function will ignore the
  * error return code from the program, so if this information is necessary, you should write a different
  * function (based on this one) using 'WBRunAsync' and a wait loop, etc. that checks the application's
  * return value on exit.
  *
  * Each additional parameter passed to this function is a parameter that is to be passed to the program.
  * The final parameter in the list must be NULL, so any call to this function will need to have at
  * least 3 parameters.
  *
  * On error this function returns a NULL value.  Any non-NULL value must be 'free'd by the caller using WBFree().\n
  *
  * To create simple piped output, pass the result of the previous 'WBRunResult' or 'WBRunResultWithInput()' as the
  * 'szStdInBuf' parameter to a subsequent 'WBRunResultWithInput()' call.
  *
  * Header File:  platform_helper.h
**/
char * WBRunResultWithInput(const char *szStdInBuf, const char *szAppName, ...);

/** \brief Run an application asynchronously, specifying file handles for STDIN, STDOUT, and STDERR
  *
  * \param hStdIn A WB_FILE_HANDLE for STDIN, or WB_INVALID_FILE_HANDLE
  * \param hStdOut A WB_FILE_HANDLE for STDOUT, or WB_INVALID_FILE_HANDLE
  * \param hStdErr A WB_FILE_HANDLE for STDERR, or WB_INVALID_FILE_HANDLE.  This can be the same handle as hStdErr, though interleaved output may not occur as expected.
  * \param szAppName A const pointer to a character string containing the path to the application
  * \returns A valid process ID or process handle, depending upon the operating system.  On error, it returns WB_INVALID_FILE_HANDLE
  *
  * Use this function to spawn an asynchronous process in which you want to track STDIN, STDOUT,
  * and/or STDERR.  The function returns an invalid process ID or process handle
  * on error.  If the process ID is an allocated resource, the caller must free it.
  * Each additional parameter passed to this function will become a parameter that is to be passed to the program.
  * The final parameter in the list must be NULL to mark the end of the list, so any call to this function will
  * need to have at least 5 parameters.\n
  * \n
  * If you do not want to re-direct a file handle, pass 'WB_INVALID_FILE_HANDLE' for its value.  It is
  * also possible to pass the SAME file handle for hStdIn, hStdOut, and hStdErr provided that it has
  * the correct read/write access available.  File handles passed to this function will be duplicated,
  * but not closed.  It is safe (and prudent) to close the original file handles immediately after calling this
  * function.\n
  * \n
  * You can monitor 'WB_PROCESS_ID' to find out if the process is running.  Additionally, you can use
  * the output of hStdOut and hStdErr by re-directing them to anonymous pipes and monitoring their activity.
  *
  * Header File:  platform_helper.h
**/
WB_PROCESS_ID WBRunAsyncPipe(WB_FILE_HANDLE hStdIn, WB_FILE_HANDLE hStdOut, WB_FILE_HANDLE hStdErr,
                             const char *szAppName, ...);

/** \brief Run an application asynchronously, specifying file handles for STDIN, STDOUT, and STDERR, using a va_list for the program's parameters
  *
  * \param hStdIn A WB_FILE_HANDLE for STDIN, or WB_INVALID_FILE_HANDLE
  * \param hStdOut A WB_FILE_HANDLE for STDOUT, or WB_INVALID_FILE_HANDLE
  * \param hStdErr A WB_FILE_HANDLE for STDERR, or WB_INVALID_FILE_HANDLE.  This can be the same handle as hStdErr, though interleaved output may not occur as expected.
  * \param szAppName A const pointer to a character string containing the path to the application
  * \param va A va_list of the arguments (the final one must be NULL)
  * \returns A valid process ID or process handle, depending upon the operating system.  On error, it returns WB_INVALID_FILE_HANDLE
  *
  * This function is identical to WBRunAsyncPipe() except that the variable argument list is passed as a va_list.\n
  * Use this function to spawn an asynchronous process in which you want to track STDIN, STDOUT,
  * and/or STDERR.  The function returns an invalid process ID or process handle
  * on error.  If the process ID is an allocated resource, the caller must free it.
  * Parameters passed to this function as part of the va_list are parameters that are to be passed to the program.
  * The final parameter in the va_list must be NULL to mark the end of the list.\n
  * If you do not want to re-direct a file handle, pass 'WB_INVALID_FILE_HANDLE' for its value.  It is
  * also possible to pass the SAME file handle for hStdIn, hStdOut, and hStdErr provided that it has
  * the correct read/write access available.  File handles passed to this function will be duplicated,
  * but not closed.  It is also safe to close the original file handles immediately after calling this
  * function.\n
  * You can monitor 'WB_PROCESS_ID' to find out if the process is running.  Additionally, you can use
  * the output of hStdOut and hStdErr by re-directing them to anonymous pipes and monitoring their activity.
  *
  * This function is used internally by the other process control functions, and is defined here in case
  * you need to write a customized version of one of the process control functions.  A typical example might
  * be the use of stderr rather than stdout for WBRunResult().
  *
  * Header File:  platform_helper.h
**/
WB_PROCESS_ID WBRunAsyncPipeV(WB_FILE_HANDLE hStdIn, WB_FILE_HANDLE hStdOut, WB_FILE_HANDLE hStdErr,
                              const char *szAppName, va_list va);



/** \brief Loads a shared library, DLL, module, or whatever you call it on your operating system
  *
  * \param szModuleName A const pointer to a character string containing the path for the library, module, DLL, or whatever
  * \returns A valid WB_MODULE module handle, depending upon the operating system
  *
  * This function is identical to LoadLibrary() under Windows, and is a wrapper for 'dlopen()' on POSIX systems.
  *
  * Use this to dynamically load a shared library (or DLL under Windows) at run-time from within your application.
  *
  * In POSIX systems, the 'dlopen' function will invoke '_init()' within the shared library, if it is present, the
  * first time the library is mapped into the address space.  In Windows, the LoadLibrary() function calls
  * 'DllMain()' within the DLL, with one of various parameters, typically DLL_PROCESS_ATTACH.  The semantics are
  * much more complicated for Windows (see MSDN documentation for 'DllMain()')
  *
  * Additionally, for POSIX systems, the 'dlopen()' function is called with access set to (RTLD_LAZY | RTLD_LOCAL).
  * This implies that a) each external reference is resolved when you call the appropriate function; and
  * b) Only symbols defined in the library and it's "DAG" of needed objects will be resolvable [default behavior].
  * If you want a different behavior, you can call 'dlopen()' yourself, and cast the return value to a WB_MODULE.
  *
  * Certain library and API functions may not be available in '_init()' or 'DllMain()' - see appropriate documentation
  * for any restrictions that might apply to your shared libraries (DLLs).
  *
  * See also: WBGetProcAddress(), WBFreeLibrary()
  *
  * Header File:  platform_helper.h
**/
WB_MODULE WBLoadLibrary(const char *szModuleName); // load a library module (shared lib, DLL, whatever)

/** \brief Frees a shared library, DLL, module, or whatever, that was loaded via 'WBLoadLibrary()'
  *
  * \param hModule A valid WB_MODULE module handle, as returned by WBLoadLibrary()
  *
  * This function is identical to FreeLibrary() under Windows, and is a wrapper for 'dlfree()' on POSIX systems.
  *
  * Use this to free a shared library (or DLL) that was opened with WBLoadLibrary().
  *
  * In POSIX systems, the 'dlfree' function will invoke '_fini()' within the shared library, if it is present, the
  * when the library is finally unmapped from the address space.  In Windows, the FreeLibrary() function calls
  * 'DllMain()' within the DLL, with one of various parameters, typically DLL_PROCESS_DETACH.  The semantics are
  * much more complicated for Windows (see MSDN documentation for 'DllMain()')
  *
  * Certain library and API functions may not be available in '_fini()' or 'DllMain()' - see appropriate documentation
  * for any restrictions that might apply to your shared libraries (DLLs).
  *
  * Header File:  platform_helper.h
**/
void WBFreeLibrary(WB_MODULE hModule);

/** \brief Obtains a function pointer for an exported function symbol in a shared library (or DLL)
  *
  * \param hModule A valid WB_MODULE module handle, as returned by WBLoadLibrary()
  * \param szProcName A const pointer to a character string containing the proc name
  * \returns A function pointer as a 'WB_PROCADDRESS' type, or NULL if the symbol does not exist
  *
  * This function is identical to GetProcAddress() under Windows, and is a wrapper for 'dlfunc()' or 'dlsym()' on POSIX systems
  *
  * Header File:  platform_helper.h
**/
WB_PROCADDRESS WBGetProcAddress(WB_MODULE hModule, const char *szProcName);

/** \brief Obtains a data pointer for an exported data symbol in a shared library (or DLL)
  *
  * \param hModule A valid WB_MODULE module handle, as returned by WBLoadLibrary()
  * \param szDataName A const pointer to a character string containing the data name
  * \returns A pointer to the data item, or NULL if the symbol does not exist.
  *
  * This function is identical to GetProcAddress() under Windows, with the return data type cast
  * to 'void *'. It is a wrapper for 'dlsym()' on POSIX systems.
  *
  * Header File:  platform_helper.h
**/
void * WBGetDataAddress(WB_MODULE hModule, const char *szDataName);


/** \brief Allocate 'thread local' storage
  *
  * \returns The 'key' that identifies the thread local storage data slot
  *
  * Allocate thread local storage, returning the identifier to that local storage slot
  *
  * Header File:  platform_helper.h
**/
WB_THREAD_KEY WBThreadAllocLocal(void);

/** \brief Free 'thread local' storage allocated by WBThreadAllocLocal()
  *
  * \returns The 'key' that identifies the thread local storage data slot
  *
  * Free an allocate thread local storage slot
  *
  * Header File:  platform_helper.h
**/
void WBThreadFreeLocal(WB_THREAD_KEY keyVal);

/** \brief Get 'thread local' data identified by 'keyVal'
  *
  * \param keyVal the 'WB_THREAD_KEY' identifier of the thread local data - see WBThreadAllocLocal()
  * \returns The stored thread-specific data value, or NULL if not assigned
  *
  * Get the data associated with a thread local storage slot
  *
  * Header File:  platform_helper.h
**/
void * WBThreadGetLocal(WB_THREAD_KEY keyVal);

/** \brief Get 'thread local' data identified by 'keyVal'
  *
  * \param keyVal the 'WB_THREAD_KEY' identifier of the thread local data - see WBThreadAllocLocal()
  * \param pValue the value to assign for thread local data
  *
  * Assign (set) the data associated with a thread local storage slot
  *
  * Header File:  platform_helper.h
**/
void WBThreadSetLocal(WB_THREAD_KEY keyVal, void *pValue);


// simplified thread support (cross-platform)

/** \brief Get 'current thread' identifier
  *
  * \returns The WB_THREAD associated with the current thread
  *
  * Returns a 'WB_THREAD' resource representing the current thread.  This returned value should
  * NOT be free'd using WBThreadClose(), nor waited on with WBThreadWait().
  *
  * Header File:  platform_helper.h
**/
WB_THREAD WBThreadGetCurrent(void);

/** \brief Create a new thread, returning its WB_THREAD identifier
  *
  * \param function A pointer to the callback function that runs the thread
  * \param pParam The parameter to be passed to 'function' when it start
  * \returns A WB_THREAD thread identifier, or INVALID_HANDLE_VALUE on error
  *
  * Call this function to create a new thread using standard attributes.
  *
  * Header File:  platform_helper.h
**/
WB_THREAD WBThreadCreate(void *(*function)(void *), void *pParam);

/** \brief Wait for a specified threat to exit
  *
  * \param hThread the WB_THREAD identifier
  * \returns The return value from the thread's thread proc or 'WBThreadExit()'
  *
  * Call this function to wait for a thread to complete and/or obtain its exit code.
  * This function will block until the thread has terminated or is canceled (pthreads only).
  *
  * Header File:  platform_helper.h
**/
void *WBThreadWait(WB_THREAD hThread);        // closes hThread, returns exit code, waits for thread to terminate (blocks)

/** \brief Determine whether a thread is running (can be suspended)
  *
  * \param hThread the WB_THREAD identifier
  * \returns A value > 0 if the thread is running, < 0 on error, == 0 if the thread has ended and has a return code.
  *
  * Use this function to determine whether a thread is still running.  For a thread that has exited, it will return 0
  * indicating that there is an exit code available.  If the thread has terminated (invalidating its WB_THREAD identifier)
  * the function will return a value < 0.  otherwise, The return value is > 0 indicating the thread is still active.\n
  * NOTE:  an active thread that has been suspended will return a value > 0.
  *
  * Header File:  platform_helper.h
**/
int WBThreadRunning(WB_THREAD hThread);

/** \brief Exit the current thread immediately, specifying return code
  *
  * \param pRval The return code for the exiting thread
  *
  * Call this function to exit the current thread immediately, specifying a return code\n
  * NOTE:  when the thread proc returns, it implies a call to WBThreadExit() on completion, using the return
  *        value as the exit code.
  *
  * Header File:  platform_helper.h
**/
void WBThreadExit(void *pRval);

/** \brief Close the specified WB_THREAD identifier
  *
  * \param hThread the WB_THREAD identifier
  *
  * Call this function to close the WB_THREAD thread identifier specified by 'hThread'.  Not closing the
  * thread identifier can result in a 'zombie' thread that consumes resources.  By closing the handle, you
  * can allow a thread to run to its completion, and automatically delete the associated resources on exit.
  * You should not call this function after a call to WBThreadWait()\n
  * NOTE:  internally it calls either pthread_detach or CloseHandle (depending)
  *
  * Header File:  platform_helper.h
**/
void WBThreadClose(WB_THREAD hThread);


// CONDITIONS AND OTHER SYNC OBJECTS


/** \brief Create a signallable condition
  *
  * \param pCond A pointer to the WB_COND that will be created (could be a struct or just a pointer)
  * \returns A zero value if successful; non-zero on error
  *
  * Use this function to create a 'condition' that can be signaled using WBCondSignal()\n
  * This is roughly the equivalent of an 'Event' object on MS Windows
  *
  * Header File:  platform_helper.h
**/
int WBCondCreate(WB_COND *pCond);

/** \brief Create a lockable mutex
  *
  * \param pMtx A pointer to the WB_MUTEX that will be created (could be a struct or just a pointer)
  * \returns A zero value if successful; non-zero on error
  *
  * Use this function to create a 'mutex' synchronization object that can be locked by only a single thread at a time
  *
  * Header File:  platform_helper.h
**/
int WBMutexCreate(WB_MUTEX *pMtx);

/** \brief Free a signallable condition
  *
  * \param pCond a pointer to the WB_COND signallable condition
  *
  * Use this function to free a WB_COND that was previously allocated with WBCondCreate()
  *
  * Header File:  platform_helper.h
**/
void WBCondFree(WB_COND *pCond);

/** \brief Free a lockable mutex
  *
  * \param pMtx a pointer to the WB_MUTEX lockable mutex object
  *
  * Use this function to free a WB_MUTEX that was previously allocated with WBMutexCreate()
  *
  * Header File:  platform_helper.h
**/
void WBMutexFree(WB_MUTEX *pMtx);

/** \brief Wait for and lock a mutex, blocking until it is available
  *
  * \param pMtx a pointer to the WB_MUTEX lockable mutex object
  * \param nTimeout the timeout period in microseconds, or a negative value to indicate 'INFINITE'
  * \returns A zero if the lock succeeded, a value > 0 if the lock period timed out, or a negative value indicating error
  *
  * This function attempts to lock the WB_MUTEX and returns a zero value if it succeeds, blocking for the period
  * of time specified by 'nTimeout' (in microseconds).  A negative 'nTimeout' causes an infinite waiting period.
  * The function will return a positive value if the timeout period was exceeded, or a negative value on error.
  *
  * Header File:  platform_helper.h
**/
int WBMutexLock(WB_MUTEX *pMtx, int nTimeout);

/** \brief Unlock a previously locked mutex
  *
  * \param pMtx a pointer to the WB_MUTEX lockable mutex object
  * \returns A zero if the unlock succeeded, non-zero on error
  *
  * This function unlocks a previously locked mutex
  *
  * Header File:  platform_helper.h
**/
int WBMutexUnlock(WB_MUTEX *pMtx);

/** \brief Signal a condition (event)
  *
  * \param pCond a pointer to the WB_COND condition object
  * \returns A zero if the signal succeeded, or non-zero on error
  *
  * This function signals a condition so that a waiting process will 'wake up'
  * see WBCondWait() and WBCondWaitMutex()
  *
  * Header File:  platform_helper.h
**/
int WBCondSignal(WB_COND *pCond);

/** \brief Wait for a signal on a condition (event)
  *
  * \param pCond a poiner to the WB_COND condition object
  * \param nTimeout the timeout (in microseconds), or a value < 0 to indicate 'INFINITE'
  * \returns A zero if the condition was triggered, a value > 0 on timeout, or a value < 0 on error
  *
  * This function waits up to a specified time (in microseconds), or indefinitely if the specified
  * wait time is negative, until the condition object 'hCond' has been signaled.  See WBCondSignal()
  *
  * Header File:  platform_helper.h
**/
int WBCondWait(WB_COND *pCond, int nTimeout);

/** \brief Wait for a signal on a condition (event)
  *
  * \param pCond a pointer to the WB_COND condition object
  * \param pMtx a pointer to a locked WB_MUTEX object
  * \param nTimeout the timeout (in microseconds), or a value < 0 to indicate 'INFINITE'
  * \returns A zero if the condition was triggered, a value > 0 on timeout, or a value < 0 on error
  *
  * This function waits up to a specified time (in microseconds), or indefinitely if the specified
  * wait time is negative, until the condition object 'hCond' has been signaled.  See WBCondSignal()\n
  * Additionally, when the waiting process begins, the WB_MUTEX referenced by 'hMtx' will be unlocked,
  * until the condition referenced by 'hCond' has been signaled, or the timeout period has been exceeded.
  * At that point, the mutex 'hMtx' will be re-locked (waiting indefinitely for the lock to be successful)\n
  * Upon return, 'hMtx' will be locked again by the calling thread.  'hMtx' must be already locked before
  * calling this function.  It will remain unlocked during the wait state.
  *
  * Header File:  platform_helper.h
**/
int WBCondWaitMutex(WB_COND *pCond, WB_MUTEX *pMtx, int nTimeout);


/** \brief Interlocked 'atomic' decrement of an unsigned integer
  *
  * \param pValue - a pointer to an 'unsigned int' to be decremented atomically.  Must be a valid pointer
  * \returns The new value stored in 'pValue' after decrementing
  *
  * This function performs an interlocked 'atomic' decrement of an unsigned integer,
  * guaranteeing that at the time the value is decremented, no other thread is allowed
  * to read or modify the value until the function returns.
  *
  * Header File:  platform_helper.h
**/
WB_UINT32 WBInterlockedDecrement(volatile WB_UINT32 *pValue);


/** \brief Interlocked 'atomic' increment of an unsigned integer
  *
  * \param pValue - a pointer to an 'unsigned int' to be incremented atomically.  Must be a valid pointer
  * \returns The new value stored in 'pValue' after incrementing
  *
  * This function performs an interlocked 'atomic' increment of an unsigned integer,
  * guaranteeing that at the time the value is incremented, no other thread is allowed
  * to read or modify the value until the function returns.
  *
  * Header File:  platform_helper.h
**/
WB_UINT32 WBInterlockedIncrement(volatile WB_UINT32 *pValue);


/** \brief Interlocked 'atomic' exchange of an unsigned integer with a specified value
  *
  * \param pValue - a pointer to an 'unsigned int' to be exchanged with the specified value.  Must be a valid pointer.
  * \param nNewVal - the new value to assign to 'pValue' atomically
  * \returns The old value previously stored in 'pValue' after exchanging
  *
  * This function performs an interlocked 'atomic' assignment of an unsigned integer,
  * guaranteeing that at the time the value is assigned, no other thread is allowed
  * to read or modify the value until the function returns.  The previous value is returned
  * by the function, effectively 'exchanging' the value with one that you specify.
  *
  * Header File:  platform_helper.h
**/
WB_UINT32 WBInterlockedExchange(volatile WB_UINT32 *pValue, WB_UINT32 nNewVal);


/** \brief Interlocked 'atomic' read of an unsigned integer
  *
  * \param pValue - a pointer to an 'unsigned int' to be read.  Must be a valid pointer.
  * \returns The value stored in 'pValue' after reading.  The value will be read while write operations have been 'locked out'
  *
  * This function performs an interlocked 'atomic' read of an unsigned integer,
  * guaranteeing that at the time the value is read, no other thread is allowed
  * to modify the value.  Once read, the value can still change; however, the value
  * as-read will be 'atomic' i.e. not a partially changed value.
  *
  * Header File:  platform_helper.h
**/
WB_UINT32 WBInterlockedRead(volatile WB_UINT32 *pValue);


// FILES


/** \brief read a file's contents into a buffer, returning the length of the buffer
  *
  * \param szFileName A const pointer to a string containing the file name
  * \param ppBuf A pointer to a 'char *' buffer that is allocated via WBAlloc() and returned by the function
  * \returns a positive value on success indicating the length of the data in the returned buffer, or negative on error.
  *  A return value of zero indicates an empty file.
  *
  * Use this function to read the entire contents of a file into a memory buffer.
  *
  * header file:  file_help.h
**/
size_t WBReadFileIntoBuffer(const char *szFileName, char **ppBuf);

/** \brief read a file's contents into a buffer, returning the length of the buffer
  *
  * \param szFileName A const pointer to a string containing the file name
  * \param pBuf A const pointer to a buffer that contains the data to write
  * \param cbBuf The length of data to write to the file.
  * \returns a value of zero on success, or non-zero on error (the actual error should be in 'errno')
  *
  * Use this function to write the entire contents of a buffer to a file.  The file will be overwritten
  * if it already exists.
  *
  * header file:  file_help.h
**/
int WBWriteFileFromBuffer(const char *szFileName, const char *pBuf, size_t cbBuf);


// SYSTEM INDEPENDENT FILE STATUS, LISTINGS, AND INFORMATION

/** \brief replicate permissions on a target file based on another file's permissions
  *
  * \param szProto The file to be used as a permission 'prototype'.  If it represents a symbolic
  * link, the file that the link points to will be used.
  * \param szTarget The target file.  If it represents a symbolic link, the link itself will be used.
  * \returns non-zero on error, zero on success
  *
  * Use this function to replicate the permissions of one file onto another.  Useful when
  * making a backup of an original file, and/or creating a new file with the same name/characteristics
  * as the old file, but not necessarily 'written in place'.
  *
  * header file:  file_help.h
**/
int WBReplicateFilePermissions(const char *szProto, const char *szTarget);

/** \brief Return allocated string containing the current working directory
  *
  * \returns valid pointer or NULL on error
  *
  * A convenience function that wraps the 'getcwd()' API and returns a 'WBAlloc'd pointer to a string.
  * The caller must free any non-NULL return values using WBFree()
  *
  * header file:  file_help.h
**/
char *WBGetCurrentDirectory(void);

/** \brief Return whether a file is a directory or a symlink to a directory
  *
  * \param szName The file name to query. Can be a symbolic link.  The name [and target path for a symlink] must exist.
  * \returns non-zero if the file is (or the symlink points to) a directory, 0 otherwise
  *
  * Use this function to determine if the specified file is a directory, or is a symbolic
  * link that points to a directory.
  *
  * header file:  file_help.h
**/
int WBIsDirectory(const char *szName);

/** \brief Return the canonical path for a file name (similar to POSIX 'realpath()' funtion)
  *
  * \param szFileName The file name to query.  If the name contains symbolic links, they will be resolved.
  * \returns A WBAlloc'd buffer containing the canonical path, or NULL on error.
  *
  * Use this function to determine the canonical name for a specified path.  If the path does not exist,
  * those elements of the path that DO exist will be resolved, and the remaining parts of the name will
  * be added to form the canonical path.  If an element of the specified directory name is NOT actually
  * a directory (or a symlink to a directory), the function will return NULL indicating an error.
  *
  * The caller must free any non-NULL return values using WBFree()
  *
  * header file:  file_help.h
**/
char *WBGetCanonicalPath(const char *szFileName);

/** \brief Allocate a 'Directory List' object for a specified directory spec
  *
  * \param szDirSpec The directory specification, using standard wildcard specifiers on the file spec only
  * \returns A pointer to a 'Directory List' object, or NULL on error
  *
  * Use this function to list a directory by creating a 'Directory List' object and
  * then subsequently passing it to 'WBNextDirectoryEntry' to obtain information about
  * each entry in the directory.  This function does NOT return '.' or '..' as valid
  * file names.\n
  *
  * The pointer returned by this function must be destroyed using \ref WBDestroyDirectoryList()\n
  *
  * NOTE:  you may not specify a wildcard within a directory name unless it is the the specification
  * for the file name that you are searching for within its parent directory.  The directory name
  * must also exist or the function will return a NULL (error).  If the specified path name ends
  * in a '/' the file spec will be assumed to be '*'.
  *
  * header file:  file_help.h
**/
void *WBAllocDirectoryList(const char *szDirSpec);

/** \brief Destroy a 'Directory List' object allocated by \ref WBAllocDirectoryList()
  *
  * \param pDirectoryList A pointer to a 'Directory List' object allocated by \ref WBAllocDirectoryList()
  *
  * Use this function to destroy a 'Directory List' object allocated by \ref WBAllocDirectoryList()
  *
  * header file:  file_help.h
**/
void WBDestroyDirectoryList(void *pDirectoryList);

/** \brief Obtain information about the next entry in a 'Directory List'
  *
  * \param pDirectoryList A pointer to the 'Directory List' object
  * \param szNameReturn A pointer to a buffer that receives the file name
  * \param cbNameReturn The size of the 'szNameReturn' buffer (in bytes)
  * \param pdwModeAttrReturn A pointer to an unsigned long integer that receives the mode/attribute bits
  * \returns An integer value < 0 on error, > 0 on EOF, or 0 if a matching entry was found
  *
  * Use this function to get the next file name from the contents of a directory listing that is
  * specified by a 'Directory List' object created by \ref WBAllocDirectoryList()
  *
  * header file:  file_help.h
**/
int WBNextDirectoryEntry(void *pDirectoryList, char *szNameReturn,
                         int cbNameReturn, unsigned long *pdwModeAttrReturn);

/** \brief Construct a fully qualified canonical path from a 'Directory List' object and a file name
  *
  * \param pDirectoryList A pointer to a 'Directory List' object.  May be NULL.
  * \param szFileName A pointer to a file within the directory.  This file does not need to exist.
  * \returns A 'WBAlloc'd buffer containing the fully qualified file name as an ASCII string.
  *
  * Use this function to get a canonical file name for a file within a directory listing.
  *
  * The caller must free any non-NULL return values using WBFree()
  *
  * header file:  file_help.h
**/
char *WBGetDirectoryListFileFullPath(const void *pDirectoryList, const char *szFileName);

/** \brief Obtain the target of a symbolic link
  *
  * \param szFileName A pointer to the file name of the symbolic link
  * \returns A 'WBAlloc'd buffer containing the link target path as an ASCII string (unaltered)
  *
  * Use this function to retrieve a symbolic link's target file name.  May be a relative path.
  * For a canonical equivalent, use WBGetCanonicalPath()
  *
  * The caller must free any non-NULL return values using WBFree()
  *
  * header file:  file_help.h
**/
char *WBGetSymLinkTarget(const char *szFileName);

/** \brief Obtain the target of a symbolic link file name with respect to a 'Directory List' object
  *
  * \param pDirectoryList A pointer to a 'Directory List' object.  May be NULL.
  * \param szFileName A pointer to the file name of the symbolic link
  * \returns A 'WBAlloc'd buffer containing the unmodified link target path as an ASCII string
  *
  * Use this function to retrieve a symbolic link's target file name with respect to a 'Directory List'
  * object (for relative paths).  The link target is unmodified, and may be a relative path.
  * For a canonical equivalent, use WBGetCanonicalPath()
  *
  * The caller must free any non-NULL return values using WBFree()
  *
  * header file:  file_help.h
**/
char *WBGetDirectoryListSymLinkTarget(const void *pDirectoryList, const char *szFileName);

/** \brief Obtain the 'stat' flags for a file name, resolving links as needed
  *
  * \param szFileName A pointer to the file name of the symbolic link
  * \param pdwModeAttrReturn A pointer to the unsigned long integer that receives the 'stat' flags
  * \returns A zero value if successful, non-zero otherwise (same return as 'stat()')
  *
  * Use this function to retrieve flags for a regular file, directory, or the target of a symbolic link
  *
  * header file:  file_help.h
**/
int WBStat(const char *szFileName, unsigned long *pdwModeAttrReturn);

/** \brief Obtain the 'stat' flags for a file name, resolving links as needed, with respect to a 'Directory List' object
  *
  * \param pDirectoryList A pointer to a 'Directory List' object.  May be NULL.
  * \param szFileName A pointer to the file name of the symbolic link
  * \param pdwModeAttrReturn A pointer to the unsigned long integer that receives the 'stat' flags
  * \returns A zero value if successful, non-zero otherwise (same return as 'stat()')
  *
  * Use this function to retrieve flags for a regular file, directory, or the target of a symbolic link with
  * respect to a 'Directory List' object (for relative paths)
  *
  * header file:  file_help.h
**/
int WBGetDirectoryListFileStat(const void *pDirectoryList, const char *szFileName,
                               unsigned long *pdwModeAttrReturn);


/** \brief Obtain the 'time_t' value for a file's modification date/time (unix time, seconds since the epoch)
  *
  * \param szFileName A pointer to the file name (must be fully qualified or relative to the current directory)
  * \returns A 64-bit unsigned integer value indicating the file's date/time as a UNIX integer time_t value.
  *
  * Use this function to obtain the modification date/time of a file.  useful to check if it was modified outside
  * of the current process, or by something else within the process.
  *
  * header file:  file_help.h
**/
unsigned long long WBGetFileModDateTime(const char *szFileName);                // return file mod date/time as time_t (seconds since epoch)


/** \brief Compare a 64-bit unsigned integer value against a file's modification date/time (unix time, seconds since the epoch)
  *
  * \param szFileName A pointer to the file name (must be fully qualified or relative to the current directory)
  * \param tVal An unsigned 64-bit integer value representing the file's previous modification date/time value
  * \returns An integer value indicating the comparison result of 'tVal' vs the current modification date/time of the file.
  *
  * Use this function to compare a previously obtained modification date/time value (via 'WBGetFileModDateTime()')
  * against the current value, which is obtained internally by calling the same function.
  *
  * This is mostly a convenience function, to make checking mod times look a lot cleaner in the code.
  *
  * header file:  file_help.h
**/
int WBCheckFileModDateTime(const char *szFileName, unsigned long long tVal);    // check time_t value against file mod date/time, return comparison



#ifdef __cplusplus
};
#endif // __cplusplus

/**
  * @}
**/

#endif // __FORKME_H_INCLUDED__


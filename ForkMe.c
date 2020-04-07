//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//               _____             _     __  __                             //
//              |  ___|___   _ __ | | __|  \/  |  ___     ___               //
//              | |_  / _ \ | '__|| |/ /| |\/| | / _ \   / __|              //
//              |  _|| (_) || |   |   < | |  | ||  __/ _| (__               //
//              |_|   \___/ |_|   |_|\_\|_|  |_| \___|(_)\___|              //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//          Copyright (c) 2019 by S.F.T. Inc. - All rights reserved         //
//  Use, copying, and distribution of this software are licensed according  //
//    to the GPLv2, LGPLv2, or BSD license, as appropriate (see COPYING)    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <limits.h>
//#include <math.h>
#include <sys/time.h>
#include <dlfcn.h> /* dynamic library support */
#include <dirent.h>
#include <fnmatch.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h> // for MAXPATHLEN and PATH_MAX (also includes limits.h in some cases)

#include "ForkMe.h"

// some defines - use 'malloc' and 'free' as-is

#define WBAlloc(X) malloc(X)
#define WBReAlloc(X,Y) realloc(X,Y)
#define WBFree(X) free(X)

// turn off the basic debug stuff
#ifndef WB_ERROR_PRINT
#define WB_ERROR_PRINT(X, ...)
#endif // WB_ERROR_PRINT
#ifndef WB_WARN_PRINT
#define WB_WARN_PRINT(X, ...)
#endif // WB_WARN_PRINT


// some basic utilities

WB_UINT64 WBGetTimeIndex(void)
{
struct timeval tv;

  gettimeofday(&tv, NULL);

  return (WB_UINT64)tv.tv_sec * (WB_UINT64)1000000
         + (WB_UINT64)tv.tv_usec;
}

void WBDelay(uint32_t uiDelay)  // approximate delay for specified period (in microseconds).  may be interruptible
{
//#ifdef HAVE_NANOSLEEP
struct timespec tsp;

  if(WB_UNLIKELY(uiDelay >= 1000000L))
  {
    tsp.tv_sec = uiDelay / 1000000L;
    uiDelay = uiDelay % 1000000L; // number of microseconds converted to nanoseconds
  }
  else
  {
    tsp.tv_sec = 0; // it's assumed that this method is slightly faster
  }

  tsp.tv_sec = 0;
  tsp.tv_nsec = uiDelay * 1000;  // wait for .1 msec

  nanosleep(&tsp, NULL);
//#else  // HAVE_NANOSLEEP
//
//  usleep(uiDelay);  // 100 microsecs - a POSIX alternative to 'nanosleep'
//
//#endif // HAVE_NANOSLEEP
}



char *WBCopyString(const char *pSrc)
{
char *pDest;
int iLen;

  if(!pSrc || !*pSrc)
  {
    pDest = WBAlloc(2);

    if(pDest)
    {
      *pDest = 0;
    }
  }
  else
  {
    iLen = strlen(pSrc);

    pDest = WBAlloc(iLen + 1);

    if(pDest)
    {
      memcpy(pDest, pSrc, iLen);
      pDest[iLen] = 0;
    }
  }

  return pDest;
}

char *WBCopyStringN(const char *pSrc, unsigned int nMaxChars)
{
char *pDest;
unsigned int iLen;
const char *p1;

  if(!pSrc || !*pSrc)
  {
    pDest = WBAlloc(2);

    if(pDest)
    {
      *pDest = 0;
    }
  }
  else
  {
    for(p1 = pSrc, iLen = 0; iLen < nMaxChars && *p1; p1++, iLen++)
    { } // determine length of 'pStr' to copy

    pDest = WBAlloc(iLen + 1);

    if(pDest)
    {
      memcpy(pDest, pSrc, iLen);
      pDest[iLen] = 0;
    }
  }

  return pDest;
}


void WBCatString(char **ppDest, const char *pSrc)  // concatenate onto WBAlloc'd string
{
int iLen, iLen2;
char *p2;

  if(!ppDest || !pSrc || !*pSrc)
  {
    return;
  }

  if(*ppDest)
  {
    iLen = strlen(*ppDest);
    iLen2 = strlen(pSrc);

    p2 = *ppDest;
    *ppDest = WBReAlloc(p2, iLen + iLen2 + 1);
    if(!*ppDest)
    {
      *ppDest = p2;
      return;  // not enough memory
    }

    p2 = iLen + *ppDest;  // re-position end of string

    memcpy(p2, pSrc, iLen2);
    p2[iLen2] = 0;  // make sure last byte is zero
  }
  else
  {
    *ppDest = WBCopyString(pSrc);
  }
}

void WBCatStringN(char **ppDest, const char *pSrc, unsigned int nMaxChars)
{
unsigned int iLen, iLen2;
char *p2;
const char *p3;


  if(!ppDest || !pSrc || !*pSrc)
  {
    return;
  }

  if(*ppDest)
  {
    iLen = strlen(*ppDest);

    for(iLen2=0, p3 = pSrc; iLen2 < nMaxChars && *p3; p3++, iLen2++)
    { }  // determine what the length of pSrc is up to a zero byte or 'nMaxChars', whichever is first

    p2 = *ppDest;
    *ppDest = WBReAlloc(p2, iLen + iLen2 + 1);
    if(!*ppDest)
    {
      *ppDest = p2; // restore the old pointer value
      return;  // not enough memory
    }

    p2 = iLen + *ppDest;  // re-position end of string

    memcpy(p2, pSrc, iLen2);
    p2[iLen2] = 0;  // make sure last byte is zero
  }
  else
  {
    *ppDest = WBCopyStringN(pSrc, nMaxChars);
  }
}



///////////////////////////////////
// DIRECTORIES PATHS AND TEMP FILES
///////////////////////////////////


// NOTE:  this does NOT canonicalize the path, so '~' and whatnot need
//        to be handled separately
int WBMkDir(const char *szFileName, int flags)
{
int iRval;

  if(!szFileName || !*szFileName)
  {
    return -1; // always an error to create a 'blank' directory
  }

#ifdef WIN32
#error not yet implemented
#else // WIN32

  if(szFileName[0] == '/' && !szFileName[1])
  {
    return 0; // always succeed if attempting to create the root dir
  }

  iRval = mkdir(szFileName, flags); // attempt it
  if(iRval && errno == ENOENT) // need to recursively create it
  {
    char *p1, *p2;
    // remove one element of the path, and recursively attempt to make THAT one

    p1 = WBCopyString(szFileName);
    if(p1)
    {
      p2 = p1 + strlen(p1) - 1;
      if(*p2 == '/') // already?
      {
        p2--;
      }
      while(p2 > p1 && *p2 != '/')
      {
        p2--;
      }

      if(p2 > p1)
      {
        *p2 = 0;
        iRval = WBMkDir(p1, flags);

        if(!iRval) // I was able to create things 'above this'
        {
          iRval = mkdir(szFileName, flags);
        }
      }

      WBFree(p1);
    }
  }
#endif // WIN32

  return iRval;
}

char * WBSearchPath(const char *szFileName)
{
char *pRval = NULL;
const char *p1, *pCur, *pPath;
#if 0
char *p2;
#endif // 0


  if(0 > WBStat(szFileName, NULL)) // file does not exist?
  {
    if(*szFileName == '/') // absolute path
    {
no_stat:
      WB_ERROR_PRINT("%s - File does not exist: \"%s\"\n", __FUNCTION__, szFileName);
      return NULL;
    }

    // check PATH environment variable, and locate first match

    pRval = WBAlloc(2 * PATH_MAX + strlen(szFileName));

    if(pRval)
    {
      pPath = getenv("PATH"); // not malloc'd, but should not modify
      if(pPath)
      {
        pCur = pPath;

        while(*pCur)
        {
          *pRval = 0; // reset

          p1 = pCur;
          while(*p1 && *p1 != ':')
          {
            p1++;
          }

          if((p1 - pCur) + 2 < 2 * PATH_MAX) // only if not a buffer overrun
          {
            // build path name
            memcpy(pRval, pCur, p1 - pCur);

            if(pRval[(p1 - pCur) - 1] != '/')
            {
              pRval[(p1 - pCur)] = '/';
              strcpy(pRval + (p1 - pCur) + 1, szFileName);
            }
            else
            {
              strcpy(pRval + (p1 - pCur), szFileName);
            }

//            fprintf(stderr, "TEMPORARY:  trying \"%s\"\n", pRval);

            if(!WBStat(pRval, NULL))
            {
              return pRval; // FOUND!
            }
          }

          if(*p1)
          {
            p1++;
          }

          pCur = p1;
        }
      }

      pPath = pCur = p1 = NULL; // make sure I NULL them out (prevent pointer re-use)

#if 0
      // if I get here I should check ONE MORE TIME at the location of X11workbench in case
      // it was installed into a non-standard path someplace and I need one of its utilities

      p2 = WBCopyString(GetStartupAppName());
      if(p2)
      {
        if(*p2 && strlen(p2) < 2 * PATH_MAX) // so I don't overflow
        {
          p1 = strrchr(p2, '/'); // find the last '/'
          if(p1)
          {
            p2[p1 - p2 + 1] = 0; // terminate with 0 byte (p1 is const)
          }
          else
          {
            WBFree(p2);
            p2 = NULL;
          }
        }
        else
        {
          WBFree(p2);
          p2 = NULL;
        }
      }
#endif // 0
      p1 = NULL; // prevents pointer re-use

#if 0
      if(p2)
      {
        strcpy(pRval, p2);         // the path for X11workbench's install directory
        strcat(pRval, szFileName); // use path of X11workbench executable with szFileName
      }
      else // could not find, nor get path info
#endif // 0
      {
        WBFree(pRval);
        pRval = NULL;
      }
    }

    if(!pRval || 0 > WBStat(pRval, NULL))
    {
      if(pRval)
      {
        WBFree(pRval);
      }

      goto no_stat;
    }
  }
  else
  {
    pRval = WBCopyString(szFileName); // file exists, so return as-is
  }

  return pRval;
}


char * WBTempFile0(const char *szExt)
{
char *pRval = NULL;
const char *szDir = NULL;
int i1;
WB_FILE_HANDLE h1;
union
{
  WB_UINT64 ullTime;
  unsigned short sA[4];
} uX;
static const char szH[16]="0123456789ABCDEF";


#ifdef WIN32
  // TODO:  the windows code, which uses the TEMP and TMP environment variables as well as the registry
#error windows version not implemented
#else // !WIN32

  // On POSIX systems, first use /var/tmp and if not available, use /tmp

  szDir = "/var/tmp";

  if(0 > WBStat(szDir, NULL))
  {
    szDir = "/tmp";
    if(0 > WBStat(szDir, NULL))
    {
      return NULL; // unable to 'stat' the temp file directory
    }
  }

#endif // !WIN32

  for(i1=0; i1 < 256; i1++) // don't try forever
  {
    pRval = WBCopyString(szDir);

    if(pRval)
    {
#ifdef WIN32
      WBCatString(&pRval, "\\wbtk0000");
#else // !WIN32
      WBCatString(&pRval, "/wbtk0000");
#endif // !WIN32
    }


    uX.ullTime = WBGetTimeIndex();
    uX.sA[0] ^= uX.sA[1];
    uX.sA[0] ^= uX.sA[2];
    uX.sA[0] ^= uX.sA[3];

    if(pRval)
    {
      char *pX = pRval + strlen(pRval) - 4; // point to first '0'

      pX[0] = szH[(uX.sA[0] >> 12) & 0xf];
      pX[1] = szH[(uX.sA[0] >> 8) & 0xf];
      pX[2] = szH[(uX.sA[0] >> 4) & 0xf];
      pX[3] = szH[uX.sA[0] & 0xf];

      if(szExt && *szExt)
      {
        if(*szExt != '.')
        {
          WBCatString(&pRval, ".");
        }
        if(pRval)
        {
          WBCatString(&pRval, szExt);
        }
      }
    }

    if(pRval)
    {
#ifdef WIN32
#error windows code not written yet
#else // !WIN32
      h1 = open(pRval, O_CREAT | O_EXCL | O_RDWR, 0644); // create file, using '644' permissions, fail if exists

      if(h1 < 0) // error
      {
        WBFree(pRval);
        pRval = NULL;

        if(errno == EEXIST)
        {
          WBDelay(499);
          continue; // try again with a different name
        }

        if(errno == ENOTDIR || errno == ENOENT || errno == EACCES ||
           errno == EPERM || errno == EROFS || errno == EMFILE || errno == ENFILE)
        {
          // these errors are fatal, so I exit now
          break;
        }
      }
      else
      {
        close(h1);

        // add this file to the existing list of temp files to be destroyed
        // on exit from the program.

        break; // file name is valid and ready for use
      }
#endif // !WIN32
    }
  }

  return pRval;
}

static void __add_to_temp_file_list(const char *szFile)
{
  // does nothing - TODO implement?
}

char * WBTempFile(const char *szExt)
{
char *pRval = WBTempFile0(szExt);

  if(pRval)
  {
    __add_to_temp_file_list(pRval);
  }

  return pRval;
}


///////////////////////////////////
// EXTERNAL APPLICATION EXECUTION
///////////////////////////////////

WB_PROCESS_ID WBRunAsyncPipeV(WB_FILE_HANDLE hStdIn, WB_FILE_HANDLE hStdOut, WB_FILE_HANDLE hStdErr,
                              const char *szAppName, va_list va)
{
const char *pArg;//, *pPath;
char *pCur, *p1, *pAppName = NULL;
char **argv;
int i1, nItems, cbItems;
va_list va2;
WB_PROCESS_ID hRval;
WB_FILE_HANDLE hIn, hOut, hErr;


  // NOTE:  to avoid zombies, must assign SIGCHLD to 'SIG_IGN' or process them correctly
  //        (this is done in 'WBInit')

  hIn = hOut = hErr = WB_INVALID_FILE_HANDLE; // by convention (WIN32 needs this anyway)

  // FIRST, locate 'szAppName'

  pAppName = WBSearchPath(szAppName);

//  WB_ERROR_PRINT("TEMPORARY: %s - AppName \"%s\"\n", __FUNCTION__, pAppName);
//
// DEBUG-ONLY code - TODO enable with debug level verbosity > ???
//  {
//    va_copy(va2, va);
//    nItems = 1;
//    while(1)
//    {
//      pArg = va_arg(va2, const char *);
//      if(!pArg)
//      {
//        break;
//      }
//
//      WB_ERROR_PRINT("TEMPORARY: %s -      Arg %d -\"%s\"\n", __FUNCTION__, nItems, pArg);
//      nItems++;
//    }
//  }

  if(hStdIn == WB_INVALID_FILE_HANDLE) // re-dir to/from /dev/null
  {
#ifndef WIN32
    hIn = open("/dev/null", O_RDONLY, 0);
#else // WIN32
    SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)WBAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

    if(pSD)
    {
      InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION);
      pSD->bInheritHandle = TRUE; // what a pain

      hIn = CreateFile("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       pSD, OPEN_EXISTING, NULL, NULL);
      WBFree(pSD);
    }
#endif // WIN32
  }
  else
  {
#ifndef WIN32
    hIn = dup(hStdIn);
#else // WIN32
    if(!DuplicateHandle(GetCurrentProcess(), hStdIn,
                        GetCurrentProcess(), &hIn, GENERIC_READ,
                        TRUE, 0))
    {
      hIn = WB_INVALID_FILE_HANDLE;
    }
#endif // WIN32
  }

  if(hStdOut == WB_INVALID_FILE_HANDLE) // re-dir to/from /dev/null
  {
#ifndef WIN32
    hOut = open("/dev/null", O_WRONLY, 0);
#else // WIN32
    SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)WBAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

    if(pSD)
    {
      InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION);
      pSD->bInheritHandle = TRUE; // what a pain

      hOut = CreateFile("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        pSD, OPEN_EXISTING, NULL, NULL);

      // ALTERNATE:  use 'SetHandleInformation(hOut, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)'

      WBFree(pSD);
    }
#endif // WIN32
  }
  else
  {
#ifndef WIN32
    hOut = dup(hStdOut);
#else // WIN32
    if(!DuplicateHandle(GetCurrentProcess(), hStdOut,
                        GetCurrentProcess(), &hOut, GENERIC_WRITE,
                        TRUE, 0))
    {
      hOut = WB_INVALID_FILE_HANDLE;
    }
#endif // WIN32
  }

  if(hStdErr == WB_INVALID_FILE_HANDLE) // re-dir to/from /dev/null
  {
#ifndef WIN32
    hErr = open("/dev/null", O_WRONLY, 0);
#else // WIN32
    SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)WBAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

    if(pSD)
    {
      InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION);
      pSD->bInheritHandle = TRUE; // what a pain

      hErr = CreateFile("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        pSD, OPEN_EXISTING, NULL, NULL);
      WBFree(pSD);
    }
#endif // WIN32
  }
  else
  {
#ifndef WIN32
    hErr = dup(hStdErr);
#else // WIN32
    if(!DuplicateHandle(GetCurrentProcess(), hStdErr,
                        GetCurrentProcess(), &hErr, GENERIC_WRITE,
                        TRUE, 0))
    {
      hErr = WB_INVALID_FILE_HANDLE;
    }
#endif // WIN32
  }

  // if file handle duplication fails, exit now with an error

  if(hIn == WB_INVALID_FILE_HANDLE ||
     hOut == WB_INVALID_FILE_HANDLE ||
     hErr == WB_INVALID_FILE_HANDLE)
  {
//    WB_ERROR_PRINT("TEMPORARY:  %s hIn=%d hOut=%d hErr=%d\n", __FUNCTION__, hIn, hOut, hErr);

    if(hIn != WB_INVALID_FILE_HANDLE)
    {
      close(hIn);
    }
    if(hOut != WB_INVALID_FILE_HANDLE)
    {
      close(hOut);
    }
    if(hErr != WB_INVALID_FILE_HANDLE)
    {
      close(hErr);
    }

    if(pAppName != szAppName)
    {
      WBFree(pAppName);
    }

    return WB_INVALID_FILE_HANDLE;
  }

  // count arguments, determine memory requirement

  nItems = 0;
  cbItems = 2 * sizeof(char *) + strlen(szAppName) + 1;
  va_copy(va2, va);

  while(1)
  {
    pArg = va_arg(va2, const char *);
    if(!pArg)
    {
      break;
    }

    cbItems += strlen(pArg) + 1 + sizeof(char *);
    nItems++;
  }

  argv = (char **)WBAlloc(64 + cbItems);
  if(!argv)
  {
    close(hIn);
    close(hOut);
    close(hErr);

    if(pAppName != szAppName)
    {
      WBFree(pAppName);
    }

//    WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (1)\n", __FUNCTION__);
    return WB_INVALID_FILE_HANDLE;
  }

  pCur = (char *)(argv + nItems + 2); // enough room for argument pointers

  p1 = strrchr(szAppName, '/');
  if(p1)
  {
    strcpy(pCur, p1 + 1); // just the name
  }
  else
  {
    strcpy(pCur, szAppName);
  }

  argv[0] = pCur;
  pCur += strlen(pCur) + 1;

  for(i1=1; i1 <= nItems; i1++)
  {
    pArg = va_arg(va, const char *);

    strcpy(pCur, pArg);
    argv[i1] = pCur;
    pCur += strlen(pCur) + 1;
  }

  argv[nItems + 1] = NULL;

  // now that I have a valid 'argv' I can spawn the process.
  // I will return the PID so that the caller can wait on it

  hRval = vfork();

  if(!hRval) // the 'forked' process
  {
    // vfork jumps here FIRST and temporarily suspends the calling thread
    // it also does NOT make a copy of memory so I must treat it as 'read only'

    if(dup2(hIn, 0) != -1 && dup2(hOut, 1) != -1 && dup2(hErr, 2) != -1) // stdin, stdout, stderr
    {
      static const char szMsg[]="ERROR: 'execve()' failure\n";
      extern char **environ; // this is what the man page says to do (it's part of libc)

      // TODO:  customize environment?

      signal(SIGHUP, SIG_IGN); // ignore 'HUP' signal before 'setsid' call ['daemon()' does this]
      setsid(); // so that I am my own process group (NOTE doing this might make it impossible to get the exit status... must verify everywhere)
      signal(SIGHUP, SIG_DFL); // restore default handling of 'HUP' ['daemon()' does this]

      execve(pAppName, argv, environ); // NOTE:  execute clears all existing signal handlers back to 'default' but retains 'ignored' signals

      write(2, szMsg, sizeof(szMsg) - 1); // stderr is still 'the old one' at this point
      fsync(2);

      // TODO:  if execve fails, should I forcibly close the duplicated handles??
//      close(0);
//      close(1);
//      close(2);
    }
    else
    {
      static const char szMsg[]="ERROR: 'dup2()' failure\n";
      write(2, szMsg, sizeof(szMsg) - 1); // stderr is still 'the old one' at this point
    }

    close(hIn); // explicitly close these if I get here
    close(hOut);
    close(hErr);

    _exit(-1); // should never get here, but this must be done if execve fails
  }

  // once I've forked, I don't have to worry about copied memory or shared memory
  // and it's safe to free the allocated 'argv' array.

  WBFree(argv);
  close(hIn);
  close(hOut);
  close(hErr);

  if(pAppName != szAppName)
  {
    WBFree(pAppName);
  }


  return hRval;
}


WB_PROCESS_ID WBRunAsync(const char *szAppName, ...)
{
WB_PROCESS_ID idRval;
va_list va;

  va_start(va, szAppName);

  idRval = WBRunAsyncPipeV(WB_INVALID_FILE_HANDLE, WB_INVALID_FILE_HANDLE,
                           WB_INVALID_FILE_HANDLE, szAppName, va);

  va_end(va);

  if(idRval == WB_INVALID_FILE_HANDLE)
  {
    WB_ERROR_PRINT("Unable to run '%s'\n", szAppName);
  }
//  else
//  {
//    WB_ERROR_PRINT("Running '%s' - pid=%d\n", szAppName, idRval);
//  }

  return idRval;
}

WB_PROCESS_ID WBRunAsyncPipe(WB_FILE_HANDLE hStdIn, WB_FILE_HANDLE hStdOut, WB_FILE_HANDLE hStdErr,
                             const char *szAppName, ...)
{
WB_PROCESS_ID idRval;
va_list va;

  va_start(va, szAppName);

  idRval = WBRunAsyncPipeV(hStdIn, hStdOut, hStdErr, szAppName, va);

  va_end(va);

  return idRval;
}


#define WBRUNRESULT_BUFFER_MINSIZE 65536
#define WBRUNRESULT_BYTES_TO_READ 256

static char * WBRunResultInternal(WB_FILE_HANDLE hStdIn, const char *szAppName, va_list va)
{
WB_PROCESS_ID idRval;
WB_FILE_HANDLE hP[2]; // [0] is read end, [1] is write end
char *p1, *p2, *pRval;
int i1, i2, iStat, iRunning;
/*unsigned*/ int cbBuf;


  cbBuf = WBRUNRESULT_BUFFER_MINSIZE;
  pRval = WBAlloc(cbBuf);

  if(!pRval)
  {
//    WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (1) WBAlloc fail %d\n", __FUNCTION__, cbBuf);
    return NULL;
  }

  // use WBRunAsyncPipeV to create a process, with all stdout piped to a char * buffer capture
  // stdin and stderr still piped to/from /dev/null

  // create an anonymous pipe.  pH[0] is the INPUT pipe, pH[1] is the OUTPUT pipe
  // this is important in windows.  for POSIX it doesn't really matter which one you use,
  // but by convention [0] will be input, [1] will be output

  hP[0] = hP[1] = WB_INVALID_FILE_HANDLE;

#ifdef WIN32 /* the WINDOWS way */
  if(!CreatePipe(&(pH[0]), &(pH[1]), NULL, 0))
#else // !WIN32 (everybody else)
  if(0 > pipe(hP))
#endif // WIN32
  {
    WBFree(pRval);
//    WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (2)\n", __FUNCTION__);
    return NULL;
  }


  idRval = WBRunAsyncPipeV(hStdIn, hP[1], // the 'write' end is passed as stdout
                           WB_INVALID_FILE_HANDLE, szAppName, va);

  if(idRval == WB_INVALID_FILE_HANDLE)
  {
//    WB_ERROR_PRINT("TEMPORARY:  %s failed to run \"%s\" errno=%d\n", __FUNCTION__, szAppName, errno);

    close(hP[0]);
    close(hP[1]);

    return NULL;
  }

  close(hP[1]); // by convention, this will 'widow' the read end of the pipe once the process is done with it

  fcntl(hP[0], F_SETFL, O_NONBLOCK); // set non-blocking I/O

  // so long as the process is alive, read data from the pipe and stuff it into the output buffer
  // (the buffer will need to be reallocated periodically if it fills up)

  p1 = pRval;
  *p1 = 0;       // always do this
  iRunning = 1;  // iRunning will be used as a flag to indicate the process exited.

  while(1)
  {
    i2 = WBRUNRESULT_BYTES_TO_READ; // number of bytes to read at one time
    if((p1 - pRval) + i2 >= cbBuf) // enough room for it?
    {
      i2 = cbBuf - (p1 - pRval);
      if(i2 < WBRUNRESULT_BYTES_TO_READ / 8) // time to re-allocate
      {
        p2 = WBReAlloc(pRval, cbBuf + WBRUNRESULT_BUFFER_MINSIZE / 2);
        if(!p2)
        {
          WBFree(pRval);
          pRval = NULL;

//          WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (4)\n", __FUNCTION__);
          break;
        }

        cbBuf += WBRUNRESULT_BUFFER_MINSIZE / 2;
        p1 = p2 + (p1 - pRval);
        pRval = p2;
        i2 = WBRUNRESULT_BYTES_TO_READ;
      }
    }

    // if no data available I'll return immediately

    i1 = read(hP[0], p1, i2);

    if(i1 <= 0)
    {
      if(!iRunning)
      {
        if(i1 == 0)
        {
          break; // end of file, process ended, bail out now
        }
//        else // this could be caused by the process forking, and the program failing to run
//        {
//          // TODO:  allow a few retries, then bail??
//
//          break; // for now, bail out on this as well.  should still get "all of it" in the output
//        }
      }

      if(errno == EAGAIN)
      {
        WBDelay(500); // wait 1/2 msec
      }
      else
      {
        break; // an error of some kind, so bail out [pipe closed?]
      }
    }
    else
    {
      p1 += i1; // point past the # of bytes I just read in
      *p1 = 0; // by convention [to make sure the string is ALWAYS terminated with a 0-byte]
    }

    if(iRunning) // only if "still running"
    {
      // for waitpid(), if WNOHANG is specified and there are no stopped, continued or exited children, 0 is returned

      if(waitpid(idRval, &iStat, WNOHANG) && // note this might return non-zero for stopped or continued processes
         WIFEXITED(iStat))                   // so test if process exits also.
      {
        iRunning = 0; // my flag that it's not running
        WBDelay(5000); // wait for a bit to make sure the I/O completes
      }
      else
      {
        WBDelay(500); // so I don't 'spin'
      }
    }
  }

  // always kill the process at this point (in case there was an error)

  kill(idRval, SIGKILL); // not so nice way but oh well
  WBDelay(5000); // wait 5msec

  close(hP[0]); // done with the pipe - close it now

//  WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (4) pRval=%p *pRval=%c\n", __FUNCTION__, pRval, (char)(pRval ? *pRval : 0));

  return pRval;
}

int WBGetProcessState(WB_PROCESS_ID idProcess, WB_INT32 *pExitCode)
{
  int iStat, iRval;

  // for waitpid(), if WNOHANG is specified and there are no stopped, continued or exited children, 0 is returned

  iStat = 0;

  iRval = waitpid(idProcess, &iStat, WNOHANG); // note this might return non-zero for stopped or continued processes

  if(iRval > 0 && (iRval == (int)idProcess || (int)idProcess == -1 || (int)idProcess == 0))
  {
    if(WIFEXITED(iStat))                   // test if process exits also.
    {
      if(pExitCode)
      {
        *pExitCode = (WB_INT32)WEXITSTATUS(iStat);
      }

      return 0; // not running
    }

    return 1; // still running
  }

  if(iRval > 0)
  {
    WB_ERROR_PRINT("ERROR:  %s - waitpid returns %d, but does not match %d\n",
                   __FUNCTION__, iRval, (int)idProcess);
  }

  return -1; // error
}

char *WBRunResult(const char *szAppName, ...)
{
char *pRval;
va_list va;


  va_start(va, szAppName);

  pRval = WBRunResultInternal(WB_INVALID_FILE_HANDLE, szAppName, va);

  va_end(va);

  return pRval;
}



char *WBRunResultWithInput(const char *szStdInBuf, const char *szAppName, ...)
{
char *pRval, *pTemp = NULL;
va_list va;
WB_FILE_HANDLE hIn = WB_INVALID_FILE_HANDLE;


  va_start(va, szAppName);

  if(szStdInBuf && *szStdInBuf)
  {
    unsigned int nLen = strlen(szStdInBuf);

    pTemp = WBTempFile0(".tmp");

    if(!pTemp)
    {
//      WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (1)\n", __FUNCTION__);

      va_end(va);
      return NULL;
    }

    hIn = open(pTemp, O_RDWR, 0);

    if(hIn < 0)
    {
bad_file:
      unlink(pTemp);
      WBFree(pTemp);

//      WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (2)\n", __FUNCTION__);

      va_end(va);
      return NULL;
    }

    if(write(hIn, szStdInBuf, nLen) != (ssize_t)nLen)
    {
      close(hIn);
      goto bad_file;
    }

    lseek(hIn, 0, SEEK_SET); // rewind file

//    WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (3) temp file \"%s\"\n", __FUNCTION__, pTemp);
  }

  pRval = WBRunResultInternal(hIn, szAppName, va);

  va_end(va);

  if(pTemp)
  {
    close(hIn);
    unlink(pTemp);

    WBFree(pTemp);
  }

  return pRval;
}


// SHARED LIBRARIES

WB_MODULE WBLoadLibrary(const char * szModuleName)
{
  return((WB_MODULE)dlopen(szModuleName, RTLD_LAZY | RTLD_LOCAL));
}

void WBFreeLibrary(WB_MODULE hModule)
{
  dlclose(hModule);
}

WB_PROCADDRESS WBGetProcAddress(WB_MODULE hModule, const char *szProcName)
{
// freebsd has the 'dlfunc' API, which is basically 'dlsym' cast to a function pointer
#ifdef __FreeBSD__
  return((WB_PROCADDRESS)dlfunc(hModule, szProcName));
#else // other POSIX systems - TODO, check for 'dlfunc' instead of the OS
  return((WB_PROCADDRESS)dlsym(hModule, szProcName));
#endif // 'dlfunc' check
}

void * WBGetDataAddress(WB_MODULE hModule, const char *szDataName)
{
  return((void *)dlsym(hModule, szDataName));
}


// THREADS

WB_THREAD_KEY WBThreadAllocLocal(void)
{
WB_THREAD_KEY keyRval;
  if(!pthread_key_create(&keyRval, NULL))
  {
    return keyRval;
  }

  return (WB_THREAD_KEY)INVALID_HANDLE_VALUE;
}

void WBThreadFreeLocal(WB_THREAD_KEY keyVal)
{
  pthread_key_delete(keyVal); // TODO:  check return?
}

void * WBThreadGetLocal(WB_THREAD_KEY keyVal)
{
  return pthread_getspecific(keyVal);
}

void WBThreadSetLocal(WB_THREAD_KEY keyVal, void *pValue)
{
  pthread_setspecific(keyVal, pValue);
}

WB_THREAD WBThreadGetCurrent(void)
{
  return pthread_self();
}




WB_THREAD WBThreadCreate(void *(*function)(void *), void *pParam)
{
  WB_THREAD thrdRval = (WB_THREAD)INVALID_HANDLE_VALUE;

  // TODO:  call my own thread startup proc, passing a struct that contains
  //        'function' and 'pParam' as the param.  use a linked list of
  //        pre-allocated buffers for that.
  // see possible implementation, above

  if(!pthread_create(&thrdRval, NULL, function, pParam))
  {
    return thrdRval;
  }

  return (WB_THREAD)INVALID_HANDLE_VALUE;
}

void *WBThreadWait(WB_THREAD hThread)        // closes hThread, returns exit code, waits for thread to terminate (blocks)
{
void *pRval = NULL;

  if(pthread_join(hThread, &pRval))
  {
    // TODO:  error return??
    pRval = (void *)-1;
  }

  return pRval;
}

int WBThreadRunning(WB_THREAD hThread)        // >0 if thread is running, <0 error - use 'pthread_kill(thread,0)' which returns ESRCH if terminated i.e. 'PS_DEAD'
{
  int iR = pthread_kill(hThread,0);

  if(!iR)
  {
    return 1; // no signal sent, handle is valid (and did not exit)
  }

  if(iR == ESRCH)
  {
    return 0; // for now, allow this to indicate 'done'
  }

  return -1;
}

void WBThreadExit(void *pRval)
{
  pthread_exit(pRval);
}

void WBThreadClose(WB_THREAD hThread)
{
  pthread_detach(hThread);
}






// FILE SYSTEM INDEPENDENT FILE AND DIRECTORY UTILITIES
// UNIX/LINUX versions - TODO windows versions?

size_t WBReadFileIntoBuffer(const char *szFileName, char **ppBuf)
{
off_t cbLen;
size_t cbF;
int cb1, iChunk;
char *pBuf;
int iFile;


  if(!ppBuf)
  {
    return (size_t)-1;
  }

  iFile = open(szFileName, O_RDONLY); // open read only (assume no locking for now)

  if(iFile < 0)
  {
    return (size_t)-1;
  }

  // how long is my file?

  cbLen = (unsigned long)lseek(iFile, 0, SEEK_END); // location of end of file

  if(cbLen == (off_t)-1)
  {
    *ppBuf = NULL; // make sure
  }
  else
  {
    lseek(iFile, 0, SEEK_SET); // back to beginning of file

    *ppBuf = pBuf = WBAlloc(cbLen + 1);

    if(!pBuf)
    {
      cbLen = (off_t)-1; // to mark 'error'
    }
    else
    {
      cbF = cbLen;

      while(cbF > 0)
      {
        iChunk = 1048576; // 1MByte at a time

        if((size_t)iChunk > cbF)
        {
          iChunk = (int)cbF;
        }

        cb1 = read(iFile, pBuf, iChunk);

        if(cb1 == -1)
        {
          if(errno == EAGAIN) // allow this
          {
            WBDelay(100);
            continue; // for now just do this
          }

          cbLen = -1;
          break;
        }
        else if(cb1 != iChunk) // did not read enough bytes
        {
          iChunk = cb1; // for now
        }

        cbF -= iChunk;
        pBuf += iChunk;
      }
    }
  }

  close(iFile);

  return (size_t) cbLen;
}

int WBWriteFileFromBuffer(const char *szFileName, const char *pBuf, size_t cbBuf)
{
int iFile, iRval, iChunk;


  if(!pBuf)
  {
    return -1;
  }

  iFile = open(szFileName, O_CREAT | O_TRUNC | O_RDWR, 0666);  // always create with mode '666' (umask should apply)

  if(iFile < 0)
  {
    return -1;
  }

  while(cbBuf > 0)
  {
    // write chunks of 1Mb or size remaining

    iChunk = 1048576;
    if((size_t)iChunk > cbBuf)
    {
      iChunk = (int)cbBuf;
    }

    iRval = write(iFile, pBuf, iChunk);

    if(iRval < 0)
    {
      if(errno == EAGAIN)
      {
        WBDelay(100);

        // TODO:  time limit?  for now, no

        continue; // try again
      }

      close(iFile);
      return -1; // error
    }
    else if(iRval != iChunk) // TODO:  allow this??
    {
//      WBDebugPrint("TEMPORARY:  writing file, only wrote %d of %d bytes\n", iRval, iChunk);
      iChunk = iRval;
    }

    pBuf += iChunk;
    cbBuf -= iChunk;
  }

  iRval = 0; // at this point, success!

  close(iFile);

  return iRval;
}

int WBReplicateFilePermissions(const char *szProto, const char *szTarget)
{
struct stat sb;
int iRval = 0;

  iRval = stat(szProto, &sb); // TODO:  lstat for symlink?
  if(!iRval)
  {
    // TODO:  chflags?
    // TODO:  what if it's a symlink?
    iRval = chmod(szTarget, sb.st_mode & 0777); // only set the rwx permissions, and ignore others
    if(!iRval)
    {
      if(geteuid() == 0 || getuid() == sb.st_uid) // only do this if owner matches or I'm root
      {
        iRval = chown(szTarget, sb.st_uid, sb.st_gid);
        if(iRval < 0 && geteuid() != 0)
        {
          iRval = chown(szTarget, -1, sb.st_gid); // don't change the user

          if(iRval < 0)
          {
            // don't bother changing anything - just warn??
            iRval = 0;  // for now...
          }
        }
      }
    }
  }

  return iRval;
}

char *WBGetCurrentDirectory(void)
{
char *pRval = WBAlloc(MAXPATHLEN + 2);
int i1;

  if(pRval)
  {
    if(!getcwd(pRval, MAXPATHLEN))
    {
      WBFree(pRval);
      pRval = NULL;
    }
  }

  // this function will always return something that ends in '/' (except on error)

  if(pRval)
  {
    i1 = strlen(pRval);

    if(i1 > 0 && pRval[i1 - 1] != '/')
    {
      pRval[i1] = '/';
      pRval[i1 + 1] = 0;
    }
  }

  return pRval;
}

int WBIsDirectory(const char *szFileName)
{
int bRval = 0;

struct stat sF;

  if(!stat(szFileName, &sF)) // NOTE:  'stat' returns info about symlink targets, not the link itself
    bRval = S_ISDIR(sF.st_mode);

  return(bRval);
}

char *WBGetCanonicalPath(const char *szFileName)
{
char *pTemp, *p1, *p2, *p3, *p4, *pRval = NULL;
struct stat sF;

  pTemp = WBCopyString(szFileName);

  if(!pTemp)
  {
    return NULL;
  }

  // step 1:  eliminate // /./

  p1 = pTemp;
  while(*p1 && p1[1])
  {
    if(*p1 == '/' && p1[1] == '/')
    {
      memmove(p1, p1 + 1, strlen(p1 + 1) + 1);
    }
    else if(*p1 == '/' && p1[1] == '.' && p1[2] == '/')
    {
      memmove(p1, p1 + 2, strlen(p1 + 2) + 1);
    }
    else
    {
      p1++;
    }
  }

  // step 2:  resolve each portion of the path, deal with '~' '.' '..', build new path.

  if(*pTemp == '~' && (pTemp[1] == '/' || !pTemp[1])) // first look for '~' at the beginning (only allowed there)
  {
    p1 = getenv("HOME");
    if(!p1 || !*p1) // no home directory?
    {
      *pTemp = '.';  // for now change it to '.'
    }
    else
    {
      p3 = WBCopyString(p1);
      if(!p3)
      {
        WBFree(pTemp);
        return NULL;
      }

      if(p3[strlen(p3) - 1] != '/')
      {
        WBCatString(&p3, "/");
      }

      p2 = pTemp + 1;
      if(*p2 == '/')
      {
        p2++; // already have an ending / on the path
      }

      if(*p2)
      {
        WBCatString(&p3, p2);
      }

      WBFree(pTemp);
      pTemp = p3;
    }
  }

  p1 = pTemp;
  while(*p1)
  {
    p2 = strchr(p1, '/');
    if(!p2)
    {
      if(*p1 == '.') // check for ending in '.' or '..' and add a '/' so I can handle it correctly
      {
        if((p1[1] == '.' && !p1[2]) || !p1[1])
        {
          p2 = pTemp; // temporary
          WBCatString(&pTemp, "/");

          p1 = (p1 - p2) + pTemp; // restore relative pointer

          WB_ERROR_PRINT("TEMPORARY:  %s  %s\n", p1, pTemp);

          continue; // let's do this again, properly
        }
      }

      // no more paths, so this is "the name".
      if(!pRval) // no existing path, use CWD
      {
        pRval = WBGetCurrentDirectory();

        if(!pRval)
        {
          break;
        }
      }

      WBCatString(&pRval, p1);

      break;
    }
    else if(p2 == p1)
    {
      pRval = WBCopyString("/");
    }
    else
    {
      if(!pRval)
      {
        pRval = WBGetCurrentDirectory();
        if(!pRval)
        {
          break;
        }
      }

      // when I assemble these paths together, deal with '..' and
      // symbolic links.  Check for cyclic paths.

      if(p2 - p1 == 1 && p1[0] == '.') // the ./ path
      {
        p1 = p2 + 1; // just ignore this part
        continue;
      }
      else if(p2 - p1 == 2 && p1[0] == '.' && p1[1] == '.') // the ../ path
      {
        p1 = p2 + 1; // I need to fix the path while ignoring the '../' part

        p3 = pRval + strlen(pRval) - 1; // NOTE:  pRval ends in '/' and I want the one BEFORE that
        while(p3 > pRval)
        {
          if(*(p3 - 1) == '/')
          {
            *p3 = 0;
            break;
          }

          p3--;
        }

        if(p3 <= pRval) // did not find a preceding '/' - this is an error
        {
          WB_ERROR_PRINT("%s:%d - did not find preceding '/' - %s\n", __FUNCTION__, __LINE__, pRval);

          WBFree(pRval);
          pRval = NULL;

          break;
        }

        continue;
      }

      // TEMPORARY:  just copy as-is to test basic logic

      WBCatStringN(&pRval, p1, p2 - p1 + 1); // include the '/' at the end
      if(!pRval)
      {
        WB_ERROR_PRINT("%s:%d - WBCatStringN returned NULL pointer\n", __FUNCTION__, __LINE__);

        break;
      }

      // see if this is a symbolic link.  exclude testing '/'

      p3 = pRval + strlen(pRval) - 1;
      if(p3 > pRval)
      {
        *p3 = 0; // temporary
        if(lstat(pRval, &sF)) // get the file 'stat' and see if we're a symlink
        {
          // error, does not exist? - leave it 'as-is' for now
          *p3 = '/';  // restore it
        }
        else if(S_ISDIR(sF.st_mode)) // an actual directory - remains as-is
        {
          // don't do anything except restore the '/'
          *p3 = '/';  // restore it
        }
        else if(S_ISLNK(sF.st_mode)) // symlink
        {
          // now I get to put the symlink contents "in place".  If the symlink is
          // relative to the current directory, I'll want that.

          p4 = (char *)WBAlloc(MAXPATHLEN + 2);

          if(!p4)
          {
            WB_ERROR_PRINT("%s:%d - not enough memory for buffer\n", __FUNCTION__, __LINE__);

            WBFree(pRval);
            pRval = NULL;
            break;
          }
          else
          {
            int iLen = readlink(pRval, p4, MAXPATHLEN);

            if(iLen <= 0)
            {
              WB_ERROR_PRINT("%s:%d - readlink returned %d for %s\n", __FUNCTION__, __LINE__, iLen, pRval);

              WBFree(p4);
              WBFree(pRval);
              pRval = NULL;

              break;
            }

            p4[iLen] = 0; // assume < MAXPATHLEN for now...
            if(p4[0] == '/') // it's an absolute path
            {
              WBFree(pRval);
              pRval = p4;
            }
            else
            {
              while(p3 > pRval && *(p3 - 1) != '/') // scan back for a '/'
              {
                p3--;
              }

              *p3 = 0;
              WBCatString(&pRval, p4); // sub in the relative path
              WBFree(p4);
            }

            if(!WBIsDirectory(pRval)) // must be a directory!
            {
              WB_ERROR_PRINT("%s:%d - %s not a directory\n", __FUNCTION__, __LINE__, pRval);

              WBFree(pRval);
              pRval = NULL;
              break; // this is an error
            }
            else
            {
              WBCatString(&pRval, "/");

              if(pRval)
              {
                p4 = WBGetCanonicalPath(pRval); // recurse

                WBFree(pRval);
                pRval = p4; // new canonical version of symlink path
              }

              if(!pRval)
              {
                WB_ERROR_PRINT("%s:%d - NULL pRval\n", __FUNCTION__, __LINE__);

                break;
              }
            }
          }
        }
      }
    }

    p1 = p2 + 1;
  }

  // if the resulting path is a symbolic link, fix it
  if(pRval)
  {
    p1 = pRval + strlen(pRval) - 1;

    if(p1 > pRval && *p1 != '/') // does not end in a slash, so it should be a file...
    {
      while(p1 > pRval && *(p1 - 1) != '/')
      {
        p1--;
      }

      if(!lstat(pRval, &sF)) // get the file 'stat' and see if we're a symlink (ignore errors)
      {
        if(S_ISDIR(sF.st_mode)) // an actual directory - end with a '/'
        {
          WBCatString(&pRval, "/"); // add ending '/'
        }
        else if(S_ISLNK(sF.st_mode)) // symlink
        {
          // now I get to put the symlink contents "in place".  If the symlink is
          // relative to the current directory, I'll want that.

          p4 = (char *)WBAlloc(MAXPATHLEN + 2);

          if(!p4)
          {
            WB_ERROR_PRINT("%s:%d - not enough memory\n", __FUNCTION__, __LINE__);

            // TODO:  assign pRval to NULL ?
          }
          else
          {
            int iLen = readlink(pRval, p4, MAXPATHLEN);

            if(iLen <= 0)
            {
              WB_ERROR_PRINT("%s:%d - readlink returned %d for %s\n", __FUNCTION__, __LINE__, iLen, pRval);

              WBFree(p4);
              WBFree(pRval);
              pRval = NULL;
            }
            else
            {
              p4[iLen] = 0; // assume < MAXPATHLEN for now...
              if(p4[0] == '/') // it's an absolute path
              {
                WBFree(pRval); // new path for old
                pRval = p4;
              }
              else
              {
                p3 = pRval + strlen(pRval); // I won't be ending in '/' for this part so don't subtract 1
                while(p3 > pRval && *(p3 - 1) != '/') // scan back for the '/' in symlink's original path
                {
                  p3--;
                }

                *p3 = 0;
                WBCatString(&pRval, p4); // sub in the relative path
                WBFree(p4);
              }

              if(pRval && WBIsDirectory(pRval)) // is the result a directory?
              {
                WBCatString(&pRval, "/");
              }

              if(pRval)
              {
                p4 = WBGetCanonicalPath(pRval); // recurse to make sure I'm canonical (deal with '..' and '.' and so on)

                WBFree(pRval);
                pRval = p4; // new canonical version of symlink path
              }
            }
          }
        }
      }
    }
  }

  if(pTemp)
  {
    WBFree(pTemp);
    pTemp = NULL; // by convention
  }

  if(!pRval)
  {
    WB_ERROR_PRINT("%s:%d - returning NULL\n", __FUNCTION__, __LINE__);
  }

  return pRval;
}

// reading directories in a system-independent way

typedef struct __DIRLIST__
{
  const char *szPath, *szNameSpec;
  DIR *hD;
  struct stat sF;
  union
  {
    char cde[sizeof(struct dirent) + NAME_MAX + 2];
    struct dirent de;
  };
// actual composite 'search name' follows
} DIRLIST;

void *WBAllocDirectoryList(const char *szDirSpec)
{
DIRLIST *pRval;
char *p1, *p2;
int iLen, nMaxLen;
char *pBuf;

  if(!szDirSpec || !*szDirSpec)
  {
    WB_WARN_PRINT("WARNING - %s - invalid directory (NULL or empty)\n", __FUNCTION__);
    return NULL;
  }

  iLen = strlen(szDirSpec);
  nMaxLen = iLen + 32;

  pBuf = WBAlloc(nMaxLen);
  if(!pBuf)
  {
    WB_ERROR_PRINT("ERROR - %s - Unable to allocate memory for buffer size %d\n", __FUNCTION__, nMaxLen);
    return NULL;
  }

  if(szDirSpec[0] == '/') // path starts from the root
  {
    memcpy(pBuf, szDirSpec, iLen + 1);
  }
  else // for now, force a path of './' to be prepended to path spec
  {
    pBuf[0] = '.';
    pBuf[1] = '/';

    memcpy(pBuf + 2, szDirSpec, iLen + 1);
    iLen += 2;
  }

  // do a reverse scan until I find a '/'
  p1 = ((char *)pBuf) + iLen;
  while(p1 > pBuf && *(p1 - 1) != '/')
  {
    p1--;
  }

//  WB_ERROR_PRINT("TEMPORARY - \"%s\" \"%s\" \"%s\"\n", pBuf, p1, szDirSpec);

  if(p1 > pBuf)
  {
    // found, and p1 points PAST the '/'.  See if it ends in '/' or if there are wildcards present
    if(!*p1) // name ends in '/'
    {
      if(p1 == (pBuf + 1) && *pBuf == '/') // root dir
      {
        p1++;
      }
      else
      {
        *(p1 - 1) = 0;  // trim the final '/'
      }

      p1[0] = '*';
      p1[1] = 0;
    }
    else if(strchr(p1, '*') || strchr(p1, '?'))
    {
      if(p1 == (pBuf + 1) && *pBuf == '/') // root dir
      {
        memmove(p1 + 1, p1, strlen(p1) + 1);
        *(p1++) = 0; // after this, p1 points to the file spec
      }
      else
      {
        *(p1 - 1) = 0;  // p1 points to the file spec
      }
    }
    else if(WBIsDirectory(pBuf)) // entire name is a directory
    {
      // NOTE:  root directory should NEVER end up here

      p1 += strlen(p1);
      *(p1++) = 0; // end of path (would be '/')
      p1[0] = '*';
      p1[1] = 0;
    }
    else
    {
      WB_WARN_PRINT("TEMPORARY:  I am confused, %s %s\n", pBuf, p1);
    }
  }
  else
  {
    // this should never happen if I'm always prepending a './'
    // TODO:  make this more consistent, maybe absolute path?

    WB_WARN_PRINT("TEMPORARY:  should not happen, %s %s\n", pBuf, p1);

    if(strchr(pBuf, '*') || strchr(pBuf, '?')) // wildcard spec
    {
      p1 = (char *)pBuf + 1; // make room for zero byte preceding dir spec
      memmove(pBuf, p1, iLen + 1);
      *pBuf = 0;  // since it's the current working dir just make it a zero byte (empty string)
    }
    else if(WBIsDirectory(pBuf))
    {
      p1 = (char *)pBuf + iLen;
      *(p1++) = 0; // end of path (would be '/')
      p1[0] = '*';
      p1[1] = 0;
    }
  }

  pRval = WBAlloc(sizeof(DIRLIST) + iLen + strlen(p1) + 2);

  if(pRval)
  {
    pRval->szPath = pBuf;
    pRval->szNameSpec = p1;

    p2 = (char *)(pRval + 1);
    strcpy(p2, pBuf);
    p2 += strlen(p2);
    *(p2++) = '/';
    strcpy(p2, p1);
    p1 = (char *)(pRval + 1);

    pRval->hD = opendir(pBuf);

//    WB_ERROR_PRINT("TEMPORARY - opendir for %s returns %p\n", pBuf, pRval->hD);

    if(pRval->hD == NULL)
    {
      WB_WARN_PRINT("WARNING - %s - Unable to open dir \"%s\", errno=%d\n", __FUNCTION__, pBuf, errno);

      WBFree(pBuf);
      WBFree(pRval);

      pRval = NULL;
    }
  }
  else
  {
    WB_ERROR_PRINT("ERROR - %s - Unable to allocate memory for DIRLIST\n", __FUNCTION__);
    WBFree(pBuf);  // no need to keep this around
  }

  return pRval;
}

void WBDestroyDirectoryList(void *pDirectoryList)
{
  if(pDirectoryList)
  {
    DIRLIST *pD = (DIRLIST *)pDirectoryList;

    if(pD->hD)
    {
      closedir(pD->hD);
    }
    if(pD->szPath)
    {
      WBFree((void *)(pD->szPath));
    }

    WBFree(pDirectoryList);
  }
}

// returns < 0 on error, > 0 on EOF, 0 for "found something"

int WBNextDirectoryEntry(void *pDirectoryList, char *szNameReturn,
                         int cbNameReturn, unsigned long *pdwModeAttrReturn)
{
struct dirent *pD;
struct stat sF;
char *p1, *pBuf;
//static char *p2; // temporary
int iRval = 1;  // default 'EOF'
DIRLIST *pDL = (DIRLIST *)pDirectoryList;


  if(!pDirectoryList)
  {
    return -1;
  }

  // TODO:  improve this, maybe cache buffer or string length...
  pBuf = WBAlloc(strlen(pDL->szPath) + 8 + NAME_MAX);

  if(!pBuf)
  {
    return -2;
  }

  strcpy(pBuf, pDL->szPath);
  p1 = pBuf + strlen(pBuf);
  if(p1 > pBuf && *(p1 - 1) != '/') // it does not already end in /
  {
    *(p1++) = '/';  // for now assume this
    *p1 = 0;  // by convention
  }

  if(pDL->hD)
  {
    while((pD = readdir(pDL->hD))
          != NULL)
    {
      // skip '.' and '..'
      if(pD->d_name[0] == '.' &&
         (!pD->d_name[1] ||
          (pD->d_name[1] == '.' && !pD->d_name[2])))
      {
//        WB_ERROR_PRINT("TEMPORARY:  skipping %s\n", pD->d_name);
        continue;  // no '.' or '..'
      }

      strcpy(p1, pD->d_name);

      if(!lstat(pBuf, &sF)) // 'lstat' returns data about a file, and if it's a symlink, returns info about the link itself
      {
        if(!fnmatch(pDL->szNameSpec, p1, 0/*FNM_PERIOD*/))  // 'tbuf2' is my pattern
        {
          iRval = 0;

          if(pdwModeAttrReturn)
          {
            *pdwModeAttrReturn = sF.st_mode;
          }

          if(szNameReturn && cbNameReturn > 0)
          {
            strncpy(szNameReturn, p1, cbNameReturn);
          }

          break;
        }
//        else
//        {
//          p2 = pDL->szNameSpec;
//
//          WB_ERROR_PRINT("TEMPORARY:  \"%s\" does not match \"%s\"\n", p1, p2);
//        }
      }
      else
      {
        WB_WARN_PRINT("%s: can't 'stat' %s, errno=%d (%08xH)\n", __FUNCTION__, pBuf, errno, errno);
      }
    }
  }

  if(pBuf)
  {
    WBFree(pBuf);
  }

  return iRval;

}

char *WBGetDirectoryListFileFullPath(const void *pDirectoryList, const char *szFileName)
{
char *pRval, *pBuf, *p1;
DIRLIST *pDL = (DIRLIST *)pDirectoryList;

  if(!pDirectoryList)
  {
    if(!szFileName || !*szFileName)
    {
      return NULL;
    }

    return WBGetCanonicalPath(szFileName);
  }

  if(szFileName && *szFileName == '/')
  {
    return WBGetCanonicalPath(szFileName); // don't need relative path
  }

  // TODO:  improve this, maybe cache buffer or string length...
  pBuf = (char *)WBAlloc(strlen(pDL->szPath) + 8 + (szFileName ? strlen(szFileName) : 0) + NAME_MAX);

  if(!pBuf)
  {
    return NULL;
  }

  strcpy(pBuf, pDL->szPath);
  p1 = pBuf + strlen(pBuf);
  if(p1 > pBuf && *(p1 - 1) != '/') // ends in a slash?
  {
    *(p1++) = '/';  // for now assume this
    *p1 = 0;  // by convention (though probably not necessary)
  }

  if(szFileName)
  {
    strcpy(p1, szFileName);
  }

  pRval = WBGetCanonicalPath(pBuf);
  WBFree(pBuf);

  return pRval;
}

char *WBGetSymLinkTarget(const char *szFileName)
{
char *pRval = WBAlloc(MAXPATHLEN + 2);

  if(pRval)
  {
    int iLen = readlink(szFileName, pRval, MAXPATHLEN);
    if(iLen <= 0)
    {
      WBFree(pRval);
      return NULL;
    }

    pRval[iLen] = 0; // assume < MAXPATHLEN for now...
  }

  return pRval;
}

char *WBGetDirectoryListSymLinkTarget(const void *pDirectoryList, const char *szFileName)
{
char *pTemp, *pRval;

  pTemp = WBGetDirectoryListFileFullPath(pDirectoryList, szFileName);

  if(!pTemp)
  {
    return NULL;
  }

  pRval = WBGetSymLinkTarget(pTemp);
  WBFree(pTemp);

  return pRval;
}

int WBStat(const char *szLinkName, unsigned long *pdwModeAttrReturn)
{
int iRval;
struct stat sF;


  iRval = stat(szLinkName, &sF);
  if(!iRval && pdwModeAttrReturn)
  {
    *pdwModeAttrReturn = sF.st_mode;
  }

  return iRval; // zero on success
}

int WBGetDirectoryListFileStat(const void *pDirectoryList, const char *szFileName,
                               unsigned long *pdwModeAttrReturn)
{
char *pTemp;
int iRval;

  pTemp = WBGetDirectoryListFileFullPath(pDirectoryList, szFileName);

  if(!pTemp)
  {
    return -1;
  }

  iRval = WBStat(pTemp, pdwModeAttrReturn);
  WBFree(pTemp);

  return iRval;
}

unsigned long long WBGetFileModDateTime(const char *szFileName)
{
int iRval;
struct stat sF;


  iRval = stat(szFileName, &sF);

  if(iRval)
  {
    return (unsigned long long)((long long)-1);
  }

  // TODO:  see whether st_mtime or st_ctime is larger, in case of total screwup by something else

  return sF.st_mtime; // mod time (as UNIX time_t value)
}

int WBCheckFileModDateTime(const char *szFileName, unsigned long long tVal)
{
unsigned long long tNewVal;


  tNewVal = WBGetFileModDateTime(szFileName);

  if(tNewVal == (unsigned long long)((unsigned long)-1)
     || tNewVal > tVal)
  {
    return 1;
  }
  else if(tNewVal < tVal)
  {
    return -1;
  }
  else
  {
    return 0;
  }
}



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
//       Copyright (c) 2019-2026 by S.F.T. Inc. - All rights reserved       //
//  Use, copying, and distribution of this software are licensed according  //
//    to the GPLv2, LGPLv2, or BSD license, as appropriate (see COPYING)    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS /* need POSIX compat without complaints */
#include <Windows.h>
#endif // WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#ifdef WIN32
#include <io.h>
#include <tchar.h>
#include <process.h>
#else // WIN32
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <dlfcn.h> /* dynamic library support */
#include <dirent.h>
#include <fnmatch.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/param.h> // for MAXPATHLEN and PATH_MAX (also includes limits.h in some cases)
#endif // WIN32
#include <sys/types.h>
#include <sys/stat.h>

#include "ForkMe.h"

#ifndef WIN32
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif // WIN32

// some defines - use 'malloc' and 'free' as-is

#define WBAlloc(X) malloc(X)
#define WBReAlloc(X,Y) realloc(X,Y)
#define WBFree(X) free(X)

#ifdef NO_DEBUG
#define WB_ERROR_PRINT(X, ...) { }
#define WB_WARN_PRINT(X, ...)  { }

#ifdef DEBUG_STDERR_TO_STDOUT
#undef DEBUG_STDERR_TO_STDOUT
#endif // DEBUG_STDERR_TO_STDOUT
#else // NO_DEBUG
// basic debug stuff
#ifndef WB_ERROR_PRINT
void error_message(const char *szFormat, ...);
//#define WB_ERROR_PRINT(X, ...)
#define WB_ERROR_PRINT error_message
#endif // WB_ERROR_PRINT
#ifndef WB_WARN_PRINT
void warning_message(const char *szFormat, ...);
#define WB_WARN_PRINT warning_message
//#define WB_WARN_PRINT(X, ...)
#endif // WB_WARN_PRINT

///////////////////////////
// TEMPORARY DEBUG DEFINES
//#define DEBUG_STDERR_TO_STDOUT

#endif // NO_DEBUG

// CONDITIONAL BUILD OPTIONS
#define NO_SHARED_LIB_SUPPORT /* when statically linking on Linux, you should enable this */


#ifdef WIN32
WB_PROCESS_ID pidInvalid = { 0, INVALID_HANDLE_VALUE, 0 };
#endif // WIN32



// some basic utilities
#define FOUR_HUNDRED_YEARS (400 * 365 + 24 * 4 + 1)
static int  total_days[13]={0,31,59,90,120,151,181,212,243,273,304,334,365};
static int  total_leap[13]={0,31,60,91,121,152,182,213,244,274,305,335,366};
static WB_TIME epoch_time = 25568; // 1900-based date offset from epoch

WB_TIME WBGetSystemTime(void)
{
#if defined(WIN32)

#ifdef _USE_32BIT_TIME_T /* see time.h */
#error you must not use 32-bit time_t value, use 64-bit only
#endif // _USE_32BIT_TIME_T

SYSTEMTIME st;
int64_t l1;
int n_years;
long adjustment;


   GetSystemTime(&st);

    // convert to days since 1/1/1970

   n_years = st.wYear - 1900;    /* # of years since 1900 */
   adjustment = 0;

   while(n_years<0)
   {
      n_years += 400;                       /* add 400 years */
      adjustment -= FOUR_HUNDRED_YEARS;    /* number of days in 400 years */
   }

   while(n_years >=400)
   {
      n_years -= 400;
      adjustment += FOUR_HUNDRED_YEARS;
   }
      /*    CALCULATE THE TOTAL NUMBER OF DAYS UP TO 1/1 THIS YEAR    */
      /* terms:  # of days + # of "Feb/29"s - # of non-leap centuries */

   l1 = 365L * n_years + ((n_years > 4)?((n_years - 1) >> 2):0)
        + ((n_years > 100)?(1 - (n_years - 1)/ 100):0);

   if(((st.wYear % 400)==0 || (st.wYear % 100)!=0) && (st.wYear % 4)==0)
      l1 += total_leap[st.wMonth - 1];    /* month-to-date totals (leap year) */
   else
      l1 += total_days[st.wMonth - 1];    /* month-to-date totals */

   l1 += st.wDay + adjustment; // days since 1/1/1900 (a '1')

   // convert to days since epoch (1/1/1970, a '1')
   l1 -= epoch_time;

   // convert to seconds (00:00:00 AM UTC)
   l1 *= 86400;

   return (WB_TIME)l1;
#elif __SIZEOF_LONG__ <= 4 // !WIN32, 32-bit time_t

#ifndef NO_DEBUG
#warning 32-bit time_t is only valid until 2038 - use 64-bit OS to avoid Y2K38
#warning rewrite this to avoid Y2K38, by getting year, month, day etc. and converting like for WIN32
#endif
//
// TODO - rewrite this to avoid Y2K38, by getting year, month, day etc. and converting like for WIN32
struct timeval tv;

  // 32-bit time_t - rewrite this to avoid Y2K38, by getting year, month, day etc. and converting like for WIN32

  gettimeofday(&tv, NULL); // for now, just use this.

  return (WB_TIME)tv.tv_sec;
#else // 64-bit time_t, !WIN32
struct timeval tv;

  gettimeofday(&tv, NULL);

  return (WB_TIME)tv.tv_sec;
#endif // WIN32
}

WB_UINT64 WBGetTimeIndex(void)
{
#ifdef WIN32
SYSTEMTIME st;
int64_t l1;
int n_years;
long adjustment;


   GetSystemTime(&st);

    // convert to days since 1/1/1970

   n_years = st.wYear - 1900;    /* # of years since 1900 */
   adjustment = 0;

   while(n_years<0)
   {
      n_years += 400;                       /* add 400 years */
      adjustment -= FOUR_HUNDRED_YEARS;    /* number of days in 400 years */
   }

   while(n_years >=400)
   {
      n_years -= 400;
      adjustment += FOUR_HUNDRED_YEARS;
   }
      /*    CALCULATE THE TOTAL NUMBER OF DAYS UP TO 1/1 THIS YEAR    */
      /* terms:  # of days + # of "Feb/29"s - # of non-leap centuries */

   l1 = 365L * n_years + ((n_years > 4)?((n_years - 1) >> 2):0)
        + ((n_years > 100)?(1 - (n_years - 1)/ 100):0);

   if(((st.wYear % 400)==0 || (st.wYear % 100)!=0) && (st.wYear % 4)==0)
      l1 += total_leap[st.wMonth - 1];    /* month-to-date totals (leap year) */
   else
      l1 += total_days[st.wMonth - 1];    /* month-to-date totals */

   l1 += st.wDay + adjustment; // days since 1/1/1900 (a '1')

   // convert to days since epoch (1/1/1970, a '1')
   l1 -= epoch_time;

   // convert to seconds (00:00:00 AM UTC)
   l1 *= 86400;

   // adjust for UTC time
   l1 += st.wHour * 3600L + st.wMinute * 60L + st.wSecond;

   // conver to mucroseconds
   l1 *= 1000000L;

   // add milliseconds from 'st' a microseconds
   l1 += st.wMilliseconds * 1000L;

   return (WB_UINT64)l1;
#elif __SIZEOF_LONG__ <= 4 // !WIN32, 32-bit time_t
//
// TODO - rewrite this to avoid Y2K38, by getting year, month, day etc. and converting like for WIN32
struct timeval tv;

  gettimeofday(&tv, NULL); // for now, just use this.

  return (WB_UINT64)tv.tv_sec * (WB_UINT64)1000000
         + (WB_UINT64)tv.tv_usec;
#else // 64-bit time_t, !WIN32
struct timeval tv;

  gettimeofday(&tv, NULL);

  return (WB_UINT64)tv.tv_sec * (WB_UINT64)1000000
         + (WB_UINT64)tv.tv_usec;
#endif // WIN32
}

void WBDelay(uint32_t uiDelay)  // approximate delay for specified period (in microseconds).  may be interruptible
{
#ifdef WIN32
  if(uiDelay <= 1000)
    Sleep(1);
  else
    Sleep(uiDelay / 1000);
#else // WIN32
#ifdef HAVE_NANOSLEEP
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

  tsp.tv_nsec = uiDelay * 1000;  // wait for .1 msec

  nanosleep(&tsp, NULL);
#else  // HAVE_NANOSLEEP

  usleep(uiDelay);  // 100 microsecs - a POSIX alternative to 'nanosleep'

#endif // HAVE_NANOSLEEP
#endif // WIN32
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
int iRval = -1;

  if(!szFileName || !*szFileName)
  {
    return -1; // always an error to create a 'blank' directory
  }

#ifdef WIN32
//#error not yet implemented
  iRval = !CreateDirectory(szFileName, NULL)   // TODO assign 'flags' for security?
        ? 0 : GetLastError();

  _chmod(szFileName, flags);  // attempt it
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
#ifdef WIN32
      int cb1 = SearchPath(NULL, szFileName, NULL, PATH_MAX, pRval,(TCHAR **)&p1);

      if(cb1 > 0)
      {
        pRval[cb1] = 0;
      }
      else
      {
        free(pRval);
        pRval = WBCopyString(szFileName);
      }

      if(pRval && !WBStat(pRval, NULL))
      {
        // this function returns non-zero if file not found
        if(0 > WBStat(pRval, NULL))
        {
          if(pRval)
          {
            free(pRval);
          }

          goto no_stat;
        }

        return pRval; // FOUND!
      }

      if(pRval)
      {
        free(pRval);
        pRval = NULL;
      }
#else // WIN32
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
#endif // WIN32
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
static const char szH[17]="0123456789ABCDEF";


#ifdef WIN32
char szTemp[MAX_PATH + 1];

  GetEnvironmentVariable("TEMP", szTemp, MAX_PATH);

  szDir = szTemp;

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
      h1 = CreateFile(pRval, GENERIC_READ|GENERIC_WRITE, 0,
                      NULL, // security descriptor - 0644 mode???
                      CREATE_ALWAYS,
                      /*FILE_ATTRIBUTE_TEMPORARY |*/ FILE_ATTRIBUTE_NORMAL,
                      NULL);
#else // !WIN32
      h1 = open(pRval, O_CREAT | O_EXCL | O_RDWR, 0644); // create file, using '644' permissions, fail if exists
      if(h1 < 0)
        h1 = INVALID_HANDLE_VALUE;
#endif // !WIN32

      if(h1 == INVALID_HANDLE_VALUE) // error
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
#ifdef WIN32
        CloseHandle(h1);
#else  // !WIN32
        close(h1);
#endif // !WIN32

        // add this file to the existing list of temp files to be destroyed
        // on exit from the program.

        break; // file name is valid and ready for use
      }
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
#ifdef WIN32
STARTUPINFO si;
PROCESS_INFORMATION pi;
#else // !WIN32
char **argv;
int i1, nItems, cbItems;
va_list va2;
#endif // WIN32
WB_PROCESS_ID hRval = WB_INVALID_PROCESS_ID;
WB_FILE_HANDLE hIn, hOut, hErr;


  // NOTE:  to avoid zombies, must assign SIGCHLD to 'SIG_IGN' or process them correctly
  //        (this is done in 'WBInit')

  hIn = hOut = hErr = WB_INVALID_FILE_HANDLE; // by convention (WIN32 needs this anyway)

  // FIRST, locate 'szAppName'

  pAppName = WBSearchPath(szAppName);

  if(!pAppName)
    return WB_INVALID_PROCESS_ID;

  if(hStdIn == WB_INVALID_FILE_HANDLE) // re-dir to/from /dev/null
  {
#ifndef WIN32
    hIn = open("/dev/null", O_RDONLY, 0);
#else // WIN32
    SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)WBAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

    if(pSD)
    {
      SECURITY_ATTRIBUTES sa;

      InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION);

      sa.nLength = sizeof(sa);
      sa.lpSecurityDescriptor = pSD;
      sa.bInheritHandle = TRUE; // what a pain

      hIn = CreateFile("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       &sa, OPEN_EXISTING, NULL, NULL);
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
      SECURITY_ATTRIBUTES sa;

      InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION);

      sa.nLength = sizeof(sa);
      sa.lpSecurityDescriptor = pSD;
      sa.bInheritHandle = TRUE; // what a pain

      hOut = CreateFile("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        &sa, OPEN_EXISTING, 0, NULL);
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
#ifdef DEBUG_STDERR_TO_STDOUT
    if(hStdOut != WB_INVALID_FILE_HANDLE)
    {
      if(!DuplicateHandle(GetCurrentProcess(), hStdOut,
                          GetCurrentProcess(), &hErr, GENERIC_WRITE,
                          TRUE, 0))
      {
        hErr = WB_INVALID_FILE_HANDLE;
      }
    }
    else
#endif // DEBUG_STDERR_TO_STDOUT
    {
      SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)WBAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

      if(pSD)
      {
        SECURITY_ATTRIBUTES sa;

        InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION);

        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = pSD;
        sa.bInheritHandle = TRUE; // what a pain

        hErr = CreateFile("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          &sa, OPEN_EXISTING, 0, NULL);
        WBFree(pSD);
      }
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
#ifndef WIN32
      close(hIn);
#else // WIN32
      CloseHandle(hIn);
#endif // WIN32
    }
    if(hOut != WB_INVALID_FILE_HANDLE)
    {
#ifndef WIN32
      close(hOut);
#else // WIN32
      CloseHandle(hOut);
#endif // WIN32
    }
    if(hErr != WB_INVALID_FILE_HANDLE)
    {
#ifndef WIN32
      close(hErr);
#else // WIN32
      CloseHandle(hErr);
#endif // WIN32
    }

    if(pAppName != szAppName)
    {
      WBFree(pAppName);
    }

    return WB_INVALID_PROCESS_ID;
  }

#ifdef WIN32

  memset(&si, 0, sizeof(si));
  memset(&pi, 0, sizeof(pi));

  si.cb = sizeof(si);

  si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  si.hStdInput = hIn;
  si.hStdOutput = hOut;
  si.hStdError = hErr;

  // make sure it is hidden
  si.wShowWindow = SW_HIDE;

#if 1
  // get application name from path

  p1 = _tcsrchr((TCHAR *)pAppName, '\\');
  if(p1)
  {
    pCur = WBCopyString(p1 + 1); // just the name
  }
  else
  {
    pCur = WBCopyString(pAppName);
  }
#else // 0,1
  pCur = WBCopyString(""); // need non-null pointer
#endif // 0

  // build the command line

  while(1)
  {
    pArg = va_arg(va, const TCHAR *);
    if(!pArg)
    {
      break;
    }

    WBCatString(&pCur, _T(" "));

    // if the parameter contains a space, windows requires quotes.
    if(strchr(pArg, ' '))
    {
      if(strchr(pArg, '"'))
      {
        WBCatString(&pCur, "'");
        WBCatString(&pCur, pArg); // caller must add quotes as needed
        WBCatString(&pCur, "'");
      }
      else
      {
        WBCatString(&pCur, "\"");
        WBCatString(&pCur, pArg); // caller must add quotes as needed
        WBCatString(&pCur, "\"");
      }
    }
    else
      WBCatString(&pCur, pArg); // caller must add quotes as needed
  }

  if(CreateProcess(pAppName, pCur, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS,
                   NULL, NULL, &si, &pi))  // it worked?
  {
      // wait for process to complete before continuing...

//        SetCursor(LoadCursor(NULL, IDC_WAIT));
    hRval.uiProcessID = pi.dwProcessId;
    hRval.hProcess = pi.hProcess;
    hRval.iCachedExitCode = -1;

    WaitForInputIdle(pi.hProcess, 1000); // wait up to 1 sec to help it run
  }
  else
  {
    hRval = WB_INVALID_PROCESS_ID;
  }

  free(pCur);
  pCur = NULL;

  CloseHandle(hIn);
  CloseHandle(hOut);
  CloseHandle(hErr);

#else // WIN32

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

#endif // WIN32

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

  if(WB_PROCESS_ID_INVALID(idRval))
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

static char * WBRunResultInternal(WB_FILE_HANDLE hStdIn, WB_INT32 *pExitCode, const char *szAppName, va_list va)
{
WB_PROCESS_ID idRval;
#ifdef WIN32
DWORD cb1;
#endif // WIN32
WB_FILE_HANDLE hP[2]; // [0] is read end, [1] is write end
char *p1, *p2, *pRval;
int i2, iRunning;
#ifndef WIN32
int i1, iStat;
#endif // WIN32
/*unsigned*/ int cbBuf;


  cbBuf = WBRUNRESULT_BUFFER_MINSIZE;
  pRval = WBAlloc(cbBuf);

  if(!pRval)
  {
    return NULL;
  }

  // use WBRunAsyncPipeV to create a process, with all stdout piped to a char * buffer capture
  // stdin and stderr still piped to/from /dev/null

  // create an anonymous pipe.  hP[0] is the INPUT pipe, hP[1] is the OUTPUT pipe
  // this is important in windows.  for POSIX it doesn't really matter which one you use,
  // but by convention [0] will be input, [1] will be output

  hP[0] = hP[1] = WB_INVALID_FILE_HANDLE;

#ifdef WIN32 /* the WINDOWS way */
  if(!CreatePipe(&(hP[0]), &(hP[1]), NULL, 0))
#else // !WIN32 (everybody else)
  if(0 > pipe(hP))
#endif // WIN32
  {
    WBFree(pRval);
//    WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (2)\n", __FUNCTION__);
    return NULL;
  }

#ifdef WIN32 /* the WINDOWS way */
  SetHandleInformation(hP[1], HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
#endif // WIN32

  idRval = WBRunAsyncPipeV(hStdIn, hP[1], // the 'write' end is passed as stdout
                           WB_INVALID_FILE_HANDLE, szAppName, va);

  if(WB_PROCESS_ID_INVALID(idRval))
  {
//    WB_ERROR_PRINT("TEMPORARY:  %s failed to run \"%s\" errno=%d\n", __FUNCTION__, szAppName, errno);

#ifdef WIN32 /* the WINDOWS way */
    CloseHandle(hP[0]);
    CloseHandle(hP[1]);
#else // !WIN32 (everybody else)
    close(hP[0]);
    close(hP[1]);
#endif // WIN32

    return NULL;
  }

#ifndef WIN32
  close(hP[1]); // by convention, this will 'widow' the read end of the pipe once the process is done with it
  hP[1] = INVALID_HANDLE_VALUE;

  fcntl(hP[0], F_SETFL, O_NONBLOCK); // set non-blocking I/O
#endif // WIN32

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
#ifdef WIN32 /* the Windows way */
    cb1 = 0;

    if(!PeekNamedPipe(hP[0], NULL, 0, NULL, &cb1, NULL))
    {
      DWORD dwErr;
check_pipe_no_data_avail:

      dwErr = GetLastError();

      if(!iRunning)
      {
        break; // assume end of file, process ended, bail out now
      }

      if(dwErr == ERROR_MORE_DATA || dwErr == ERROR_PIPE_BUSY || dwErr == ERROR_IO_PENDING
         || dwErr == ERROR_NO_MORE_ITEMS/* EAGAIN */ )
      {
        Sleep(10); // wait 1/2 msec
      }
      else
      {
        break; // an error of some kind, so bail out [pipe closed?]
      }
    }
    else if(!cb1)
    {
      if(!iRunning)
        break; // end of file
    }
    else if(!ReadFile(hP[0], p1, i2, &cb1, NULL))
    {
      goto check_pipe_no_data_avail;
    }
    else if(!cb1)  // this just might be an error
    {
      if(!iRunning)
        break; // end of file
    }
    else
    {
      p1 += cb1; // point past the # of bytes I just read in
      *p1 = 0; // by convention [to make sure the string is ALWAYS terminated with a 0-byte]
    }

    if(iRunning) // only if "still running"
    {
      // for waitpid(), if WNOHANG is specified and there are no stopped, continued or exited children, 0 is returned

      if(WBGetProcessState(idRval, pExitCode) <= 0) // ended or error
      {
        iRunning = 0; // my flag that it's not running
      }

      Sleep(10); // so I don't 'spin' and IO completes at end of process
    }
  }

  // always kill the process at this point (in case there was an error)

  Sleep(10); // wait 10 msec - prevents certain problems

  if(hP[1] != INVALID_HANDLE_VALUE)
    CloseHandle(hP[1]); // done with the pipes - close them now
  if(hP[0] != INVALID_HANDLE_VALUE)
    CloseHandle(hP[0]);

#else // !WIN32 - everybody else

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

#endif // WIN32

//  WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (4) pRval=%p *pRval=%c\n", __FUNCTION__, pRval, (char)(pRval ? *pRval : 0));

  return pRval;
}

int WBGetProcessState(WB_PROCESS_ID idProcess, WB_INT32 *pExitCode)
{
#ifdef WIN32
  int iRval = -1;
  HANDLE hProcess = idProcess.hProcess; //OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, idProcess);
  DWORD dwWait, dwTemp = -1;

  if(pExitCode)
    *pExitCode = -1;;

  if(hProcess == INVALID_HANDLE_VALUE)
  {
    if(idProcess.uiProcessID != 0) // for now this indicates valid.  process 0 is inaccessible
      return idProcess.iCachedExitCode; // return cached value

    return -1;
  }

  dwWait = WaitForMultipleObjects(1, &hProcess, FALSE, 10);

  if(dwWait == WAIT_TIMEOUT)
  {
    iRval = 1; // still running
  }
  else if(dwWait == WAIT_OBJECT_0) // process has stopped
  {
    iRval = 0;
  }
  else // if(dwWait == WAIT_ABANDONED_0)
  {
    iRval = -1;  // like 'quit' baby, quit!
  }

  if(!iRval)
  {
    if(GetExitCodeProcess(hProcess, &dwTemp))
    {
      if(pExitCode)
        *pExitCode = dwTemp;
      idProcess.iCachedExitCode = dwTemp;
    }
    else
    {
      iRval = -1; // oops
    }

    CloseHandle(idProcess.hProcess); // clean up
    idProcess.hProcess = INVALID_HANDLE_VALUE; //  because I closed it
  }

//  CloseHandle(hProcess);
  return iRval;

#else // WIN32
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
#endif // WIN32
}

char *WBRunResult(const char *szAppName, ...)
{
char *pRval;
va_list va;


  va_start(va, szAppName);

  pRval = WBRunResultInternal(WB_INVALID_FILE_HANDLE, NULL, szAppName, va);

  va_end(va);

  return pRval;
}

char *WBRunResult2(const char *szAppName, ...)
{
char *pRval;
int32_t nExitCode;
va_list va;


  va_start(va, szAppName);

  pRval = WBRunResultInternal(INVALID_HANDLE_VALUE, &nExitCode, szAppName, va);

  if(pRval && nExitCode)
  {
//    fprintf(stderr, "%s - %s ended with error code %d\n", __FUNCTION__, szAppName, nExitCode);
//    fflush(stderr);

    free(pRval);
    pRval = NULL;
  }

  va_end(va);

  return pRval;
}

#ifdef WIN32
struct __RunResult3_worker_thread_params
{
  volatile HANDLE hStdin[2];  // pipe for STDIN
  const void * pStdin;
  int cbStdin;
  volatile HANDLE hThread;
  volatile unsigned int dwThreadID;
  volatile int bStateFlag; // 0 = not running, 1 = running, -1 force stop
};

static unsigned int __stdcall __RunResult3_worker_thread(void *pData)
{
int i2;
const BYTE *p1;
DWORD cb1, cbData;
HANDLE hTemp;
struct __RunResult3_worker_thread_params *pParams = (struct __RunResult3_worker_thread_params *)pData;

  // so long as the process is alive, write data to the pipe
#define RUNRESULT3_BUFFER_LENGTH 256
  pParams->bStateFlag = 1; // indicates I'm running

  p1 = (const BYTE *)pParams->pStdin;
  cbData = pParams->cbStdin;

  while(cbData && pParams->dwThreadID)
  {
    cb1 = 0;
     // if buffer empty, fill it
#if 0
    if(!PeekNamedPipe(pParams->hStdin[0], NULL, 0, NULL, &cb1, NULL))
    {
      goto check_pipe_io_error;
    }
    else if(cb1 > RUNRESULT3_BUFFER_LENGTH / 2)
    {
      Sleep(20);
    }
    else
#endif // 0
    {
      i2 = RUNRESULT3_BUFFER_LENGTH - cb1; // available write space
      if(i2 > (int)cbData)
        i2 = cbData;

      if(!WriteFile(pParams->hStdin[1], p1, i2, &cb1, NULL))
      {
        DWORD dwErr;
#if 0
check_pipe_io_error:
#endif // 0

        dwErr = GetLastError();

        if(dwErr == ERROR_MORE_DATA || dwErr == ERROR_PIPE_BUSY || dwErr == ERROR_IO_PENDING
           || dwErr == ERROR_NO_MORE_ITEMS/* EAGAIN */ )
        {
          Sleep(10); // wait 1/2 msec
        }
        else
        {
          break; // an error of some kind, so bail out [pipe closed?]
        }
      }
      else if(!cb1)  // this just might be an error
      {
        // TODO: anything?
        Sleep(50);
      }
      else
      {
        p1 += cb1; // point past the # of bytes I just read in
        cbData -= cb1;
      }
    }
  }

  Sleep(10); // wait 10 msec

  // close my end of the pipe
  hTemp = pParams->hStdin[1];
  pParams->hStdin[1] = INVALID_HANDLE_VALUE;
  CloseHandle(hTemp);

  hTemp = pParams->hStdin[0];
  pParams->hStdin[0] = INVALID_HANDLE_VALUE;
  CloseHandle(hTemp);

  // mark as 'exited'
  pParams->bStateFlag = -1; // indicates I'm ending
  _endthreadex(0);
  return 0;
}
#endif // WIN32


char *RunResult3(const void *pStdin, int cbStdin, const char *szAppName, ...) // windows-specific
{
#ifdef WIN32
char *pRval;
INT32 nExitCode;
va_list va;
struct __RunResult3_worker_thread_params xParams = {{INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE},
                                                    NULL, 0, INVALID_HANDLE_VALUE, 0, 0 };


  va_start(va, szAppName);

  // create a pipe and worker thread for stdin using pStdin and cbStdin

  if(!CreatePipe((HANDLE *)&(xParams.hStdin[0]), (HANDLE *)&(xParams.hStdin[1]), NULL, 0))
  {
//    WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (2)\n", __FUNCTION__);
    return NULL;
  }

  SetHandleInformation(xParams.hStdin[0], HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

  // start worker thread
  xParams.hThread = (HANDLE)_beginthreadex(NULL, 65536, __RunResult3_worker_thread,
                                           &xParams, CREATE_SUSPENDED,
                                           (unsigned int *)&xParams.dwThreadID);

  if(xParams.hThread == INVALID_HANDLE_VALUE)
  {
//    WB_ERROR_PRINT("* ERROR * - unable to create thread, %d (%x) (a)\n",
//                   GetLastError(), GetLastError());
    goto error_exit;
  }

  xParams.pStdin = pStdin;
  xParams.cbStdin = cbStdin;

  ResumeThread(xParams.hThread);

  while(xParams.dwThreadID && xParams.bStateFlag == 0)
    Sleep(10); // wait for it to begin (avoid race condition if program does not start)

  pRval = WBRunResultInternal(xParams.hStdin[0], &nExitCode, szAppName, va);

  // wait for worker thread close

  if(pRval && nExitCode)
  {
#ifdef _DEBUG
    {
      char tbuf[1024];
      _sntprintf(tbuf, sizeof(tbuf), "%s ended with error code %d\n", szAppName, nExitCode);
#ifdef __cplusplus
      ::MessageBox
#else // __cplusplus
      MessageBox
#endif // __cplusplus
        (NULL, tbuf, "** EXTERNAL APPLICATION ERROR **", MB_OK |  MB_ICONERROR);
      OutputDebugString(pRval);
    }
#endif _DEBUG

    free(pRval);
    pRval = NULL;
  }

  va_end(va);

error_exit:

  xParams.dwThreadID = 0; // tells thread to exit

  if(xParams.hThread != INVALID_HANDLE_VALUE)
  {
    while(xParams.bStateFlag > 0)
      Sleep(10); // wait for it to end

    CloseHandle(xParams.hThread);
  }

  if(xParams.hStdin[1] != INVALID_HANDLE_VALUE)
    CloseHandle(xParams.hStdin[1]);

  if(xParams.hStdin[0] != INVALID_HANDLE_VALUE)
    CloseHandle(xParams.hStdin[0]);

  return pRval;

#else // WIN32

char *pRval, *pTemp = NULL;
va_list va;
WB_FILE_HANDLE hIn = WB_INVALID_FILE_HANDLE;


  va_start(va, szAppName);

  if(pStdin && cbStdin > 0)
  {
    unsigned int nLen = cbStdin;
    const unsigned char *szStdInBuf = (const unsigned char *)pStdin;

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

  pRval = WBRunResultInternal(hIn, NULL, szAppName, va);

  va_end(va);

  if(pTemp)
  {
    close(hIn);
    unlink(pTemp);

    WBFree(pTemp);
  }

  return pRval;

#endif // WIN32
}



char *WBRunResultWithInput(const char *szStdInBuf, const char *szAppName, ...)
{
char *pRval, *pTemp = NULL;
va_list va;
#ifdef WIN32
DWORD cb1;
#endif // WIN32
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

#ifdef WIN32
    hIn = CreateFile(pTemp, GENERIC_READ|GENERIC_WRITE,0,NULL,
                     OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
#else // WIN32
    hIn = open(pTemp, O_RDWR, 0);
#endif // WIN32

    if(hIn < 0)
    {
bad_file:
#ifdef WIN32
      DeleteFile(pTemp);
#else // WIN32
      unlink(pTemp);
#endif // WIN32
      WBFree(pTemp);

//      WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (2)\n", __FUNCTION__);

      va_end(va);
      return NULL;
    }

    if(
#ifdef WIN32
       !WriteFile(hIn, szStdInBuf, nLen, &cb1,NULL)
        || cb1 != nLen
#else // WIN32
       write(hIn, szStdInBuf, nLen) != (ssize_t)nLen
#endif // WIN32
       )
    {
#ifdef WIN32
      CloseHandle(hIn);
#else // WIN32
      close(hIn);
#endif // WIN32
      goto bad_file;
    }

#ifdef WIN32
    SetFilePointer(hIn, 0, NULL, FILE_BEGIN);
#else // WIN32
    lseek(hIn, 0, SEEK_SET); // rewind file
#endif // WIN32

//    WB_ERROR_PRINT("TEMPORARY:  %s HERE I AM (3) temp file \"%s\"\n", __FUNCTION__, pTemp);
  }

  pRval = WBRunResultInternal(hIn, NULL, szAppName, va);

  va_end(va);

  if(pTemp)
  {
#ifdef WIN32
      DeleteFile(pTemp);
#else // WIN32
    close(hIn);
    unlink(pTemp);
#endif // WIN32

    WBFree(pTemp);
  }

  return pRval;
}


// SHARED LIBRARIES

WB_MODULE WBLoadLibrary(const char * szModuleName)
{
#ifdef WIN32
  return LoadLibrary(szModuleName);
#else // !WIN32 aka POSIX
#ifndef NO_SHARED_LIB_SUPPORT
  return((WB_MODULE)dlopen(szModuleName, RTLD_LAZY | RTLD_LOCAL));
#else  // NO_SHARED_LIB_SUPPORT
  return NULL;
#endif // NO_SHARED_LIB_SUPPORT
#endif // WIN3,POSIX
}

void WBFreeLibrary(WB_MODULE hModule)
{
#ifdef WIN32
  FreeLibrary(hModule);
#else // !WIN32 aka POSIX
#ifndef NO_SHARED_LIB_SUPPORT
  dlclose(hModule);
#endif // NO_SHARED_LIB_SUPPORT
#endif // WIN32,POSIX
}

WB_PROCADDRESS WBGetProcAddress(WB_MODULE hModule, const char *szProcName)
{
#ifdef WIN32
  return GetProcAddress(hModule, szProcName);
#else // !WIN32 aka POSIX
#ifndef NO_SHARED_LIB_SUPPORT
// freebsd has the 'dlfunc' API, which is basically 'dlsym' cast to a function pointer
#ifdef __FreeBSD__
  return((WB_PROCADDRESS)dlfunc(hModule, szProcName));
#else // other POSIX systems - TODO, check for 'dlfunc' instead of the OS
  return((WB_PROCADDRESS)dlsym(hModule, szProcName));
#endif // 'dlfunc' check
#else  // NO_SHARED_LIB_SUPPORT
  return NULL;
#endif // NO_SHARED_LIB_SUPPORT
#endif // WIN32,POSIX
}

void * WBGetDataAddress(WB_MODULE hModule, const char *szDataName)
{
#ifdef WIN32
  return (void *)GetProcAddress(hModule, szDataName);
#else // !WIN32 aka POSIX
#ifndef NO_SHARED_LIB_SUPPORT
  return((void *)dlsym(hModule, szDataName));
#else  // NO_SHARED_LIB_SUPPORT
  return NULL;
#endif // NO_SHARED_LIB_SUPPORT
#endif // WIN32,POSIX
}


// THREADS

WB_THREAD_KEY WBThreadAllocLocal(void)
{
#ifdef WIN32
  return TlsAlloc();
#else // !WIN32 aka POSIX
WB_THREAD_KEY keyRval;
  if(!pthread_key_create(&keyRval, NULL))
  {
    return keyRval;
  }

  return (WB_THREAD_KEY)INVALID_HANDLE_VALUE;
#endif // WIN32,POSIX
}

void WBThreadFreeLocal(WB_THREAD_KEY keyVal)
{
#ifdef WIN32
  TlsFree(keyVal);
#else // !WIN32 aka POSIX
  pthread_key_delete(keyVal); // TODO:  check return?
#endif // WIN32,POSIX
}

void * WBThreadGetLocal(WB_THREAD_KEY keyVal)
{
#ifdef WIN32
  return TlsGetValue(keyVal);
#else // !WIN32 aka POSIX
  return pthread_getspecific(keyVal);
#endif // WIN32,POSIX
}

void WBThreadSetLocal(WB_THREAD_KEY keyVal, void *pValue)
{
#ifdef WIN32
  TlsSetValue(keyVal, pValue);
#else // !WIN32 aka POSIX
  pthread_setspecific(keyVal, pValue);
#endif // WIN32,POSIX
}

WB_THREAD WBThreadGetCurrent(void)
{
#ifdef WIN32
  return GetCurrentThreadId();
#else // !WIN32 aka POSIX
  return pthread_self();
#endif // WIN32,POSIX
}


#ifdef WIN32
struct WB_THREAD_PROC_PARAM
{
  WB_THREAD_PROC pFunction;
  void *pParam;
  volatile HANDLE hThread; // becomes NULL as a flag
};

DWORD WINAPI W32ThreadCaller(void *pParam0)
{
WB_THREAD_PROC pFunction;
void *pParam;
HANDLE hThread; // handle to me
struct WB_THREAD_PROC_PARAM *pP = (struct WB_THREAD_PROC_PARAM *)pParam0;
DWORD dwRval;

  pFunction = pP->pFunction;
  pParam = pP->pParam;
  hThread = pP->hThread;

  pP->hThread = NULL; // flag to creator

  dwRval = (DWORD)pFunction(pParam);  // do the thread

  CloseHandle(hThread);

  return dwRval;
}
#endif // WIN32

WB_THREAD WBThreadCreate(WB_THREAD_PROC function, void *pParam)
{
  WB_THREAD thrdRval = WB_INVALID_THREAD;
#ifdef WIN32
  struct WB_THREAD_PROC_PARAM pp;
  pp.pFunction = function;
  pp.pParam = pParam;

  // CreateThread returns a handle
  // consider calling _beginthreadex and using a wrapper to call _endthreadex
  pp.hThread = CreateThread(NULL, 0, W32ThreadCaller, &pp, CREATE_SUSPENDED, &thrdRval);

  if(pp.hThread) // returns NULL on error
  {
    ResumeThread(pp.hThread);  // thread proc wrapper closes it

    while(pp.hThread) // assigned 0 when safe for this function to return
      Sleep(0); // this should happen quickly

    return thrdRval;
  }
#else // !WIN32 aka POSIX

  // TODO:  call my own thread startup proc, passing a struct that contains
  //        'function' and 'pParam' as the param.  use a linked list of
  //        pre-allocated buffers for that.
  // see possible implementation, above

  if(!pthread_create(&thrdRval, NULL, function, pParam))
  {
    return thrdRval;
  }
#endif // WIN32,POSIX

  return WB_INVALID_THREAD;
}

void *WBThreadWait(WB_THREAD hThread)        // closes hThread, returns exit code, waits for thread to terminate (blocks)
{
#ifdef WIN32
  HANDLE hT = OpenThread(READ_CONTROL|SYNCHRONIZE,FALSE,hThread);
  if(hT)
  {
    DWORD dwR;
    WaitForSingleObject(hT, INFINITE);

    GetExitCodeThread(hT, &dwR);
    CloseHandle(hT);
    return (void *)dwR;
  }

  return NULL;
#else // !WIN32 aka POSIX
void *pRval = NULL;

  if(pthread_join(hThread, &pRval))
  {
    // TODO:  error return??
    pRval = (void *)-1;
  }

  return pRval;
#endif // WIN32,POSIX
}

int WBThreadRunning(WB_THREAD hThread)        // >0 if thread is running, <0 error - use 'pthread_kill(thread,0)' which returns ESRCH if terminated i.e. 'PS_DEAD'
{
int iR = -1;
#ifdef WIN32
  // use 'WaitForSingleObject with 0 timeout
  HANDLE hT = OpenThread(READ_CONTROL|SYNCHRONIZE,FALSE,hThread);
  if(hT)
  {
    DWORD dwR = WaitForSingleObject(hT, 0);
    if(dwR == WAIT_TIMEOUT)
      iR = 1;
    else if(dwR == WAIT_OBJECT_0)
      iR = 0;
    else
      iR = -1;

    CloseHandle(hT);
  }
  return iR;
#else // !WIN32 aka POSIX
  iR = pthread_kill(hThread,0);

  if(!iR)
  {
    return 1; // no signal sent, handle is valid (and did not exit)
  }

  if(iR == ESRCH)
  {
    return 0; // for now, allow this to indicate 'done'
  }

  return -1;
#endif // WIN32,POSIX
}

void WBThreadExit(void *pRval)
{
#ifdef WIN32
#else // !WIN32 aka POSIX
  pthread_exit(pRval);
#endif // WIN32,POSIX
}

void WBThreadClose(WB_THREAD hThread)
{
#ifdef WIN32
#else // !WIN32 aka POSIX
  pthread_detach(hThread);
#endif // WIN32,POSIX
}






// FILE SYSTEM INDEPENDENT FILE AND DIRECTORY UTILITIES
// UNIX/LINUX versions - TODO windows versions?

#define CHAR_MODE_BUFFSIZE 1048576

size_t WBReadFileIntoBuffer(const char *szFileName, char **ppBuf)
{
off_t cbLen = (off_t)-1;
size_t cbF;
int iChunk;
#ifdef WIN32
DWORD cb1;
#else // WIN32
int cb1, bCharMode = 0;
#endif // WIN32
char *pBuf;
WB_FILE_HANDLE hFile;


  // if the file cannot be "seek"d I use char mode but limit to 1Mb
  // this lets me read /proc files easily

  if(!ppBuf)
  {
    return (size_t)-1;
  }

#ifdef WIN32
  hFile = CreateFile(szFileName,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);

  if(hFile == INVALID_HANDLE_VALUE)
#else // WIN32
  hFile = open(szFileName, O_RDONLY); // open read only (assume no locking for now)

  if(hFile < 0)
#endif // WIN32
  {
    return (size_t)-1;
  }

  // how long is my file?

#ifdef WIN32
  cbLen = (unsigned long)SetFilePointer(hFile, 0, NULL, FILE_END); // location of end of file
#else // WIN32
  cbLen = (unsigned long)lseek(hFile, 0, SEEK_END); // location of end of file
#endif // WIN32

  if(cbLen == (off_t)-1)
  {
#ifdef WIN32
    CloseHandle(hFile);
    return (size_t)-1;
#else // WIN32
    *ppBuf = NULL; // make sure

    if(errno == EINVAL) // char mode file like /proc var?
    {
      cbLen = (off_t)CHAR_MODE_BUFFSIZE;
      bCharMode = 1;
    }
#endif // WIN32
  }
  else
  {
#ifdef WIN32
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN); // location of end of file
#else // WIN32
    lseek(hFile, 0, SEEK_SET); // back to beginning of file
#endif // WIN32
  }

  *ppBuf = pBuf = WBAlloc(cbLen + 1);

  if(!pBuf)
  {
    cbLen = (off_t)-1; // to mark 'error'
  }
  else if(cbLen >= 0)
  {
    *pBuf = 0;
    cbF = cbLen;

#ifndef WIN32
    if(bCharMode)
      cbLen = 0;
#endif // WIN32

    while(cbF > 0)
    {
#ifdef WIN32
      iChunk = 1048576; // 1MByte at a time
#else // WIN32
      if(bCharMode)
      {
        iChunk = 16;
      }
      else
      {
        iChunk = 1048576; // 1MByte at a time
      }
#endif // WIN32

      if((size_t)iChunk > cbF)
      {
        iChunk = (int)cbF;
      }

#ifdef WIN32
      if(ReadFile(hFile, pBuf, iChunk, &cb1, NULL))
        cb1 = -1;
#else // WIN32
      cb1 = read(hFile, pBuf, iChunk);
#endif // WIN32

      if(cb1 == -1)
      {
#ifndef WIN32
        if(errno == EAGAIN) // allow this
        {
          WBDelay(100);
          continue; // for now just do this
        }
#endif // WIN32

        cbLen = -1;
        break;
      }
      else if(!cb1) // EOF
      {
        break;  // done
      }

      if(cb1 != iChunk) // did not read enough bytes
      {
        iChunk = cb1; // for now
      }

      cbF -= iChunk;
      pBuf += iChunk;
      *pBuf = 0;  // I allocated an extra byte for this

#ifndef WIN32
      if(bCharMode)
      {
        cbLen += iChunk;  // reported length in char mode
      }
#endif // WIN32
    }
  }


#ifdef WIN32
  CloseHandle(hFile);
#else // WIN32
  close(hFile);
#endif // WIN32

  return (size_t) cbLen;
}

int WBWriteFileFromBuffer(const char *szFileName, const char *pBuf, size_t cbBuf)
{
WB_FILE_HANDLE hFile;
int iRval, iChunk;
#ifdef WIN32
DWORD cb1;
#endif // WIN32


  if(!pBuf)
  {
    return -1;
  }

#ifdef WIN32
  hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if(hFile == INVALID_HANDLE_VALUE)
#else // WIN32
  hFile = open(szFileName, O_CREAT | O_TRUNC | O_RDWR, 0666);  // always create with mode '666' (umask should apply)

  if(hFile < 0)
#endif // WIN32
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

#ifdef WIN32
    if(WriteFile(hFile, pBuf, iChunk, &cb1, NULL))
      iRval = -1;
    else
      iRval = cb1;
#else // WIN32
    iRval = write(hFile, pBuf, iChunk);
#endif // WIN32

    if(iRval < 0)
    {
      if(errno == EAGAIN)
      {
        WBDelay(100);

        // TODO:  time limit?  for now, no

        continue; // try again
      }

#ifdef WIN32
      CloseHandle(hFile);
#else // WIN32
      close(hFile);
#endif // WIN32
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

#ifdef WIN32
  CloseHandle(hFile);
#else // WIN32
  close(hFile);
#endif // WIN32

  return iRval;
}

int WBReplicateFilePermissions(const char *szProto, const char *szTarget)
{
#ifdef WIN32
  return -1; // just fail for now
#else // WIN32
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
#endif // WIN32
}

char *WBGetCurrentDirectory(void)
{
char *pRval = WBAlloc(MAXPATHLEN + 2);
int i1, cDirSep;

  if(pRval)
  {
#ifdef WIN32
    cDirSep = '\\';
    if(!GetCurrentDirectory(MAXPATHLEN + 1, pRval))
#else // WIN32
    cDirSep = '/';
    if(!getcwd(pRval, MAXPATHLEN))
#endif // WIN32
    {
      WBFree(pRval);
      pRval = NULL;
    }
  }

  // this function will always return something that ends in '/' (except on error)

  if(pRval)
  {
    i1 = strlen(pRval);

    if(i1 > 0 && pRval[i1 - 1] != cDirSep)
    {
      pRval[i1] = cDirSep;
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
#ifdef WIN32
    bRval = (sF.st_mode & _S_IFDIR) ? 1 : 0;
#else // WIN32
    bRval = S_ISDIR(sF.st_mode);
#endif // WIN32

  return(bRval);
}

char *WBGetCanonicalPath(const char *szFileName)
{
char *pTemp, *p1, *p2, *p3, *pRval = NULL;
#ifdef WIN32
char cDirSep = '\\';
#else // WIN32
char cDirSep = '/';
char *p4;
struct stat sF;
#endif // WIN32

  pTemp = WBCopyString(szFileName);

  if(!pTemp)
  {
    return NULL;
  }

  // step 1:  eliminate // /./

  p1 = pTemp;
#ifdef WIN32
  if(*p1 == cDirSep && p1[1] == cDirSep) // UNC path
    p1 += 2;
  else if(*p1 && *p1 != cDirSep && p1[1] == ':') // drive letter
    p1 += 2;
#endif // WIN32
  while(*p1 && p1[1])
  {
    if(*p1 == cDirSep && p1[1] == cDirSep)
    {
      memmove(p1, p1 + 1, strlen(p1 + 1) + 1);
    }
    else if(*p1 == cDirSep && p1[1] == '.' && p1[2] == cDirSep)
    {
      memmove(p1, p1 + 2, strlen(p1 + 2) + 1);
    }
    else
    {
      p1++;
    }
  }

  // step 2:  resolve each portion of the path, deal with '~' '.' '..', build new path.

  if(*pTemp == '~' && (pTemp[1] == cDirSep || !pTemp[1])) // first look for '~' at the beginning (only allowed there)
  {
    p1 = WBCopyString(getenv("HOME"));
    if(!p1 || !*p1)
    {
      if(p1)
        free(p1);

      p1 = WBCopyString(getenv("HOMEDRIVE"));
      p2 = WBCopyString(getenv("HOMEPATH"));

      if(p1 && !*p1)
      {
        free(p1);
        p1 = NULL;
      }

      if(p2 && !*p2)
      {
        free(p2);
        p2 = NULL;
      }

      if(!p1 && p2)
      {
        p1 = p2;
        p2 = NULL;
      }
      else if(p1)
      {
        p3 = WBCopyString(p1);
        if(p2)
        {
          WBCatString(&p3, p2);
          free(p2);
        }

        free(p1);
        p1 = p3;
      }
    }

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

      if(p3[strlen(p3) - 1] != cDirSep)
      {
        char bb[2];

        bb[0] = cDirSep;
        bb[1] = 0;

        WBCatString(&p3, bb);
      }

      p2 = pTemp + 1;
      if(*p2 == cDirSep)
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
    p2 = strchr(p1, cDirSep);
    if(!p2)      // no more '/'
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
    else if(p1 == p2   // from the root
#ifdef WIN32
            || (p2 == p1 + 2 && p1[1] == ':') // not a drive
#endif // WIN32
           )
    {
      char bb[4];

#ifdef WIN32
      if(p2 > p1)
      {
        bb[0] = p1[0];    // drive letter
        bb[1] = p1[1];
        bb[2] = cDirSep;
        bb[3] = 0;
      }
      else if(p1[1] == cDirSep) // UNC
      {
        bb[0] = cDirSep;  // root dir
        bb[1] = cDirSep;  // root dir
        bb[2] = 0;
        p2++;    // advance to network name next time
      }
      else
      {
        bb[0] = cDirSep;  // root dir
        bb[1] = 0;
      }
#else // WIN32
      bb[0] = cDirSep;
      bb[1] = 0;
#endif // WIN32

      pRval = WBCopyString(bb);
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
          if(*(p3 - 1) == cDirSep)
          {
            *p3 = 0;
            break;
          }

          p3--;
        }

        if(p3 <= pRval) // did not find a preceding '/' - this is an error
        {
          WB_ERROR_PRINT("%s:%d - did not find preceding '%c' - %s\n",
                         __FUNCTION__, __LINE__, cDirSep, pRval);

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

#ifndef WIN32
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
#endif // WIN32
    }

    p1 = p2 + 1;
  }

#ifndef WIN32
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
#endif // WIN32

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


char *WBGetSymLinkTarget(const char *szFileName)
{
#ifdef WIN32
    return NULL;  // fail it
#else // WIN32
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
#endif // WIN32
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

  return iRval; // zero on success (i.e. file exists)
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


// reading directories in a system-independent way

typedef struct __DIRLIST__
{
  const char *szPath, *szNameSpec;
#ifdef WIN32
  HANDLE hD;
  WIN32_FIND_DATA wfd;
#else // WIN32
  DIR *hD;
  struct stat sF;
  union
  {
    char cde[sizeof(struct dirent) + NAME_MAX + 2];
    struct dirent de;
  };
// actual composite 'search name' follows
#endif // WIN32
} DIRLIST;


void *WBAllocDirectoryList(const char *szDirSpec)
{
DIRLIST *pRval;
char *p1, *p2, cDirSep;
#ifdef WIN32
#endif // WIN32
int iLen, nMaxLen;
char *pBuf;
char tbuf[MAX_PATH * 2];

#ifdef WIN32
    cDirSep = '\\';
#else // WIN32
    cDirSep = '/';
#endif // WIN32

  if(!szDirSpec || !*szDirSpec)
  {
    WB_WARN_PRINT("WARNING - %s - invalid directory (NULL or empty)\n", __FUNCTION__);
    return NULL;
  }

  iLen = strlen(szDirSpec);
  nMaxLen = iLen + 32;
#ifdef WIN32
  nMaxLen += MAX_PATH;
#endif // WIN32

  pBuf = WBAlloc(nMaxLen);
  if(!pBuf)
  {
    WB_ERROR_PRINT("ERROR - %s - Unable to allocate memory for buffer size %d\n", __FUNCTION__, nMaxLen);
    return NULL;
  }

  if((szDirSpec[0] == cDirSep // path starts from the root
#ifdef WIN32
      && szDirSpec[0] == cDirSep) ||
     ((szDirSpec[0] >= 'A' && szDirSpec[0] <= 'Z' ||
       szDirSpec[0] >= 'a' && szDirSpec[0] <= 'z') &&
      szDirSpec[1] == ':'
#endif // WIN32
    ))
  {
    memcpy(pBuf, szDirSpec, iLen + 1);
  }
#ifdef WIN32
  else if(szDirSpec[0] == cDirSep) // path starts from the root, no drive letter
  {
    GetCurrentDirectory(sizeof(tbuf)-1, tbuf);
    if(tbuf[0] && tbuf[1] == ':' && tbuf[2] == cDirSep)
    {
      pBuf[0] = tbuf[0];
      pBuf[1] = ':';

      memcpy(pBuf + 2, szDirSpec, iLen + 1);
      iLen += 2;
    }
    else
      memcpy(pBuf, szDirSpec, iLen + 1);
  }
#endif // WIN32
  else // for now, force a path of './' to be prepended to path spec
  {
    pBuf[0] = '.';
    pBuf[1] = cDirSep;

    memcpy(pBuf + 2, szDirSpec, iLen + 1);
    iLen += 2;
  }

  // do a reverse scan until I find a '/'
  p1 = ((char *)pBuf) + iLen;
  while(p1 > pBuf && *(p1 - 1) != cDirSep)
  {
    p1--;
  }

//  WB_ERROR_PRINT("TEMPORARY - \"%s\" \"%s\" \"%s\"\n", pBuf, p1, szDirSpec);

  if(p1 > pBuf)
  {
    // found, and p1 points PAST the '/'.  See if it ends in '/' or if there are wildcards present
    if(!*p1) // name ends in '/'
    {
      if(p1 == (pBuf + 1) && *pBuf == cDirSep) // root dir
      {
        p1++;
      }
      else
      {
        *(p1 - 1) = 0;  // trim the final '/'
      }

      p1[0] = '*';
#ifdef WIN32
      p1[1] = '.';
      p1[2] = '*';
      p1[3] = 0;
#else // WIN32
      p1[1] = 0;
#endif // WIN32
    }
    else if(strchr(p1, '*') || strchr(p1, '?'))
    {
      if(p1 == (pBuf + 1) && *pBuf == cDirSep) // root dir
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
#ifdef WIN32
      p1[1] = '.';
      p1[2] = '*';
      p1[3] = 0;
#else // WIN32
      p1[1] = 0;
#endif // WIN32
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
#ifdef WIN32
      p1[1] = '.';
      p1[2] = '*';
      p1[3] = 0;
#else // WIN32
      p1[1] = 0;
#endif // WIN32
    }
  }

  pRval = WBAlloc(sizeof(DIRLIST) + iLen + strlen(p1) + 2);

  if(pRval)
  {
    pRval->szPath = pBuf;
    pRval->szNameSpec = p1;

    p2 = (char *)(pRval + 1);   // cache for dir + spec
    strcpy(p2, pBuf);
    p2 += strlen(p2);
    *(p2++) = cDirSep;
    strcpy(p2, p1);
    p1 = (char *)(pRval + 1);// complete assembled path spec

#ifdef WIN32
    memset(&pRval->wfd, 0, sizeof(pRval->wfd));
    pRval->hD = FindFirstFile(p1, &pRval->wfd);

    if(pRval->hD == INVALID_HANDLE_VALUE &&
       GetLastError() != ERROR_FILE_NOT_FOUND)  // can happen if there are no files to upload
#else // WIN32

    pRval->hD = opendir(pBuf);

//    WB_ERROR_PRINT("TEMPORARY - opendir for %s returns %p\n", pBuf, pRval->hD);

    if(pRval->hD == NULL)
#endif // WIN32
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

#ifdef WIN32
    if(pD->hD != INVALID_HANDLE_VALUE)
      FindClose(pD->hD);
#else // WIN32
    if(pD->hD)
      closedir(pD->hD);
#endif // WIN32

    if(pD->szPath)
    {
      WBFree((void *)(pD->szPath));
    }

    WBFree(pDirectoryList);
  }
}


#ifdef WIN32

int WBNextDirectoryEntry(void *pDirectoryList, char *szNameReturn,
                         int cbNameReturn, unsigned long *pdwModeAttrReturn)
{
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
  if(p1 > pBuf && *(p1 - 1) != '\\') // it does not already end in /
  {
    *(p1++) = '\\';  // for now assume this
    *p1 = 0;  // by convention
  }

  if(!pDL->wfd.cFileName[0])
  {
    goto exit_point;    // none left
  }

  iRval = 0;

  strncpy(p1, pDL->wfd.cFileName, NAME_MAX + 4);

  if(szNameReturn && cbNameReturn > 0)
  {
    strncpy(szNameReturn, p1, cbNameReturn);  // just the name
  }

  if(pdwModeAttrReturn)
  {
    *pdwModeAttrReturn = 0;
    if(WBStat(szNameReturn, pdwModeAttrReturn))
    {
      WB_WARN_PRINT("%s: can't 'stat' %s, errno=%d (%08xH)\n", __FUNCTION__, pBuf, errno, errno);
    }
  }

  if(pDL->hD != INVALID_HANDLE_VALUE)
  {
    if(!FindNextFile(pDL->hD, &pDL->wfd))
    {
      FindClose(pDL->hD);
      pDL->hD = INVALID_HANDLE_VALUE;
      memset(&pDL->wfd, 0, sizeof(pDL->wfd));
    }
  }

exit_point:
  if(pBuf)
  {
    WBFree(pBuf);
  }

  return iRval;

}

#else // WIN32

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

#endif // WIN32


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
    strcpy(p1, szFileName); // already checked buffer
  }

  pRval = WBGetCanonicalPath(pBuf);
  WBFree(pBuf);

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


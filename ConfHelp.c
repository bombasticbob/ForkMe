//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//            ____                __  _   _        _                        //
//           / ___| ___   _ __   / _|| | | |  ___ | | _ __     ___          //
//          | |    / _ \ | '_ \ | |_ | |_| | / _ \| || '_ \   / __|         //
//          | |___| (_) || | | ||  _||  _  ||  __/| || |_) |_| (__          //
//           \____|\___/ |_| |_||_|  |_| |_| \___||_|| .__/(_)\___|         //
//                                                   |_|                    //
//                                                                          //
//                       helper API for 'conf' files                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                    Copyright (c) 2020 by S.F.T. Inc.                     //
//  Use, copying, and distribution of this software are licensed according  //
//    to the GPLv2, LGPLv2, or BSD license, as appropriate (see COPYING)    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

/** \file ConfHelp.c
  * \brief 'configuration helper' implementation file for the X11 Work Bench Toolkit API
  *
  * X11 Work Bench Toolkit Toolkit API
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h> // for MAXPATHLEN and PATH_MAX (also includes limits.h in some cases)
#include <fcntl.h>
#include <netinet/in.h> // for htonl, htons, etc.
#include "ConfHelp.h"
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

// STRUCTURES

typedef struct _CONF_FILE_
{
  int iGlobal, iLocal;  // handles for global and local (typically read-only on global)
  int iOffsGlobal, iOffsLocal;  // offset in struct to global and local conf file names

  char *pBuf;  // a buffer for storing file names and other variable length data

  int cbBuf, cbBufMax;   // current size and max size of 'pBuf'

  file_help_buf_t *pfhbL, *pfhbG;  // local and global file help buffers

} CONF_FILE;


// GLOBAL VARIABLES



// INLINE UTILITIES

static __inline__ void trim_ends(const char **ppLeft, const char **ppRight)
{
  const char *p1=*ppLeft, *p2 = *ppRight;
  while(p1 < p2 && *p1 <= ' ')
  {
    p1++;
  }
  while(p2 > p1 && *(p2 - 1) <= ' ')
  {
    p2--;
  }

  *ppLeft = p1;
  *ppRight = p2;
}

static __inline__ char * DoMakePath(char *pSrc, const char *szPath, const char *szName, const char *szExt)
{
  const char *p1;
  char *pRval = pSrc;

  for(p1=szPath; *p1; p1++)
  {
    *(pRval++) = *p1;
  }

  if(*szPath && *(p1 - 1) != '/')
  {
    *(pRval++) = '/';  // add a slash to the end of the path
  }

  for(p1=szName; *p1; p1++)
  {
    *(pRval++) = *p1;
  }

  for(p1=szExt; *p1; p1++)
  {
    *(pRval++) = *p1;
  }

  *(pRval++) = 0;
  *pRval = 0;

  return pRval;
}


// configuration file utilities

void * CHOpenConfFile(const char *szAppName, int iFlags)
{
  CONF_FILE *pRval;
  char *p1, *p2, *p3, *p4, /* *p5, */ *p6;
  struct stat st;

  static const char szGlobalPath[] = "/etc/"; GLOBAL_PATH;
//  static const char szGlobalXPath[] = GLOBAL_XPATH;
  static const char szLocalPath[] = "~/.config/"; //LOCAL_PATH;
  static const char szConf[] = ".conf";
  char szLocalPath0[PATH_MAX];


  // create struct

  pRval = (CONF_FILE *)WBAlloc(sizeof(*pRval) + strlen(szAppName) * 4 + sizeof(szGlobalPath)
                               /*+ sizeof(szGlobalXPath)*/ + sizeof(szLocalPath) + 16 + PATH_MAX * 2);
  if(!pRval)
  {
    return NULL;
  }

  bzero(pRval, sizeof(*pRval));  // make sure it's zero'd out

  // construct file names

  p1 = (char *)pRval + sizeof(*pRval);  // this is the start of string buffers
  // it is also the global path, by default either /etc or /usr/local/etc

  p2 = DoMakePath(p1, szGlobalPath, szAppName, szConf);
  p3 = p2; //  p3 = DoMakePath(p2, szGlobalXPath, szAppName, szConf);

//  if(!(iFlags & CH_FLAGS_GLOBAL)) // not "global only"
  {
    // NOTE:  the new standard for config files is:
    //
    // ~/.config/application/whatever
    // ~/.local/share/application/whatever
    //
    // the old standard was ~/.application/whatever
    //
    // TODO:  a utility to move old files to new location?  yeah probably not...


    strlcpy(szLocalPath0, szLocalPath, sizeof(szLocalPath0)); // ~/.local/share/
    if(!szLocalPath0[0] || szLocalPath0[strlen(szLocalPath0)-1] != '/') // unlikely
    {
      strlcat(szLocalPath0, "/", sizeof(szLocalPath0));
    }
    strlcat(szLocalPath0, szAppName, sizeof(szLocalPath0));  // now contains ~/.local/share/appname

    // make sure the directory 'szLocalPath0' exists
    p4 = WBGetCanonicalPath(szLocalPath0);
    if(p4)
    {
      if(!WBIsDirectory(p4)) // see if it exists first...
      {
        WBMkDir(p4, 0755); // TODO:  check user's UMASK ???
      }
      strlcpy(szLocalPath0, p4, sizeof(szLocalPath0)); // the canonical path
      WBFree(p4);
    }

    if(szLocalPath0[strlen(szLocalPath0) - 1] != '/')
    {
      strlcat(szLocalPath0, "/", sizeof(szLocalPath0));
    }

    p4 = DoMakePath(p3, szLocalPath0, LOCAL_CONF_NAME, szConf); // first THIS one

    p6 = WBGetCanonicalPath(szLocalPath);
    if(p6)
    {
// NOTE:  p5 not being used; commented out because of linux gcc warnings
//      p5 = DoMakePath(p4, p6, szAppName, szConf); // then THIS one
      WBFree(p6);
    }
    else
    {
// NOTE:  p5 not being used; commented out because of linux gcc warnings
//      p5 = DoMakePath(p4, szLocalPath, szAppName, szConf); // alternate (uncanonical) name
    }
  }
//  else
//  {
//    p4 = NULL;  // warning avoidance (uninitialized variable, actually won't matter)
//  }

  if(!stat(p1, &st) && S_ISREG(st.st_mode))
  {
//    if(iFlags & CH_FLAGS_WRITE)
    {
      pRval->iGlobal = open(p1,O_RDWR);
    }
//    else
//    {
//      pRval->iGlobal = -1;
//    }

    if(pRval->iGlobal == -1)
    {
      pRval->iGlobal = open(p1,O_RDONLY);
    }
  }

  if(pRval->iGlobal != -1)
  {
    pRval->iOffsGlobal = (int)(p1 - (char *)pRval);
  }
  else if(!stat(p2, &st) && S_ISREG(st.st_mode))
  {
//    if(iFlags & CH_FLAGS_WRITE)
    {
      pRval->iGlobal = open(p2,O_RDWR);
    }
//    else
//    {
//      pRval->iGlobal = -1;
//    }
    if(pRval->iGlobal == -1)
    {
      pRval->iGlobal = open(p2,O_RDONLY);
    }
    if(pRval->iGlobal != -1)
    {
      pRval->iOffsGlobal = (int)(p2 - (char *)pRval);
    }
  }

#if 0
  if(!(iFlags & CH_FLAGS_GLOBAL)) // not "global only"
  {
    // if local file does not exist, create it (always open read/write)

    if(!stat(p3, &st) && S_ISREG(st.st_mode))
    {
      pRval->iLocal = open(p3,O_RDWR);
    }
    else
    {
      pRval->iLocal = open(p3,O_CREAT|O_RDWR,0644); // use 0644 for now - TODO:  check umask
    }

    if(pRval->iLocal != -1)
    {
      pRval->iOffsLocal = (int)(p3 - (char *)pRval);
    }
    else // try the OTHER local
    {
      // if local file does not exist, create it (always open read/write)

      if(!stat(p4, &st) && S_ISREG(st.st_mode))
      {
        pRval->iLocal = open(p4,O_RDWR);
      }
      else
      {
        pRval->iLocal = open(p4,O_CREAT|O_RDWR,0644); // use 0644 for now - TODO:  check umask
      }

      if(pRval->iLocal != -1)
      {
        pRval->iOffsLocal = (int)(p4 - (char *)pRval);
      }
    }
  }
//  else // GLOBAL ONLY
//  {
//    pRval->iLocal = -1;
//    pRval->iOffsLocal = 0;
//  }
#endif // 0

  // next read and parse the files

  if(pRval->iLocal != -1)
  {
    pRval->pfhbL = FBGetFileBufViaHandle(pRval->iLocal);
    if(pRval->pfhbL)
    {
      FBParseFileBuf(pRval->pfhbL);
    }
    else
    {
      CHDestroyConfFile(pRval);
      return NULL;
    }
  }

  if(pRval->iGlobal != -1)
  {
    pRval->pfhbG = FBGetFileBufViaHandle(pRval->iGlobal);
    if(pRval->pfhbG)
    {
      FBParseFileBuf(pRval->pfhbG);
    }
    else
    {
      CHDestroyConfFile(pRval);
      return NULL;
    }
  }

  return pRval;
}

void CHCloseConfFile(void * pFile)
{
  CONF_FILE *pTemp = (CONF_FILE *)pFile;

  if(!pTemp)
  {
    return;
  }

  if(pTemp->iLocal != -1)
  {
    if(pTemp->pfhbL && FBIsFileBufDirty(pTemp->pfhbL))
    {
      FBWriteFileBufHandle(pTemp->iLocal, pTemp->pfhbL);
    }

    close(pTemp->iLocal);
    pTemp->iLocal = -1;
  }

  if(pTemp->iGlobal != -1)
  {
    if(pTemp->pfhbG && FBIsFileBufDirty(pTemp->pfhbG))
    {
      FBWriteFileBufHandle(pTemp->iGlobal, pTemp->pfhbG);
    }

    close(pTemp->iGlobal);
    pTemp->iGlobal = -1;
  }
}

void CHDestroyConfFile(void * pFile)
{
  CONF_FILE *pTemp = (CONF_FILE *)pFile;

  if(!pTemp)
  {
    return;
  }

  CHCloseConfFile(pFile);

  if(pTemp->pfhbL)
  {
    FBDestroyFileBuf(pTemp->pfhbL);
  }

  if(pTemp->pfhbG)
  {
    FBDestroyFileBuf(pTemp->pfhbG);
  }
}


// Utilities to find stuff within a config file
// on entry ppStart points to the beginning of the line, and ppEnd points to the end of it
// on return, ppStart points to the start of the data, ppEnd points to the end of it (excluding comments)

static void __get_line_strip_comments__(const char **ppStart, const char **ppEnd)
{
  const char *p1 = *ppStart, *p2 = *ppEnd;

  // skip leading white space
  while(p1 < p2 && *p1 <= ' ')
  {
    p1++;
  }
  if(p1 >= p2)
  {
    *ppStart = *ppEnd = p2;

    return;
  }

  *ppStart = p1;

  while(p1 < p2)
  {
    if(*p1 == '=')  // everything to the right of '=' isn't a comment
    {
      break;
    }

    if(*p1 == ';')  // comment?
    {
      p2 = p1;
      break;
    }

    p1++;
  }

  // now trim off any trailing white space
  p1 = *ppStart;
  while(p2 > p1 && *(p2 - 1) <= ' ')
  {
    p2--;
  }

  *ppEnd = p2;
}

static void __find_section__(void *hFile, const char *szSection,
                             const char **ppSection, const char **ppEndSection)
{
  int i1, /* i2,*/ iSectionLen;
  const char *p1, *p2, *pSection, *pEndSection;
  CONF_FILE *pTemp = (CONF_FILE *)hFile;

  *ppSection = pSection = NULL;
  *ppEndSection = pEndSection = NULL;

  if(!pTemp ||
     ((!pTemp->pfhbL || !pTemp->pfhbL->ppLineBuf) &&
      (!pTemp->pfhbG || !pTemp->pfhbG->ppLineBuf)))
  {
    return;
  }

  iSectionLen = strlen(szSection);

  if(pTemp->pfhbL)
  {
    for(i1=0; i1 < pTemp->pfhbL->lLineCount; i1++)
    {
      // search for the section header
      p1 = pTemp->pfhbL->ppLineBuf[i1];
      p2 = pTemp->pfhbL->ppLineBuf[i1 + 1];
      if(!p2)
      {
        p2 = pTemp->pfhbL->cData + pTemp->pfhbL->lBufferCount;
      }

      __get_line_strip_comments__(&p1, &p2);

      if((p2 - p1) >= iSectionLen + 2 && *p1 == '[' && *(p2 - 1) == ']')
      {
        p1++;
        p2--;
        trim_ends(&p1, &p2);
        if((p2 - p1) == iSectionLen && !strncasecmp(p1, szSection, iSectionLen))
        {
          pSection = pTemp->pfhbL->ppLineBuf[i1 + 1];
          break;
        }
      }
    }

    if(pSection)
    {
      // starting with the current position, keep going until I find another section

      for(i1++; i1 < pTemp->pfhbL->lLineCount; i1++)
      {
        // search for the section header
        p1 = pTemp->pfhbL->ppLineBuf[i1];
        p2 = pTemp->pfhbL->ppLineBuf[i1 + 1];
        if(!p2)
        {
          p2 = pTemp->pfhbL->cData + pTemp->pfhbL->lBufferCount;
        }

        __get_line_strip_comments__(&p1, &p2);

        if((p2 - p1) > 2 && *p1 == '[' && *(p2 - 1) == ']')
        {
          pEndSection = pTemp->pfhbL->ppLineBuf[i1];
          break;
        }
      }

      if(!pEndSection)
      {
        pEndSection = pTemp->pfhbL->cData + pTemp->pfhbL->lBufferCount;
      }

      // return values
      *ppSection = pSection;
      *ppEndSection = pEndSection;
    }
  }

  if(pTemp->pfhbG && !*ppSection) // not found yet
  {
    *ppSection = pSection = NULL; // make sure
    *ppEndSection = pEndSection = NULL;

    for(i1=0; i1 < pTemp->pfhbG->lLineCount; i1++)
    {
      // search for the section header
      p1 = pTemp->pfhbG->ppLineBuf[i1];
      p2 = pTemp->pfhbG->ppLineBuf[i1 + 1];
      if(!p2)
      {
        p2 = pTemp->pfhbG->cData + pTemp->pfhbG->lBufferCount;
      }

      __get_line_strip_comments__(&p1, &p2);

      if((p2 - p1) >= iSectionLen + 2 && *p1 == '[' && *(p2 - 1) == ']')
      {
        p1++;
        p2--;
        trim_ends(&p1, &p2);
        if((p2 - p1) == iSectionLen && !strncasecmp(p1, szSection, iSectionLen))
        {
          pSection = pTemp->pfhbG->ppLineBuf[i1 + 1];
          break;
        }
      }
    }

    if(!pSection)
    {
//      fprintf(stderr, "pSection is NULL\n");
      return;
    }

    // starting with the current position, keep going until I find another section

    for(i1++; i1 < pTemp->pfhbG->lLineCount; i1++)
    {
      // search for the section header
      p1 = pTemp->pfhbG->ppLineBuf[i1];
      p2 = pTemp->pfhbG->ppLineBuf[i1 + 1];
      if(!p2)
      {
        p2 = pTemp->pfhbG->cData + pTemp->pfhbG->lBufferCount;
      }

      __get_line_strip_comments__(&p1, &p2);

      if((p2 - p1) > 2 && *p1 == '[' && *(p2 - 1) == ']')
      {
        pEndSection = pTemp->pfhbG->ppLineBuf[i1];
        break;
      }
    }

    if(!pEndSection)
    {
      pEndSection = pTemp->pfhbG->cData + pTemp->pfhbG->lBufferCount;
    }

    // return values
    *ppSection = pSection;
    *ppEndSection = pEndSection;
  }

//  WB_ERROR_PRINT("TEMPORARY: %s \"%s\" section %d:\n", __FUNCTION__, szSection, (int)(pEndSection - pSection));
//  WB_ERROR_PRINT("%-.*s\n------------------------\n", (int)(pEndSection - pSection), pSection);

  fflush(stderr);
}

static int __enum_conf_file_sections__(void *hFile, char *szData, int cbData)
{
  int i1, i2;
  const char *p1, *p2;
  CONF_FILE *pTemp = (CONF_FILE *)hFile;

  if(!pTemp || !pTemp->pfhbL || !pTemp->pfhbL->ppLineBuf)
  {
    // TODO:  check globals also
    return -1;
  }

  for(i1=0, i2=0; i1 < pTemp->pfhbL->lLineCount; i1++)
  {
    // search for the section header
    p1 = pTemp->pfhbL->ppLineBuf[i1];
    p2 = pTemp->pfhbL->ppLineBuf[i1 + 1];
    if(!p2)
    {
      p2 = pTemp->pfhbL->cData + pTemp->pfhbL->lBufferCount;
    }

    __get_line_strip_comments__(&p1, &p2);

//    WB_ERROR_PRINT("TEMPORARY:  %s  \"%-.*s\"\n", __FUNCTION__, (int)(p2 - p1), p1);

    if((p2 - p1) >= 2 && *p1 == '[' && *(p2 - 1) == ']')
    {
      p1++;
      p2--;
      trim_ends(&p1, &p2);
      if(p2 > p1)
      {
        i2 += (p2 - p1) + 1;  // calculate additional space needed (always)
        if(cbData >= (p2 - p1) + 2)
        {
          // add the text for the section header to 'szData' if there's room for it
          memcpy(szData, p1, p2 - p1);
          szData[p2 - p1] = 0;
          szData[p2 - p1 + 1] = 0;

          szData += (p2 - p1) + 1;
          cbData -= (p2 - p1) + 1;
        }
        else
        {
          cbData = 0;  // because there's no more room
        }
      }
    }
  }

  // TODO:  enumerate global sections also

  i2++;  // always need room for 1 more
  return i2;
}


// Utilities to query and assign values within a config file

int CHGetConfFileString(void * hFile, const char *szSection,
                        const char *szIdentifier, char *szData, int cbData)
{
  int i1, i2, iIdentifierLen;
  const char *p1, *p2, *pSection, *pEndSection;
  CONF_FILE *pTemp = (CONF_FILE *)hFile;
  file_help_buf_t *pFHB = NULL;


  if(!pTemp || ((!pTemp->pfhbL || !pTemp->pfhbL->ppLineBuf)
             && (!pTemp->pfhbG || !pTemp->pfhbG->ppLineBuf)))
  {
    if(!pTemp)
    {
      WB_ERROR_PRINT("%s - hFile/pTemp is NULL\n", __FUNCTION__);
    }
    else
    {
      WB_ERROR_PRINT("%s - 'linebuf' problem - %p %p %p %p\n", __FUNCTION__,
                     pTemp->pfhbL, pTemp->pfhbL->ppLineBuf,
                     pTemp->pfhbG, pTemp->pfhbG->ppLineBuf);
    }

    return -1;
  }

  if(!szSection || !*szSection)  // empty section == get a list of all of them (ignore szIdentifier)
  {
    return __enum_conf_file_sections__(hFile, szData, cbData);
  }

  iIdentifierLen = strlen(szIdentifier);

  __find_section__(hFile, szSection, &pSection, &pEndSection);
  if(!pSection)
  {
    WB_ERROR_PRINT("TEMPORARY: %s - did not find section \"%s\"\n", __FUNCTION__, szSection);
    return -1;
  }

  // search for 'szIdentifier string' followed by '='

  if(pTemp->pfhbL &&
     pTemp->pfhbL->ppLineBuf &&
     pTemp->pfhbL->lLineCount > 0 &&
     pTemp->pfhbL->ppLineBuf[0] < pSection &&
     pTemp->pfhbL->ppLineBuf[pTemp->pfhbL->lLineCount - 1] >= pSection)
  {
    pFHB = pTemp->pfhbL;
  }
  else if(pTemp->pfhbG &&
          pTemp->pfhbG->ppLineBuf &&
          pTemp->pfhbG->lLineCount > 0 &&
          pTemp->pfhbG->ppLineBuf[0] < pSection &&
          pTemp->pfhbG->ppLineBuf[pTemp->pfhbG->lLineCount - 1] >= pSection)
  {
    pFHB = pTemp->pfhbG;
  }
  else
  {
    return -1;
  }


  for(i1=0; i1 < pFHB->lLineCount && pFHB->ppLineBuf[i1] < pSection; i1++)
    ;

  for(; i1 < pFHB->lLineCount && pFHB->ppLineBuf[i1] < pEndSection; i1++)
  {
    p1 = pFHB->ppLineBuf[i1];
    p2 = pFHB->ppLineBuf[i1 + 1];

    if(!p2)
    {
      p2 = pFHB->cData + pFHB->lBufferCount;
    }

    while(p1 < p2 && *p1 <= ' ')
    {
      p1++;
    }

    if(*p1 == ';') // comment
    {
      continue;
    }

    if(p2 - p1 > iIdentifierLen &&
       !strncasecmp(szIdentifier, p1, iIdentifierLen) &&
       p1[iIdentifierLen] == '=')
    {
      // FOUND!  eliminate trailing newline but keep other white space (for now)
      p1 += iIdentifierLen + 1;
      if(p2 > p1 && *(p2 - 1) == '\n')
      {
        if(p2 > (p1 + 1) && *(p2 - 1) == '\n' && *(p2 - 2) == '\r')
        {
          p2--;
        }

        p2--;
      }
      // copy string into destination buffer and return the length of the actual data
      i2 = cbData;
      if(i2 > (p2 - p1))
      {
        i2 = p2 - p1;
      }
      if(i2 > 0)
      {
        memcpy(szData, p1, i2);
      }

      if(i2 < cbData)
      {
        szData[i2] = 0;  // as a matter of course
      }

      return p2 - p1;
    }
  }

  // not found locally - try global

  // TODO:  global


//  WB_ERROR_PRINT("TEMPORARY: %s - did not find section \"%s\" item \"%s\"\n", __FUNCTION__, szSection, szIdentifier);

  return -1;  // not found
}

int CHWriteConfFileString(void * hFile, const char *szSection,
                          const char *szIdentifier, const char *szData)
{
  int i1, i2, iSectionLine, iSectionLen, iIdentifierLen, iDataLen;
  char *pBuf;
  const char *p1, *p2, *pSection, *pEndSection;
  CONF_FILE *pTemp = (CONF_FILE *)hFile;
  file_help_buf_t *pFHB;


  if(!pTemp ||
     ((!pTemp->pfhbL || !pTemp->pfhbL->ppLineBuf) &&
      (!pTemp->pfhbG || !pTemp->pfhbG->ppLineBuf)))
  {
    return -1;
  }
  if(!szSection || !*szSection)
  {
    return -1;  // don't allow this on write
  }

  iIdentifierLen = strlen(szIdentifier);
  iDataLen = szData ? strlen(szData) : -1;

  __find_section__(hFile, szSection, &pSection, &pEndSection);
  if(pSection)
  {
    if(pTemp->pfhbL &&
       pTemp->pfhbL->ppLineBuf &&
       pTemp->pfhbL->lLineCount > 0 &&
       pTemp->pfhbL->ppLineBuf[0] < pSection &&
       pTemp->pfhbL->ppLineBuf[pTemp->pfhbL->lLineCount - 1] >= pSection)
    {
      pFHB = pTemp->pfhbL;
    }
    else if(pTemp->pfhbG &&
            pTemp->pfhbG->ppLineBuf &&
            pTemp->pfhbG->lLineCount > 0 &&
            pTemp->pfhbG->ppLineBuf[0] < pSection &&
            pTemp->pfhbG->ppLineBuf[pTemp->pfhbG->lLineCount - 1] >= pSection)
    {
      // TODO:  verify global CAN be written.  if not, assume 'local'
      pFHB = pTemp->pfhbG;
    }
    else
    {
      pSection = NULL; // will be added to 'local' or 'global' as needed
      pFHB = NULL;
    }
  }
  else
  {
    pFHB = NULL;
  }


  if(pFHB)
  {
    // search for 'szIdentifier string' followed by '='

    for(i1=0; i1 < pFHB->lLineCount && pFHB->ppLineBuf[i1] < pSection; i1++)
      ;

    iSectionLine = i1;

    for(; i1 < pFHB->lLineCount && pFHB->ppLineBuf[i1] < pEndSection; i1++)
    {
      p1 = pFHB->ppLineBuf[i1];
      p2 = pFHB->ppLineBuf[i1 + 1];

      if(!p2)
      {
        p2 = pFHB->cData + pFHB->lBufferCount;
      }

      while(p1 < p2 && *p1 <= ' ')
      {
        p1++;
      }

      if(*p1 == ';') // comment
      {
        continue;
      }

      if(p2 - p1 > iIdentifierLen &&
         !strncasecmp(szIdentifier, p1, iIdentifierLen) &&
         p1[iIdentifierLen] == '=')
      {
        // FOUND!  replace or delete contents (NULL szData --> delete)
        if(!szData)
        {
          FBDeleteLineFromFileBuf(pFHB, i1);
        }
        else
        {
          pBuf = (char *)WBAlloc(iDataLen + iIdentifierLen + 4);

          if(!pBuf)
          {
            WB_ERROR_PRINT("%s - 'pBuf' NULL (c)\n", __FUNCTION__);

            return -1;  // error
          }

          // todo:  search backwards from 'i1' for blanks & comments
          sprintf(pBuf, "%s=%s", szIdentifier, szData);

          if(pFHB == pTemp->pfhbG)
          {
            FBReplaceLineInFileBuf(&(pTemp->pfhbG), i1, pBuf);
          }
          else if(pFHB == pTemp->pfhbL)
          {
            FBReplaceLineInFileBuf(&(pTemp->pfhbL), i1, pBuf);
          }
          else
          {
            WBFree(pBuf);
            return -1; // error
          }

          WBFree(pBuf);
        }

        return 0;  // success
      }
    }

    if(!szData)
    {
      return 0; // success [deleting something that's not there is 'OK']
    }


    // since the section doesn't contain this entry, create it after the
    // last entry, searching back from 'pEndSection' skipping blank lines and comments
    // until I get to 'pSection' or an entry, whichever happens first.

    pBuf = (char *)WBAlloc(iDataLen + iIdentifierLen + 4);

    if(!pBuf)
    {
      WB_ERROR_PRINT("%s - 'pBuf' NULL (a)\n", __FUNCTION__);

      return -1;  // error
    }

    // search backwards from 'i1' for blank lines.  insert right after
    // the last non-blank line ABOVE where I am.

    while(i1 > iSectionLine)
    {
      p1 = pFHB->ppLineBuf[i1 - 1];
      if(p1 && *p1 != '\r' && *p1 != '\n')
      {
        break;
      }

      i1--;
    }

    sprintf(pBuf, "%s=%s", szIdentifier, szData);

    if(pTemp->pfhbL /*&&
       (!(pTemp->pfhbG) || !(pTemp->iFlags & CH_FLAGS_GLOBAL))*/) // local first, unless the 'global' flag is set
    {
      FBInsertLineIntoFileBuf(&(pTemp->pfhbL), i1, pBuf);
    }
    else if(pTemp->pfhbG)
    {
      FBInsertLineIntoFileBuf(&(pTemp->pfhbG), i1, pBuf);
    }

    WBFree(pBuf);

    return 0;  // ok!
  }

  if(!szData)
  {
    return 0;  // already deleted, no need for further effort
  }

  iSectionLen = strlen(szSection);

  // allocate memory for the section and the entry, and add each in its turn
  i1 = iSectionLen + 4;
  if(i1 < (iDataLen + iIdentifierLen + 2))
  {
    i1 =  iDataLen + iIdentifierLen + 2;
  }

  pBuf = (char *)WBAlloc(i1 + 2);

  if(!pBuf)
  {
    WB_ERROR_PRINT("%s - 'pBuf' NULL (b)\n", __FUNCTION__);

    return -1;  // error
  }

  sprintf(pBuf, "[%s]\n", szSection);

  if(pTemp->pfhbL /*&& // always do local FIRST if I can
     (!(pTemp->pfhbG) || !(pTemp->iFlags & CH_FLAGS_GLOBAL))*/)
  {
    i2 = pTemp->pfhbL->lLineCount;
    FBInsertLineIntoFileBuf(&(pTemp->pfhbL), pTemp->pfhbL->lLineCount, pBuf);
    if(i2 >= pTemp->pfhbL->lLineCount)
    {
      WBFree(pBuf);

      WB_ERROR_PRINT("%s - FBInsertLineIntoFileBuf failed (a)\n", __FUNCTION__);
      return -1;
    }

    // now do it again, this time for the data
    sprintf(pBuf, "%s=%s\n", szIdentifier, szData);

    i2 = pTemp->pfhbL->lLineCount;
    FBInsertLineIntoFileBuf(&(pTemp->pfhbL), pTemp->pfhbL->lLineCount, pBuf);
    WBFree(pBuf);

    if(i2 >= pTemp->pfhbL->lLineCount)
    {
      WB_ERROR_PRINT("%s - FBInsertLineIntoFileBuf failed (b)\n", __FUNCTION__);
      return -1;
    }
  }
  else if(pTemp->pfhbG) // global [TODO:  verify global can be written]
  {
    i2 = pTemp->pfhbG->lLineCount;
    FBInsertLineIntoFileBuf(&(pTemp->pfhbG), pTemp->pfhbG->lLineCount, pBuf);
    if(i2 >= pTemp->pfhbG->lLineCount)
    {
      WBFree(pBuf);
      WB_ERROR_PRINT("%s - FBInsertLineIntoFileBuf failed (c)\n", __FUNCTION__);
      return -1;
    }
    // now do it again, this time for the data
    sprintf(pBuf, "%s=%s\n", szIdentifier, szData);

    i2 = pTemp->pfhbG->lLineCount;
    FBInsertLineIntoFileBuf(&(pTemp->pfhbG), pTemp->pfhbG->lLineCount, pBuf);
    WBFree(pBuf);

    if(i2 >= pTemp->pfhbG->lLineCount)
    {
      WB_ERROR_PRINT("%s - FBInsertLineIntoFileBuf failed (d)\n", __FUNCTION__);
      return -1;
    }
  }
  else
  {
    WBFree(pBuf);
    WB_ERROR_PRINT("%s - pfhbG and pfhbL are both NULL\n", __FUNCTION__);

    return -1; // error
  }

  return 0;  // success!
}

int CHGetConfFileInt(void * hFile, const char *szSection, const char *szIdentifier)
{
  int iLen;
  char tbuf[64];

  memset(tbuf, 0, sizeof(tbuf));

  if((iLen = CHGetConfFileString(hFile, szSection, szIdentifier, tbuf, sizeof(tbuf) - 1)) > 0)
  {
    char *p1, *p2;

    tbuf[iLen] = 0;

    for(p1 = tbuf; *p1 && *p1 <= ' '; p1++)
      ; // find first non-white-space

    p2 = tbuf;
    while(*p1)
    {
      *(p2++) = *(p1++);
    }

    *p2 = 0;

    if(tbuf[0])
    {
      return atoi(tbuf);
    }
  }

  return 0;
}

int CHWriteConfFileInt(void * hFile, const char *szSection, const char *szIdentifier, int iData)
{
  int iFlag;
  char tbuf[64];
  char *p1;

  if(iData < 0)
  {
    iFlag = -1;
    iData = -iData;
  }
  else
  {
    iFlag = 0;  // a sign flag, temporarily
  }

  p1 = tbuf + sizeof(tbuf) - 1;

  *p1 = 0;
  do
  {
    *(--p1) = (char)('0' + iData % 10);

    iData /= 10;

  } while(p1 > (tbuf + 1) && iData);

  if(iFlag < 0)
  {
    *(--p1) = '-';
  }

  return CHWriteConfFileString(hFile, szSection, szIdentifier, p1);
}



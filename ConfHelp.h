//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//           ____                __  _   _        _           _             //
//          / ___| ___   _ __   / _|| | | |  ___ | | _ __    | |__          //
//         | |    / _ \ | '_ \ | |_ | |_| | / _ \| || '_ \   | '_ \         //
//         | |___| (_) || | | ||  _||  _  ||  __/| || |_) |_ | | | |        //
//          \____|\___/ |_| |_||_|  |_| |_| \___||_|| .__/(_)|_| |_|        //
//                                                  |_|                     //
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


// conf file format:
// [section header]
// data identifier=data value

/** \file ConfHelp.h
  * \brief 'configuration helper' main header file for the X11 Work Bench Toolkit API
  *
  * X11 Work Bench Toolkit Toolkit API
**/

/** \defgroup conf_file Configuration File Utilities
  * \ingroup ConfHelp
  *
  * \brief Configuration File Utilities, for application or user-defined config files
**/

/** \defgroup desktop_settings Desktop Settings utilities
  * \ingroup ConfHelp
  *
  * \brief Desktop Settings Utilities, to query desktop-defined settings like fonts, colors, mouse click times, etc.
**/

/** \defgroup text_xml XML-specific Text Utilities
  * \ingroup text
  *
  * \brief Specialized text utility functions for parsing XML data
**/

#ifndef CONFHELP_H_INCLUDED
#define CONFHELP_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


//////////////////////////////////
// configuration file  management
//////////////////////////////////

/** \ingroup conf_file
  * \brief open configuration file for read/write, optionally creating it, based on application name
  *
  * \param szAppName The name of the application for which a configuration file is to be opened (no extension)
  * \param iFlags One or more of the CH_FLAGS, specifically CH_FLAGS_DEFAULT, CH_FLAGS_GLOBAL, or CH_FLAGS_WRITE
  * \return pointer to a configuration file object that can be queried, or NULL on error
  *
  * open conf file (if it exists), optionally for writing.  Creates file as needed (write mode only)\n
  * write mode for global config may require root privileges\n
  * when done, you must close the returned configuration file object pointer with \ref CHCloseConfFile\n
  * The function may block as needed.  Use \ref CHDestroyConfFile to close AND free memory resources.
  *
  * Header File:  ConfHelp.h
**/
void * CHOpenConfFile(const char *szAppName, int iFlags);

/** \ingroup conf_file
  * \brief close configuration file opened by \ref CHOpenConfFile(), but does NOT free memory resources
  *
  * \param hFile The configuration file object returned by \ref CHOpenConfFile()
  *
  * Use this function to close the configuration file that was opened by \ref CHOpenConfFile()\n
  * Memory resources will NOT be freed (values will remain cached as needed).  Use this function to simply
  * remove the open file reference but leave the data in memory for later use.
  *
  * Header File:  ConfHelp.h
**/
void CHCloseConfFile(void * hFile);

/** \ingroup conf_file
  * \brief destroy configuration file opened by \ref CHOpenConfFile(), freeing memory resources (but not the files)
  *
  * \param hFile The configuration file object returned by \ref CHOpenConfFile()
  *
  * This function will close the configuration file and destroy the cached information created by \ref CHOpenConfFile()
  * but does not physically destroy the file on disk.
  *
  * Header File:  ConfHelp.h
**/
void CHDestroyConfFile(void * hFile);

/** \ingroup conf_file
  * \brief obtain a string from a configuration file
  *
  * \param hFile the configuration file object pointer returned by \ref CHOpenConfFile
  * \param szSection a pointer to an ASCII 0-byte terminated string containing the section name
  * \param szIdentifier a pointer to an ASCII 0-byte terminated string identifying the (unique) data entry
  * \param szData a buffer in which to return the string data
  * \param cbData the size of the buffer pointed to by szData
  * \return the total length of the string (excluding the zero byte), or -1 on error (or if value not found)
  *
  * Use this function to obtain a configuration parameter 'szIdentifier' from section 'szSection', and save
  * the data into 'szData', which is 'cbData' bytes in length.  The function will return the total length
  * of the resulting string.  The function returns -1 on error, or if the data value is not located.
  *
  * Header File:  ConfHelp.h
**/
int CHGetConfFileString(void * hFile, const char *szSection,
                        const char *szIdentifier, char *szData, int cbData);

/** \ingroup conf_file
  * \brief write a string to a configuration file
  *
  * \param hFile the configuration file object pointer returned by \ref CHOpenConfFile
  * \param szSection a pointer to an ASCII 0-byte terminated string containing the section name
  * \param szIdentifier a pointer to an ASCII 0-byte terminated string identifying the (unique) data entry
  * \param szData a buffer pointer to an ASCII 0-byte terminated string containing the data value
  * \return non-zero value on success, or zero on error
  *
  * Writes a string value to the configuration file for 'szIdentifier' in 'szSection'.  The function returns a
  * non-zero value if successful, or a value of 0 if an error occurred.
  *
  * Header File:  ConfHelp.h
**/
int CHWriteConfFileString(void * hFile, const char *szSection,
                          const char *szIdentifier, const char *szData);


/** \ingroup conf_file
  * \brief obtain an integer value from a configuration file
  *
  * \param hFile the configuration file object pointer returned by \ref CHOpenConfFile
  * \param szSection a pointer to an ASCII 0-byte terminated string containing the section name
  * \param szIdentifier a pointer to an ASCII 0-byte terminated string identifying the (unique) data entry
  * \return the integer value, or zero on error or if not found
  *
  * Use this function to query an integer value for 'szIdentifier' in 'szSection'.  The function will return
  * a non-zero value on success, or a zero value on error, or if not found.\n
  * If you need to distinguish an actual zero value from an error or 'not found' condition, consider using
  * the \ref CHGetConfFileString() function instead.
  *
  * Header File:  ConfHelp.h
**/
int CHGetConfFileInt(void * hFile, const char *szSection, const char *szIdentifier);

/** \ingroup conf_file
  * \brief write an integer value to a configuration file
  *
  * \param hFile the configuration file object pointer returned by \ref CHOpenConfFile
  * \param szSection a pointer to an ASCII 0-byte terminated string containing the section name
  * \param szIdentifier a pointer to an ASCII 0-byte terminated string identifying the (unique) data entry
  * \param iData the integer data value
  * \return non-zero value on success, or zero on error
  *
  * Writes an integer value to the configuration file for 'szIdentifier' in 'szSection'.  The function returns a
  * non-zero value if successful, or a value of 0 if an error occurred.
  *
  * Header File:  ConfHelp.h
**/
int CHWriteConfFileInt(void * hFile, const char *szSection, const char *szIdentifier, int iData);



#ifdef __cplusplus
};
#endif // __cplusplus


#endif // CONFHELP_H_INCLUDED


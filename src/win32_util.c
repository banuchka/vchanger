/*  win32_util.c
 *
 *  This file is part of vchanger by Josh Fisher.
 *
 *  vchanger copyright (C) 2008-2012 Josh Fisher
 *
 *  vchanger is free software.
 *  You may redistribute it and/or modify it under the terms of the
 *  GNU General Public License version 2, as published by the Free
 *  Software Foundation.
 *
 *  vchanger is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vchanger.  See the file "COPYING".  If not,
 *  write to:  The Free Software Foundation, Inc.,
 *             59 Temple Place - Suite 330,
 *             Boston,  MA  02111-1307, USA.
 */

#include "config.h"

#ifdef HAVE_WINDOWS_H

#include "targetver.h"
#include <windows.h>
#include <winerror.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "win32_util.h"

#ifndef ENODATA
#define ENODATA EIO
#endif
#ifndef ENOMEDIUM
#define ENOMEDIUM ENOSPC
#endif

/*-------------------------------------------------
 *  Convert 100 ns intervals since 1601-01-01 00:00:00 UTC in 'ftime' to
 *  seconds since 1970-01-01 00:00:00 UTC and return as type time_t.
 *-------------------------------------------------*/
time_t ftime2utime(const FILETIME *ftime)
{
   uint64_t ut = ftime->dwHighDateTime;
   ut <<= 32;
   ut |= ftime->dwLowDateTime; /* 100 ns intervals since FILETIME epoch */
   ut -= EPOCH_FILETIME; /* convert to 100 ns intervals since Unix epoch */
   ut /= 10000000; /* convert to seconds since Unix epoch */
   return (time_t)ut;
}


/*-------------------------------------------------
 *  Utility function to convert an ANSI string 'str' to a UTF-16 string.
 *  'u16str' is pointer to a pointer to a caller supplied buffer where the
 *  UTF-16 string will be created. 'u16size' is a pointer to a size_t variable
 *  that contains the size of the supplied buffer. On entry, if the pointer that
 *  'u16str' points to is NULL, then a buffer for the result UTF-16 string will
 *  be allocated using malloc. A pointer to the allocated buffer will be
 *  returned in *u16str and the size of the allocated buffer (in bytes) will be
 *  returned in *u16size. It is the caller's responsibility to free the
 *  allocated buffer. If the caller supplies a buffer then no memory will be
 *  allocated, and if the buffer is too small then NULL will be returned.
 *  On success, a pointer to the UTF-16 string is returned.
 *  On error, NULL is returned.
 *-------------------------------------------------*/
wchar_t* AnsiToUTF16(const char *str, wchar_t **u16str, size_t *u16size)
{
   size_t needed;
   UINT acp = GetACP();  /* Use current active code page for conversion */

   if (!str) {
      return NULL; /* NULL input string is an error */
   }
   /* Determine size of buffer needed for the UTF-16 string */
   needed = MultiByteToWideChar(acp, 0, str, -1, NULL, 0);
   if (*u16str) {
      /* If caller supplied a buffer, check that it's large enough */
      if (*u16size < needed) {
         return NULL; /* caller's buffer is too small */
      }
   } else {
      /* If caller did not supply buffer, then allocate one */
      *u16size = needed * sizeof(wchar_t);
      *u16str = (wchar_t*)malloc(*u16size);
   }
   /* Do the ANSI to UTF-16 conversion */
   MultiByteToWideChar(acp, 0, str, -1, *u16str, *u16size);
   return *u16str;
}


/*-------------------------------------------------
 *  Utility function to convert UTF-16 string 'u16str' to an ANSI string.
 *  'str' is a pointer to a pointer to a caller supplied buffer where the UTF-8
 *  string will be created. 'str_size' is a pointer to a size_t variable that
 *  contains the size of the supplied buffer. On entry, if the pointer that
 *  'str' points to is NULL, then a buffer for the result ANSI string will
 *  be allocated using malloc. A pointer to the allocated buffer will be
 *  returned in *str and the size of the allocated buffer (in bytes) will
 *  be returned in *str_size. It is the caller's responsibility to free the
 *  allocated buffer.If the caller supplies a buffer then no memory will be
 *  allocated, and if the buffer is too small then NULL will be returned.
 *  On success, a pointer to the ANSI string is returned.
 *  On error, NULL is returned.
 *-------------------------------------------------*/
char *UTF16ToAnsi(const wchar_t *u16str, char **str, size_t *str_size)
{
   size_t needed;
   UINT acp = GetACP();  /* Use current code page for conversion */

   if (!u16str) {
      return NULL; /* NULL input string is an error */
   }
   /* Determine size of buffer needed for UTF-8 string */
   needed = WideCharToMultiByte(acp, 0, u16str, -1, NULL, 0, NULL, NULL);
   if (*str) {
      /* If caller supplied a buffer, check that it's large enough */
      if (*str_size < needed) {
         return NULL; // caller's buffer is too small */
      }
   } else {
      /* If caller did not supply buffer, then allocate one */
      *str_size = needed;
      *str = (char*)malloc(*str_size);
   }
   /* Do the UTF-16 to ANSI conversion */
   WideCharToMultiByte(acp, 0, u16str, -1, *str, *str_size, NULL, NULL);
   return *str;
}


/*-------------------------------------------------
 *  Translate win32 error codes to errno
 *-------------------------------------------------*/
int w32errno(DWORD werr)
{
   switch (werr) {
   case ERROR_SUCCESS:
      return 0;
   case ERROR_ACCESS_DENIED:
   case ERROR_SHARING_VIOLATION:
   case ERROR_LOCK_VIOLATION:
      return EACCES;
   case ERROR_NO_PROC_SLOTS:
   case ERROR_MORE_DATA:
   case ERROR_MAX_THRDS_REACHED:
   case ERROR_ACTIVE_CONNECTIONS:
   case ERROR_DEVICE_IN_USE:
      return EAGAIN;
   case ERROR_INVALID_HANDLE:
      return EBADF;
   case ERROR_BUSY:
   case ERROR_PIPE_BUSY:
   case ERROR_PIPE_CONNECTED:
   case ERROR_SIGNAL_PENDING:
   case ERROR_CHILD_NOT_COMPLETE:
      return EBUSY;
   case ERROR_WAIT_NO_CHILDREN:
      return ECHILD;
   case ERROR_POSSIBLE_DEADLOCK:
      return EDEADLOCK;
   case ERROR_FILE_EXISTS:
   case ERROR_ALREADY_EXISTS:
      return EEXIST;
   case ERROR_PROCESS_ABORTED:
   case ERROR_NOACCESS:
      return EFAULT;
   case ERROR_INVALID_AT_INTERRUPT_TIME:
      return EINTR;
   case ERROR_INVALID_DATA:
   case ERROR_INVALID_PARAMETER:
   case ERROR_FILENAME_EXCED_RANGE:
   case ERROR_META_EXPANSION_TOO_LONG:
   case ERROR_INVALID_SIGNAL_NUMBER:
   case ERROR_THREAD_1_INACTIVE:
   case ERROR_BAD_PIPE:
   case ERROR_NO_TOKEN:
   case ERROR_NEGATIVE_SEEK:
   case ERROR_BAD_USERNAME:
      return EINVAL;
   case ERROR_OPEN_FAILED:
   case ERROR_SIGNAL_REFUSED:
   case ERROR_NO_SIGNAL_SENT:
   case ERROR_IO_DEVICE:
   case ERROR_CRC:
      return EIO;
   case ERROR_TOO_MANY_OPEN_FILES:
      return EMFILE;
   case ERROR_NO_MORE_SEARCH_HANDLES:
   case ERROR_NO_MORE_FILES:
      return ENFILE;
   case ERROR_HANDLE_EOF:
      return ENODATA;
   case ERROR_BAD_DEVICE:
   case ERROR_BAD_UNIT:
   case ERROR_INVALID_DRIVE:
      return ENODEV;
   case ERROR_FILE_NOT_FOUND:
   case ERROR_PATH_NOT_FOUND:
   case ERROR_INVALID_NAME:
   case ERROR_BAD_PATHNAME:
   case ERROR_BAD_NETPATH:
   case ERROR_BAD_NET_NAME:
      return ENOENT;
   case ERROR_SHARING_BUFFER_EXCEEDED:
      return ENOLCK;
   case ERROR_NOT_CONNECTED:
   case ERROR_NOT_READY:
      return ENOMEDIUM;
   case ERROR_NOT_ENOUGH_MEMORY:
   case ERROR_OUTOFMEMORY:
      return ENOMEM;
   case ERROR_DISK_FULL:
   case ERROR_END_OF_MEDIA:
   case ERROR_EOM_OVERFLOW:
   case ERROR_NO_DATA_DETECTED:
   case ERROR_HANDLE_DISK_FULL:
      return ENOSPC;
   case ERROR_CALL_NOT_IMPLEMENTED:
   case ERROR_NOT_SUPPORTED:
      return ENOSYS;
   case ERROR_DIRECTORY:
      return ENOTDIR;
   case ERROR_DIR_NOT_EMPTY:
      return ENOTEMPTY;
   case ERROR_FILE_INVALID:
      return ENXIO;
   case ERROR_NOT_OWNER:
   case ERROR_CANNOT_MAKE:
      return EPERM;
   case ERROR_BROKEN_PIPE:
   case ERROR_NO_DATA:
      return EPIPE;
   case ERROR_WRITE_PROTECT:
      return EROFS;
   case ERROR_SETMARK_DETECTED:
   case ERROR_BEGINNING_OF_MEDIA:
      return ESPIPE;
   case ERROR_NOT_SAME_DEVICE:
      return EXDEV;
   }
   return ENOSYS;
}

#endif


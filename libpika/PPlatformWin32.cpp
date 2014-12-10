/*
 *  PPlatform.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PPlatform.h"

#include <cerrno>
#include <ctime>

#ifdef PIKA_WIN
#   include <windows.h>
#endif

#pragma comment(lib, "Winmm.lib") // timeGetTime

#if defined(PIKA_BORLANDC)
#   include <dir.h>
#   define _finddata_t                      ffblk
#   define _findfirst(buf, data)            findfirst(buf, data, 0)
#   define _findnext(srchHandle, data)      findnext(data)
#   define _findclose(srchHandle)           0
#endif

errno_t setenv(const char* name, const char* val)
{
    return _putenv_s(name, val);
}

errno_t unsetenv(const char* name)
{
	return setenv(name, "");
}

void Pika_Sleep(u4 msecs)
{
    Sleep(msecs);
}

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#   define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#   define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

int Pika_gettimeofday(Pika_timeval* tv, Pika_timezone* tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag = 0;
    
    if (tv)
    {
        GetSystemTimeAsFileTime(&ft);

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        /*converting file time to unix epoch*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS; 
        tmpres /= 10;  /*convert into microseconds*/
        tv->tv_sec = (s8)(tmpres / 1000000UL);
        tv->tv_usec = (s8)(tmpres % 1000000UL);
    }
    
    if (tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }
    return 0;
}

struct TimePeriod
{
    TimePeriod() 
    { 
        end_period = (timeBeginPeriod(1) != TIMERR_NOERROR); 
    }
    
    ~TimePeriod()
    {
        if (end_period)
          timeEndPeriod(1); 
    }
    bool end_period;
};

static TimePeriod sTimePeriod;

unsigned long Pika_Milliseconds()
{
    return timeGetTime();
}

bool Pika_CreateDirectory(const char* pathName)
{
    return CreateDirectory(pathName, 0) != 0;
}

bool Pika_RemoveDirectory(const char* pathName)
{
    return RemoveDirectory(pathName) != 0;
}

bool Pika_DeleteFile(const char* filename, bool evenReadOnly)
{
    if (evenReadOnly)
        SetFileAttributes(filename, FILE_ATTRIBUTE_NORMAL);

    return DeleteFile(filename) != 0;
}

bool Pika_MoveFile(const char* src, const char* dest, bool canReplace)
{
    u4 flags = MOVEFILE_COPY_ALLOWED;

    if (canReplace)
        flags |= MOVEFILE_REPLACE_EXISTING;

    return MoveFileEx(src, dest, MOVEFILE_COPY_ALLOWED) != 0;
}

bool Pika_CopyFile(const char* src, const char* dest, bool canReplace)
{
    return CopyFile(src, dest, !canReplace) != 0;
}

bool Pika_SetCurrentDirectory(const char* dir)
{
    return SetCurrentDirectory(dir) != 0;
}

bool Pika_GetCurrentDirectory(char* buff, size_t len)
{
    return GetCurrentDirectory((DWORD)len, buff) != 0;
}

char* Pika_GetCurrentDirectory()
{
    char* buff = (char*)Pika_malloc(sizeof(char) * (PIKA_MAX_PATH + 1));

    if (Pika_GetCurrentDirectory(buff, PIKA_MAX_PATH))
        return buff;
    return 0;
}

bool Pika_FileExists(const char* filename)
{
    return GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES;
}

bool Pika_IsFile(const char* filename)
{
    DWORD attr = GetFileAttributes(filename);

    if (attr == INVALID_FILE_ATTRIBUTES)
        return false;

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return false;

    return true;
}

bool Pika_IsDirectory(const char* filename)
{
    DWORD attr = GetFileAttributes(filename);

    if (attr == INVALID_FILE_ATTRIBUTES)
        return false;

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return true;

    return false;
}

bool Pika_GetFullPath(const char* pathname, char* dest, size_t destlen)
{
    if (!pathname || !dest || !destlen)
        return false;
    return GetFullPathName(pathname, (DWORD)destlen, dest, 0) != 0;
}

char* Pika_GetFullPath(const char* pathname)
{
    if (!pathname)
        return 0;
    
    size_t count = GetFullPathName(pathname, 0, 0, 0);
    
    if (!count)
        return 0;
        
    char* res = (char*)Pika_malloc(count * sizeof(char));
    
    if (!res)
        return 0;

    if (GetFullPathName(pathname, (DWORD)count, res, 0) == 0)
    {
        Pika_free(res);
        return 0;
    }
    return res;
}

#include <io.h>
/*
 * POSIX like directory browsing functions for windows
 *
 * Portions Copyright Kevlin Henney, 1997, 2003. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose is hereby granted without fee, provided
 * that this copyright and permissions notice appear in all copies and
 * derivatives.
 *
 * This software is supplied "as is" without express or implied warranty.
 */
struct Pika_DirectoryEntry
{
    char* Name;
};

struct Pika_Directory
{
    long                handle; // -1 for failed rewind
    struct _finddata_t  info;
    Pika_DirectoryEntry result; // Name null iff first time
    char*               name;   // null-terminated char string
};

Pika_Directory* Pika_OpenDirectory(const char* name)
{
    Pika_Directory* dir = 0;

    if (name && name[0])
    {
        size_t base_length = strlen(name);

        /* search pattern must end with suitable wildcard */
        const char* all = strchr("/\\", name[base_length - 1]) ? "*" : "/*";

        if (((dir = (Pika_Directory*) Pika_malloc(sizeof * dir)) != 0) &&
            ((dir->name = (char*) Pika_malloc(base_length + strlen(all) + 1)) != 0))
        {
            strcat(strcpy(dir->name, name), all);

            if ((dir->handle = (long)_findfirst(dir->name, &dir->info)) != -1)
            {
                dir->result.Name = 0;
            }
            else /* rollback */
            {
                Pika_free(dir->name);
                Pika_free(dir);
                dir = 0;
            }
        }
        else /* rollback */
        {
            Pika_free(dir);
            dir   = 0;
            errno = ENOMEM;
        }
    }
    else
    {
        errno = EINVAL;
    }

    return dir;
}

int Pika_CloseDirectory(Pika_Directory* dir)
{
    int result = -1;

    if (dir)
    {
        if (dir->handle != -1)
        {
            result = _findclose(dir->handle);
        }

        Pika_free(dir->name);
        Pika_free(dir);
    }

    if (result == -1) /* map all errors to EBADF */
    {
        errno = EBADF;
    }

    return result;
}

const char* Pika_ReadDirectoryEntry(Pika_Directory* dir)
{
    Pika_DirectoryEntry* result = 0;

    if (dir && dir->handle != -1)
    {
        if (!dir->result.Name || _findnext(dir->handle, &dir->info) != -1)
        {
#ifdef PIKA_BORLANDC
            result         = &dir->result;
            result->Name   = &dir->info.ff_name[0];
#else
            result         = &dir->result;
            result->Name   = dir->info.name;
#endif
        }
    }
    else
    {
        errno = EBADF;
    }

    return result ? result->Name : NULL;
}

void Pika_RewindDirectory(Pika_Directory* dir)
{
    if (dir && dir->handle != -1)
    {
        _findclose(dir->handle);
        dir->handle = (long) _findfirst(dir->name, &dir->info);
        dir->result.Name = 0;
    }
    else
    {
        errno = EBADF;
    }
}

// Shared Library Functions ========================================================================

intptr_t Pika_OpenShared(const char* path)
{
    return (intptr_t)LoadLibraryEx(path, 0, 0);
}

bool Pika_CloseShared(intptr_t handle)
{
    return FreeLibrary((HMODULE)handle) ? true : false;
}

void* Pika_GetSymbolAddress(intptr_t handle, const char* symbol)
{
    return (void*)GetProcAddress((HMODULE)handle, symbol);
}

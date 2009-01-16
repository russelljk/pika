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

#if defined(PIKA_BORLANDC)
#   include <dir.h>
#   define _finddata_t                      ffblk
#   define _findfirst(buf, data)            findfirst(buf, data, 0)
#   define _findnext(srchHandle, data)      findnext(data)
#   define _findclose(srchHandle)           0
#endif

void Soda_Sleep(u4 msecs)
{
    Sleep(msecs);
}

struct TimePeriod
{
    TimePeriod() 
    { 
        end_period = (::timeBeginPeriod(1) != TIMERR_NOERROR); 
    }
    
    ~TimePeriod()
    {
        if (end_period)
          ::timeEndPeriod(1); 
    }
    bool end_period;
};

static TimePeriod sTimePeriod;

unsigned long Soda_Milliseconds()
{
    return ::timeGetTime();
}

bool Soda_CreateDirectory(const char* pathName)
{
    return CreateDirectory(pathName, 0) != 0;
}

bool Soda_RemoveDirectory(const char* pathName)
{
    return RemoveDirectory(pathName) != 0;
}

bool Soda_DeleteFile(const char* filename, bool evenReadOnly)
{
    if (evenReadOnly)
        SetFileAttributes(filename, FILE_ATTRIBUTE_NORMAL);

    return DeleteFile(filename) != 0;
}

bool Soda_MoveFile(const char* src, const char* dest, bool canReplace)
{
    u4 flags = MOVEFILE_COPY_ALLOWED;

    if (canReplace)
        flags |= MOVEFILE_REPLACE_EXISTING;

    return MoveFileEx(src, dest, MOVEFILE_COPY_ALLOWED) != 0;
}

bool Soda_CopyFile(const char* src, const char* dest, bool canReplace)
{
    return CopyFile(src, dest, !canReplace) != 0;
}

bool Soda_SetCurrentDirectory(const char* dir)
{
    return SetCurrentDirectory(dir) != 0;
}

bool Soda_GetCurrentDirectory(char* buff, size_t len)
{
    return GetCurrentDirectory((DWORD)len, buff) != 0;
}

char* Soda_GetCurrentDirectory()
{
    char* buff = (char*)Soda_malloc(sizeof(char) * (PIKA_MAX_PATH + 1));

    if (Soda_GetCurrentDirectory(buff, PIKA_MAX_PATH))
        return buff;
    return 0;
}

bool Soda_FileExists(const char* filename)
{
    return GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES;
}

bool Soda_IsFile(const char* filename)
{
    DWORD attr = GetFileAttributes(filename);

    if (attr == INVALID_FILE_ATTRIBUTES)
        return false;

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return false;

    return true;
}

bool Soda_IsDirectory(const char* filename)
{
    DWORD attr = GetFileAttributes(filename);

    if (attr == INVALID_FILE_ATTRIBUTES)
        return false;

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return true;

    return false;
}

bool Soda_GetFullPath(const char* pathname, char* dest, size_t destlen)
{
    if (!pathname || !dest || !destlen)
        return false;
    return GetFullPathName(pathname, (DWORD)destlen, dest, 0) != 0;
}

char* Soda_GetFullPath(const char* pathname)
{
    if (!pathname)
        return NULL;

    char* res = (char*)Soda_malloc(PIKA_MAX_PATH * sizeof(char));
    if (!res)
        return NULL;

    if (GetFullPathName(pathname, (DWORD)PIKA_MAX_PATH, res, 0) == 0)
    {
        Soda_free(res);
        return NULL;
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
struct Soda_DirectoryEntry
{
    char* Name;
};

struct Soda_Directory
{
    long                handle; // -1 for failed rewind
    struct _finddata_t  info;
    Soda_DirectoryEntry result; // Name null iff first time
    char*               name;   // null-terminated char string
};

Soda_Directory* Soda_OpenDirectory(const char* name)
{
    Soda_Directory* dir = 0;

    if (name && name[0])
    {
        size_t base_length = strlen(name);

        /* search pattern must end with suitable wildcard */
        const char* all = strchr("/\\", name[base_length - 1]) ? "*" : "/*";

        if (((dir = (Soda_Directory*) Soda_malloc(sizeof * dir)) != 0) &&
            ((dir->name = (char*) Soda_malloc(base_length + strlen(all) + 1)) != 0))
        {
            strcat(strcpy(dir->name, name), all);

            if ((dir->handle = (long)_findfirst(dir->name, &dir->info)) != -1)
            {
                dir->result.Name = 0;
            }
            else /* rollback */
            {
                Soda_free(dir->name);
                Soda_free(dir);
                dir = 0;
            }
        }
        else /* rollback */
        {
            Soda_free(dir);
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

int Soda_CloseDirectory(Soda_Directory* dir)
{
    int result = -1;

    if (dir)
    {
        if (dir->handle != -1)
        {
            result = _findclose(dir->handle);
        }

        Soda_free(dir->name);
        Soda_free(dir);
    }

    if (result == -1) /* map all errors to EBADF */
    {
        errno = EBADF;
    }

    return result;
}

const char* Soda_ReadDirectoryEntry(Soda_Directory* dir)
{
    Soda_DirectoryEntry* result = 0;

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

void Soda_RewindDirectory(Soda_Directory* dir)
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

intptr_t Soda_OpenShared(const char* path)
{
    return (intptr_t)LoadLibraryEx(path, 0, 0);
}

bool Soda_CloseShared(intptr_t handle)
{
    return FreeLibrary((HMODULE)handle) ? true : false;
}

void* Soda_GetSymbolAddress(intptr_t handle, const char* symbol)
{
    return (void*)GetProcAddress((HMODULE)handle, symbol);
}


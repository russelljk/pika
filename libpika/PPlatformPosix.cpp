/*
 *  PPlatformPosix.cpp
 *  See Copyright Notice in Pika.h
 *  -------------------------------------------------------------------------------------------
 *  Implementation of platform specific operations using POSIX. This has been tested on OSX and 
 *  many Linux distros without change. Porting to another POSIX platform should be easy enough.
 */
#include "Pika.h"
#include "PPlatform.h"

#if defined(HAVE_COPYFILE)
#   include <copyfile.h>
//#   define USE_COPYFILE_STATE
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>    // stat
#include <sys/time.h>    // gettimeofday
#include <fcntl.h>
#include <dirent.h>

void Pika_Sleep(u4 msecs)
{
    if (msecs)
    {
        struct timespec t;
        t.tv_sec  = (msecs / 1000);
        t.tv_nsec = (msecs % 1000) * 1000000L;
        nanosleep(&t, 0);
    }
    else
    {
        struct timespec t;
        t.tv_sec  = 0;
        t.tv_nsec = 1000L;
        nanosleep(&t, 0);
    }
}

unsigned long Pika_Milliseconds()
{
	timeval tp;
	gettimeofday(&tp, 0);
	return (unsigned long)(tp.tv_usec / 1000 + // µs to ms
                           tp.tv_sec  * 1000); // s to ms
}

bool Pika_CreateDirectory(const char* pathName)
{
    return mkdir(pathName, S_IRUSR | S_IWUSR | S_IXUSR) == 0;
}

bool Pika_RemoveDirectory(const char* pathName)
{
    return rmdir(pathName) == 0;
}

bool Pika_DeleteFile(const char* filename, bool evenReadOnly)
{
    if (evenReadOnly)
        chmod(filename, S_IRUSR | S_IWUSR);
    return unlink(filename) == 0;
}

bool Pika_MoveFile(const char* src, const char* dest, bool canReplace)
{
    bool dest_exists = Pika_FileExists(dest);
    if ((dest_exists && canReplace) || !dest_exists)
    {
        if (Pika_CopyFile(src, dest))
        {
            return Pika_DeleteFile(src, true);
        }
    }
    return false;
}

bool Pika_CopyFile(const char* src, const char* dest, bool canReplace)
{
#if defined(HAVE_COPYFILE)
    u4 flags = COPYFILE_ALL;
    if (!canReplace)
        flags |= COPYFILE_EXCL;
#   if defined( USE_COPYFILE_STATE )
    copyfile_state_t st = copyfile_state_alloc();
    if (!st)
        return false;
    bool res = copyfile(src, dest, st, flags) == 0; // XXX: copyfile, is it Mac OSX 10.5 only? what about BSD?
    copyfile_state_free(st);
    return res;
#   else
    return copyfile(src, dest, 0, flags) == 0; // XXX: copyfile, is it Mac OSX 10.5 only? what about BSD?
#endif
#else
    return false;
#endif
}

bool Pika_SetCurrentDirectory(const char* dir)
{
    return chdir(dir) == 0;
}

bool Pika_GetCurrentDirectory(char* buff, size_t len)
{
    return getcwd(buff, len);
}

char* Pika_GetCurrentDirectory()
{
    char* buff = (char*)Pika_malloc(sizeof(char) * (PIKA_MAX_PATH + 1));
    return getcwd(buff, PIKA_MAX_PATH);
}

bool Pika_FileExists(const char* filename)
{
    return access(filename, F_OK) == 0;
}

bool Pika_IsFile(const char* filename)
{
    struct stat ino;

    if (stat(filename, &ino))
        return false;

    if (S_ISREG(ino.st_mode))
        return true;

    return false;
}

bool Pika_IsDirectory(const char* filename)
{
    struct stat ino;

    if (stat(filename, &ino))
        return false;

    if (S_ISDIR(ino.st_mode))
        return true;

    return false;
}

bool Pika_GetFullPath(const char* pathname, char* dest, size_t destlen)
{
    if (!pathname || !dest || !destlen)
        return false;
    char* result = (char*)Pika_malloc(PIKA_MAX_PATH * sizeof(char));
    if (!result) 
        return false;
    bool res = false;
    if (realpath(pathname, result))
    {
        strncpy(dest, result, Min<size_t>(destlen, PIKA_MAX_PATH));
        res = true;
    }
    Pika_free(result);
    return res;
}

char* Pika_GetFullPath(const char* pathname)
{
    if (!pathname)
        return 0;

    char* res = (char*)Pika_malloc(PIKA_MAX_PATH * sizeof(char));
    if (!res)
        return 0;

    if (!realpath(pathname, res))
    {
        Pika_free(res);
        return 0;  
    }
    return res;
}

Pika_Directory* Pika_OpenDirectory(const char* name)
{
    return (Pika_Directory*)((void*)opendir(name));
}

int Pika_CloseDirectory(Pika_Directory* dir)
{
    return closedir((DIR*)((void*)(dir)));
}

const char* Pika_ReadDirectoryEntry(Pika_Directory* dir)
{
    dirent* en = readdir( (DIR*)((void*)(dir)) );
    if (en && en->d_name)
        return en->d_name;
    else
        return 0;
}

void Pika_RewindDirectory(Pika_Directory* dir)
{
    rewinddir((DIR*)((void*)(dir)));
}

#if defined( HAVE_LIBDL ) || defined( HAVE_LIBC_DL )

#if defined( HAVE_DLFCN_H )
#   include <dlfcn.h>
#elif defined( HAVE_DL_H )
#   include <dl.h>
#elif defined( HAVE_SYS_DL_H )
#   include <sys/dl.h>
#endif

intptr_t Pika_OpenShared(const char* path)
{
    intptr_t res = (intptr_t)dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    return res;
}

bool Pika_CloseShared(intptr_t handle)
{
    return dlclose((void*)handle) == 0;
}

void* Pika_GetSymbolAddress(intptr_t handle, const char* symbol)
{
    return dlsym((void*)handle, symbol);
}

#else

intptr_t    Pika_OpenShared(const char* path) { return 0; }
bool        Pika_CloseShared(intptr_t handle) { return false; }
void*       Pika_GetSymbolAddress(intptr_t handle, const char* symbol) { return 0; }

#endif

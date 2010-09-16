/*
 *  PPlatform.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_PLATFORM_HEADER
#define PIKA_PLATFORM_HEADER
extern const char* Pika_readline(const char* prompt);
extern void Pika_addhistory(const char* ln);

// ---- String Functions ----

/** Compares two strings that might contain non-ascii or null characters. */
extern int         Pika_StringCompare(const char* a, size_t lena,
                                      const char* b, size_t lenb);
extern size_t      Pika_StringHash(const char* str);
extern size_t      Pika_StringHash(const char* str, size_t len);
extern int         Pika_snprintf(char* buff, size_t count, const char* fmt, ...);
extern char*       Pika_strtok(char*, const char*, char**);
extern const char* Pika_index(const char* str, int x);
extern const char* Pika_rindex(const char* str, int x);

// ===== Platform Specific ====

// ---- Timing Functions ----

extern void          Pika_Sleep(u4 msecs);
extern unsigned long Pika_Milliseconds();

// ---- File/Directory Functions ----

extern bool     Pika_CreateDirectory(const char* pathName);
extern bool     Pika_RemoveDirectory(const char* pathName);

extern bool     Pika_DeleteFile(const char* filename, bool evenReadOnly = false);
extern bool     Pika_MoveFile(const char* src, const char* dest, bool canReplace = true);
extern bool     Pika_CopyFile(const char* src, const char* dest, bool canReplace = true);

extern bool     Pika_SetCurrentDirectory(const char* dir);

extern bool     Pika_GetCurrentDirectory(char* buff, size_t len);
extern char*    Pika_GetCurrentDirectory(); // Call Pika_free on the result.

extern bool     Pika_FileExists(const char* filename);

extern bool     Pika_IsFile(const char* filename);
extern bool     Pika_IsDirectory(const char* filename);

extern bool     Pika_GetFullPath(const char* pathname, char* dest, size_t destlen);
extern char*    Pika_GetFullPath(const char* pathname); // Call Pika_free on the result.

// ---- Directory browsing functions ----

struct Pika_Directory;

extern Pika_Directory*  Pika_OpenDirectory(const char*);
extern int              Pika_CloseDirectory(Pika_Directory*);
extern const char*      Pika_ReadDirectoryEntry(Pika_Directory*);
extern void             Pika_RewindDirectory(Pika_Directory*);

// ---- Shared Library Functions ----

extern intptr_t Pika_OpenShared(const char* path);
extern bool     Pika_CloseShared(intptr_t handle);
extern void*    Pika_GetSymbolAddress(intptr_t handle, const char* symbol);

#endif


/*
 *  PPathManager.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PPathManager.h"
#include "PNativeBind.h"
#include "PPlatform.h"

namespace pika {
PIKA_IMPL(PathManager)

namespace {

bool EndsWithSeperator(const char* str, size_t len)
{
    if (len > 0 && str[len - 1] == PIKA_PATH_SEP_CHAR)
        return true;
    return false;
}

}

PathManager::PathManager(Engine* eng, Type* type)
    : ThisSuper(eng, type)
{
    char* const buff = Pika_GetCurrentDirectory();
    String* startingPath = 0;
    
    if (buff)
    {
        startingPath = engine->GetString(buff);
        Pika_free(buff);
    }
    else
    {
        startingPath = engine->GetString("./");
    }
    
    if (startingPath)
        AddPath(startingPath);
}

PathManager::~PathManager() {}

bool PathManager::IsValidFile(const char* path)
{
    return Pika_FileExists(path) && !Pika_IsDirectory(path);
}

bool PathManager::BracketRead(const Value& key, Value& res)
{
    if (key.IsInteger()) {
        pint_t ival = key.val.integer;
        if (ival >= 0) {
            size_t idx = static_cast<size_t>(ival);
            if (idx < searchPaths.GetSize())
            {
                res.Set(searchPaths[idx]);
                return true;
            }
        }
    }
    return ThisSuper::BracketRead(key, res);
}

String* PathManager::FindFile(String* file)
{
    // first determine the maximum length our path string can be
    size_t max_sz = 0;
    for (size_t i = 0; i < searchPaths.GetSize(); ++i) // should move this to AddPath and keep an instance variable maxPathSize
    {
        size_t const sz = searchPaths[i]->GetLength() + 1;
        if (sz > max_sz || max_sz == 0)
        {
            max_sz = sz;
        }
    }
    size_t const file_name_sz = file->GetLength();
    size_t BUFF_SZ = file_name_sz + max_sz;
    
    if (SizeAdditionOverflow(max_sz, file_name_sz) || // bigger than max(size_t) or
        BUFF_SZ > PIKA_STRING_MAX_LEN) // Too big the fit in a String 
    {
        BUFF_SZ = PIKA_STRING_MAX_LEN;
    }
    
    // allocate a temp buffer
    char* temp = (char*)Pika_malloc(sizeof(char) * (BUFF_SZ + 1));
    int oldErrno = errno;
    errno = 0;
    for (size_t i = 0; i < searchPaths.GetSize(); ++i)
    {
        String* currpath = searchPaths[i];
        
        if (Pika_snprintf(temp, BUFF_SZ, "%s%s", currpath->GetBuffer(), file->GetBuffer()) < 0)
        {
            continue;
        }
        temp[BUFF_SZ] = '\0';
        
        if (IsValidFile(temp))
        {
            String* res = engine->GetString(temp);
            Pika_free(temp);
            errno = oldErrno;
            return res;
        }
    }
    Pika_free(temp);
    
    if (IsValidFile(file->GetBuffer()))
    {
        return file;
    }
    errno = oldErrno;
    return 0;
}

void PathManager::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    
    size_t sz = searchPaths.GetSize();
    for (size_t i = 0; i < sz; ++i) {
        searchPaths[i]->Mark(c);
    }
}

bool PathManager::Finalize()
{
    return false;
}

void PathManager::AddPath(String* path)
{
    if ( !EndsWithSeperator(path->GetBuffer(),
                            path->GetLength())
       )
    {
        path = String::Concat(path, engine->GetString(PIKA_PATH_SEP));
    }
    
    engine->GetGC()->WriteBarrier(this, path);
    searchPaths.Push(path);
}

void PathManager::AddEnvPath(String* env)
{
    if (!env || env->GetLength() == 0)
        return;
            
    char* path = 0;
    const char* next_pos = 0;
    int m, n;
    const char delim = ':';
    const char* var = env->GetBuffer();
    
    for (path = getenv(var); path && *path; path += m)
    {
        if ((next_pos = Pika_index(path, delim)))
        {
            n = next_pos - path;
            m = n + 1;
        }
        else
        {
            m = n = strlen(path);
        }
        AddPath(engine->GetString(path, n));
    }
}

void PathManager::Constructor(Engine* eng, Type* type, Value& res)
{
    PathManager* p = 0;
    GCNEW(eng, PathManager, p, (eng, type));
    res.Set(p);
}

void PathManager::StaticInitType(Engine* eng)
{
    GCPAUSE_NORUN(eng);
    eng->PathManager_Type = Type::Create(eng, eng->AllocString("PathManager"), eng->Object_Type, PathManager::Constructor, eng->GetWorld());
    SlotBinder<PathManager>(eng, eng->PathManager_Type, eng->PathManager_Type)
    .Method(&PathManager::AddPath,      "addPath")
    .Method(&PathManager::AddEnvPath,   "addEnvPath")
    .Method(&PathManager::FindFile,     "findPathOf")
    .PropertyR("length",
            &PathManager::GetSize,      "getLength")
    ;
}

}// pika

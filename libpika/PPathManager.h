/*
 *  PPathManager.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_PATH_MANAGER_HEADER
#define PIKA_PATH_MANAGER_HEADER

#ifndef PIKA_OBJECT_HEADER
#   include "PObject.h"
#endif

#ifndef PIKA_BUFFER_HEADER
#   include "PBuffer.h"
#endif

namespace pika {

#if defined(PIKA_DLL) && defined(_WIN32)
template class PIKA_API Buffer<String*>;
#endif

/* TODO: paths added need to be checked for consistency? or not? Can we
 * predict what is and is not a valid path on any given system?
 */
class PIKA_API PathManager : public Object {
    PIKA_DECL(PathManager, Object)
public:
    PathManager(Engine*, Type*);
    
    virtual ~PathManager();
    
    virtual String* FindFile(String* file);
    virtual void    MarkRefs(Collector* c);
    virtual bool    Finalize();
    virtual void    AddPath(String* path);
    virtual void    AddEnvPath(String* env);
    virtual bool    BracketRead(const Value& key, Value& res);
    
    INLINE size_t GetSize() const { return searchPaths.GetSize(); }
    
    INLINE String* At(size_t idx) {
        if (idx < GetSize())
            return searchPaths[idx];
        return 0;
    }
    
    static void Constructor(Engine* eng, Type* type, Value& res);
    static void StaticInitType(Engine* eng);
private:
    bool IsValidFile(const char*); 
    
    Buffer<String*> searchPaths;
};

}// pika

#endif

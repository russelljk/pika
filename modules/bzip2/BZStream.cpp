/*
 *  BZStream.cpp
 *  See Copyright Notice in Pika.h
 */
#include "BZip2.h"

namespace pika {

BZStream::BZStream(Engine* engine, Type* type) : ThisSuper(engine, type)
{
    this->Reset();
}

BZStream::~BZStream()
{
}

ClassInfo* BZStream::GetErrorClass()
{
    return BZError::StaticGetClass();
}

void BZStream::Begin()
{
    stream.bzalloc = 0;
    stream.bzfree = 0;
    stream.opaque = 0;
}

int BZStream::Call(int flush)
{
    return 0;
}

void BZStream::End()
{    
}

String* BZStream::Process(String* in)
{
    Buffer<char> out;
    out.SetCapacity(in->GetLength());
    
    this->DoProcess(in->GetBuffer(), in->GetLength(), out);
    return engine->GetString(reinterpret_cast<char*>(out.GetAt(0)), out.GetSize());
}

void BZStream::DoProcess(const char* in, size_t in_length, Buffer<char>& out)
{
    this->Begin();        
    size_t const CHUNK_SIZE = 2048;
    
    const char* in_curr = in;
    
    Buffer<char> buff(CHUNK_SIZE);
        
    int flush = BZ_FINISH;
    
    stream.next_in = const_cast<char*>(in_curr); // next_in isn't modified by zlib (why don't they make it const?)
    stream.avail_in = in_length;
    errno = 0;
    do {        
        stream.avail_out = buff.GetSize();
        stream.next_out = buff.GetAt(0);
        
        int ret = this->Call(flush);
        
        if (errno)
        {
            ErrorStringHandler handler(errno);
            this->End();
            this->Reset();
            RaiseException(GetErrorClass(), "Attempt to process stream failed with message \"%s\".", handler.GetBuffer());
        }
        
        size_t out_amt = CHUNK_SIZE - stream.avail_out;
                
        if (out_amt > 0) {
            size_t pos = out.GetSize();
            out.Resize(pos + out_amt);
            Pika_memcpy(out.GetAt(pos), buff.GetAt(0), Min<size_t>(out_amt, buff.GetSize()));
        }        
        // flush = BZ_FINISH;
        
    } while (stream.avail_out == 0);
    this->Reset();
}

void BZStream::Reset()
{
    Pika_memzero(&stream, sizeof(bz_stream));
}

PIKA_IMPL(BZStream)

}// pika

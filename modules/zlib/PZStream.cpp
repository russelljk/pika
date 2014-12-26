/*
 *  PZStream.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PZlib.h"

namespace pika {

ZStream::ZStream(Engine* engine, Type* type) : ThisSuper(engine, type)
{
    this->Reset();
}

ZStream::~ZStream()
{
}

ClassInfo* ZStream::GetErrorClass()
{
    return ZlibError::StaticGetClass();
}

void ZStream::Begin()
{
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
}

int ZStream::Call(int flush)
{
    return 0;
}

void ZStream::End()
{    
}

String* ZStream::Process(String* in)
{
    Buffer<u1> out;
    out.SetCapacity(in->GetLength());
    
    this->DoProcess(reinterpret_cast<const u1*>(in->GetBuffer()), in->GetLength(), out);
    return engine->GetString(reinterpret_cast<char*>(out.GetAt(0)), out.GetSize());
}

void ZStream::DoProcess(const u1* in, size_t in_length, Buffer<u1>& out)
{
    this->Begin();        
    size_t const CHUNK_SIZE = 2048;
    
    const u1* in_curr = in;
    // const u1* in_end = in + in_length;
    
    u1 buff[CHUNK_SIZE];
    
    int flush = Z_FINISH;
    
    stream.next_in = const_cast<u1*>(in_curr); // next_in isn't modified by zlib (why don't they make it const?)
    stream.avail_in = in_length;
    errno = 0;
    do {        
        stream.avail_out = CHUNK_SIZE;
        stream.next_out = &buff[0];
        
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
            Pika_memcpy(out.GetAt(pos), buff, out_amt);
        }        
        // flush = Z_FINISH;
        
    } while (stream.avail_out == 0);
    this->Reset();
}

void ZStream::Reset()
{
    Pika_memzero(&stream, sizeof(z_stream));
}

PIKA_IMPL(ZStream)

}// pika

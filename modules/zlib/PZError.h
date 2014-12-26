#ifndef PIKA_ZERROR_HEADER
#define PIKA_ZERROR_HEADER

namespace pika {

struct ZlibError: RuntimeError {
    PIKA_DECL(ZlibError, ErrorClass)
};

struct CompressError: ZlibError {
    PIKA_DECL(CompressError, ZlibError)
};

struct DecompressError: ZlibError {
    PIKA_DECL(DecompressError, ZlibError)
};

}// pika

#endif

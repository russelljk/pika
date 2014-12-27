#ifndef PIKA_BZERROR_HEADER
#define PIKA_BZERROR_HEADER

namespace pika {

struct BZError: RuntimeError {
    PIKA_DECL(BZError, ErrorClass)
};

struct CompressError: BZError {
    PIKA_DECL(CompressError, BZError)
};

struct DecompressError: BZError {
    PIKA_DECL(DecompressError, BZError)
};

}// pika

#endif

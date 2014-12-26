/*
 *  PSocketError.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_SOCKET_ERROR_HEADER
#define PIKA_SOCKET_ERROR_HEADER

namespace pika {

void RaiseExceptionFromErrno(Exception::Kind errKind, const char* errMsg, int errNum=0);

}// pika

#endif
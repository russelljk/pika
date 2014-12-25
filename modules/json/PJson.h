/*
 *  PJson.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_JSON_HEADER
#define PIKA_JSON_HEADER

#include "Pika.h"

namespace pika {
    struct JsonError: RuntimeError {
        PIKA_DECL(JsonError, ErrorClass)
    };
    
    struct JsonEncodeError: JsonError {
        PIKA_DECL(JsonEncodeError, JsonError)
    };
    
    struct JsonDecodeError: JsonError {
        PIKA_DECL(JsonDecodeError, JsonError)
    };
    
    struct JsonSyntaxError: JsonDecodeError {
        PIKA_DECL(JsonSyntaxError, JsonDecodeError)
    };
}// pika

#endif

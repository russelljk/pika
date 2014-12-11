/*
 *  PEventHelpers.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_EVENT_HELPERS_HEADER
#define PIKA_EVENT_HELPERS_HEADER

namespace pika {
void Pika_MakeTimeval(preal_t secs, timeval& tv);

pint_t  GetFilenoFrom(Context* ctx, Value file);

}// pika

#endif

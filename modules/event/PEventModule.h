/*
 *  PEventModule.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_EVENT_MODULE_HEADER
#define PIKA_EVENT_MODULE_HEADER

#include <event2/event.h>
#include <event2/listener.h>
#define LIBEVENT_V21 (_EVENT_NUMERIC_VERSION >= 0x02010000)
#define LIBEVENT_V212 (_EVENT_NUMERIC_VERSION >= 0x02010200)

#include "Pika.h"
#include "PEvent.h"
#include "PEventBase.h"
#include "PEventConfig.h"
#include "PEventHelpers.h"

#endif

/* Pika.h
 *
 * Pika
 * Copyright (c) 2008, Russell J. Kyle <russell.j.kyle@gmail.com>
 *  
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef PIKA_HEADER
#define PIKA_HEADER

#define PIKA_VERSION_STR        "Pika 0.0.1"
#define PIKA_VERSION_MAJOR      0
#define PIKA_VERSION_MINOR      0
#define PIKA_VERSION_REVISION   1
#define PIKA_COPYRIGHT_STR      "Copyright (C) 2006 Russell Kyle"
#define PIKA_UNDERLINE_STR      "-------------------------------"
#define PIKA_AUTHOR_STR         "Russell Kyle"

////////////////////////////////////////// Class Hierarchy /////////////////////////////////////////

namespace pika
{
class ClassInfo;
class Collector;
class GCObject;
class       Basic;
class           Enumerator;
class               DummyEnumerator;
class               ValueEnumerator;
class           Object;
class               Package;
class                   Type;
class                   Module;
class                   Script;
class               Array;
class               Function;
class               Context;
class				Debugger;
class           Property;
class           String;
class           UserData;
class       Def;
class       LiteralPool;
class       LexicalEnv;
class Engine;
class StringTable;
class Table;
}
using pika::Array;
using pika::Basic;
using pika::ClassInfo;
using pika::Function;
using pika::Context;
using pika::Debugger;
using pika::Enumerator;
using pika::Def;
using pika::LiteralPool;
using pika::Object;
using pika::Package;
using pika::Property;
using pika::String;
using pika::Type;
using pika::UserData;

////////////////////////////// Dependencies ////////////////////////////////////

#include "PMemory.h"
#include "PConfig.h"
#include "PUtil.h"

#include "PMemPool.h"
#include "PBuffer.h"

#include "PError.h"
#include "PValue.h"
#include "PCollector.h"
#include "PLiteralPool.h"
#include "PDef.h"

#include "PTable.h"
#include "PEngine.h"
#include "PBasic.h"
#include "PEnumerator.h"
#include "PString.h"
#include "PProperty.h"
#include "PUserData.h"
#include "PClassInfo.h"
#include "PObject.h"
#include "PContext.h"
#include "PFunction.h"
#include "PArray.h"
#include "PValueEnumerator.h"
#include "PPackage.h"
#include "PType.h"
#include "PScript.h"
#include "PModule.h"
#include "PDebugger.h"
#include "PNativeBind.h"

#endif

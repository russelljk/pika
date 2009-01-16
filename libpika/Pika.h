/*  Pika.h
 *  Pika
 *  Copyright (c) 2006, Russell J. Kyle <russell.j.kyle@gmail.com>
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
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

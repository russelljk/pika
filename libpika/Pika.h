/* Pika.h
 * ----------------------------------------------------------------------------
 * Pika programing language
 * Copyright (c) 2008, Russell J. Kyle <russell.j.kyle@gmail.com>
 *  
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 
 * 3. This notice may not be removed or altered from any source distribution.
 * ----------------------------------------------------------------------------
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

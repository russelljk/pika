/*
 *  PLineInfo.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_LINEINFO_HEADER
#define PIKA_LINEINFO_HEADER

#ifndef PIKA_CONFIG_HEADER
#   include "PConfig.h"
#endif

namespace pika {

class String;

struct LineInfo
{
	u2      line;
	code_t* pos;
};

struct LocalVarInfo
{
    INLINE LocalVarInfo() : name(0), beg(0), end(0), param(false) {}
    
    // Local's name
    String* name;
    // Range of this local is visible in the function's bytecode.
    // Use the function's LineInfo buffer to convert the bytecode offsets into line numbers.
    ptrdiff_t beg;
    ptrdiff_t end;
    bool param;
};

}// pika

#endif

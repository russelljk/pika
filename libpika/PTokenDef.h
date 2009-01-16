/*
 *  PTokenDef.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_TOKENDEF_HEADER
#define PIKA_TOKENDEF_HEADER

namespace pika
{

#ifdef TOKEN_DEF
#undef TOKEN_DEF
#endif

#define TOKEN_DEF(XTOK, XNAME) XTOK,
enum ETokenType
{
    TOK_min = 257,
#   include "PToken.def"
    TOK_max
};

union YYSTYPE 
{
    struct
    {
        char*   str;
        size_t  len;
    };
    pint_t       integer;
    preal_t      real;
};

struct Token2String
{
    static const char** GetNames();
    static const int    min;
    static const int    max;
    static const int    diff;
};

}// pika

#endif

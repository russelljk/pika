/*
 *  PTokenDef.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_TOKENDEF_HEADER
#define PIKA_TOKENDEF_HEADER

namespace pika {

#ifdef TOKEN_DEF
#undef TOKEN_DEF
#endif

#define TOKEN_DEF(XTOK, XNAME) XTOK,
enum ETokenType
{
    // characters occupy positions 0 - 255
    TOK_min = 257, 
#   include "PToken.def"
    TOK_max
};

// Token type union
union YYSTYPE 
{
    struct 
    {
        char* str; // identifiers and strings
        size_t len;
    };

    pint_t integer;  // integers
    preal_t real;    // reals
};

struct Token2String
{
    static const char** GetNames();
    static const int min;
    static const int max;
    static const int diff;
};

}// pika

#endif

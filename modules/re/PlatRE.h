/*
 * PlatRE.h
 *  See Copyright Notice in PRegExp.h
 */
#ifndef PLATRE_HEADER
#define PLATRE_HEADER

struct Pika_regmatch
{
    int start;
    int end;
};

struct Pika_regex;

extern int const RE_NO_UTF8_CHECK;
extern int const RE_UTF8;
extern int const RE_MULTILINE;
extern int const RE_INSENSITIVE;

/* Compile the regular expression. */
extern Pika_regex*  Pika_regcomp(const char* pattern, int cflags, char* errmsg, size_t errlen, int* errorcode);
extern size_t       Pika_regerror(int errcode, const Pika_regex* preg, char* errbuf, size_t errbuf_size);
extern int          Pika_regexec(const Pika_regex* preg, const char* subj, size_t const subjlen, Pika_regmatch* pmatch, size_t const nmatch, int eflags);
extern void         Pika_regfree(Pika_regex* preg);

#endif

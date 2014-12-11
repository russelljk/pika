/*
 *  PBigNum.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_BIGNUM_HEADER
#define PIKA_BIGNUM_HEADER

#include "bignum_config.h"
#include <gmp.h>

#ifdef HAVE_MPFR
#   include <mpfr.h>
#endif

#include "Pika.h"
#include "PBigInteger.h"
#ifdef HAVE_MPFR
#   include "PBigReal.h"
#   define DEFAULT_RND MPFR_RNDN
#endif

#endif

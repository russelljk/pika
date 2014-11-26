#ifndef PIKA_MP_HEADER
#define PIKA_MP_HEADER

#include "bignum_config.h"
#include <gmp.h>

#ifdef HAVE_MPFR
#   include <mpfr.h>
#endif

#include "Pika.h"
#include "PBigInteger.h"
#ifdef HAVE_MPFR
#   include "PBigReal.h"
#endif

#endif

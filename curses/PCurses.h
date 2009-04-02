/*
 *  PCurses.h
 *  Copyright (c) 2008, Russell J. Kyle <russell.j.kyle@gmail.com>
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
#ifndef PCURSES_HEADER
#define PCURSES_HEADER

#include "Pika.h"
#include "curses_config.h"

//#define _XOPEN_SOURCE_EXTENDED 1

#if defined( HAVE_CURSES_H )
#   include <curses.h>
#elif defined( HAVE_NCURSES_H )
#   include <ncurses.h>
#elif defined( HAVE_NCURSES_NCURSES_H )
#   include <ncurses/ncurses.h>
#elif defined( HAVE_NCURSES_CURSES_H )
#   include <ncurses/curses.h>
#elif defined( HAVE_PDCURSES_H )
#   include <pdcurses.h>
#else
#   error "No curses header file specified"
#endif

#endif

/*  
 *  PCurses.h
 *  ----------------------------------------------------------------------------
 *  Pika programing language
 *  Copyright (c) 2008, Russell J. Kyle <russell.j.kyle@gmail.com>
 *  
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *  
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *  
 *  1. The origin of this software must not be misrepresented; you must not
 *  claim that you wrote the original software. If you use this software
 *  in a product, an acknowledgment in the product documentation would be
 *  appreciated but is not required.
 *  
 *  2. Altered source versions must be plainly marked as such, and must not be
 *  misrepresented as being the original software.
 *  
 *  3. This notice may not be removed or altered from any source distribution.
 *  
 */
#ifndef PCURSES_HEADER
#define PCURSES_HEADER

#include "Pika.h"
#include "curses_config.h"

#if defined(CURSES_HAVE_NCURSES_H)
#   include <ncurses.h>
#elif defined(CURSES_HAVE_NCURSES_NCURSES_H)
#   include <ncurses/ncurses.h>
#elif defined(CURSES_HAVE_NCURSES_CURSES_H)
#   include <ncurses/curses.h>
#else
#   include <curses.h>
#endif

#endif

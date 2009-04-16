/*
 *  PCurses.cpp
 *  See Copyright Notice in PCurses.h
 */
#include "PCurses.h"

// TODO: Find out what the most common subset of curses is, 
//       solaris' curses package does not support certain methods + ACS constants.

#if defined(PIKA_WIN)

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH: break;
    }
    return TRUE;
}

#endif

#define WINDOW_TYPENAME "curses.Window"

#define GETWINDOW(X)                                                        \
if (!(self.IsUserData() && self.val.userdata->GetInfo() == &Window_Info))   \
{                                                                           \
    RaiseException("invalid self object");                                  \
}                                                                           \
WINDOW* win = (WINDOW*)self.val.userdata->GetData();

UserDataInfo Window_Info = 
{
    "Window",   //name
    NULL,       //fMark
    NULL,       //fFinalize
    NULL,       //fGet
    NULL,       //fSet
    0,          //fLength
    0,          //fAlignment 
};

inline WINDOW* GetWindowFrom(Value& v)
{
    if (!(v.IsUserData() && v.val.userdata->GetInfo() == &Window_Info))
    {
        RaiseException("invalid window object");
    }
    WINDOW* win = (WINDOW*)v.val.userdata->GetData();
    return win;
}

Type* GetWindowType(Context* ctx)
{
    Engine* eng = ctx->GetEngine();
    return (Type*)eng->GetBaseType( eng->AllocString(WINDOW_TYPENAME) );
}

UserData* CreateNewWindow(Context* ctx, WINDOW* w)
{
    Type* window_type = GetWindowType(ctx);
    if (!window_type)
    {
        RaiseException("could not create new Window. Window-Type not found.");
        return 0;
    }
    UserData* ud = UserData::CreateWithPointer(ctx->GetEngine(), window_type, (void*)w, &Window_Info);
    return ud;
}

/* Curses' initscr calls exit when a terminal cannot be opened. We want the raise an exception instead of exiting. */
Value Curses_Init(Context* ctx)
{
    Value res(NULL_VALUE);
    setlocale(LC_ALL, "");    
    char* name = getenv("TERM");
    if (name == 0 || newterm(name, stdout, stdin) == 0)
    {
        RaiseException("cannot open terminal '%s'\n", name ? name : "unknown");
    }
    else
    {        
        WINDOW* win = stdscr;//initscr();
        if (win)
        {
            UserData* ud = CreateNewWindow(ctx, win);
            res.Set(ud);
        }
    }
    return res;
}

int Window_mvaddstr(Context* ctx, Value& self)
{
    GETWINDOW(win);
    pint_t y = ctx->GetIntArg(0);
    pint_t x = ctx->GetIntArg(1);
    String* str = ctx->GetStringArg(2);    
    pint_t res = mvwaddstr(win, y, x, str->GetBuffer());
    ctx->Push( res );
    return 1;
}

int Window_addstr(Context* ctx, Value& self)
{
    GETWINDOW(win);
    String* str = ctx->GetStringArg(0);    
    pint_t res = waddstr(win, str->GetBuffer());
    ctx->Push( res );
    return 1;
}

int Window_getyx(Context* ctx, Value& self)
{
    GETWINDOW(win);
    pint_t y = getcury(win);
    pint_t x = getcurx(win);
    ctx->Push( y );
    ctx->Push( x );
    return 2;
}

int Window_begyx(Context* ctx, Value& self)
{
    GETWINDOW(win);
    pint_t y = getbegy(win);
    pint_t x = getbegx(win);
    ctx->Push( y );
    ctx->Push( x );
    return 2;
}

int Window_maxyx(Context* ctx, Value& self)
{
    GETWINDOW(win);
    pint_t y = getmaxy(win);
    pint_t x = getmaxx(win);
    ctx->Push( y );
    ctx->Push( x );
    return 2;
}

pint_t GetCharArg(Value& v)
{
    if (v.IsString())
    {
        if (v.val.str->GetLength() == 1)
        {
            return v.val.str->GetBuffer()[0];
        }
        else if (v.val.str->GetLength() == 0)
        {
            return 0;
        }
    }
    else if (v.IsInteger())
    {
        return v.val.integer;
    }
    RaiseException("Expected character argument");
    return 0;
}

int Window_mvaddch(Context* ctx, Value& self)
{
    GETWINDOW(win);
    pint_t    y = ctx->GetIntArg(0);
    pint_t    x = ctx->GetIntArg(1);
    pint_t   ax = GetCharArg(ctx->GetArg(2));
    chtype  ch = ax;
    pint_t  res = mvwaddch(win, y, x, ch);
    
    ctx->Push( res );    
    return 1;
}

int Window_addch(Context* ctx, Value& self)
{
    GETWINDOW(win);
    pint_t   ax = GetCharArg(ctx->GetArg(0));
    chtype  ch = ax;
    pint_t  res = waddch(win, ch);    
    ctx->Push( res );    
    return 1;
}

int Window_wborder(Context* ctx, Value& self)
{
    GETWINDOW(win);
    pint_t ls = ctx->GetIntArg(0);
    pint_t rs = ctx->GetIntArg(1);
    pint_t ts = ctx->GetIntArg(2);
    pint_t bs = ctx->GetIntArg(3);
    pint_t tl = ctx->GetIntArg(4);
    pint_t tr = ctx->GetIntArg(5);
    pint_t bl = ctx->GetIntArg(6);
    pint_t br = ctx->GetIntArg(7);
    
    pint_t res = wborder(win, ls, rs, ts, bs, tl, tr, bl, br);
    
    ctx->Push( res );    
    return 1;
}


#define CURSES_NA_I(FNNAME)                     \
int Window_##FNNAME(Context* ctx, Value& self)  \
{                                               \
    GETWINDOW(win);                             \
    pint_t  res = FNNAME(win);                  \
    ctx->Push( res );                           \
    return 1;                                   \
}

CURSES_NA_I(wclear)
CURSES_NA_I(wrefresh)
CURSES_NA_I(werase)

CURSES_NA_I(wclrtobot)
CURSES_NA_I(wclrtoeol)
CURSES_NA_I(wdelch)
CURSES_NA_I(wdeleteln)
CURSES_NA_I(wgetch)
CURSES_NA_I(winch)
CURSES_NA_I(winsertln)
CURSES_NA_I(wstandout)
CURSES_NA_I(wstandend)
CURSES_NA_I(scroll)

#define CURSES_NA_NA(FNNAME)                    \
int Window_##FNNAME(Context* ctx, Value& self)  \
{                                               \
    GETWINDOW(win);                             \
    FNNAME(win);                                \
    return 0;                                   \
}

CURSES_NA_NA(wsyncdown)
CURSES_NA_NA(wsyncup)

#define CURSES_IIII_I(FNNAME)                   \
int Window_##FNNAME(Context* ctx, Value& self)  \
{                                               \
    GETWINDOW(win);                             \
    pint_t  a   = ctx->GetIntArg(0);            \
    pint_t  b   = ctx->GetIntArg(1);            \
    pint_t  c   = ctx->GetIntArg(2);            \
    pint_t  d   = ctx->GetIntArg(3);            \
    pint_t  res = FNNAME(win, a, b, c, d);      \
    ctx->Push( res );                           \
    return 1;                                   \
}

CURSES_IIII_I( mvwvline )
CURSES_IIII_I( mvwhline )

#define CURSES_II_I(FNNAME)                     \
int Window_##FNNAME(Context* ctx, Value& self)  \
{                                               \
    GETWINDOW(win);                             \
    pint_t  a   = ctx->GetIntArg(0);            \
    pint_t  b   = ctx->GetIntArg(1);            \
    pint_t  res = FNNAME(win, a, b);            \
    ctx->Push( res );                           \
    return 1;                                   \
}

CURSES_II_I(mvwin)
CURSES_II_I(wmove)
CURSES_II_I(wredrawln)
CURSES_II_I(wsetscrreg)
CURSES_II_I(mvwinch)
CURSES_II_I(touchline)
CURSES_II_I(wresize)
CURSES_II_I(mvderwin)
CURSES_II_I(mvwdelch)
CURSES_II_I(mvwgetch)
CURSES_II_I(box)
CURSES_II_I(whline)
CURSES_II_I(wvline)

#define CURSES_Z_I(FNNAME)                      \
int Window_##FNNAME(Context* ctx, Value& self)  \
{                                               \
    GETWINDOW(win);                             \
    bool   b   = ctx->GetBoolArg(0);            \
    pint_t  res = FNNAME(win, b);               \
    ctx->Push( res );                           \
    return 1;                                   \
}

CURSES_Z_I(meta)
CURSES_Z_I(clearok)
CURSES_Z_I(nodelay)
CURSES_Z_I(notimeout)
CURSES_Z_I(scrollok)
CURSES_Z_I(intrflush)
CURSES_Z_I(keypad)
CURSES_Z_I(leaveok)
CURSES_Z_I(syncok) 
CURSES_Z_I(idlok)

#define CURSES_Z_NA(FNNAME)                     \
int Window_##FNNAME(Context* ctx, Value& self)  \
{                                               \
    GETWINDOW(win);                             \
    bool   b   = ctx->GetBoolArg(0);            \
    FNNAME(win, b);                             \
    return 0;                                   \
}

CURSES_Z_NA(idcok)
CURSES_Z_NA(immedok)

#define CURSES_I_I(FNNAME)                      \
int Window_##FNNAME(Context* ctx, Value& self)  \
{                                               \
    GETWINDOW(win);                             \
    pint_t  a   = ctx->GetIntArg(0);            \
    pint_t  res = FNNAME(win, a);               \
    ctx->Push( res );                           \
    return 1;                                   \
}

CURSES_I_I(wattron)
CURSES_I_I(wattroff)
CURSES_I_I(wattrset)
CURSES_I_I(winsdelln)
CURSES_I_I(wscrl)
CURSES_I_I(is_linetouched)
CURSES_I_I(wbkgd)

#define CURSES_I_NA(FNNAME)                     \
int Window_##FNNAME(Context* ctx, Value& self)  \
{                                               \
    GETWINDOW(win);                             \
    pint_t  a = ctx->GetIntArg(0);              \
    FNNAME(win, a);                             \
    return 0;                                   \
}

CURSES_I_NA(wtimeout)
CURSES_I_NA(wbkgdset)

#define STATIC_I_I(FNNAME)                      \
int Static_##FNNAME(Context* ctx, Value& self)  \
{                                               \
    pint_t  a   = ctx->GetIntArg(0);            \
    pint_t  res = FNNAME( a );                  \
    ctx->Push( res );                           \
    return 1;                                   \
}

STATIC_I_I(slk_init)
STATIC_I_I(slk_attron)
STATIC_I_I(slk_attroff)
STATIC_I_I(slk_attrset)
STATIC_I_I(slk_color)

#define STATIC_NA_I(FNNAME)                     \
int Static_##FNNAME(Context* ctx, Value& self)  \
{                                               \
    pint_t  res = FNNAME();                     \
    ctx->Push( res );                           \
    return 1;                                   \
}

STATIC_NA_I( slk_refresh )
STATIC_NA_I( slk_noutrefresh )
STATIC_NA_I( slk_clear )
STATIC_NA_I( slk_restore )
STATIC_NA_I( slk_touch )
STATIC_NA_I( slk_attr )


int Static_slk_label(Context* ctx, Value&)
{
    pint_t labnum = ctx->GetIntArg(0);
    
    char* cstrlabel = slk_label(labnum);
    
    if (cstrlabel)
    {
        String* label = ctx->GetEngine()->AllocString(cstrlabel);
        ctx->Push( label );
        return 1;
    }

    return 0;
}

int Static_slk_set(Context* ctx, Value&)
{
    pint_t labnum = ctx->GetIntArg(0);
    String* label = ctx->GetStringArg(1);
    pint_t fmt = ctx->GetIntArg(2);    
    pint_t res = slk_set(labnum, label->GetBuffer(), fmt);
    ctx->Push( res );
    return 1;
}
/*
TODO: Unimplemented slk methods.
        int     slk_attr_on(attr_t attrs, void* opts);
        int     slk_attr_off(const attr_t attrs, void * opts);
        int     slk_attr_set(const attr_t attrs, short color_pair_number, void* opts);      
       
*/

pint_t Curses_color_pair(pint_t ch)
{
    return (pint_t)COLOR_PAIR(ch);
}

pint_t Curses_pair_number(pint_t ch)
{
    return (pint_t)PAIR_NUMBER(ch);
}

#define DEFINE_KEY(K) \
    { #K, K },

PIKA_MODULE(curses, eng, curses)
{
    GCPAUSE(eng);
    
    // Curses //////////////////////////////////////////////////////////////////////////////////////
    
    SlotBinder<Object>(eng, curses, curses)
    .StaticMethodVA(Curses_Init,      "init")
    .StaticMethod(endwin,             "quit")
    .StaticMethod(move,               "move")
    .StaticMethod(clear,              "clear")
    .StaticMethod(getch,              "getch")
    .StaticMethod(has_colors,         "has_colors")
    .StaticMethod(start_color,        "start_color")
    .StaticMethod(init_pair,          "init_pair")
    .StaticMethod(init_color,         "init_color")
    .StaticMethod(Curses_color_pair,  "color_pair")
    .StaticMethod(Curses_pair_number, "pair_number")
    .StaticMethod(nl,                 "nl")
    .StaticMethod(nonl,               "nonl")
    .StaticMethod(noecho,             "noecho")
    .StaticMethod(echo,               "echo")
    .StaticMethod(cbreak,             "cbreak")
    .StaticMethod(nocbreak,           "nocbreak")
    .StaticMethod(noraw,              "noraw")    
    .Constant((pint_t)ERR,              "ERROR")
    .Constant((pint_t)OK,               "OK")
    ;
    
    // Curses.Acs //////////////////////////////////////////////////////////////////////////////////
    
    static NamedConstant CursesAcs[] = {    
    { "ULCORNER",   ACS_ULCORNER },
    { "LLCORNER",   ACS_LLCORNER },
    { "URCORNER",   ACS_URCORNER },
    { "LRCORNER",   ACS_LRCORNER },
    { "LTEE",       ACS_LTEE     },
    { "RTEE",       ACS_RTEE     },
    { "BTEE",       ACS_BTEE     },
    { "TTEE",       ACS_TTEE     },
    { "HLINE",      ACS_HLINE    },
    { "VLINE",      ACS_VLINE    },
    { "PLUS",       ACS_PLUS     },
    { "S1",         ACS_S1       },
    { "S9",         ACS_S9       },
    { "DIAMOND",    ACS_DIAMOND  },
    { "CKBOARD",    ACS_CKBOARD  },
    { "DEGREE",     ACS_DEGREE   },
    { "PLMINUS",    ACS_PLMINUS  },
    { "BULLET",     ACS_BULLET   },
    { "LARROW",     ACS_LARROW   },
    { "RARROW",     ACS_RARROW   },
    { "DARROW",     ACS_DARROW   },
    { "UARROW",     ACS_UARROW   },
    { "BOARD",      ACS_BOARD    },
    { "LANTERN",    ACS_LANTERN  },
    { "BLOCK",      ACS_BLOCK    },
    { "S3",         ACS_S3       },
    { "S7",         ACS_S7       },
    { "LEQUAL",     ACS_LEQUAL   },
    { "GEQUAL",     ACS_GEQUAL   },
    { "PI",         ACS_PI       },
    { "NEQUAL",     ACS_NEQUAL   },
    { "STERLING",   ACS_STERLING },
    { "BSSB",       ACS_BSSB     },
    { "SSBB",       ACS_SSBB     },
    { "BBSS",       ACS_BBSS     },
    { "SBBS",       ACS_SBBS     },
    { "SBSS",       ACS_SBSS     },
    { "SSSB",       ACS_SSSB     },
    { "SSBS",       ACS_SSBS     },
    { "BSSS",       ACS_BSSS     },
    { "BSBS",       ACS_BSBS     },
    { "SBSB",       ACS_SBSB     },
    { "SSSS",       ACS_SSSS     },
    };
    
    Package* acs_Pkg = eng->OpenPackage(eng->AllocString("Acs"), curses, true);
    Basic::EnterConstants(acs_Pkg, CursesAcs, countof(CursesAcs));

    // Curses.Attributes ///////////////////////////////////////////////////////////////////////////
    
    static NamedConstant CursesAttributes[] = {
    { "NORMAL",     A_NORMAL     },
    { "ATTRIBUTES", A_ATTRIBUTES },
    { "CHARTEXT",   A_CHARTEXT   },
    { "COLOR",      A_COLOR      },
    { "STANDOUT",   A_STANDOUT   },
    { "UNDERLINE",  A_UNDERLINE  },
    { "REVERSE",    A_REVERSE    },
    { "BLINK",      A_BLINK      },
    { "DIM",        A_DIM        },
    { "BOLD",       A_BOLD       },
    { "ALTCHARSET", A_ALTCHARSET },
    { "INVIS",      A_INVIS      },
    { "PROTECT",    A_PROTECT    },
    { "HORIZONTAL", A_HORIZONTAL },
    { "LEFT",       A_LEFT       },
    { "LOW",        A_LOW        },
    { "RIGHT",      A_RIGHT      },
    { "TOP",        A_TOP        },
    { "VERTICAL",   A_VERTICAL   },
    };
    
    Package* attr_Pkg = eng->OpenPackage(eng->AllocString("Attr"), curses, true); 
    Basic::EnterConstants(attr_Pkg, CursesAttributes, countof(CursesAttributes));
        
    // TODO: Add Function keys.
    static NamedConstant CursesKeys[] = {
    DEFINE_KEY( KEY_CODE_YES)
    DEFINE_KEY( KEY_MIN		)
    DEFINE_KEY( KEY_BREAK	)
    DEFINE_KEY( KEY_SRESET	)
    DEFINE_KEY( KEY_RESET	)
    DEFINE_KEY( KEY_DOWN	)
    DEFINE_KEY( KEY_UP		)
    DEFINE_KEY( KEY_LEFT	)
    DEFINE_KEY( KEY_RIGHT	)
    DEFINE_KEY( KEY_HOME	)
    DEFINE_KEY( KEY_BACKSPACE)
    DEFINE_KEY( KEY_DL		)
    DEFINE_KEY( KEY_IL		)
    DEFINE_KEY( KEY_DC		)
    DEFINE_KEY( KEY_IC		)
    DEFINE_KEY( KEY_EIC		)
    DEFINE_KEY( KEY_CLEAR	)
    DEFINE_KEY( KEY_EOS		)
    DEFINE_KEY( KEY_EOL		)
    DEFINE_KEY( KEY_SF		)
    DEFINE_KEY( KEY_SR		)
    DEFINE_KEY( KEY_NPAGE	)
    DEFINE_KEY( KEY_PPAGE	)
    DEFINE_KEY( KEY_STAB	)
    DEFINE_KEY( KEY_CTAB	)
    DEFINE_KEY( KEY_CATAB	)
    DEFINE_KEY( KEY_ENTER	)
    DEFINE_KEY( KEY_PRINT	)
    DEFINE_KEY( KEY_LL		)
    DEFINE_KEY( KEY_A1		)
    DEFINE_KEY( KEY_A3		)
    DEFINE_KEY( KEY_B2		)
    DEFINE_KEY( KEY_C1		)
    DEFINE_KEY( KEY_C3		)
    DEFINE_KEY( KEY_BTAB	)
    DEFINE_KEY( KEY_BEG		)
    DEFINE_KEY( KEY_CANCEL	)
    DEFINE_KEY( KEY_CLOSE	)
    DEFINE_KEY( KEY_COMMAND	)
    DEFINE_KEY( KEY_COPY	)
    DEFINE_KEY( KEY_CREATE	)
    DEFINE_KEY( KEY_END		)
    DEFINE_KEY( KEY_EXIT	)
    DEFINE_KEY( KEY_FIND	)
    DEFINE_KEY( KEY_HELP	)
    DEFINE_KEY( KEY_MARK	)
    DEFINE_KEY( KEY_MESSAGE	)
    DEFINE_KEY( KEY_MOVE	)
    DEFINE_KEY( KEY_NEXT	)
    DEFINE_KEY( KEY_OPEN	)
    DEFINE_KEY( KEY_OPTIONS	)
    DEFINE_KEY( KEY_PREVIOUS	)
    DEFINE_KEY( KEY_REDO	)    
    DEFINE_KEY( KEY_REFERENCE)
    DEFINE_KEY( KEY_REFRESH	)
    DEFINE_KEY( KEY_REPLACE	)
    DEFINE_KEY( KEY_RESTART	)
    DEFINE_KEY( KEY_RESUME	)
    DEFINE_KEY( KEY_SAVE	)    
    DEFINE_KEY( KEY_SBEG	)
    DEFINE_KEY( KEY_SCANCEL	)
    DEFINE_KEY( KEY_SCOMMAND)
    DEFINE_KEY( KEY_SCOPY	)
    DEFINE_KEY( KEY_SCREATE	)
    DEFINE_KEY( KEY_SDC		)
    DEFINE_KEY( KEY_SDL		)
    DEFINE_KEY( KEY_SELECT	)
    DEFINE_KEY( KEY_SEND	)
    DEFINE_KEY( KEY_SEOL	)
    DEFINE_KEY( KEY_SEXIT	)
    DEFINE_KEY( KEY_SFIND	)
    DEFINE_KEY( KEY_SHELP	)
    DEFINE_KEY( KEY_SHOME	)
    DEFINE_KEY( KEY_SIC		)
    DEFINE_KEY( KEY_SLEFT	)
    DEFINE_KEY( KEY_SMESSAGE)
    DEFINE_KEY( KEY_SMOVE	)
    DEFINE_KEY( KEY_SNEXT	)
    DEFINE_KEY( KEY_SOPTIONS )
    DEFINE_KEY( KEY_SPREVIOUS)
    DEFINE_KEY( KEY_SPRINT	)
    DEFINE_KEY( KEY_SREDO	)
    DEFINE_KEY( KEY_SREPLACE)
    DEFINE_KEY( KEY_SRIGHT	)
    DEFINE_KEY( KEY_SRSUME	)
    DEFINE_KEY( KEY_SSAVE	)
    DEFINE_KEY( KEY_SSUSPEND)
    DEFINE_KEY( KEY_SUNDO	)
    DEFINE_KEY( KEY_SUSPEND	)
    DEFINE_KEY( KEY_UNDO	)
    DEFINE_KEY( KEY_MOUSE	)
    DEFINE_KEY( KEY_RESIZE	)
    DEFINE_KEY( KEY_EVENT	)
    DEFINE_KEY( KEY_MAX		)
    };

    Basic::EnterConstants(curses, CursesKeys, countof(CursesKeys));

    // Curses.Colors ///////////////////////////////////////////////////////////////////////////////
    
    static NamedConstant CursesColors[] = {
    { "BLACK",      COLOR_BLACK   },
    { "RED",        COLOR_RED     },
    { "GREEN",      COLOR_GREEN   },
    { "YELLOW",     COLOR_YELLOW  },
    { "BLUE",       COLOR_BLUE    },
    { "MAGENTA",    COLOR_MAGENTA },
    { "CYAN",       COLOR_CYAN    },
    { "WHITE",      COLOR_WHITE   },
    }; 
       
    Package* color_Pkg = eng->OpenPackage(eng->AllocString("Color"), curses, true); 
    Basic::EnterConstants(color_Pkg, CursesColors, countof(CursesColors));
    
    // Curses.Slk //////////////////////////////////////////////////////////////////////////////////
    
    static RegisterFunction SlkMethods[] = {
    {"init",            Static_slk_init,        1, 0, 1 },
    {"attron",          Static_slk_attron,      1, 0, 1 },
    {"attroff",         Static_slk_attroff,     1, 0, 1 },
    {"attrset",         Static_slk_attrset,     1, 0, 1 },
    {"color",           Static_slk_color,       1, 0, 1 },
    
    {"refresh",         Static_slk_refresh,     0, 0, 1 },
    {"noutrefresh",     Static_slk_noutrefresh, 0, 0, 1 },
    {"clear",           Static_slk_clear,       0, 0, 1 },
    {"restore",         Static_slk_restore,     0, 0, 1 },
    {"touch",           Static_slk_touch,       0, 0, 1 },
    {"attr",            Static_slk_attr,        0, 0, 1 },
            
    {"label",           Static_slk_label,       1, 0, 1 },
    {"set",             Static_slk_set,         3, 0, 1 },
    };
    
    Package* slk_Pkg = eng->OpenPackage(eng->AllocString("Slk"), curses, true);
    slk_Pkg->EnterFunctions(SlkMethods, countof(SlkMethods));

    // Curses.Window ///////////////////////////////////////////////////////////////////////////////
    
    static RegisterFunction WindowMethods[] = {
    { "mvaddch",        Window_mvaddch,         3, 0, 1 },  
    { "mvaddstr",       Window_mvaddstr,        3, 0, 1 },
    
    { "addch",          Window_addch,           1, 0, 1 },
    { "addstr",         Window_addstr,          1, 0, 1 },
    
    { "refresh",        Window_wrefresh,        0, 0, 1 },
    { "erase",          Window_werase,          0, 0, 1 },
    { "clear",          Window_wclear,          0, 0, 1 },    
    { "clrtobot",       Window_wclrtobot,       0, 0, 1 },
    { "clrtoeol",       Window_wclrtoeol,       0, 0, 1 },
    { "delch",          Window_wdelch,          0, 0, 1 },
    { "deleteln",       Window_wdeleteln,       0, 0, 1 },
    { "getch",          Window_wgetch,          0, 0, 1 },
    { "inch",           Window_winch,           0, 0, 1 },
    { "insertln",       Window_winsertln,       0, 0, 1 },
    { "standout",       Window_wstandout,       0, 0, 1 },
    { "standend",       Window_wstandend,       0, 0, 1 },
    { "scroll",         Window_scroll,          0, 0, 1 },
    { "syncdown",       Window_wsyncdown,       0, 0, 1 },
    { "syncup",         Window_wsyncup,         0, 0, 1 },
    
    { "mvwin",          Window_mvwin,           2, 0, 1 },
    { "move",           Window_wmove,           2, 0, 1 },
    { "redrawln",       Window_wredrawln,       2, 0, 1 },
    { "setscrreg",      Window_wsetscrreg,      2, 0, 1 },
    { "mvinch",         Window_mvwinch,         2, 0, 1 },
    { "touchline",      Window_touchline,       2, 0, 1 },
    { "resize",         Window_wresize,         2, 0, 1 },
    { "mvderwin",       Window_mvderwin,        2, 0, 1 },
    { "mvdelch",        Window_mvwdelch,        2, 0, 1 },
    { "mvgetch",        Window_mvwgetch,        2, 0, 1 },            
    
    { "getyx",          Window_getyx,           0, 0, 1 },    
    { "begyx",          Window_begyx,           0, 0, 1 },   
    { "maxyx",          Window_maxyx,           0, 0, 1 },      
    
    { "meta",           Window_meta,            1, 0, 1 },
    { "clearok",        Window_clearok,         1, 0, 1 },
    { "nodelay",        Window_nodelay,         1, 0, 1 },
    { "notimeout",      Window_notimeout,       1, 0, 1 },
    { "scrollok",       Window_scrollok,        1, 0, 1 },
    { "intrflush",      Window_intrflush,       1, 0, 1 },
    { "keypad",         Window_keypad,          1, 0, 1 },
    { "leaveok",        Window_leaveok,         1, 0, 1 },
    { "syncok",         Window_syncok,          1, 0, 1 },
    { "idlok",          Window_idlok,           1, 0, 1 },
    { "idcok",          Window_idcok,           1, 0, 1 },
    { "immedok",        Window_immedok,         1, 0, 1 },
    
    { "attron",         Window_wattron,         1, 0, 1 },
    { "attroff",        Window_wattroff,        1, 0, 1 },
    { "attrset",        Window_wattrset,        1, 0, 1 },
    
    { "insdelln",       Window_winsdelln,       1, 0, 1 },
    { "scrl",           Window_wscrl,           1, 0, 1 },
    { "timeout",        Window_wtimeout,        1, 0, 1 },
    { "linetouched?",   Window_is_linetouched,  1, 0, 1 },
    
    { "bkgdset",        Window_wbkgdset,        1, 0, 1 },
    { "bkgd",           Window_wbkgd,           1, 0, 1 },

    { "box",            Window_box,             2, 0, 1 },
    { "vline",          Window_wvline,          2, 0, 1 },
    { "hline",          Window_whline,          2, 0, 1 },
    
    { "mvvline",        Window_mvwvline,        4, 0, 1 },
    { "mvhline",        Window_mvwhline,        4, 0, 1 },
    
    { "border",         Window_wborder,         8, 0, 1 },           
    };
    
    Type* Window_Type = Type::Create(eng, eng->AllocString("Window"), eng->Object_Type, 0, curses);
    Window_Type->EnterMethods(WindowMethods, countof(WindowMethods));
    
    curses->SetSlot(eng->AllocString("Window"), Window_Type);
    eng->AddBaseType(eng->AllocString(WINDOW_TYPENAME), Window_Type);
    return curses;
}

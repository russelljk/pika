/*  Main.cpp
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
 *  3. This notice may not be removed or altered from any source
 *  distribution.
 *
 */
#include "Pika.h"
using namespace pika;

void Pika_DisplayUsage(const char* name)
{
    std::cerr << '\n' << PIKA_BANNER_STR", "PIKA_COPYRIGHT_STR << '\n';
    std::cerr << "\nUsage: " << name <<  " file [options]\n";
    std::cerr << "\nOptions:\n";
    std::cerr << "\t--arg,  -a    : White space seperated arguments i.e. \"arg1 arg2 arg3\"\n";
    std::cerr << "\t--file, -f    : File to execute.\n";
    std::cerr << "\t--path, -p    : Add a search path. Multiple paths may be specified.\n";
    std::cerr << "\t--supress, -s : Supress startup banner.\n";
    std::cerr << "\t--version, -v : White space seperated arguments i.e. \"arg1 arg2 arg3\"\n";
    std::cerr << std::endl;
    
    exit(1);
}

static void Pika_PrintBanner()
{
    std::cout << PIKA_BANNER_STR   << ' ' <<
    PIKA_COPYRIGHT_STR << std::endl;
}

struct CommandLine
{
    CommandLine(int argc, char** argv) : has_dash(false), count(argc), args(argv)
    {
        pos  = 1;
        kind = '\0';
        options  = 0;
        
        if (pos < count)
        {
            Next();
        }
    }
    
    void Next()
    {
        has_dash = false;
        if (pos < count)
        {
            const char* curr = args[pos];
            size_t len = strlen(curr);
            if (curr[0] == '-')
                has_dash = true;
            if (len >= 2 && has_dash)
            {
                kind = curr[1]; //set it to - temporarily
                if (kind == '-' && len > 2) //need more than 2 chars
                {
                    if (strcmp(curr+2, "path") == 0) 
                    {
                        kind = 'p';
                    } 
                    else if (strcmp(curr+2, "file") == 0) 
                    {
                        kind = 'f';                        
                    } 
                    else if (strcmp(curr+2, "arg") == 0) 
                    {
                        kind = 'a';                        
                    } 
                    else if (strcmp(curr+2, "version") == 0) 
                    {
                        kind = 'v';                        
                    } 
                    
                    if (kind != '-' && kind != 'v') // Should be a space between the kind and option
                    {
                        int nextPos = pos + 1;
                        if (nextPos < count)
                        {
                            const char* nextCurr = args[nextPos];
                            if (nextCurr[0] != '-')
                            {
                                options = nextCurr;
                                pos += 2;
                                return;
                            }
                        }
                    }
                    options = 0; // at this point something went wrong
                }
                else {
                    if (len > 2)
                    {
                        options = curr + 2; //skip past "-X" where X is the kind of argument
                    }
                    else if (kind == 'v')
                    {
                        options = 0;
                    }                    
                    else
                    {
                        // Allow a space between the kind and option
                        int nextPos = pos + 1;
                        if (nextPos < count)
                        {
                            const char* nextCurr = args[nextPos];
                            if (nextCurr[0] != '-')
                            {
                                options = nextCurr;
                                pos += 2;
                                return;
                            }
                        }
                        options = 0;
                    }
                }
            }
            else
            {
                // no dash or null argument
                kind = '\0';
                options = curr;
            }
            pos++;
        }
        else
        {
            kind = EOF; options = 0;
        }
    }
    
    operator    bool()    { return kind != EOF; }
    int         Kind()    { return kind; }
    const char* Opt()     { return options; }
    const char* CurrArg() { return (pos < count) ? args[pos] : 0; }
    bool        HasDash() { return has_dash; }
    bool        has_dash;
    int         count;
    int         pos;
    char**      args;
    int         kind;
    const char* options;
};

Array* CreateArguments(Engine* eng, const char* cstr)
{
    GCPAUSE_NORUN( eng );
    String* str  = eng->AllocString(cstr ? cstr : "");
    String* delm = eng->AllocString(" \t\n\r");
    
    return str->Split(delm);
}

void AddArgument(Engine* eng, const char* opt, Array* arguments)
{
#if 0
    Array* opt_arguments = 0;

    if ((opt_arguments = CreateArguments( eng, opt )))
    {
        if (arguments)
        {
            arguments->Append( opt_arguments );
        }
        else
        {
            arguments = opt_arguments;
        }
    }
#else
    String* arg_str = eng->GetString(opt ? opt : "");
    Value varg(arg_str);
    arguments->Push(varg);
#endif
}

void ReadExecutePrintLoop(Engine* eng)
{
    std::cerr << "Entering Interactive Session..." << std::endl;
    eng->ReadExecutePrintLoop();
}

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        //Pika_DisplayUsage(argv[0]);
        Engine* eng = Engine::Create();
        eng->ReadExecutePrintLoop();
        eng->Release();
        return 0;
    }
        
    if (argc >= 2)
    {
        Array* arguments = 0;
        Engine* eng = 0;
        const char* fileName = 0;
        try
        {
            eng = Engine::Create();
            {
                GCPAUSE_NORUN( eng );
                arguments = Array::Create(eng, eng->Array_Type, 0, 0);
            }
            
            CommandLine cl(argc, argv);
            
            if (cl.Kind() == '\0' && cl.Opt())
            {
                fileName = cl.Opt();
                cl.Next();
            }
            
            while (cl)
            {
                if (cl.Opt() == 0)
                {
                    if (cl.Kind() != 'v')
                    {
                        Pika_DisplayUsage(argv[0]);
                    }
                }
                                
                switch (cl.Kind())
                {
                    /*
                     * Search path specification: -pPathToSearch
                     * -------------------------------------------------------------
                     * Used by import to find modules, scripts and open files.
                     */
                case 'p':
                {
                    if ((cl.Opt() == 0))
                    {
                        Pika_DisplayUsage(argv[0]);
                    }
                    eng->AddSearchPath(cl.Opt());
                    break;
                }
                /*
                 * Argument specification: -a"Arg1 Arg2 Arg3" or -aArg1
                 * -------------------------------------------------------------
                 * Each argument should be space seperated.
                 * If arguments have already been specified then these arguments
                 * are appended to the previous array of arguments.
                 * All arguments are strings.
                 */
                case 'a':
                {
                    AddArgument(eng, cl.Opt(), arguments);
                }
                break;
                /*
                 * Script File: -fFileToExecute
                 */
                case 'f':
                {
                    fileName = cl.Opt();
                    break;
                }
                /*
                 * Suppress Banner: -s
                 */
                case 'v':
                {                                        
                    if (!fileName)
                    {
                        Pika_PrintBanner();
                        exit(0);
                    }
                    break;
                }
                default: 
                {
                    if (cl.Kind() == '\0' && cl.Opt())
                    {
                        if (!fileName)
                        {
                            fileName = cl.Opt();                            
                        }
                        else
                        {
                            AddArgument(eng, cl.Opt(), arguments);
                        }
                    }
                    else
                    {
                        std::cerr << "Unknown Option: " << cl.CurrArg();
                        Pika_DisplayUsage(argv[0]);
                    }
                }
                }// switch
                cl.Next();
            }
            
            if (!fileName) 
            {
                ReadExecutePrintLoop(eng);
            }
            else
            {
                // Depending on the size of the script, arguments might be collected
                // before Compile returns.
                //
                // So we add them to the roots to prevent collection.
                
                eng->AddToRoots(arguments);
                
                // Add paths defined in $PIKA_PATH to our PathManager.
                eng->AddEnvPath("PIKA_PATH");
                
                // Compile the script.
                Script* script = eng->Compile(fileName);
                
                if (script)
                {
                    script->Run(arguments);
                }
                else
                {
                    std::cerr << "\n** Could not run script: " << fileName << std::endl;
                }
            }
        }
        catch (Exception& e)
        {
            std::cerr << "Uncaught " << e.GetErrorKind() << ": " << e.GetMessage() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Uncaught exception. Exiting pika ..." << std::endl;
        }
        if (eng)
        {
            eng->Release();
        }
    }
    return 0;
}

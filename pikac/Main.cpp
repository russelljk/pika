/*
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include <iostream>

// TODO: Specifiy arguments.
void Pika_DisplayUsage(const char* name)
{
    std::cerr << '\n' << PIKA_VERSION_STR", "PIKA_COPYRIGHT_STR << '\n';
    std::cerr << "\nUsage: " << name <<  "[options] file [arguments]\n";
    std::cerr << "\nArguments:\n\tSpace seperated arguments to pass to the script" << std::endl;
    
    exit(1);
}

static void Pika_PrintBanner()
{
    std::cerr << PIKA_UNDERLINE_STR << '\n' <<
    PIKA_VERSION_STR   << '\n' <<
    PIKA_COPYRIGHT_STR << '\n' <<
    PIKA_UNDERLINE_STR << std::endl;
}

struct CommandLine
{
    CommandLine(int argc, char** argv) : count(argc), args(argv)
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
        if (pos < count)
		{
            const char* curr = args[pos];
            size_t len = strlen(curr);
            
            if (curr[0] == '-')
			{
                kind = curr[1];
                if (len > 2) 
				{
                    options = curr + 2; //skip past "-X" where X is the kind of argument
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
    
    bool        is_valid;
    int         count;
    int         pos;
    char**      args;
    int         kind;
    const char* options;
};

Array* CreateArguments(Engine* eng, const char* cstr)
{
    GCPAUSE_NORUN( eng );
    String* str  = eng->AllocString(cstr);
    String* delm = eng->AllocString(" \t\n\r");
    
    return str->Split(delm);
}

int main(int argc, char* argv[])
{
    if (argc == 1)
	{
        Pika_DisplayUsage(argv[0]);
    }
    bool printBanner = true;

    if (argc >= 2)
	{
        Engine* eng = 0;
        const char* fileName = 0;
        Array* arguments = 0;
        try
		{
            eng = Engine::Create();
            
            CommandLine cl(argc, argv);
            
            if (cl.Kind() == '\0' && cl.Opt())
			{
                fileName = cl.Opt();
                cl.Next();
            }
            
            while (cl)
			{
                if ((cl.Opt() == 0) || (strlen(cl.Opt()) == 0))
				{
                    if (cl.Kind() != 's')
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
                    Array* opt_arguments = 0;
                    
                    if ((opt_arguments = CreateArguments( eng, cl.Opt() )))
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
                case 's':
				{
                    printBanner = printBanner;
                    break;
                }
                default:
				{
                    std::cerr << "Unknown Option: " << cl.CurrArg();
                    Pika_DisplayUsage(argv[0]);
                }
                }
                cl.Next();
            }
            
            if (printBanner)
			{
                Pika_PrintBanner();
            }
            eng->AddEnvPath("PIKA_PATH");
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
        catch (...)
		{
            std::cerr << "Uncaught exception. Exiting pika ..." << std::endl;
        }
        if (eng)
		{
            eng->Release();
        }
    }
}

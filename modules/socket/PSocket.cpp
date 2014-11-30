#if defined(PIKA_WIN)
#   if defined(HAVE_WINSOCK2_H)
#       pragma comment(lib, "ws2_32.dll")
#   elif defined(HAVE_WINSOCK_H)
#       pragma comment(lib, "wsock32.dll")
#   endif

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

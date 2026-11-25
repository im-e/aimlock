#pragma once
#include <Windows.h>
#include <cstdio>

//CHECK(call) - wraps any call that returns an HRESULT
//if it fails it prints the call, error code, file and line to the Output window,
//breaks into the debugger if one is attached, otherwise shows a message box and quits
inline void checkHRESULT(HRESULT hr, const char* call, const char* file, int line)
{
    if (SUCCEEDED(hr))
        return;

    //ask windows for a readable description of the error code
    char description[256] = "unknown error";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr,
        0, description, sizeof(description), NULL);

    char message[1024];
    sprintf_s(message, "%s(%d): CHECK failed\n%s\nHRESULT 0x%08X: %s\n",
        file, line, call, (unsigned int)hr, description);
    OutputDebugStringA(message); //double click the line in the Output window to jump to it

    if (IsDebuggerPresent())
        __debugbreak(); //stops right here - step out to see the failing call

    MessageBoxA(NULL, message, "DirectX error", MB_OK | MB_ICONERROR);
    ExitProcess((UINT)hr);
}

#define CHECK(call) checkHRESULT((call), #call, __FILE__, __LINE__)

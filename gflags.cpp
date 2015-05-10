/*
 * Global flags editor
 *
 * Copyright (c) 2015 Mark Jansen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Windows.h>
#include <Strsafe.h>
#include "gflags.h"

#define GLOBALFLAG_REGKEY           L"SYSTEM\\CurrentControlSet\\Control\\Session Manager"
#define GLOBALFLAG_VALUENAME        L"GlobalFlag"

#define IMAGE_FILE_OPTIONS          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%s"

// reactos/include/ndk/extypes.h
#define SystemFlagsInformation          9
#define SystemRefTraceInformation       86
#define SystemSpecialPoolInformation    87

typedef struct _SYSTEM_FLAGS_INFORMATION
{
    ULONG Flags;
} SYSTEM_FLAGS_INFORMATION, *PSYSTEM_FLAGS_INFORMATION;


// Table from https://msdn.microsoft.com/en-us/library/windows/hardware/ff549596(v=vs.85).aspx

const FlagInfo g_Flags[] =
{
    {FLG_STOP_ON_EXCEPTION, L"soe", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Stop on exception"},
    {FLG_SHOW_LDR_SNAPS, L"sls", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Show loader snaps"},
    {FLG_DEBUG_INITIAL_COMMAND, L"dic", (DEST_REGISTRY), L"Debug initial command"},
    {FLG_STOP_ON_HUNG_GUI, L"shg", (DEST_KERNEL), L"Stop on hung GUI"},
    {FLG_HEAP_ENABLE_TAIL_CHECK, L"htc", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap tail checking"},
    {FLG_HEAP_ENABLE_FREE_CHECK, L"hfc", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap free checking"},
    {FLG_HEAP_VALIDATE_PARAMETERS, L"hpc", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap parameter checking"},
    {FLG_HEAP_VALIDATE_ALL, L"hvc", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap validation on call"},
    {FLG_APPLICATION_VERIFIER, L"vrf", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable application verifier"},
    // FLG_MONITOR_SILENT_PROCESS_EXIT
    {FLG_POOL_ENABLE_TAGGING, L"ptg", (DEST_REGISTRY), L"Enable pool tagging"},
    {FLG_HEAP_ENABLE_TAGGING, L"htg", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap tagging"},
    {FLG_USER_STACK_TRACE_DB, L"ust", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Create user mode stack trace database"},
    {FLG_KERNEL_STACK_TRACE_DB, L"kst", (DEST_REGISTRY), L"Create kernel mode stack trace database"},
    {FLG_MAINTAIN_OBJECT_TYPELIST, L"otl", (DEST_REGISTRY), L"Maintain a list of objects for each type"},
    {FLG_HEAP_ENABLE_TAG_BY_DLL, L"htd", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap tagging by DLL"},
    {FLG_DISABLE_STACK_EXTENSION, L"dse", (DEST_IMAGE), L"Disable stack extension"},

    {FLG_ENABLE_CSRDEBUG, L"d32", (DEST_REGISTRY), L"Enable debugging of Win32 subsystem"},
    {FLG_ENABLE_KDEBUG_SYMBOL_LOAD, L"ksl", (DEST_REGISTRY | DEST_KERNEL), L"Enable loading of kernel debugger symbols"},
    {FLG_DISABLE_PAGE_KERNEL_STACKS, L"dps", (DEST_REGISTRY), L"Disable paging of kernel stacks"},
    {FLG_ENABLE_SYSTEM_CRIT_BREAKS, L"scb", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable system critical breaks"},
    {FLG_HEAP_DISABLE_COALESCING, L"dhc", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Disable heap coalesce on free"},
    {FLG_ENABLE_CLOSE_EXCEPTIONS, L"ece", (DEST_REGISTRY | DEST_KERNEL), L"Enable close exception"},
    {FLG_ENABLE_EXCEPTION_LOGGING, L"eel", (DEST_REGISTRY | DEST_KERNEL), L"Enable exception logging"},
    {FLG_ENABLE_HANDLE_TYPE_TAGGING, L"eot", (DEST_REGISTRY | DEST_KERNEL), L"Enable object handle type tagging"},
    {FLG_HEAP_PAGE_ALLOCS, L"hpa", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable page heap"},
    {FLG_DEBUG_INITIAL_COMMAND_EX, L"dwl", (DEST_REGISTRY), L"Debug WinLogon"},
    {FLG_DISABLE_DBGPRINT, L"ddp", (DEST_REGISTRY | DEST_KERNEL), L"Buffer DbgPrint Output"},
    {FLG_CRITSEC_EVENT_CREATION, L"cse", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Early critical section event creation"},
    {FLG_STOP_ON_UNHANDLED_EXCEPTION, L"sue", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Stop on unhandled user-mode exception"},
    {FLG_ENABLE_HANDLE_EXCEPTIONS, L"bhd", (DEST_REGISTRY | DEST_KERNEL), L"Enable bad handles detection"},
    {FLG_DISABLE_PROTDLLS, L"dpd", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Disable protected DLL verification"},
};

//{FLG_MONITOR_SILENT_PROCESS_EXIT, NULL, (DEST_REGISTRY), L"Enable silent process exit monitoring"},
//{0, NULL, (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Object Reference Tracing},
//{0, L"spp", (DEST_REGISTRY | DEST_KERNEL), Special Pool"},    // kernel only in vista


size_t g_FlagCount = sizeof(g_Flags) / sizeof(g_Flags[0]);

DWORD g_ValidRegistryFlags = 0;
DWORD g_ValidKernelFlags = 0;
DWORD g_ValidImageFlags = 0;
DWORD g_PoolTaggingEnabled = 0;

typedef NTSTATUS (NTAPI* tNtQuerySystemInformation)(ULONG SystemInformationClass, PVOID SystemInformation, ULONG InformationLength, PULONG ResultLength);
typedef NTSTATUS (NTAPI* tNtSetSystemInformation)(ULONG SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength);
typedef NTSTATUS (NTAPI* tRtlGetVersion)(_Inout_ LPOSVERSIONINFOW lpVersionInformation);


static tNtQuerySystemInformation g_NtQuerySystemInformation = NULL;
static tNtSetSystemInformation g_NtSetSystemInformation = NULL;
static tRtlGetVersion g_RtlGetVersion = NULL;

template<typename TYP_, typename FNC_, FNC_ Func>
struct AutoClose
{
    AutoClose( TYP_ value ) : Value(value) {;}
    ~AutoClose()
    {
        Func( Value );
    }
    TYP_ Value;
};

struct AutoCloseHandle : AutoClose<HANDLE, BOOL (WINAPI*)(HANDLE), CloseHandle>
{
    AutoCloseHandle( HANDLE value ) : AutoClose( value ) {;}
};

struct AutoCloseReg : AutoClose<HKEY, LONG (WINAPI*)(HKEY), RegCloseKey>
{
    AutoCloseReg( HKEY value ) : AutoClose( value ) {;}
};

BOOL InitFunctionPointers()
{
    if(!g_NtQuerySystemInformation || !g_NtSetSystemInformation || !g_RtlGetVersion)
    {
        HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
        g_NtQuerySystemInformation = (tNtQuerySystemInformation)GetProcAddress(hNtdll, "NtQuerySystemInformation");
        g_NtSetSystemInformation = (tNtSetSystemInformation)GetProcAddress(hNtdll, "NtSetSystemInformation");
        g_RtlGetVersion = (tRtlGetVersion)GetProcAddress(hNtdll, "RtlGetVersion");
    }
    return g_NtQuerySystemInformation && g_NtSetSystemInformation;
}

void UpdateValidFlags()
{
    g_ValidRegistryFlags = 0;
    g_ValidKernelFlags = 0;
    g_ValidImageFlags = 0;
    for( size_t n = 0; n < g_FlagCount; ++n ) {
        g_ValidRegistryFlags |= (g_Flags[n].wDest & DEST_REGISTRY) ? g_Flags[n].dwFlag : 0;
        g_ValidKernelFlags |= (g_Flags[n].wDest & DEST_KERNEL) ? g_Flags[n].dwFlag : 0;
        g_ValidImageFlags |= (g_Flags[n].wDest & DEST_IMAGE) ? g_Flags[n].dwFlag : 0;
    }

    if(!InitFunctionPointers())
    {
        return;
    }
    OSVERSIONINFOW osv = {sizeof(osv), NULL};
    g_RtlGetVersion(&osv);
    if(osv.dwMajorVersion > 5 || (osv.dwMajorVersion == 5 && osv.dwMinorVersion >= 2))
    {
        g_PoolTaggingEnabled = 1;
    }
}

BOOL EnableDebug()
{
    static BOOL debugEnabled = FALSE;
    if( !debugEnabled )
    {
        HANDLE hToken;
        if( OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken ) )
        {
            AutoCloseHandle raii(hToken);
            TOKEN_PRIVILEGES tp = {0};
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            if( LookupPrivilegeValueW( NULL, L"SeDebugPrivilege", &tp.Privileges[0].Luid) )
            {
                debugEnabled = AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof(tp), NULL, NULL );
            }
        }
    }
    return debugEnabled;
}


BOOL ReadGlobalFlagsFromRegistry( _Out_ DWORD* Flag )
{
    HKEY hKey;
    if(EnableDebug() && ERROR_SUCCESS == RegOpenKeyExW( HKEY_LOCAL_MACHINE, GLOBALFLAG_REGKEY, 0, KEY_READ, &hKey ) )
    {
        AutoCloseReg raii(hKey);
        DWORD Type = 0, cbData = sizeof(*Flag);
        if( ERROR_SUCCESS == RegQueryValueExW( hKey, GLOBALFLAG_VALUENAME, NULL, &Type, (LPBYTE)Flag, &cbData ) && Type == REG_DWORD )
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL WriteGlobalFlagsToRegistry( _In_ DWORD Flag )
{
    HKEY hKey;
    if(EnableDebug() && ERROR_SUCCESS == RegOpenKeyExW( HKEY_LOCAL_MACHINE, GLOBALFLAG_REGKEY, 0, KEY_WRITE, &hKey ) )
    {
        AutoCloseReg raii(hKey);
        if( ERROR_SUCCESS == RegSetValueExW( hKey, GLOBALFLAG_VALUENAME, NULL, REG_DWORD, (LPBYTE)&Flag, sizeof(Flag) ) )
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ReadImageGlobalFlagsFromRegistry( _In_z_ PCWSTR ImageName, _Out_ ULONG* Flag )
{
    HKEY hKey;
    WCHAR FullKey[260];
    if(!ImageName || !ImageName[0])
    {
        *Flag = 0;
        return TRUE;
    }
    StringCchPrintfW(FullKey, 260, IMAGE_FILE_OPTIONS, ImageName);
    if(EnableDebug())
    {
        LONG lRet = RegOpenKeyExW( HKEY_LOCAL_MACHINE, FullKey, 0, KEY_READ, &hKey );
        if( ERROR_SUCCESS == lRet )
        {
            AutoCloseReg raii(hKey);
            DWORD Type = 0, cbData = sizeof(*Flag);
            if( ERROR_SUCCESS == RegQueryValueExW( hKey, GLOBALFLAG_VALUENAME, NULL, &Type, (LPBYTE)Flag, &cbData ) && Type == REG_DWORD )
            {
                return TRUE;
            }
        }
        else if(ERROR_FILE_NOT_FOUND == lRet)
        {
            *Flag = 0;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL WriteImageGlobalFlagsToRegistry( _In_z_ PCWSTR ImageName,_In_ ULONG Flag )
{
    HKEY hKey;
    WCHAR FullKey[260];
    StringCchPrintfW(FullKey, 260, IMAGE_FILE_OPTIONS, ImageName);
    DWORD dwDisposition = 0;
    if(EnableDebug() && ERROR_SUCCESS == RegCreateKeyExW( HKEY_LOCAL_MACHINE, FullKey, 0, 0, 0, KEY_WRITE, NULL, &hKey, &dwDisposition ))
    {
        AutoCloseReg raii(hKey);
        //dwDisposition == REG_CREATED_NEW_KEY || REG_OPENED_EXISTING_KEY;
        if( ERROR_SUCCESS == RegSetValueExW( hKey, GLOBALFLAG_VALUENAME, NULL, REG_DWORD, (LPBYTE)&Flag, sizeof(Flag) ) )
        {
            return TRUE;
        }
    }
    return FALSE;

}

BOOL ReadGlobalFlagsFromKernel( _Out_ DWORD* Flag )
{
    if(InitFunctionPointers())
    {
        ULONG Length = 0;
        SYSTEM_FLAGS_INFORMATION sfi = {0};
        sfi.Flags = 0;
        if(SUCCEEDED(g_NtQuerySystemInformation(SystemFlagsInformation, &sfi, sizeof(sfi), &Length)) && Length == sizeof(sfi))
        {
            *Flag = sfi.Flags;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL WriteGlobalFlagsToKernel( _In_ DWORD Flag )
{
    if(InitFunctionPointers())
    {
        SYSTEM_FLAGS_INFORMATION sfi = {0};
        sfi.Flags = Flag;
        return SUCCEEDED(g_NtSetSystemInformation(SystemFlagsInformation, &sfi, sizeof(sfi)));
    }
    return FALSE;
}

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

#pragma once

#define FLG_STOP_ON_EXCEPTION               0x1
#define FLG_SHOW_LDR_SNAPS                  0x2
#define FLG_DEBUG_INITIAL_COMMAND           0x4
#define FLG_STOP_ON_HUNG_GUI                0x8
#define FLG_HEAP_ENABLE_TAIL_CHECK          0x10
#define FLG_HEAP_ENABLE_FREE_CHECK          0x20
#define FLG_HEAP_VALIDATE_PARAMETERS        0x40
#define FLG_HEAP_VALIDATE_ALL               0x80
#define FLG_APPLICATION_VERIFIER            0x100
#define FLG_MONITOR_SILENT_PROCESS_EXIT     0x200
#define FLG_POOL_ENABLE_TAGGING             0x400
#define FLG_HEAP_ENABLE_TAGGING             0x800
#define FLG_USER_STACK_TRACE_DB             0x1000
#define FLG_KERNEL_STACK_TRACE_DB           0x2000
#define FLG_MAINTAIN_OBJECT_TYPELIST        0x4000
#define FLG_HEAP_ENABLE_TAG_BY_DLL          0x8000
#define FLG_DISABLE_STACK_EXTENSION         0x10000
#define FLG_ENABLE_CSRDEBUG                 0x20000
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD       0x40000
#define FLG_DISABLE_PAGE_KERNEL_STACKS      0x80000
#define FLG_ENABLE_SYSTEM_CRIT_BREAKS       0x100000
#define FLG_HEAP_DISABLE_COALESCING         0x200000
#define FLG_ENABLE_CLOSE_EXCEPTIONS         0x400000
#define FLG_ENABLE_EXCEPTION_LOGGING        0x800000
#define FLG_ENABLE_HANDLE_TYPE_TAGGING      0x1000000
#define FLG_HEAP_PAGE_ALLOCS                0x2000000
#define FLG_DEBUG_INITIAL_COMMAND_EX        0x4000000
#define FLG_DISABLE_DBGPRINT                0x8000000
#define FLG_CRITSEC_EVENT_CREATION          0x10000000
#define FLG_STOP_ON_UNHANDLED_EXCEPTION     0x20000000
#define FLG_ENABLE_HANDLE_EXCEPTIONS        0x40000000
#define FLG_DISABLE_PROTDLLS                0x80000000


struct FlagInfo
{
    DWORD dwFlag;
    const wchar_t* szAbbr;
    WORD wDest;
    const wchar_t* szDesc;
};

#define DEST_REGISTRY       1
#define DEST_KERNEL         2
#define DEST_IMAGE          4


extern const FlagInfo g_Flags[];
extern size_t g_FlagCount;

extern DWORD g_ValidRegistryFlags;
extern DWORD g_ValidKernelFlags;
extern DWORD g_ValidImageFlags;
extern DWORD g_PoolTaggingEnabled;

void UpdateValidFlags();
BOOL EnableDebug();

BOOL ReadGlobalFlagsFromRegistry( _Out_ ULONG* Flag );
BOOL WriteGlobalFlagsToRegistry( _In_ ULONG Flag );

BOOL ReadImageGlobalFlagsFromRegistry( _In_z_ PCWSTR ImageName, _Out_ ULONG* Flag );
BOOL WriteImageGlobalFlagsToRegistry( _In_z_ PCWSTR ImageName,_In_ ULONG Flag );

BOOL ReadGlobalFlagsFromKernel( _Out_ ULONG* Flag );
BOOL WriteGlobalFlagsToKernel( _In_ ULONG Flag );



void ParseCommandline(int argc, PCWSTR argv[]);
int ShowDialog();


// https://msdn.microsoft.com/en-us/library/windows/hardware/ff549557(v=vs.85).aspx


/*

Program-specific settings ("Image file") for all users of the computer.
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\ImageFileName\GlobalFlag

Silent exit settings for a specific program ("Silent Process Exit") for all users of the computer.
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\SilentProcessExit\ImageFileName

Page heap options for an image file for all users of the computer
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\ImageFileName\PageHeapFlags

User mode stack trace database size (tracedb)
HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\ImageFileName\StackTraceDatabaseSizeInMb

Create user mode stack trace database (ust, 0x1000) for an image file
Windows adds the image file name to the value of the USTEnabled registry entry (HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\USTEnabled).

Load image using large pages if possible
HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\ImageFileName\UseLargePages.

Special Pool
(Kernel Special Pool Tag)
HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management\PoolTag

Verify Start / Verify End
HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management\PoolTagOverruns. The Verify Start option sets the value to 0. The Verify End option sets the value to 1.

Debugger for an image file
HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\ImageFileName\Debugger

Object Reference Tracing
HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Kernel\ObTraceProcessName, ObTracePermanent and ObTracePoolTags

*/


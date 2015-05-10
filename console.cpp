#include <Windows.h>
#include <strsafe.h>
#include <stdio.h>
#include "gflags.h"


PCWSTR g_License =
L"Global flags editor\r\n"
L"\r\n"
L"Copyright (c) 2015 Mark Jansen\r\n"
L"\r\n"
L"Permission is hereby granted, free of charge, to any person obtaining a copy\r\n"
L"of this software and associated documentation files (the \"Software\"), to deal\r\n"
L"in the Software without restriction, including without limitation the rights\r\n"
L"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\r\n"
L"copies of the Software, and to permit persons to whom the Software is\r\n"
L"furnished to do so, subject to the following conditions:\r\n"
L"\r\n"
L"The above copyright notice and this permission notice shall be included in all\r\n"
L"copies or substantial portions of the Software.\r\n"
L"\r\n"
L"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\r\n"
L"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\r\n"
L"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\r\n"
L"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\r\n"
L"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\r\n"
L"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\r\n"
L"SOFTWARE.\r\n";

PCWSTR g_CommandlineUsage = L"\r\n"
L"usage: gflags [-i <ImageName> [<Flags>]]\r\n"
L"       gflags [-k [<Flags>]]\r\n"
L"       gflags [-r [<Flags>]]\r\n"
L"\r\n"
L"where: -i operates on flags for a specific image.\r\n"
L"          this mode also requires an image name to operate on.\r\n"
L"       -k operates on flags of the running system.\r\n"
L"       -r operates on flags in the system registry.\r\n"
L"\r\n"
L"       If only -r, -k or -i are specified, then the current\r\n"
L"       flags are displayed.\r\n"
L"       If no arguments are specified, gflags will show the UI.\r\n"
L"\r\n"
L"       Flags can either be a hex number, or a combination of the\r\n"
L"       abbreviations listed below. Prefix a number or an abbrev\r\n"
L"       with a + to set the bits, or with a - to remove the bits.\r\n"
L"       Valid abbreviations are:\r\n"
L"\r\n";

void PrintUsage(FILE* dst)
{
    fwprintf(dst, g_CommandlineUsage);
    for(size_t n = 0; n < g_FlagCount; ++n)
    {
        fwprintf(dst, L"       %s - %s\r\n", g_Flags[n].szAbbr, g_Flags[n].szDesc);
    }
}

void PrintFlags(FILE* dst, ULONG Flags, DWORD Dest)
{
    PWSTR Name = (Dest & DEST_KERNEL) ? L"Running Kernel" : L"";
    Name = (Dest & DEST_REGISTRY) ? L"Boot Registry" : Name;
    fwprintf(dst, L"Current %s Settings are: %08x\r\n", Name, Flags);
    for(size_t n = 0; n < g_FlagCount; ++n)
    {
        if(Flags & g_Flags[n].dwFlag)
        {
            fwprintf(dst, L"    %s - %s\r\n", g_Flags[n].szAbbr, g_Flags[n].szDesc);
        }
    }
}

void ShowLicense(FILE* dst)
{
    fwprintf(dst, g_License);
}

BOOL IsCommandlineOption(PCWSTR lpArgString, PCWSTR Option)
{
    return (lpArgString[0] == '/' || lpArgString[0] == '-') && !wcscmp(lpArgString + 1, Option);
}

void MaskFlags( DWORD ActiveDest, DWORD ActiveFlags, PDWORD ApplyFlags, PDWORD IgnoredFlags)
{
    DWORD Mask = 0;
    if(ActiveDest & DEST_IMAGE)
        Mask = g_ValidImageFlags;
    else if(ActiveDest & DEST_KERNEL)
        Mask = g_ValidKernelFlags;
    else
        Mask = g_ValidRegistryFlags;

    *ApplyFlags = ActiveFlags & Mask;
    *IgnoredFlags = ActiveFlags & ~Mask;
}

static DWORD g_ActiveDest = 0;
static DWORD g_ActiveFlags = 0;
static WCHAR g_ImageName[128] = {NULL};

static void ParseFlags(PCWSTR Arg)
{
    if(Arg[0] == L'+' || Arg[0] == L'-')
    {
        for(size_t n = 0; n < g_FlagCount; ++n)
        {
            if( !_wcsicmp(Arg+1, g_Flags[n].szAbbr) )
            {
                if(Arg[0] == '+')
                {
                    g_ActiveFlags |= g_Flags[n].dwFlag;
                }
                else
                {
                    g_ActiveFlags &= ~g_Flags[n].dwFlag;
                }
                return;
            }
        }
    }
    if(Arg[0] != '+' && Arg[0] != '-')
    {
        g_ActiveFlags = wcstoul(Arg, NULL, 16);
    }
    else if(Arg[0] == '+')
    {
        g_ActiveFlags |= wcstoul(Arg+1, NULL, 16);
    }
    else
    {
        g_ActiveFlags &= ~wcstoul(Arg+1, NULL, 16);
    }
}

void ParseCommandline(int argc, PCWSTR argv[])
{
    BOOL DisplayUsage = FALSE;
    BOOL DisplayFlags = TRUE;
    for(int n = 1; n < argc; ++n)
    {
        PCWSTR Arg = argv[n];
        BOOL IsImage = IsCommandlineOption(Arg,L"i");
        BOOL IsRegistry = !IsImage && IsCommandlineOption(Arg,L"r");
        if(IsImage || IsRegistry || IsCommandlineOption(Arg,L"k"))
        {
            if(g_ActiveDest)
            {
                fwprintf(stderr, L"gflags: Only one of the options -r -k can be specified\r\n", Arg);
                DisplayUsage = TRUE;
                break;
            }
            if (IsImage)
            {
                g_ActiveDest = DEST_IMAGE;
                if (n+1 < argc)
                {
                    StringCchCopy(g_ImageName, sizeof(g_ImageName) / sizeof(g_ImageName[0]), argv[n+1]);
                    ++n;
                }
                else
                {
                    DisplayUsage = TRUE;
                    break;
                }
                if(!ReadImageGlobalFlagsFromRegistry(g_ImageName, &g_ActiveFlags))
                {
                    fwprintf(stderr, L"gflags: Could not read image flags from registry\r\n");
                    exit(1);
                }
            }
            else if(IsRegistry)
            {
                g_ActiveDest = DEST_REGISTRY;
                if(!ReadGlobalFlagsFromRegistry(&g_ActiveFlags))
                {
                    fwprintf(stderr, L"gflags: Could not read global flags from registry\r\n");
                    exit(1);
                }
            }
            else
            {
                g_ActiveDest = DEST_KERNEL;
                if(!ReadGlobalFlagsFromKernel(&g_ActiveFlags))
                {
                    fwprintf(stderr, L"gflags: Could not read global flags from kernel\r\n");
                    exit(1);
                }
            }
        }
        else if(IsCommandlineOption(Arg,L"lic") || IsCommandlineOption(Arg,L"license"))
        {
            ShowLicense(stdout);
        }
        else if( g_ActiveDest )
        {
            ParseFlags(Arg);
        }
        else
        {
            fwprintf(stderr, L"gflags: Unexpected argument - '%s'\r\n", Arg);
            DisplayUsage = TRUE;
            break;
        }
    }

    if(DisplayUsage)
    {
        PrintUsage(stderr);
        exit(1);
    }
    else if(g_ActiveDest)
    {
        DWORD ApplyFlags = 0, IgnoredFlags = 0;
        MaskFlags(g_ActiveDest, g_ActiveFlags, &ApplyFlags, &IgnoredFlags);
        if (!DisplayFlags)
        {
            /* do set it. */
        }
        PrintFlags(stdout, g_ActiveFlags, g_ActiveDest);
        exit(0);
    }
}

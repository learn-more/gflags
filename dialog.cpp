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
#include <Prsht.h>
#include <Strsafe.h>
#include <stdio.h>
#include <assert.h>
#include "gflags.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

static ULONG g_KernelSettings = 0;
static ULONG g_RegistrySettings = 0;
static ULONG g_ImageSettings = 0;

static
void UpdateDialogFromFlags(HWND hDlg, ULONG Flags, DWORD Dest, BOOL Enable)
{
    for( size_t n = 0; n < g_FlagCount; ++n )
    {
        HWND Ctrl = GetDlgItem(hDlg, IDC_CHECK1 + n);
        ShowWindow(Ctrl, (g_Flags[n].wDest & Dest) ? SW_SHOWNORMAL : SW_HIDE);
        SendMessage(Ctrl, BM_SETCHECK, (g_Flags[n].dwFlag & Flags) ? 1 : 0, 0);
        EnableWindow(Ctrl, Enable);
    }
}

static
DWORD FlagsFromDialog(HWND hDlg)
{
    DWORD Flags = 0;
    for( size_t n = 0; n < g_FlagCount; ++n )
    {
        if(SendDlgItemMessage(hDlg, n + IDC_CHECK1, BM_GETCHECK, 0, 0) == BST_CHECKED)
        {
            Flags |= g_Flags[n].dwFlag;
        }
    }
    return Flags;
}

static
void HandleWMCommand(HWND hDlg, WPARAM wParam)
{
    if( LOWORD(wParam) >= IDC_CHECK1 && LOWORD(wParam) <= IDC_CHECK31 )
    {
        HWND hParent = GetParent(hDlg);
        SendMessage( hParent, PSM_CHANGED, (WPARAM)hDlg, 0 );   //TODO: PSM_UNCHANGED with correct flags!
    }
}

INT_PTR CALLBACK SystemRegistryProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
    case WM_INITDIALOG:
        if (!ReadGlobalFlagsFromRegistry(&g_RegistrySettings))
        {
            g_RegistrySettings = 0;
        }
        break;
    case WM_COMMAND:
        HandleWMCommand(hDlg, wParam);
        break;
    case WM_NOTIFY:
        switch(((LPNMHDR)lParam)->code)
        {
        case PSN_APPLY:
            g_RegistrySettings = FlagsFromDialog(hDlg) & g_ValidRegistryFlags;
            if(!WriteGlobalFlagsToRegistry(g_RegistrySettings))
            {
                MessageBoxW(hDlg, L"Unable to write flags to registry", L"gflags Error", MB_OK | MB_ICONERROR);
            }
            UpdateDialogFromFlags(hDlg, g_RegistrySettings, DEST_REGISTRY, 1);
            break;
        case PSN_SETACTIVE:
            UpdateDialogFromFlags(hDlg, g_RegistrySettings, DEST_REGISTRY, 1);
            break;
        }
        break;
    }
    return 0;
}

INT_PTR CALLBACK KernelRegistryProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
    case WM_INITDIALOG:
        if( !ReadGlobalFlagsFromKernel(&g_KernelSettings))
        {
            g_KernelSettings = 0;
        }
        break;
    case WM_COMMAND:
        HandleWMCommand(hDlg, wParam);
        break;
    case WM_NOTIFY:
        switch(((LPNMHDR)lParam)->code)
        {
        case PSN_APPLY:
            g_KernelSettings = FlagsFromDialog(hDlg) & g_ValidKernelFlags;
            if(!WriteGlobalFlagsToKernel(g_KernelSettings))
            {
                MessageBoxW(hDlg, L"Unable to write flags to kernel", L"gflags Error", MB_OK | MB_ICONERROR);
            }
            UpdateDialogFromFlags( hDlg, g_KernelSettings, DEST_KERNEL, 1);
            break;
        case PSN_SETACTIVE:
            UpdateDialogFromFlags( hDlg, g_KernelSettings, DEST_KERNEL, 1);
            break;
        }
        break;
    }
    return 0;
}

static
void UpdateImagePage(HWND hDlg)
{
    WCHAR Buffer[128] = {0};
    SendDlgItemMessageW(hDlg, IDC_EDIT_IMAGENAME, WM_GETTEXT, 128, (LPARAM)Buffer);
    ReadImageGlobalFlagsFromRegistry(Buffer, &g_ImageSettings);
    UpdateDialogFromFlags( hDlg, g_ImageSettings, DEST_IMAGE, Buffer[0] ? 1 : 0);
}

static
void StoreImageFlags(HWND hDlg)
{
    WCHAR Buffer[128] = {0};
    SendDlgItemMessageW(hDlg, IDC_EDIT_IMAGENAME, WM_GETTEXT, 128, (LPARAM)Buffer);
    g_ImageSettings = FlagsFromDialog(hDlg) & g_ValidImageFlags;
    if(!WriteImageGlobalFlagsToRegistry(Buffer, g_ImageSettings))
    {
        WCHAR ErrorBuffer[256];
        StringCchPrintfW(ErrorBuffer, 256, L"Unable to write image flags for %s", Buffer);
        MessageBoxW(hDlg, ErrorBuffer, L"gflags Error", MB_OK | MB_ICONERROR);
    }
}

INT_PTR CALLBACK ImageFileProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
    case WM_INITDIALOG:
        g_ImageSettings = 0;
        break;
    case WM_COMMAND:
        HandleWMCommand(hDlg, wParam);
        if(LOWORD(wParam) == IDC_EDIT_IMAGENAME && HIWORD(wParam) == EN_KILLFOCUS)
        {
            UpdateImagePage(hDlg);
        }
        break;
    case WM_NOTIFY:
        switch(((LPNMHDR)lParam)->code)
        {
        case PSN_APPLY:
            StoreImageFlags(hDlg);
            UpdateImagePage(hDlg);
            break;
        case PSN_SETACTIVE:
            UpdateImagePage(hDlg);
            break;
        }
        break;
    }
    return 0;
}

static
void HandleWMSize(HWND hDlg)
{
    RECT ClientRect ={ 0 }, WindowRect ={ 0 };
    GetClientRect(hDlg, &ClientRect);
    HWND TextBox = GetDlgItem(hDlg, IDC_README_TEXT);
    GetWindowRect(TextBox, &WindowRect);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&WindowRect, 2);
    WindowRect.right = ClientRect.right - WindowRect.left;
    WindowRect.bottom = ClientRect.bottom - WindowRect.left;
    SetWindowPos(TextBox, NULL, 0, 0,
        WindowRect.right-WindowRect.left, WindowRect.bottom - WindowRect.top,
        SWP_NOMOVE);
}


extern PCWSTR g_CommandlineUsage;
extern PCWSTR g_License;

static
void UpdateReadmePage(HWND hDlg)
{
    HWND TextBox = GetDlgItem(hDlg, IDC_README_TEXT);
    if( SendDlgItemMessage(hDlg, IDC_COMMANDLINE, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        size_t Length = wcslen(g_CommandlineUsage) + 1;
        for (size_t n = 0; n < g_FlagCount; ++n)
        {
            Length += wcslen(g_Flags[n].szAbbr);
            Length += wcslen(g_Flags[n].szDesc);
            Length += (7 + 3 + 2);  /* whitespace + ' - ' + newline */
        }

        WCHAR* Commandline = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Length * sizeof(WCHAR));
        StringCchCopy(Commandline, Length, g_CommandlineUsage);
        WCHAR Buffer[260];
        for (size_t n = 0; n < g_FlagCount; ++n)
        {
            StringCchPrintfW(Buffer, 260, L"       %s - %s\r\n", g_Flags[n].szAbbr, g_Flags[n].szDesc);
            StringCchCat(Commandline, Length, Buffer);
        }
        SetWindowText(TextBox, Commandline);
        HeapFree(GetProcessHeap(), 0, Commandline);
    }
    else if( SendDlgItemMessage(hDlg, IDC_LICENSE, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        SetWindowText(TextBox, g_License);
    }
    else
    {
        SetWindowText(TextBox, L"Well hello there, you hacker :)");
    }
}

INT_PTR CALLBACK ReadmeProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(hDlg, IDC_COMMANDLINE, BM_SETCHECK, 1, 0);
        break;
    case WM_SIZE:
        HandleWMSize(hDlg);
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_COMMANDLINE:
        case IDC_LICENSE:
            UpdateReadmePage(hDlg);
            SetFocus(GetDlgItem(hDlg, IDC_README_TEXT));
            break;
        default:
            break;
        }
        break;
    case WM_NOTIFY:
        switch(((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            UpdateReadmePage(hDlg);
            break;
        }
        break;
    }
    return 0;
}

int ShowDialog()
{
    PROPSHEETPAGE psp[4] = {0};
    PROPSHEETHEADER psh = {0};
    UINT numPages = sizeof(psp) / sizeof(psp[0]);
    UINT page = 0;

    for( UINT n = 0; n < numPages; ++n ) {
        psp[n].dwSize = sizeof(psp[0]);
        psp[n].dwFlags = PSP_USETITLE;
        psp[n].hInstance = GetModuleHandle(NULL);
    }

    psp[page].pszTemplate = MAKEINTRESOURCE(DLG_SYSTEM_REGISTRY);
    psp[page].pfnDlgProc = SystemRegistryProc;
    psp[page++].pszTitle = MAKEINTRESOURCE(IDS_SYSTEM_REGISTRY);

    psp[page].pszTemplate = MAKEINTRESOURCE(DLG_SYSTEM_REGISTRY);
    psp[page].pfnDlgProc = KernelRegistryProc;
    psp[page++].pszTitle = MAKEINTRESOURCE(IDS_KERNEL_FLAGS);

    psp[page].pszTemplate = MAKEINTRESOURCE(DLG_IMAGE_FILE);
    psp[page].pfnDlgProc = ImageFileProc;
    psp[page++].pszTitle = MAKEINTRESOURCE(IDS_IMAGE_FILE);
    
    psp[page].pszTemplate = MAKEINTRESOURCE(DLG_README);
    psp[page].pfnDlgProc = ReadmeProc;
    psp[page++].pszTitle = MAKEINTRESOURCE(IDS_README);

    assert(page <= numPages);

    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_USECALLBACK | PSH_PROPSHEETPAGE;
    psh.hInstance = GetModuleHandle(NULL);
    psh.pszCaption = L"Global Flags";
    psh.nPages = numPages;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = NULL;
    
    if( PropertySheet(&psh) == -1 )
    {
        return GetLastError();
    }
    return 0;
}


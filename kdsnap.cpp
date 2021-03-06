﻿/*++

 Copyright (c) 2017 CyberArk

Module Name:

    kdsnap.c

--*/

#include "kdsnap.h"

#include <ntverp.h>

//
// globals
//
EXT_API_VERSION         ApiVersion = { 1, 0, EXT_API_VERSION_NUMBER64, 0 };
WINDBG_EXTENSION_APIS   ExtensionApis;
ULONG SavedMajorVersion;
ULONG SavedMinorVersion;



int DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

	dprintf("[+]\t\t  _  __   _ ____                    \r\n");
	dprintf("[+]\t\t | |/ /__| / ___| _ __   __ _ _ __  \r\n");
	dprintf("[+]\t\t | ' // _\\` \\___ \\| '_ \\ / _\\` | '_ \\ \r\n");
	dprintf("[+]\t\t | . \\ (_| |___) | | | | (_| | |_) |\r\n");
	dprintf("[+]\t\t |_|\\_\\__,_|____/|_| |_|\\__,_| .__/ \r\n");
	dprintf("[+]\t\t\\                             |_|    \r\n");
	dprintf("[+]\t\t Usage: \r\n\r\n");
	dprintf(CONNECTVMSYNTAX);
	dprintf(SNAPSHOTSYNTAX);
	dprintf(REVERTSYNTAX);

    return;
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    //
    // ExtensionApiVersion should return EXT_API_VERSION_NUMBER64 in order for APIs
    // to recognize 64 bit addresses.  KDEXT_64BIT also has to be defined before including
    // wdbgexts.h to get 64 bit headers for WINDBG_EXTENSION_APIS
    //
    return &ApiVersion;
}

//
// Routine called by debugger after load
//
VOID
CheckVersion(
    VOID
    )
{
    return;
}


/*++
   Copyright (c) 2017 CyberArk

Module:

    Kdsnap.h


    Common header file for extensions

--*/

#include <windows.h>
#include <assert.h>

//
// Define KDEXT_64BIT to make all wdbgexts APIs recognize 64 bit addresses
// It is recommended for extensions to use 64 bit headers from wdbgexts so
// the extensions could support 64 bit targets.
//
#define KDEXT_64BIT
#include <wdbgexts.h>
#include <comutil.h>

//VBox
#include "vb_i.h"
#include "VirtualBox.h"


//
// VIX (VMWare) headers 
//
#include "vix.h"
#include "vm_basic_types.h"


//
// Vix (VMWare) connection settings 
//
#define USE_WORKSTATION
#define  CONNTYPE    VIX_SERVICEPROVIDER_VMWARE_WORKSTATION
#define  HOSTNAME ""
#define  HOSTPORT 0
#define  USERNAME ""
#define  PASSWORD ""
#define  VMPOWEROPTIONS   VIX_VMPOWEROP_LAUNCH_GUI


// Exts Usage
//
#define CONNECTVMSYNTAX "!virtualMachine <virtual machine's name/vmx full path>\r\n"
#define SNAPSHOTSYNTAX "!snapshot <snapshot_name> \t takes snapshot of the current debugee state\r\n"
#define REVERTSYNTAX "!revert <snapshot_name> \t reverts the debugee state to the last saved snapshot\r\n"
#define PLATFORMSYNTAX "!platform <vbox/vmware> Default is VMWare\r\n"

//
//Safe release Virtualbox objects
//
#define SAFE_RELEASE(x)  if (x) {   x->Release();  x = NULL;  }



enum platform {
	vmware,
	vbox
};
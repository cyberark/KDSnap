/*-----------------------------------------------------------------------------
   Copyright (c) 2017 CyberArk

Module:
  exts.c

------------------------------------------------------------------------------*/

#include "kdsnap.h"

//
// VIX globals
//
VixError err;
char *vmxPath;
VixHandle hostHandle = VIX_INVALID_HANDLE;
VixHandle jobHandle = VIX_INVALID_HANDLE;
VixHandle vmHandle = VIX_INVALID_HANDLE;
VixHandle snapshotHandle = VIX_INVALID_HANDLE;
VixPowerState vmPowerState;
int numSnapshots;

//
// VBOX globals
//
IVirtualBoxClient *virtualBoxClient;
IVirtualBox *virtualBox;
IMachine *machine, *m = NULL;
ISession *session = NULL;
IConsole *console = NULL;
IProgress *progress = NULL;
BSTR guid, machineName, snapID, snapshotName = NULL;
HRESULT rc;
char * errorMsg;



//
// platform define, default: vmware.
//

platform current_platform = vmware;

//
// VIX critical error handler
//
void abortVM() 
{
	Vix_ReleaseHandle(jobHandle);
	Vix_ReleaseHandle(vmHandle);
	Vix_ReleaseHandle(snapshotHandle);
	VixHost_Disconnect(hostHandle);
}

//
// Performs indicative status non-blocking wait
//
int nonBlockWaitVMWARE(char *msg)
{
	Bool jobStatus = FALSE;
	while (!jobStatus) {
		err = VixJob_CheckCompletion(jobHandle, &jobStatus);
		if (VIX_OK != err) {
			dprintf(msg);
			return 0;
		}
		Sleep(1500);
		dprintf(".");
	}

	return 1;
}

//
// Waits for an IProgress to finish, Prints progress
//
void nonBlockWaitVBOX(IProgress * progress)
{
	BOOL jobStatus = FALSE;
	while (jobStatus != TRUE) {
		dprintf(".");
		progress->WaitForCompletion(1500);
		progress->get_Completed(&jobStatus);
	}
	return;
}


//
// Converts char array to BSTR
//
BSTR charToBSTR(char * str) {
	BSTR bstr;
	int wslen = MultiByteToWideChar(CP_ACP, 0, str, strlen(str), 0, 0);
	bstr = SysAllocStringLen(0, wslen);
	MultiByteToWideChar(CP_ACP, 0, str, strlen(str), bstr, wslen);
	return bstr;
}

//
// Retrieves snapshot handle by name
//
int getSnapshotByName(char *name, VixHandle *out) {

	return VixVM_GetNamedSnapshot(vmHandle, name, out) == VIX_OK ? 1 : 0;
}

//
// Extension to connect the extension to VMWare Workstation
//
// Usage:		!connectVM <vmx-path>
//
DECLARE_API(virtualMachine) 
{

	if (current_platform == vbox) {
		goto VBOX;
	}

	if (strlen(args) > 0) {
		vmxPath = (char *)args;
	}
	else {
		dprintf("[-] The command syntax is:\t");
		dprintf(CONNECTVMSYNTAX);
		return;
	}


	jobHandle = VixHost_Connect(VIX_API_VERSION, CONNTYPE, HOSTNAME, HOSTPORT, USERNAME, PASSWORD, 0, VIX_INVALID_HANDLE, NULL, NULL);
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_JOB_RESULT_HANDLE, &hostHandle, VIX_PROPERTY_NONE);
	if (VIX_FAILED(err)) {
		dprintf("[-] Could not connect to the virtual machine!\n\n");
		abortVM();
		return;
	}

	Vix_ReleaseHandle(jobHandle);

	jobHandle = VixVM_Open(hostHandle, vmxPath, NULL, NULL);
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_JOB_RESULT_HANDLE, &vmHandle, VIX_PROPERTY_NONE);
	if (VIX_FAILED(err)) {
		dprintf("[-] Could not open the virtual machine!\n\n");
		abortVM();
		return;
	}

	Vix_ReleaseHandle(jobHandle);


	err = Vix_GetProperties(vmHandle, VIX_PROPERTY_VM_POWER_STATE, &vmPowerState, VIX_PROPERTY_NONE);
	if (VIX_OK != err) {
		dprintf("[-] Could not obtain the virtual machine state!\n\n");
		abortVM();
		return;
	}

	if (vmPowerState != VIX_POWERSTATE_POWERED_ON) {
		dprintf("[!] The virtual machine is not powered on! (%d) \n\n", vmPowerState);
		abortVM();
		return;
	}
	return;
VBOX:

	if (!(strlen(args) > 0)) {
		dprintf("[-] The command syntax is:\t");
		dprintf(CONNECTVMSYNTAX);
		return;
	}

	machineName = charToBSTR((char *)args);

	CoInitialize(NULL);
	rc = CoCreateInstance(CLSID_VirtualBoxClient, NULL, CLSCTX_INPROC_SERVER, IID_IVirtualBoxClient, (void**)&virtualBoxClient);
	if (!SUCCEEDED(rc)) {
		goto ErrorExit;
	}
	rc = virtualBoxClient->get_VirtualBox(&virtualBox);
	if (!SUCCEEDED(rc)) {
		goto ErrorExit;
	}
	rc = virtualBox->FindMachine(machineName, &machine);
	if (FAILED(rc)) {
		goto ErrorExit;
	}
	rc = machine->get_Id(&guid);
	if (!SUCCEEDED(rc)) {
		errorMsg = "[-] Could not connect to the virtual machine! Error code: 0x%x\n";
		goto ErrorExit;
	}
	rc = CoCreateInstance(CLSID_Session, NULL, CLSCTX_INPROC_SERVER, IID_ISession, (void**)&session);
	if (!SUCCEEDED(rc))
	{
		errorMsg = "[-] Could not open the virtual machine! Error code: 0x%x\n";
		goto ErrorExit;
	}
	return;

ErrorExit:
	dprintf(errorMsg, rc);
	return;

}

//
// Extension to take a snapshot of the current machine state
//  
//    !snapshot <snapshot_name>
//
DECLARE_API(snapshot) 
{
	if (current_platform == vbox) {
		goto VBOX;
	}

	char *snapshotNameVMW;
	VixHandle sstmp;
	
	if (strlen(args) > 0) {
		snapshotNameVMW = (char *)args;
	}
	else {
		dprintf("[-] The command syntax is:\t");
		dprintf(SNAPSHOTSYNTAX);
		return;
	}

	if (vmHandle == VIX_INVALID_HANDLE) {
		dprintf("[-] Please use !connectVM first!");
		return;
	}

	if (getSnapshotByName(snapshotNameVMW, &sstmp)) {
		dprintf("[-] Snapshot name already exists!\r\n");
		Vix_ReleaseHandle(sstmp);
		return;
	}

	jobHandle = VixVM_CreateSnapshot(vmHandle, snapshotNameVMW, "taken by kdsnap", VIX_SNAPSHOT_INCLUDE_MEMORY, VIX_INVALID_HANDLE, NULL, NULL);
	dprintf("[+] Snapshot has started\r\n");
	//err = VixJob_Wait(jobHandle, VIX_PROPERTY_JOB_RESULT_HANDLE, &snapshotHandle, VIX_PROPERTY_NONE);

	if (!nonBlockWaitVMWARE("[!] Could not take a snapshot!\n\n")) {
		abortVM();
		Vix_ReleaseHandle(sstmp);
		return;
	}

	Vix_ReleaseHandle(sstmp); //make this more elegant
	
	dprintf("\r\n[+] Snapshot progress completed\r\n");


	Vix_ReleaseHandle(jobHandle);
	return;

VBOX:

	if (!(strlen(args) > 0)) {
		dprintf("[-] The command syntax is:\t");
		dprintf(SNAPSHOTSYNTAX);
		return;
	}

	snapshotName = charToBSTR((char *)args);

	BSTR snapshotDesc = L"taken by kdsnap";
	if (machine == NULL) {
		errorMsg = "[-] Please use !connectVM first!\n";
		goto Exit;
	}
	rc = machine->LockMachine(session, LockType_Shared);
	if (!SUCCEEDED(rc))
	{
		errorMsg = "[!] Could not lock the machine! Error code: 0x%x\n";
		goto Exit;
	}
	session->get_Machine(&m);
	rc = m->TakeSnapshot(snapshotName, snapshotDesc, false, &snapID, &progress);
	if (!SUCCEEDED(rc))
	{
		errorMsg = "[!] Could not take a snapshot! Error code: 0x%x\n";
		goto Exit;
	}
	dprintf("[+] Taking a snapshot, please wait ...\n");
	nonBlockWaitVBOX(progress);
	dprintf("\r\n[+] Snapshot progress completed\r\n");

Exit:
	if (!SUCCEEDED(rc)) {
		dprintf(errorMsg, rc);
	}
	SysFreeString(snapshotName);
	return;
}

//
// Extension to take revert a snapshot 
//  
//    !revert <snapshot_name>
//
DECLARE_API(revert) 
{

	if (current_platform == vbox) {
		goto VBOX;
	}

	char *snapshotNameVMW;
	VixHandle sstmp;

	if (strlen(args) > 0) {
		snapshotNameVMW = (char *)args;
	}
	else {
		dprintf("[-] The command syntax is:\t");
		dprintf(REVERTSYNTAX);
		return;
	}

	if (vmHandle == VIX_INVALID_HANDLE) {
		dprintf("[-] Please use !connectVM first!");
		return;
	}

	if (!getSnapshotByName(snapshotNameVMW, &sstmp)) {
		dprintf("[-] Incorrect snapshot name!\r\n");
		return;
	}

	if (sstmp == VIX_INVALID_HANDLE) {
		dprintf("[!] Error while trying to revert!");
		Vix_ReleaseHandle(sstmp);
		return;
	}

	jobHandle = VixVM_RevertToSnapshot(vmHandle, sstmp, VMPOWEROPTIONS, VIX_INVALID_HANDLE, NULL, NULL);
	//err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	if (!nonBlockWaitVMWARE("[!] Could not revert to the snapshot!\n\n")) {
		abortVM();
		Vix_ReleaseHandle(sstmp);
		return;
	}

	dprintf("\r\n[+] Revert progress completed\r\n");

	Vix_ReleaseHandle(sstmp); // make this more elegant

	return;

VBOX:

	ISnapshot *s = NULL;
	MachineState mState;
	BSTR sessiontype = L"gui";

	if (strlen(args)) {
		dprintf("[-] The command syntax is:\t");
		dprintf(REVERTSYNTAX);
		return;
	}

	snapshotName = charToBSTR((char *)args);
	if (machine == NULL) {
		errorMsg = "[-] Please use !connectVM first!\n";
		goto Exit;
	}
	session->get_Machine(&m);
	rc = machine->FindSnapshot(snapshotName, &s);
	if (!SUCCEEDED(rc)) {
		errorMsg = "[-] Incorrect snapshot name!\r\n";
		goto Exit;
	}
	session->get_Console(&console);

	machine->get_State(&mState);
	if (mState == MachineState_Running) {
		rc = console->PowerDown(&progress);
		nonBlockWaitVBOX(progress);
	}
	rc = CoCreateInstance(CLSID_Session, NULL, CLSCTX_INPROC_SERVER, IID_ISession, (void**)&session);
	rc = machine->LockMachine(session, LockType_Shared);
	if (!SUCCEEDED(rc)) {
		errorMsg = "[!] Could not lock the machine!  Error code: 0x%x\n";
		goto Exit;
	}
	session->get_Machine(&m);
	rc = m->RestoreSnapshot(s, &progress);
	if (!SUCCEEDED(rc)) {
		errorMsg = "[!] Could not revert to the snapshot! Error code: 0x%x\n";
		goto Exit;
	}

	dprintf("[-] Restoring the snapshot, please wait ...\n");
	nonBlockWaitVBOX(progress);
	rc = session->UnlockMachine();
	Sleep(500);
	rc = machine->LaunchVMProcess(session, sessiontype, NULL, &progress);
	if (!SUCCEEDED(rc))
	{
		errorMsg = "[!] Could not power on the machine after restoring the snapshot!  Error code: 0x%x\n";
		goto Exit;
	}
	nonBlockWaitVBOX(progress);
	session->get_Console(&console);
	machine->ShowConsoleWindow(0);
	dprintf("\r\n[+] Revert progress completed\r\n");

Exit:
	if (!SUCCEEDED(rc)) {
		dprintf(errorMsg, rc);
	}
	SysFreeString(snapshotName);
	return;
}


//
// Extension to take revert a snapshot 
//  
//    !platform <vbox/vmware>
//
DECLARE_API(platform)
{
	char *plt;

	if (strlen(args) > 0) {
		plt = (char *)args;
	}
	else {
		goto SyntaxError;
	}

	if (strcmp(plt, "vbox")) {
		current_platform = vbox;
		dprintf("[+] Current platform is now set to VirtualBox\r\n");
	}
	else if (strcmp(plt, "vmware")) {
		current_platform = vmware;
		dprintf("[+] Current platform is now set to VMWare\r\n");
	}
	else { 
		goto SyntaxError;
	}

	return;

SyntaxError:
	dprintf("[-] The command syntax is:\t");
	dprintf(PLATFORMSYNTAX);
	return;
}
# KDSnap

KDSnap is a DLL extension for WinDbg that integrates your debugger with your virtualization platform of choice. You can read more on the related [article](https://www.cyberark.com/threat-research-blog/kdsnap) on our blog.

## Overview
 _KDSnap_ allows you to connect to your debugged VM, and save or restore its state, using commands from within the debugger itself. _KDSnap_ Currently supports **VMware Workstation** and Oracle’s **VirtualBox**.

## Usage:


Change the virtualization platform (default is VMware).
```
kd> !platform<VMware / VBox> 
```
Connecting to the virtual machine is done using the *.vmx* file full path when using **VMware** or using the machine's fullname when using **VirtualBox**.
```
kd> !virtualMachine <VMX Full Path / VM name>
```
Taking a snapshot is done simply by the snapshot coammnd.
```
kd> !snapshot <snapshot_name>
```
Reverting to presvious snapshot is done in the same way.
```
kd> !revert <snapshot_name>
```

###### * _To use VBox you must run the debbuger as admin._ ######


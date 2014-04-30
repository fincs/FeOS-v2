# FeOS Kernel Architecture Overview

## Overview

The FeOS Kernel Architecture: Based on a modular monolithic/hybrid architecture.
The kernel implements all necessary functionality so that a Userland driver can be programmed and
used, which will in turn implement the userland portion of the operating system.

- Bootstrapping code - sets up MMU and the like.
- Core CPU code - CPU exception vectors & others.
- Memory management:
  - Physical memory manager (PMM) - deals with ph. mem. page allocation and the like.
  - Virtual memory manager (VMM) - deals with mapping and unmapping of pages in the address space.
  - Virtual address space manager (VASM) - deals with dynamic allocation of virtual addresses.
  - Kernel heap manager (KHM): implements malloc/free for use by kernel-mode code (dlmalloc+sbrk).
- CPU management:
  - Process/Thread manager (PTM): implements preemptive multitasking and (hopefully) SMP/multicore support.
  - Interrupt manager (IM): implements things related to ISRs for use by driver code.
  - Communication manager (CM): implements inter-process communication (IPC).
- Resource management:
  - Device manager (DM): implements the device table and interface object model.
  - Filesystem manager (FSM): implements filesystem mounting and interface.
- Loadable kernel module manager (LKMM): implements loadable kernel modules.
- Linked-in system-specific device and filesystem drivers: required for booting and basic operation:
  namely video hardware, basic input, FAT filesystem and SD card/etc access (but theoretically they
  can be external loadable modules, only that doing that would be a Catch-22)

Notably missing features (for now at least):
- Paging to disk - may be really (REALLY) tricky to implement.
- File mapping to memory - still tricky to implement due to usage of CPU exceptions and the like. Could
  be implemented by the userland driver somehow, as it is the one who takes care of user-mode CPU exceptions.
- Unlimited number of processes (hardlimit of 256 due to ASIDs being 8-bit, however that's more than
  enough IMO, on my Windows 8 system I currently have 70 processes in total, just little over a quarter
  of FeOS' hardlimit).
- Internally recursive interrupt subroutines - currently ISRs will keep interrupts disabled and no recursion
  will be possible.

## Boot sequence:

- Startup code runs, including basic MMU setup
- Memory management initialization, finishes MMU setup and initializes internal bookkeeping
- CPU management initialization: processes and the like
- Resource management initialization: device and filesystem managers
- Loadable kernel module manager initialization
- Linked-in drivers are initialized (their initialization functions are executed, and they will register
  themselves through the DM or FSM)
- Kernel parses configuration file containing list of drivers to load from filesystem, which includes:
- Loading of userland driver, which implements the OS userland. In turn it performs:
- Creation of initial user process. This process has the critical flag set and it cannot be terminated.
  This process then sets up all necessary things so that the OS shell can be loaded.
- OS shell is loaded, thus finishing the boot sequence.

## Userland Drivers

Native userland driver: default userland for FeOS applications. Implements the syscall table,
  FXE3 module loading and their address space mapping, and the standard exception handling model.
  Full access to the capabilities of the OS, and full speed with no emulation involved. Assisted
  by the FeOS C Runtime Library (feoscrt) and other system libraries.

FeOS/DS userland driver: used for running legacy FeOS/DS software. Emulates necessary DS hardware
  register accesses and loads a heavily modified self-contained version of FeOS/DS which implements
  things such as FXE2 module loading and its cooperative multitasking. This userland is limited to one
  single process, and the userland driver exposes control mechanisms through the DM (accessible by native
  userland applications), including but not limited to rendered video output, "remote" execution of
  FeOS/DS software, "remote" stdin/stdout/stderr or ARM7 FIFO message thunking (for emulation of
  loadable ARM7 modules by other processes).

Other userland drivers: ARM Linux userland may be able to be supported through the dynamic userland model.

## Summary

The FeOS kernel is just a nice toolbox to make possible the usage of the userland of your choice.
A complete FeOS system ships with the native userland driver, but in theory it is possible to design
your entirely own userland, or exclusively run software designed for "foreign" userlands. But really, to
the kernel's eyes there are no foreign userlands. It is all dynamic, modular and beautiful; yet it mostly
maintains the full efficency of a traditional monolithic approach.

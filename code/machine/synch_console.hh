/// Data structures to export a synchronous interface to the raw cobnsole device.
///
/// Copyright (c) 2019-2019 Erlanguys
///
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_MACHINE_SYNCHCONSOLE__HH
#define NACHOS_MACHINE_SYNCHCONSOLE__HH


#include "console.hh"
#include "threads/system.hh"
#include "threads/synch.hh"


/// The following class defines a "synchronous" disk abstraction.
///
/// As with other I/O devices, the raw physical disk is an asynchronous
/// device -- requests to read or write portions of the disk return
/// immediately, and an interrupt occurs later to signal that the operation
/// completed.  (Also, the physical characteristics of the disk device assume
/// that only one operation can be requested at a time).
///
/// This class provides the abstraction that for any individual thread making
/// a request, it waits around until the operation finishes before returning.
class SynchConsole {
public:

    /// Initialize a synchronous disk, by initializing the raw Disk.
    SynchConsole();

    /// De-allocate the synch disk data.
    ~SynchConsole();

private:
    static Console   *console;
    static Semaphore *readAvail;
    static Semaphore *writeDone;

    static void ReadAvail(void *arg);
    static void WriteDone(void *arg);
};


#endif

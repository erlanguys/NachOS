/// Data structures to export a synchronous interface to the raw cobnsole device.
///
/// Copyright (c) 2019-2019 Erlanguys
///
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_MACHINE_SYNCHCONSOLE__HH
#define NACHOS_MACHINE_SYNCHCONSOLE__HH


#include "machine/console.hh"
// #include "threads/system.hh"
#include "threads/synch.hh"


class SynchConsole {
public:

    /// Initialize a synchronous disk, by initializing the raw Console.
    SynchConsole();

    /// De-allocate the synch console data.
    ~SynchConsole();

    void PutChar(char ch);
    char GetChar();

private:
    Console   *console;
    Semaphore *readAvail;
    Semaphore *writeDone;

    static void ReadAvail(void *arg);
    static void WriteDone(void *arg);
};


#endif

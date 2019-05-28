///
///
/// Copyright (c) 2019-2019 Erlanguys
///
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "synch_console.hh"


SynchConsole::SynchConsole()
{
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    console   = new Console("input.in", "output.out", ReadAvail, WriteDone, this);
}

/// De-allocate data structures needed for the synchronous console abstraction.
SynchDisk::~SynchDisk()
{
    delete readAvail;
    delete writeDone;
    delete console;
}

static void
SynchConsole::ReadAvail(void *arg)
{
    ((SynchConsole* ) arg)->readAvail->V();
}

static void
SynchConsole::WriteDone(void *arg)
{
    ((SynchConsole* ) arg)->writeDone->V();
}

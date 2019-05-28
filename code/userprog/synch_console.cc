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
    lock      = new Lock("lock cons");
}

/// De-allocate data structures needed for the synchronous console abstraction.
SynchConsole::~SynchConsole()
{
    delete readAvail;
    delete writeDone;
    delete console;
    delete lock;
}

void
SynchConsole::PutChar(char c)
{
    lock->Acquire();  // only one disk I/O at a time
    console->PutChar(c);
    writeDone->P();   // wait for interrupt
    lock->Release();
}

char
SynchConsole::GetChar()
{
    lock->Acquire();  // Only one disk I/O at a time.
    auto c = console->GetChar();
    readAvail->P();   // Wait for interrupt. WHY HERE CONHO TODO
    lock->Release();

    return c;
}

void
SynchConsole::ReadAvail(void *arg)
{
    ((SynchConsole* ) arg)->readAvail->V();
}

void
SynchConsole::WriteDone(void *arg)
{
    ((SynchConsole* ) arg)->writeDone->V();
}

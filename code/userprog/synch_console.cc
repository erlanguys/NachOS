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
    console   = new Console(nullptr, nullptr, ReadAvail, WriteDone, this);
}

/// De-allocate data structures needed for the synchronous console abstraction.
SynchConsole::~SynchConsole()
{
    delete readAvail;
    delete writeDone;
    delete console;
}

void
SynchConsole::PutChar(char c)
{
    console->PutChar(c);
    writeDone->P();   // wait for interrupt
}

char
SynchConsole::GetChar()
{
    auto c = console->GetChar();
    readAvail->P();   // Wait for interrupt.

    return c;
}

void
SynchConsole::ReadAvail(void *arg)
{
    ((SynchConsole *) arg)->readAvail->V();
}

void
SynchConsole::WriteDone(void *arg)
{
    ((SynchConsole *) arg)->writeDone->V();
}

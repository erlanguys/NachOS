/// Data structures to keep track of executing user programs (address
/// spaces).
///
/// For now, we do not keep any information about address spaces.  The user
/// level CPU state is saved and restored in the thread executing the user
/// program (see `thread.hh`).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_USERPROG_ADDRESSSPACE__HH
#define NACHOS_USERPROG_ADDRESSSPACE__HH


#include "syscall.h"
#include "filesys/file_system.hh"
#include "machine/translation_entry.hh"
#include "bin/noff.h"


const unsigned USER_STACK_SIZE = 1024;  ///< Increase this as necessary!


class AddressSpace {
public:

    /// Create an address space, initializing it with the program stored in
    /// the file `executable`.
    ///
    /// * `executable` is the open file that corresponds to the program.
    AddressSpace(OpenFile *executable, SpaceId pid);

    /// De-allocate an address space.
    ~AddressSpace();

    /// Get page table
    TranslationEntry *GetPageTable() const;

    /// Initialize user-level CPU registers, before jumping to user code.
    void InitRegisters();

    /// Save/restore address space-specific info on a context switch.

    void SaveState();
    void RestoreState();

    /// LoadPage (from executable)
    unsigned LoadPage(int);

    /// Number of pages in the virtual address space.
    unsigned numPages;

private:

    /// Assume linear page table translation for now!
    TranslationEntry *pageTable;

    // Executable header
    noffHeader exec_header;

    // Executable
    OpenFile *executable;

    // Process id
    SpaceId pid;
};


#endif

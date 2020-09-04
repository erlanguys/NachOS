/// Entry points into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2019 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"
#include "threads/system.hh"
#include "userprog/args.hh"


static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

/// Do some default behavior for an unexpected exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et)
{
    int exceptionArg = machine->ReadRegister(2);
    int badAddr = machine->ReadRegister(BAD_VADDR_REG);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d. Bad address: %u\n",
            ExceptionTypeToString(et), exceptionArg, badAddr);
    ASSERT(false);
}

/// Given the virtual address of a string that represents a filename
/// it saves that string to filename[].
///
/// * `filenameAddr` virtual address where the filename string is located.
/// * `filename` string where the filename should be loaded, it must have a
/// size of at least FILE_NAME_MAX_LEN + 1.
///
/// It returns 0 if the string was read successfully.
static int
readFilenameFromUser(int filenameAddr, char filename[])
{
    if (filenameAddr == 0) {
        DEBUG('c', "Error: address to filename string is null.\n");
        return 1;
    }

    if (!ReadStringFromUser(filenameAddr, filename, FILE_NAME_MAX_LEN)) {
        DEBUG('c', "Error: filename string too long (maximum is %u bytes).\n",
                FILE_NAME_MAX_LEN);
        return 1;
    }

    return 0;
}

/// Handle a system call exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
///
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!)
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2);

    switch (scid) {

        case SC_HALT:
            DEBUG('c', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;

        case SC_CREATE: {
            DEBUG('c', "Syscall Create\n");
            int filenameAddr = machine->ReadRegister(4);
            char filename[FILE_NAME_MAX_LEN + 1]{};
            if (!readFilenameFromUser(filenameAddr, filename)) {
                DEBUG('c', "Creation requested for file `%s`.\n", filename);
                fileSystem->Create(filename, 0);
            }
            break;
        }

        case SC_READ: {
            int storeAddr = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fid = machine->ReadRegister(6);
            machine->WriteRegister(2, 0); // The return value is zero until proven otherwise

            if (storeAddr == 0) {
                DEBUG('c', "Error: storeAddr is null.\n");
                break;
            }

            if (size < 0) {
                DEBUG('c', "Error: size must be non-negative.\n");
                break;
            }

            if (size > MAX_READ_SIZE) {
                DEBUG('c', "Error: size should be reasonable.\n");
                break;
            }

            char systemBuffer[size + 1]{};

            if (fid == CONSOLE_INPUT){
                // Juani: is this ok?
                // Rom: Yes, I've fixed it
                int it;
                for(it = 0; it < size; it++)
                    systemBuffer[it] = globalConsole->GetChar(); // read BYTES, not chars

                int bytesRead = it;
                WriteBufferToUser(systemBuffer, size, storeAddr);
                machine->WriteRegister(2, bytesRead);
                break;
            }

            if (fid == CONSOLE_OUTPUT){
                DEBUG('c', "Error: cannot read from CONSOLE_OUTPUT.\n");
                break;
            }

            OpenFile *of = currentThread->GetOpenFile(fid);

            if (of == nullptr) {
                DEBUG('c', "Error: the file descriptor is not associated to any file.\n");
                break;
            }

            DEBUG('c', "Reading file\n");

            int bytesRead = of->Read(systemBuffer, size);
            // IF COULDNT READ ANYTHING THEN???
            if( bytesRead > 0 )
                WriteBufferToUser(systemBuffer, bytesRead, storeAddr);
            machine->WriteRegister(2, bytesRead);
            break;
        }

        case SC_WRITE: {
            int address = machine->ReadRegister(4);
            int size    = machine->ReadRegister(5);
            int fid     = machine->ReadRegister(6);

            if (size < 0) {
                DEBUG('c', "Error: size must be non-negative.\n");
                break;
            }

            if (size > MAX_WRITE_SIZE) {
                DEBUG('c', "Error: size should be reasonable.\n");
                break;
            }

            if (address == 0) {
                DEBUG('c', "Error: address is null.\n");
                break;
            }

            char systemBuffer[size+1]{};
            ReadBufferFromUser(address, systemBuffer, size);

            /// chequear si consola wea
            if( fid == CONSOLE_OUTPUT ) {
                // escribir en la consola
                for(int i = 0; i < size; ++i)
                    globalConsole->PutChar(systemBuffer[i]);
                break;
            }

            if( fid == CONSOLE_INPUT ) {
                DEBUG('c', "Error: cannot WRITE to CONSOLE_INPUT.\n");
                break;
            }

            OpenFile *of = currentThread->GetOpenFile(fid);

            if (of == nullptr) {
                DEBUG('c', "Error: the file descriptor is not associated to any file.\n");
                break;
            }

            DEBUG('c', "Writing file\n");

            of->Write(systemBuffer, size);
            break;
        }

        case SC_OPEN: {
            int filenameAddr = machine->ReadRegister(4);
            char filename[FILE_NAME_MAX_LEN + 1];
            if (!readFilenameFromUser(filenameAddr, filename)) {
                DEBUG('c', "Open requested for file `%s`.\n", filename);
                OpenFile *of = fileSystem->Open(filename);
                if (of == nullptr) {
                    DEBUG('c', "Error: no file found with that name.\n");
                    machine->WriteRegister(2, -1); // Returns -1 indicating a an file descriptor.
                    break;
                }
                int fid = currentThread->AddFileDescriptor(of);
                machine->WriteRegister(2, fid); // Returns the successfully read file descriptor.
            }
            break;
        }

        case SC_CLOSE: {
            int fid = machine->ReadRegister(4);
            if (0 > fid || fid >= NUM_FILE_DESCRIPTORS) {
                DEBUG('c', "Invalid file descriptor to close.\n");
                break;
            }
            OpenFile *of = currentThread->GetOpenFile(fid);
            if (of == nullptr) {
                DEBUG('c', "The file descriptor to close is already closed.\n");
                break;
            }
            DEBUG('c', "Closing file descriptor id %u.\n", fid);
            delete of;
            currentThread->RemoveFileDescriptor(fid);
            break;
        }

        case SC_EXIT: {
            int exit_status = machine->ReadRegister(4);
            currentThread->Finish(exit_status);
            break;
        }

        case SC_JOIN: {
            SpaceId pid = machine->ReadRegister(4);

            if ( !threadPool->HasKey(pid) ) {
                DEBUG('c', "Invalid PID\n");
                break;
            }
            auto *t = threadPool->Get(pid);

            int exit_status = t->Join();
            machine->WriteRegister(2, exit_status);
            break;
        }

        case SC_EXEC: {

            int filenameAddr = machine->ReadRegister(4);
            char **argv = SaveArgs(machine->ReadRegister(5));
            char filename[FILE_NAME_MAX_LEN + 1]{};
            if (readFilenameFromUser(filenameAddr, filename)) {
                DEBUG('c', "Failed reading the file name.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            OpenFile *executable = fileSystem->Open(filename);

            if (executable == nullptr) {
                DEBUG('c', "Unable to open file %s\n", filename);
                machine->WriteRegister(2, -1);
                break;
            }

            DEBUG('c', "Running EXEC of file %s!\n", filename);

            Thread *newThread = new Thread("<executed-thread>", true, currentThread->GetPriority());
            AddressSpace *space = new AddressSpace(executable, newThread->GetPID());
            newThread->space = space;

            auto fun = [](void *args){
                DEBUG('c', "Forking\n");
                AddressSpace *s = currentThread->space;
                s->InitRegisters();  // Set the initial register values.
                s->RestoreState();   // Load page table register.

                if (args) {
                    machine->WriteRegister(4, WriteArgs(static_cast<char**>(args)));
                    machine->WriteRegister(5, machine->ReadRegister(STACK_REG) + 16);
                }

                machine->Run();  // Jump to the user progam.
            };

            DEBUG('c', "About to run a fork!\n");

            newThread->Fork(fun, argv);
            machine->WriteRegister(2, newThread->GetPID());
            break;
        }
#ifdef FILESYS
        case SC_LS: {
            fileSystem->List();
            break;
        }

        case SC_CD: {
            DEBUG('q', "SYSCALL CD\n");
            int filenameAddr = machine->ReadRegister(4);
            char filename[FILE_NAME_MAX_LEN + 1];

            if (!filenameAddr)
                DEBUG('y', "Error: address to filename string is null.\n");
            else if (!ReadStringFromUser(filenameAddr, filename, sizeof filename))
                DEBUG('y', "Error: filename string too long (maximum is %u bytes).\n", FILE_NAME_MAX_LEN);
            else if (!fileSystem->CD(filename))
                DEBUG('y', "Error: can't change to directory %s.\n", filename);
            else DEBUG('y', "Changed to directory %s.\n", filename);

            break;
        }
#endif

        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);

    }

    IncrementPC();
}


static void
PageFaultHandler(ExceptionType _et)
{
    /// TODO: EMBELISH Y AGREGAR COMENTARIOS
    unsigned vAddr = machine->ReadRegister(BAD_VADDR_REG);
    unsigned vPage = vAddr / PAGE_SIZE;

    auto *space = currentThread->space;

    TranslationEntry *pageTable = space->GetPageTable();
    TranslationEntry *tlb = machine->GetMMU()->tlb;

    ASSERT(vPage >= 0);
    ASSERT(vPage < space->numPages);

    // DEMAND LOADING
    if( not pageTable[vPage].valid ){
        space->LoadPage(vPage);
    } else if( not pageTable[vPage].inMemory ){
        space->LoadPageFromSwap(vPage);
        // pageTable[vPage].inMemory = true;
    }

    // PAGE REPLACEMENT STRATEGY
    static int refreshedIndex = 0;
    tlb[refreshedIndex] = pageTable[vPage];
    refreshedIndex = (refreshedIndex + 1) % TLB_SIZE;
}

/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &PageFaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}

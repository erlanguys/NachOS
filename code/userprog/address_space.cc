/// Routines to manage address spaces (executing user programs).
///
/// In order to run a user program, you must:
///
/// 1. Link with the `-N -T 0` option.
/// 2. Run `coff2noff` to convert the object file to Nachos format (Nachos
///    object code format is essentially just a simpler version of the UNIX
///    executable object code format).
/// 3. Load the NOFF file into the Nachos file system (if you have not
///    implemented the file system yet, you do not need to do this last
///    step).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "bin/noff.h"
#include "machine/endianness.hh"
#include "threads/system.hh"
#include <algorithm>
#include <cstring>
#include <string>

int Translate(int virtAddr, TranslationEntry t[]) {
  int page = virtAddr / PAGE_SIZE;
  int offset = virtAddr % PAGE_SIZE;
  int frame = t[page].physicalPage;
  return PAGE_SIZE * frame + offset;
}

/// Do little endian to big endian conversion on the bytes in the object file
/// header, in case the file was generated on a little endian machine, and we
/// are re now running on a big endian machine.
static void
SwapHeader(noffHeader *noffH)
{
    ASSERT(noffH != nullptr);

    noffH->noffMagic              = WordToHost(noffH->noffMagic);
    noffH->code.size              = WordToHost(noffH->code.size);
    noffH->code.virtualAddr       = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr        = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size          = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr   = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr    = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size        = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr =
      WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr  = WordToHost(noffH->uninitData.inFileAddr);
}

/// Create an address space to run a user program.
///
/// Load the program from a file `executable`, and set everything up so that
/// we can start executing user instructions.
///
/// Assumes that the object code file is in NOFF format.
///
/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
///
/// * `executable` is the file containing the object code to load into
///   memory.
AddressSpace::AddressSpace(OpenFile *_executable, SpaceId _pid)
{
    ASSERT(_executable != nullptr);
    
    executable = _executable;
    pid = _pid;

    executable->ReadAt((char *) &exec_header, sizeof exec_header, 0);
    if (exec_header.noffMagic != NOFF_MAGIC &&
          WordToHost(exec_header.noffMagic) == NOFF_MAGIC)
        SwapHeader(&exec_header);
    ASSERT(exec_header.noffMagic == NOFF_MAGIC);

    // How big is address space?
    unsigned size = exec_header.code.size + exec_header.initData.size
                    + exec_header.uninitData.size + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    // First, set up the translation.

    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
        pageTable[i].physicalPage = -1;
        pageTable[i].valid        = false;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
        pageTable[i].inMemory     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }

    std::string swapFileName = "swap." + std::to_string(pid);
    if (fileSystem->Create(swapFileName.c_str(), 0)) {
      swapFile = fileSystem->Open(swapFileName.c_str());
    } else {
      const int CANT_CREATE_SWAP_FILE = 1;
      ASSERT(CANT_CREATE_SWAP_FILE == 0);
    }
}

/// Maps virtual page to physical frame and initializes it
/// with correspoding code/data segments. 
void
AddressSpace::LoadPage(unsigned vpn){
  unsigned pfn = coreMap.ReserveNextAvailableFrame(vpn, pid);
  unsigned frameAddress = pfn * PAGE_SIZE;

  ///TODO: SWAP

  auto *RAM = machine->GetMMU()->mainMemory;
  
  unsigned pageStart = vpn * PAGE_SIZE;
  unsigned pageEnd = pageStart + PAGE_SIZE;

  // clear page - a beloved feature !
  std::memset(RAM + frameAddress, 0, PAGE_SIZE);

  // check for intersection with code segment
  unsigned codeStart = exec_header.code.virtualAddr;
  unsigned codeEnd = codeStart + exec_header.code.size;

  if( not( pageStart >= codeEnd or pageEnd <= codeStart)){
    int from = std::max(pageStart, codeStart);
    int until = std::min(codeEnd, pageEnd);
    int position = from - codeStart + exec_header.code.inFileAddr;
    executable->ReadAt(RAM + frameAddress + from - pageStart, until - from, position);
  }
  //  check for intersection with initialized data segment
  unsigned dataStart = exec_header.initData.virtualAddr;
  unsigned dataEnd = dataStart + exec_header.initData.size;
  if( not( pageStart >= dataEnd or pageEnd <= dataStart)){
    int from = std::max(pageStart, dataStart);
    int until = std::min(dataEnd, pageEnd);
    int position = from - exec_header.initData.virtualAddr + exec_header.initData.inFileAddr;
    executable->ReadAt(RAM + frameAddress + from - pageStart, until - from, position);
  }

  /// Update pageTable entry 
  pageTable[vpn] = {
    vpn,
    pfn,
    true, // valid
    false, // readOnly
    false, // use
    true, // dirty
    true // inMemory
  };
}

void
AddressSpace::LoadPageFromSwap(unsigned vpn)
{
  unsigned pfn = coreMap.ReserveNextAvailableFrame(vpn, pid);
  auto *RAM = machine->GetMMU()->mainMemory;
  int memoryOffset = pfn * PAGE_SIZE;
  int fileOffset = vpn * PAGE_SIZE;
  DEBUG('u', "Getting from SWAP (pid: %d, vpn: %u, swapFileSize: %u)\n", pid, vpn, swapFile->Length());
  swapFile->ReadAt(RAM + memoryOffset, PAGE_SIZE, fileOffset);
  printf("Read page:  <<<");
  for (unsigned i = 0; 4*i < PAGE_SIZE; ++i) {
    printf("%d", RAM[memoryOffset + 4*i]);
  }
  puts(">>>");
  pageTable[vpn].use = false;
  pageTable[vpn].dirty = false;
  pageTable[vpn].inMemory = true;
  pageTable[vpn].physicalPage = pfn;
  ASSERT(pageTable[vpn].virtualPage == vpn);
  ASSERT(pageTable[vpn].valid == true);
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    coreMap.FreeProcessFrames(pid);
    delete [] pageTable;
    delete swapFile;
    // AddressSpace has now owbnership of OpenFile
    delete executable;
}

TranslationEntry *
AddressSpace::GetPageTable() const
{
    return pageTable;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{

    /*
    As TLB is dependant on AddressSpace, a context switch invalides it
    */
    for (unsigned i = 0; i < TLB_SIZE; i++)
      machine->GetMMU()->tlb[i].valid = false;

}


OpenFile *
AddressSpace::GetSwapFile() const {
    return swapFile;
}
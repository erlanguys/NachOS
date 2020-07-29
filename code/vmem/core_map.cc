#include "core_map.hh"
#include "system.hh"

/// Finds the best physical frame to be evicted to secondary storage according to
/// improved second chance algorithm.
unsigned
CoreMap::GetFrameToSwap()
{
    DEBUG('k', "GetFrameToSwap\n");
    while(true) {
        DEBUG('k', "\tEvaluating victim %d : [%d %d]\n", nextVictim, core[nextVictim].accessed, core[nextVictim].modified);
        if (core[nextVictim].modified) {
            core[nextVictim].modified = false;
            nextVictim = (nextVictim + 1) % NUM_PHYS_PAGES;
        } else if (core[nextVictim].accessed) {
            core[nextVictim].accessed = false;
            core[nextVictim].modified = true;
            nextVictim = (nextVictim + 1) % NUM_PHYS_PAGES;
        } else {
            break;
        }
    }
    // nextVictim = (nextVictim + 1) % NUM_PHYS_PAGES; /// When using only this line, it breaks with matmult with DIM 22
    // nextVictim = rand() % NUM_PHYS_PAGES;
    DEBUG('k', "\tnextVictim %d\n", nextVictim);
    return nextVictim;
}

/// Finds an available frame.
///
/// In case no frame is available selects one based on
/// FIFO and sends it to swap file.
unsigned
CoreMap::ReserveNextAvailableFrame(int vpn, SpaceId pid)
{
    for (unsigned fpn = 0; fpn < NUM_PHYS_PAGES; ++fpn) {
        if (core[fpn].vpn == -1) {
            core[fpn] = {vpn, pid};
            return fpn;
        }
    }
    /// Send some frame to SWAP
    unsigned frameToSwap = this->GetFrameToSwap();
    auto preOwnerVPN = core[frameToSwap].vpn;
    auto preOwnerPID = core[frameToSwap].id;
    DEBUG('u', "Sending to SWAP (pid: %d, vpn: %u)\n", preOwnerPID, preOwnerVPN);
    auto *preOwnerPageTable = threadPool->Get(preOwnerPID)->space->GetPageTable();
    auto *preOwnerSwapFile = threadPool->Get(preOwnerPID)->space->GetSwapFile();
    auto &preOwnerTE = preOwnerPageTable[preOwnerVPN];
    ASSERT(preOwnerTE.physicalPage == frameToSwap);
    ASSERT(preOwnerTE.valid);
    if (preOwnerTE.dirty) {
        auto *RAM = machine->GetMMU()->mainMemory;
        int memoryOffset = frameToSwap * PAGE_SIZE;
        int fileOffset = preOwnerVPN * PAGE_SIZE;
        // DEBUG('u', "Writing in SWAP file\n");
        preOwnerSwapFile->WriteAt(RAM + memoryOffset, PAGE_SIZE, fileOffset);
        // printf("Wrote page: <<<");
        // for (unsigned i = 0; 4*i < PAGE_SIZE; ++i) {
        //     printf("%d", RAM[memoryOffset + 4*i]);
        // }
        // puts(">>>");
        // DEBUG('u', "WROTE\n");
    }
    /// Invalidate outdated TLB entry
    if (preOwnerPID == pid) {
        auto tlb = machine->GetMMU()->tlb;
        for (unsigned i = 0; i < TLB_SIZE; ++i)
            if (tlb[i].virtualPage == static_cast<unsigned>(preOwnerVPN))
                tlb[i].valid = false;
    }       
    preOwnerTE.inMemory = false;
    ASSERT(preOwnerTE.valid == true);
    core[frameToSwap] = {vpn, pid};
    return frameToSwap;
}

void 
CoreMap::FreeProcessFrames(SpaceId pid)
{
    for (unsigned i = 0; i < NUM_PHYS_PAGES; ++i)
        if (core[i].id == pid) 
            core[i] = CoreEntry();
}

void
CoreMap::MarkAccessed(unsigned pfn)
{
    ASSERT(0 <= pfn && pfn < NUM_PHYS_PAGES);
    core[pfn].accessed = true;
}

void
CoreMap::MarkModified(unsigned pfn)
{
    ASSERT(0 <= pfn && pfn < NUM_PHYS_PAGES);
    core[pfn].accessed = true;
    core[pfn].modified = true;
}
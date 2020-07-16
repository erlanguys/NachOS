#include "core_map.hh"
#include "system.hh"

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
    static unsigned indexToSwap = 0;
    indexToSwap = (indexToSwap + 1) % NUM_PHYS_PAGES;
    auto [preOwnerVPN, preOwnerPID] = core[indexToSwap];
    DEBUG('u', "Sending to SWAP (pid: %d, vpn: %u)\n", preOwnerPID, preOwnerVPN);
    auto *preOwnerPageTable = threadPool->Get(preOwnerPID)->space->GetPageTable();
    auto *preOwnerSwapFile = threadPool->Get(preOwnerPID)->space->GetSwapFile();
    auto &preOwnerTE = preOwnerPageTable[preOwnerVPN];
    ASSERT(preOwnerTE.physicalPage == indexToSwap);
    ASSERT(preOwnerTE.valid);
    if (preOwnerTE.dirty) {
        auto *RAM = machine->GetMMU()->mainMemory;
        int memoryOffset = indexToSwap * PAGE_SIZE;
        int fileOffset = preOwnerVPN * PAGE_SIZE;
        // DEBUG('u', "Writing in SWAP file\n");
        preOwnerSwapFile->WriteAt(RAM + memoryOffset, PAGE_SIZE, fileOffset);
        printf("Wrote page: <<<");
        for (unsigned i = 0; 4*i < PAGE_SIZE; ++i) {
            printf("%d", RAM[memoryOffset + 4*i]);
        }
        puts(">>>");
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
    core[indexToSwap] = {vpn, pid};
    return indexToSwap;
}

void
CoreMap::Reset(int pfn)
{
    core[pfn] = CoreEntry();
}
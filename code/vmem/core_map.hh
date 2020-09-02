#ifndef NACHOS_VMEM_COREMAP__HH
#define NACHOS_VMEM_COREMAP__HH

#include "syscall.h"
#include "mmu.hh"

struct CoreEntry {
    int vpn = -1;
    SpaceId id = -1;
    bool accessed = false;
    bool modified = false;
};

class CoreMap {
public:
    CoreMap() = default;

    ~CoreMap() = default;

    unsigned GetFrameToSwap();

    unsigned ReserveNextAvailableFrame(int vpn, SpaceId id);

    void FreeProcessFrames(SpaceId id);

    void MarkAccessed(unsigned pfn);

    void MarkModified(unsigned pfn);
private:
    CoreEntry core[NUM_PHYS_PAGES];
    unsigned nextVictim = 0;
};

#endif
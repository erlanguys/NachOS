#ifndef NACHOS_VMEM_COREMAP__HH
#define NACHOS_VMEM_COREMAP__HH

#include "syscall.h"
#include "mmu.hh"

struct CoreEntry {
    int vpn = -1;
    SpaceId id = -1;
};

class CoreMap {
public:
    CoreMap() = default;

    ~CoreMap() = default;

    int ReserveNextAvailableFrame(int vpn, SpaceId id);

    void Reset(int pfn);
private:
    CoreEntry core[NUM_PHYS_PAGES];
};

#endif
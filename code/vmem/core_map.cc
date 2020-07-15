#include "core_map.hh"

int
CoreMap::ReserveNextAvailableFrame(int vpn, SpaceId id)
{
    for (unsigned i = 0; i < NUM_PHYS_PAGES; ++i) {
        if (core[i].vpn == -1) {
            core[i] = {vpn, id};
            return i;
        }
    }
    return -1;
}

void
CoreMap::Reset(int pfn)
{
    core[pfn] = CoreEntry();
}
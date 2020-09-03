/// Routines for managing the disk file header (in UNIX, this would be called
/// the i-node).
///
/// The file header is used to locate where on disk the file's data is
/// stored.  We implement this as a fixed size table of pointers -- each
/// entry in the table points to the disk sector containing that portion of
/// the file data (in other words, there are no indirect or doubly indirect
/// blocks). The table size is chosen so that the file header will be just
/// big enough to fit in one disk sector,
///
/// Unlike in a real system, we do not keep track of file permissions,
/// ownership, last modification date, etc., in the file header.
///
/// A file header can be initialized in two ways:
///
/// * for a new file, by modifying the in-memory data structure to point to
///   the newly allocated data blocks;
/// * for a file already on disk, by reading the file header from disk.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_header.hh"
#include "threads/system.hh"

/// Initialize a fresh file header for a newly created file.  Allocate data
/// blocks for the file out of the map of free disk blocks.  Return false if
/// there are not enough free blocks to accomodate the new file.
///
/// * `freeMap` is the bit map of free disk sectors.
/// * `fileSize` is the bit map of free disk sectors.
bool
FileHeader::Allocate(Bitmap *freeMap, unsigned fileSize)
{
    ASSERT(freeMap != nullptr);

    raw.numBytes = fileSize;
    raw.numSectors = getSectorCount();

    if (freeMap->CountClear() < raw.numSectors){
        return false;  // Not enough space.
    }

    for(unsigned i = 0; i < std::min(raw.numSectors, NUM_DIRECT - 1); i++){
        int freeSector = freeMap->Find();
        if(freeSector == -1){
            localCleanup(freeMap, 0, i);
            return false; // Not enough space.
        }
        raw.dataSectors[i] = unsigned(freeSector);
    }

    if(raw.numSectors >= NUM_DIRECT){
        
        int freeSector = freeMap->Find();
        if(freeSector == -1){
            localCleanup(freeMap, 0, NUM_DIRECT - 1);
            return false; // Not enough space.
        }
        raw.dataSectors[NUM_DIRECT - 1] = unsigned(freeSector);

        FileHeader nextFH;
        bool couldAllocate = nextFH.Allocate(freeMap, raw.numBytes - (NUM_DIRECT - 1) * SECTOR_SIZE);
        if(not couldAllocate){
            localCleanup(freeMap, 0, NUM_DIRECT);
            return false; // Not enough space.
        }

        synchDisk->WriteSector(freeSector, (char *) nextFH.GetRaw());
    }
    
    return true;
}

bool 
FileHeader::Extend(Bitmap *freeMap, unsigned size)
{
    ASSERT(freeMap != nullptr);

    unsigned oldNumBytes = raw.numBytes;
    unsigned oldNumSectors = raw.numSectors;

    raw.numBytes += size;
    raw.numSectors = getSectorCount();

    if(raw.numSectors == oldNumSectors){
        return true; // Already allocated enough space
    }

    if(oldNumSectors >= NUM_DIRECT){
        // Indirected FileHeader at the end
        FileHeader nextFH;
        nextFH.FetchFrom(raw.dataSectors[NUM_DIRECT - 1]);
        bool couldExtend = nextFH.Extend(freeMap, size);
        if(not couldExtend){
            localCleanup(freeMap, 0, 0, oldNumBytes, oldNumSectors);
            return false; // Not enough space
        } 
    } else {
        // No indirected FileHeader at the end
        // Fill up remaining raw.dataSectors spaces
        unsigned allocatedSectors = 0;
        for(unsigned i = oldNumSectors; i < std::min(raw.numSectors, NUM_DIRECT - 1); i++){
            int freeSector = freeMap->Find();
            if(freeSector == -1){
                localCleanup(freeMap, oldNumSectors, i, oldNumBytes, oldNumSectors);
                return false; // Not enough space.
            }
            raw.dataSectors[i] = unsigned(freeSector);
            allocatedSectors++;
        }

        // If the remaining sectors were not enough, allocate a new indirection
        if(allocatedSectors < raw.numSectors - oldNumSectors){
            int freeSector = freeMap->Find();
            if(freeSector == -1){
                localCleanup(freeMap, oldNumSectors, NUM_DIRECT - 1, oldNumBytes, oldNumSectors);
                return false; // Not enough space.
            }
            raw.dataSectors[NUM_DIRECT - 1] = unsigned(freeSector);
            
            FileHeader nextFH;
            bool couldAllocate = nextFH.Allocate(freeMap, size - allocatedSectors * SECTOR_SIZE);
            if(not couldAllocate){
                localCleanup(freeMap, oldNumSectors, NUM_DIRECT, oldNumBytes, oldNumSectors);
                return false; // Not enough space.
            }

            synchDisk->WriteSector(freeSector, (char *) nextFH.GetRaw());
        }
    }

    return true;
}

/// De-allocate all the space allocated for data blocks for this file.
///
/// * `freeMap` is the bit map of free disk sectors.
void
FileHeader::Deallocate(Bitmap *freeMap)
{
    ASSERT(freeMap != nullptr);

    if(raw.numSectors >= NUM_DIRECT){
        FileHeader nextFH;
        nextFH.FetchFrom(raw.dataSectors[NUM_DIRECT - 1]);
        nextFH.Deallocate(freeMap);
        localCleanup(freeMap, 0, NUM_DIRECT);
    } else {
        localCleanup(freeMap, 0, raw.numSectors);
    }
}

/// Fetch contents of file header from disk.
///
/// * `sector` is the disk sector containing the file header.
void
FileHeader::FetchFrom(unsigned sector)
{
    synchDisk->ReadSector(sector, (char *) GetRaw());
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
void
FileHeader::WriteBack(unsigned sector)
{
    synchDisk->WriteSector(sector, (char *) GetRaw());
}

/// Return which disk sector is storing a particular byte within the file.
/// This is essentially a translation from a virtual address (the offset in
/// the file) to a physical address (the sector where the data at the offset
/// is stored).
///
/// * `offset` is the location within the file of the byte in question.
unsigned
FileHeader::ByteToSector(unsigned offset)
{ 
    ASSERT(offset >= 0);
    unsigned simpleIndex = offset / SECTOR_SIZE;
    if(simpleIndex < NUM_DIRECT - 1){
        return raw.dataSectors[simpleIndex];
    } else {
        FileHeader nextFH;
        nextFH.FetchFrom(raw.dataSectors[NUM_DIRECT - 1]);
        return nextFH.ByteToSector(offset - (NUM_DIRECT - 1) * SECTOR_SIZE);
    }
}

/// Return the number of bytes in the file.
unsigned
FileHeader::FileLength() const
{
    return raw.numBytes;
}

/// Print the contents of the file header, and the contents of all the data
/// blocks pointed to by the file header.
void
FileHeader::Print()
{
    char *data = new char [SECTOR_SIZE];

    printf("FileHeader contents.\n"
        "    Size: %u bytes\n"
        "    Block numbers: ",
        raw.numBytes);

        
    for (unsigned i = 0; i < std::min(raw.numSectors, NUM_DIRECT - 1); i++){
        printf("%u ", raw.dataSectors[i]);
    }
    
    printf("\n    Contents:\n");
    for (unsigned i = 0, k = 0; i < std::min(raw.numSectors, NUM_DIRECT - 1); i++){
        synchDisk->ReadSector(raw.dataSectors[i], data);
        for (unsigned j = 0; j < SECTOR_SIZE && k < raw.numBytes; j++, k++) {
            if ('\040' <= data[j] && data[j] <= '\176')  // isprint(data[j])
                printf("%c", data[j]);
            else
                printf("\\%X", (unsigned char) data[j]);
        }
        printf("\n");
    }

    delete [] data;

    if(raw.numSectors >= NUM_DIRECT){
        FileHeader fh;
        unsigned nextFileHeaderSector = raw.dataSectors[NUM_DIRECT - 1];
        fh.FetchFrom(nextFileHeaderSector);
        fh.Print();
    }
}

const RawFileHeader *
FileHeader::GetRaw() const
{
    return &raw;
}

unsigned
FileHeader::getSectorCount() const
{
    unsigned rawSectors = DivRoundUp(raw.numBytes, SECTOR_SIZE);
    unsigned withIndirectionSectors = rawSectors + rawSectors / NUM_DIRECT;
    return withIndirectionSectors;
}

void
FileHeader::localCleanup(Bitmap *freeMap, unsigned startSectorIndex, unsigned numSectors, unsigned newNumBytes, unsigned newNumSectors)
{
    for (unsigned i = startSectorIndex; i < numSectors; i++) {
        ASSERT(freeMap->Test(raw.dataSectors[i]));  // ought to be marked!
        freeMap->Clear(raw.dataSectors[i]);
    }
    raw.numBytes = newNumBytes;
    raw.numSectors = newNumSectors;
}
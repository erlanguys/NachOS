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

    if(getIndirectionDepth() > 0){
        for(unsigned totalBytesToWrite = raw.numBytes; totalBytesToWrite > 0; totalBytesToWrite -= std::min(totalBytesToWrite, MAX_FILE_SIZE)){
            raw.dataSectors[_indirectionTable.size()] = freeMap->Find();
            _indirectionTable.push_back(new FileHeader);
            bool allocated = _indirectionTable.back()->Allocate(freeMap, std::min(totalBytesToWrite, MAX_FILE_SIZE));
            ASSERT(allocated); // TODO : Deallocate and return false, or check for better preconditions
        }
    } else {
        for(unsigned i = 0; i < raw.numSectors; i++){
            raw.dataSectors[i] = freeMap->Find();
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

    for(FileHeader* indir : _indirectionTable){
        indir->Deallocate(freeMap);
        delete indir;
    }
    _indirectionTable.clear();

    for (unsigned sectorIndex = 0; sectorIndex < raw.numSectors; sectorIndex++) {
        ASSERT(freeMap->Test(raw.dataSectors[sectorIndex]));  // ought to be marked!
        freeMap->Clear(raw.dataSectors[sectorIndex]);
    }
}

/// Fetch contents of file header from disk.
///
/// * `sector` is the disk sector containing the file header.
void
FileHeader::FetchFrom(unsigned sector)
{
    synchDisk->ReadSector(sector, (char *) GetRaw());

    if(getIndirectionDepth() > 0){
        _indirectionTable.clear();

        for(unsigned sectorIndex = 0; sectorIndex < raw.numSectors; sectorIndex++){
            _indirectionTable.push_back(new FileHeader);
            _indirectionTable.back()->FetchFrom(raw.dataSectors[sectorIndex]);
        }
    }
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
void
FileHeader::WriteBack(unsigned sector)
{
    synchDisk->WriteSector(sector, (char *) GetRaw());

    if(getIndirectionDepth() > 0){
        for(unsigned indirIndex = 0; indirIndex < _indirectionTable.size(); indirIndex++){
            _indirectionTable[indirIndex]->WriteBack(raw.dataSectors[indirIndex]);
        }
    }
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
    if(getIndirectionDepth() == 0){
        return raw.dataSectors[offset / SECTOR_SIZE];
    } else {
        // TODO : Implemented only for depth <= 1
        return _indirectionTable[offset / MAX_FILE_SIZE]->ByteToSector(offset % MAX_FILE_SIZE);
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

    if(getIndirectionDepth() == 0){
        printf("FileHeader contents.\n"
            "    Size: %u bytes\n"
            "    Block numbers: ",
            raw.numBytes);
        for (unsigned i = 0; i < raw.numSectors; i++)
            printf("%u ", raw.dataSectors[i]);
        printf("\n    Contents:\n");
        for (unsigned i = 0, k = 0; i < raw.numSectors; i++) {
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
    } else {
        for(FileHeader* indir : _indirectionTable){
            indir->Print();
        }
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
    unsigned simpleSectorCount = DivRoundUp(raw.numBytes, SECTOR_SIZE);
    if(getIndirectionDepth() == 0){
        return simpleSectorCount;
    } else {
        // TODO : Implemented only for depth == 1
        return DivRoundUp(simpleSectorCount, NUM_DIRECT);
    }
}

unsigned
FileHeader::getIndirectionDepth() const
{
    // TODO : Implemented only for depth <= 1
    return MAX_FILE_SIZE < raw.numBytes;
}
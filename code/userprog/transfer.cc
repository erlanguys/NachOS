#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"


bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount > 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        *outString++ = (unsigned char) temp;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void ReadBufferFromUser(int userAddress, char *outBuffer,
                        unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount > 0);

    while (byteCount--)
    {
        int temp;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        *outBuffer++ = (unsigned char) temp;
    }
}

void WriteStringToUser(const char *string, int userAddress)
{
    ASSERT(userAddress != 0);
    ASSERT(string != nullptr);

    do {
        ASSERT(machine->WriteMem(userAddress++, 1, *string));
    } while (*string++);
}

void WriteBufferToUser(const char *buffer, unsigned byteCount,
                       int userAddress)
{
    ASSERT(buffer != 0);
    ASSERT(byteCount > 0);

    while (byteCount--)
        ASSERT(machine->WriteMem(userAddress++, 1, *buffer++));
}

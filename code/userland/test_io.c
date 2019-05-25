/// Opens a file named "hola.txt", then reads the first 5 bytes
/// and uses them to create a new file with that name.
///
/// CAREFUL: the maximum name of a file is small.
#include "syscall.h"

int
main(void)
{
    int fid = Open("hola.txt");
    char buffer[10]={};
    Read(buffer, 5, fid);
    Create(buffer);
    Halt();
}

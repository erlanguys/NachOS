/// Opens a file named "hola.txt", then reads the first 5 bytes
/// and uses them to create a new file with that name.
#include "syscall.h"

int
main(void)
{
    int fid = Open("hola.txt");
    char buffer[10]={};
    Read(buffer, 5, fid);
    Create(buffer);
    int fid2 = Open(buffer);
    Write("yay!\n", 5, fid2);
    Close(fid2);
    Write("yay 40!\n", 5, fid2);
    Halt();
}

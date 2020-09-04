/// Opens a file named "hola.txt", then reads the first 5 bytes
/// and uses them to create a new file with that name.
#include "syscall.h"

int
main(void)
{
    char buf[2];
    Read(buf, 2, CONSOLE_INPUT);
    Write(buf, 2, CONSOLE_OUTPUT);
    Write("pepito\n", 7, CONSOLE_OUTPUT);
    int fid = Open("hola.txt");
    if (fid == -1) {
        Write("puuuuu\n", 7, CONSOLE_OUTPUT);
        Exit(0);
    }
    char buffer[10]={};
    Read(buffer, 5, fid);
    Create(buffer);
    int fid2 = Open(buffer);
    if (fid2 == -1) {
        Write("teeeee\n", 7, CONSOLE_OUTPUT);
        Exit(0);
    }
    Write("yay!\n", 5, fid2);
    Close(fid2);
    // Write("yay 40!\n", 5, fid2); // Wrong!
    Exit(0);
}

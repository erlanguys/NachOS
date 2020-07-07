#include "syscall.h"
#include "stdio.h"
#include "assert.h"


int main(int argc, char** argv) {

    assert(argc == 2);

    int fd = Open(argv[1]);

    if (fd == -1 ) {
        printf("Can't open file %s.", argv[1]);
        Exit(-1);
    } 

    char buffer;
    int n;
    while (n = Read(&buffer, 1, fd))
        Write(&buffer, 1, CONSOLE_OUTPUT);

    Exit(0);
}

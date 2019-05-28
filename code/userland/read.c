#include <stdio.h>
#include "syscall.h"


int
main(void)
{
    char buffer[100]={};
    Read(buffer, 5, 0);
    printf("%s\n", buffer);
    Halt();
    // Not reached.
}

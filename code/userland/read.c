#include <stdio.h>
#include "syscall.h"


int
main(void)
{
    char buffer[100]={};
    read(buffer, 5, 0);
    printf("%s\n", buffer);
    Halt();
    // Not reached.
}

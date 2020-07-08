#include "syscall.h"

int
main(void)
{
    Exec("userland/create", 0); // second 0 argument is temporal, check it out
    return 0;
}

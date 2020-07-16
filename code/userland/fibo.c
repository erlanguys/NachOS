#include "syscall.h"

int f(int n) {
    if (n < 2) return n;
    return f(n-1) + f(n-2);
}

int
main(void)
{
    int x = f(30);
    Write(x == 832040 ? "FIBOOKA\n" : "FIBOMAL\n", 8, CONSOLE_OUTPUT);
    Exit(0);
    // Not reached.
}

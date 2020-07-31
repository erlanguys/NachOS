#include "syscall.h"

int f(int n) {
    if (n < 2) return n;
    return f(n-1) + f(n-2);
}

int g(int n){
    if (n == 0) return 0;
    return g(n-1) + 1;
}

int
main(void)
{
    int x = g(44);
    Write(x == 44 ? "FIBOOKA\n" : "FIBOMAL\n", 8, CONSOLE_OUTPUT);
    Exit(0);
    // Not reached.
}

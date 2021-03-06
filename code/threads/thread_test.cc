/// Simple test case for the threads assignment.
///
/// Create several threads, and have them context switch back and forth
/// between themselves by calling `Thread::Yield`, to illustrate the inner
/// workings of the thread system.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "system.hh"
#include "threads/synch.hh"

#ifdef SEMAPHORE_TEST
Semaphore s{"<semaphore-0>", 3};
#endif

/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.
void
SimpleThread(void *name_)
{
    // Reinterpret arg `name` as a string.
    char *name = (char *) name_;

    #ifdef SEMAPHORE_TEST
    s.P();
    DEBUG('t', "%s has p'ed\n", name);
    #endif
    

    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    for (unsigned num = 0; num < 10; num++) {
        printf("*** Thread `%s` is running: iteration %u\n", name, num);
        currentThread->Yield();
    }
    printf("!!! Thread `%s` has finished\n", name);
    #ifdef SEMAPHORE_TEST
    s.V();
    DEBUG('t', "%s has v'ed\n", name);
    #endif
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching ten threads which call `SimpleThread`, and finally
/// calling `SimpleThread` ourselves.
void thread_original();
void garden();
void prodcons();
void inversion();

void
ThreadTest()
{
    DEBUG('t', "Entering thread test\n");

    for (int i = 0; i < 4; ++i) {
        char *name = new char [64];
        sprintf(name,  "<thread-%c>", i + 'a');
        Thread *newThread = new Thread(name);
        newThread->Fork(SimpleThread, (void *) name);
    }
    
    SimpleThread((void *) "<thread-e>");
}

void
PortTest()
{
    //~ Port P("Erick");
    
    //~ P.Send(5);
    //~ int x = -1;
    //~ int *p = &x;
    //~ P.Receive(p);
    
    //~ printf("x = %d\n", x);
    
    //~ P.Receive(p);
    //~ P.Send(8);
    
    //~ printf("x = %d\n", x);
}


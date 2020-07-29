#ifndef FILESYS_RWMUTEX__HH
#define FILESYS_RWMUTEX__HH

#include "threads/synch.hh"
#include "machine/interrupt.hh"

class Lock;
class Semaphore;

class RWMutex {
    IntStatus interruptLevel;
    void startAtomic();
    void endAtomic();

    Lock *atomic, *writerMutex;
    Semaphore *readerSem, *writerSem;
    int readerCount, readerWait;

    static int maxReaders;

public:
    RWMutex();
    ~RWMutex();

    // Read
    void RLock();
    void RUnlock();

    // Write
    void WLock();
    void WUnlock();    
};

#endif
#ifndef RWMUTEX_HH
#define RWMUTEX_HH

#include "threads/synch.hh"

class Lock;
class Semaphore;

class RWMutex {
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
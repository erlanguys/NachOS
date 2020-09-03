#include "rwmutex.hh"

// Based on Golang sync.RWMutex
// See https://golang.org/src/sync/rwmutex.go

int RWMutex::maxReaders = 1 << 30;

RWMutex::RWMutex(){
    atomic = new Lock("atomic");
    writerMutex = new Lock("writerMutex");

    readerSem = new Semaphore("readerSem", 0);
    writerSem = new Semaphore("writerSem", 0);

    readerCount = 0;
    readerWait = 0;
}

RWMutex::~RWMutex(){
    delete atomic;
    delete writerMutex;

    delete readerSem;
    delete writerSem;
}

void
RWMutex::RLock(){
    // Warning, the following if is a "hack" to allow the unique writer to freely RLock()/RUnlock():
    // In such case, the writer must not do the corresponding RUnlock() after WUnlock(). 
    // Original implementation avoids recursive read locking as WLock() excludes new readers from acquiring the lock.
    if(not writerMutex->IsHeldByCurrentThread()){  
        atomic->Acquire();
        readerCount += 1;
        int r = readerCount;
        atomic->Release();
        
        if(r < 0) {
            // A writer is pending
            readerSem->P();
        }
    }
}

void
RWMutex::RUnlock(){
    // Warning, same as RLock()
    if(not writerMutex->IsHeldByCurrentThread()){
        atomic->Acquire();
        readerCount -= 1;
        int r = readerCount;
        atomic->Release();

        if(r < 0) {
            if(r+1 == 0 || r+1 == -maxReaders){
                // RACE : RUnlock of unlocked RWMutex
            }

            // A writer is pending
            atomic->Acquire();
            readerWait -= 1;
            bool check = readerWait == 0;
            atomic->Release();

            if(check) {
                // Last reader unblocks the writer
                writerSem->V();
            }
        }
    }
}

void
RWMutex::WLock(){
    // Resolve competition with other writers
    writerMutex->Acquire();

    atomic->Acquire();
    readerWait += readerCount;
    bool check = readerCount != 0 && readerWait != 0;
    readerCount -= maxReaders;
    atomic->Release();

    if(check) {
        // Wait for active readers
        writerSem->P();
    }
}

void
RWMutex::WUnlock(){
    atomic->Acquire();
    readerCount += maxReaders;
    int r = readerCount;
    atomic->Release();

    if(r >= maxReaders) {
        // RACE : Unlock of unlocked RWMutex
    }

    for(int i = 0; i < r; i++) {
        // Unlock blocked readers
        readerSem->V();
    }

    // Allow other writers to proceed
    writerMutex->Release();
}

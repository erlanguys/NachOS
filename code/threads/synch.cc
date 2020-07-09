/// Routines for synchronizing threads.
///
/// Three kinds of synchronization routines are defined here: semaphores,
/// locks and condition variables (the implementation of the last two are
/// left to the reader).
///
/// Any implementation of a synchronization routine needs some primitive
/// atomic operation.  We assume Nachos is running on a uniprocessor, and
/// thus atomicity can be provided by turning off interrupts.  While
/// interrupts are disabled, no context switch can occur, and thus the
/// current thread is guaranteed to hold the CPU throughout, until interrupts
/// are reenabled.
///
/// Because some of these routines might be called with interrupts already
/// disabled (`Semaphore::V` for one), instead of turning on interrupts at
/// the end of the atomic operation, we always simply re-set the interrupt
/// state back to its original value (whether that be disabled or enabled).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2018 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include <algorithm>
#include "synch.hh"
#include "system.hh"


/// Initialize a semaphore, so that it can be used for synchronization.
///
/// * `debugName` is an arbitrary name, useful for debugging.
/// * `initialValue` is the initial value of the semaphore.
Semaphore::Semaphore(const char *debugName, int initialValue)
{
    name  = debugName;
    value = initialValue;
    queue = new List<Thread *>;
}

/// De-allocate semaphore, when no longer needed.
///
/// Assume no one is still waiting on the semaphore!
Semaphore::~Semaphore()
{
    delete queue;
}

const char *
Semaphore::GetName() const
{
    return name;
}

/// Wait until semaphore `value > 0`, then decrement.
///
/// Checking the value and decrementing must be done atomically, so we need
/// to disable interrupts before checking the value.
///
/// Note that `Thread::Sleep` assumes that interrupts are disabled when it is
/// called.
void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);
      // Disable interrupts.

    while (value == 0) {  // Semaphore not available.
        queue->Append(currentThread);  // So go to sleep.
        currentThread->Sleep();
    }
    value--;  // Semaphore available, consume its value.

    interrupt->SetLevel(oldLevel);  // Re-enable interrupts.
}

/// Increment semaphore value, waking up a waiter if necessary.
///
/// As with `P`, this operation must be atomic, so we need to disable
/// interrupts.  `Scheduler::ReadyToRun` assumes that threads are disabled
/// when it is called.
void
Semaphore::V()
{
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);

    Thread *thread = queue->Pop();
    if (thread != nullptr)
        // Make thread ready, consuming the `V` immediately.
        scheduler->ReadyToRun(thread);
    value++;

    interrupt->SetLevel(oldLevel);
}

/// Dummy functions -- so we can compile our later assignments.
///
/// Note -- without a correct implementation of `Condition::Wait`, the test
/// case in the network assignment will not work!

Lock::Lock(const char *debugName, unsigned _priority)
{
    name = debugName;
    ownerThread = nullptr;
    priority = _priority;
    old_priority = nullptr;
    lockSemaphore = new Semaphore(debugName, 1);
}

Lock::~Lock()
{
    delete lockSemaphore;
    delete old_priority;
}

const char *
Lock::GetName() const
{
    return name;
}

void
Lock::Acquire()
{
    ASSERT(not IsHeldByCurrentThread());

    priority = std::max(priority, ::currentThread->GetPriority());
    if( ownerThread and priority > ownerThread->GetPriority() ){
        if( not old_priority )
          old_priority = new unsigned {ownerThread->GetPriority()};
        ownerThread->SetPriority(priority);
    }

    lockSemaphore->P();
    ownerThread = ::currentThread;
}

void
Lock::Release()
{
    DEBUG('c', "Current thread: %s\n", currentThread->GetName());
    ASSERT(IsHeldByCurrentThread());
    if( old_priority ){
        ownerThread->SetPriority( *old_priority );
        delete old_priority;
        old_priority = nullptr;
    }
    ownerThread = nullptr;
    lockSemaphore->V();
}

bool
Lock::IsHeldByCurrentThread() const
{
    return ownerThread == ::currentThread;
}

Condition::Condition(const char *debugName, Lock *_conditionLock)
{
    name = debugName;
    conditionLock = _conditionLock;
    waiters = 0;
    conditionSemaphoreS = new Semaphore(debugName, 0);
    conditionSemaphoreX = new Semaphore(debugName, 1);
    conditionSemaphoreH = new Semaphore(debugName, 0);
}

Condition::~Condition()
{
    delete conditionSemaphoreS;
    delete conditionSemaphoreX;
    delete conditionSemaphoreH;
}

const char *
Condition::GetName() const
{
    return name;
}

void
Condition::Wait()
{
    conditionSemaphoreX->P();
    waiters++;
    conditionSemaphoreX->V();
    conditionLock->Release();

    conditionSemaphoreS->P();
    conditionSemaphoreH->V();
    conditionLock->Acquire();
}

void
Condition::Signal()
{
    conditionSemaphoreX->P();
    if( waiters > 0 ){
        waiters--;
        conditionSemaphoreS->V();
        conditionSemaphoreH->P();
    }
    conditionSemaphoreX->V();
}

void
Condition::Broadcast()
{
    conditionSemaphoreX->P();
    for(int i = 0; i < waiters; ++i)
        conditionSemaphoreS->V();
    while( waiters > 0 ){
        --waiters;
        conditionSemaphoreH->P();
    }
    conditionSemaphoreX->V();
}


Port::Port(const char *debugName)
{
    name = debugName;
    lockPort = new Lock(debugName);

    sendStarted = new Condition(debugName, lockPort);
    sendEnded = new Condition(debugName, lockPort);
    receiveEnded = new Condition(debugName, lockPort);

    state = STATE::IDLE;
};

Port::~Port()
{
    delete lockPort;
    delete sendStarted;
    delete sendEnded;
    delete receiveEnded;
}

const char *
Port::GetName() const
{
    return name;
}

void
Port::Send(int message)
{
    // esperamos turno
    lockPort->Acquire();
    // esperamos a que el canal este libre
    while(state != STATE::IDLE)
        sendEnded->Wait();
    // almacenamos el mensaje en el buffer
    buffer = message;
    // avisamos que ya se puede recibir
    sendStarted->Signal(); state = STATE::STARTED;
    // aguardamos hasta que se termine de recibir
    while( state != STATE::ENDED )
        receiveEnded->Wait();
    // avisamos a los demas emisores que se termino de enviar el mensaje
    sendEnded->Broadcast(); state = STATE::IDLE; 
    // liberamos el lock
    lockPort->Release();
}

void
Port::Receive(int *destination)
{
    // esperamos turno
    lockPort->Acquire();
    // esperamos a que haya algo que recibir
    while( state != STATE::STARTED )
        sendStarted->Wait();
    // recibimos el mensaje
    *destination = buffer;
    // avisamos que ya hemos recibido el mensaje
    receiveEnded->Signal(); state = STATE::ENDED;
    // liberamos el lock
    lockPort->Release();
}

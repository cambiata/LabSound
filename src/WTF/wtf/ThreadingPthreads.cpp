/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabSoundConfig.h"
#include "Threading.h"
#include <mutex>

#if USE(PTHREADS)

#include <WTF/CurrentTime.h>
#include <WTF/ThreadFunctionInvocation.h>
#include <WTF/ThreadIdentifierDataPthreads.h>
#include <WTF/ThreadSpecific.h>
#include <WTF/RefPtr.h>
#include <WTF/PassOwnPtr.h>
#include <errno.h>
#include <map>

#if !COMPILER(MSVC)
#include <limits.h>
#include <sched.h>
#include <sys/time.h>
#endif

#if OS(MAC_OS_X) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
#include <objc/objc-auto.h>
#endif

namespace WTF {

class PthreadState {
public:
    enum JoinableState {
        Joinable, // The default thread state. The thread can be joined on.

        Joined, // Somebody waited on this thread to exit and this thread finally exited. This state is here because there can be a 
                // period of time between when the thread exits (which causes pthread_join to return and the remainder of waitOnThreadCompletion to run) 
                // and when threadDidExit is called. We need threadDidExit to take charge and delete the thread data since there's 
                // nobody else to pick up the slack in this case (since waitOnThreadCompletion has already returned).

        Detached // The thread has been detached and can no longer be joined on. At this point, the thread must take care of cleaning up after itself.
    };

    // Currently all threads created by WTF start out as joinable. 
    PthreadState(pthread_t handle)
        : m_joinableState(Joinable)
        , m_didExit(false)
        , m_pthreadHandle(handle)
    {
    }

    JoinableState joinableState() { return m_joinableState; }
    pthread_t pthreadHandle() { return m_pthreadHandle; }
    void didBecomeDetached() { m_joinableState = Detached; }
    void didExit() { m_didExit = true; }
    void didJoin() { m_joinableState = Joined; }
    bool hasExited() { return m_didExit; }

private:
    JoinableState m_joinableState;
    bool m_didExit;
    pthread_t m_pthreadHandle;
};

typedef std::map<ThreadIdentifier, OwnPtr<PthreadState> > ThreadMap;

static std::mutex* atomicallyInitializedStaticMutex;

void unsafeThreadWasDetached(ThreadIdentifier);
void threadDidExit(ThreadIdentifier);
void threadWasJoined(ThreadIdentifier);

// Use these to declare and define a static local variable (static T;) so that
//  it is leaked so that its destructors are not called at exit. Using this
//  macro also allows workarounds a compiler bug present in Apple's version of GCC 4.0.1.
#ifndef DEFINE_STATIC_LOCAL
#if COMPILER(GCC) && defined(__APPLE_CC__) && __GNUC__ == 4 && __GNUC_MINOR__ == 0 && __GNUC_PATCHLEVEL__ == 1
#define DEFINE_STATIC_LOCAL(type, name, arguments) \
static type* name##Ptr = new type arguments; \
type& name = *name##Ptr
#else
#define DEFINE_STATIC_LOCAL(type, name, arguments) \
static type& name = *new type arguments
#endif
#endif

static std::mutex& threadMapMutex()
{
    DEFINE_STATIC_LOCAL(std::mutex, mutex, ());
    return mutex;
}

#if OS(QNX) && CPU(ARM_THUMB2)
static void enableIEEE754Denormal()
{
    // Clear the ARM_VFP_FPSCR_FZ flag in FPSCR.
    unsigned fpscr;
    asm volatile("vmrs %0, fpscr" : "=r"(fpscr));
    fpscr &= ~0x01000000u;
    asm volatile("vmsr fpscr, %0" : : "r"(fpscr));
}
#endif

void initializeThreading()
{
    if (atomicallyInitializedStaticMutex)
        return;

#if OS(QNX) && CPU(ARM_THUMB2)
    enableIEEE754Denormal();
#endif

    atomicallyInitializedStaticMutex = new std::mutex;
    threadMapMutex();
    ThreadIdentifierData::initializeOnce();
}

void lockAtomicallyInitializedStaticMutex()
{
    ASSERT(atomicallyInitializedStaticMutex);
    atomicallyInitializedStaticMutex->lock();
}

void unlockAtomicallyInitializedStaticMutex()
{
    atomicallyInitializedStaticMutex->unlock();
}

static ThreadMap& threadMap()
{
    DEFINE_STATIC_LOCAL(ThreadMap, map, ());
    return map;
}

static ThreadIdentifier identifierByPthreadHandle(const pthread_t& pthreadHandle)
{
    std::lock_guard<std::mutex> locker(threadMapMutex());

    ThreadMap::iterator i = threadMap().begin();
    for (; i != threadMap().end(); ++i) {
        if (pthread_equal(i->second->pthreadHandle(), pthreadHandle) && !i->second->hasExited())
            return i->first;
    }

    return 0;
}

static ThreadIdentifier establishIdentifierForPthreadHandle(const pthread_t& pthreadHandle)
{
    ASSERT(!identifierByPthreadHandle(pthreadHandle));
    std::lock_guard<std::mutex> locker(threadMapMutex());
    static ThreadIdentifier identifierCount = 1;
    threadMap()[identifierCount] =adoptPtr(new PthreadState(pthreadHandle));
    return identifierCount++;
}

static pthread_t pthreadHandleForIdentifierWithLockAlreadyHeld(ThreadIdentifier id)
{
    auto i = threadMap().find(id);
    if (i != threadMap().end())
        return i->second->pthreadHandle();
    else
        return 0;
}

static void* wtfThreadEntryPoint(void* param)
{
    // Balanced by .leakPtr() in createThreadInternal.
    OwnPtr<ThreadFunctionInvocation> invocation = adoptPtr(static_cast<ThreadFunctionInvocation*>(param));
    invocation->function(invocation->data);
    return 0;
}

ThreadIdentifier createThreadInternal(ThreadFunction entryPoint, void* data, const char*)
{
    OwnPtr<ThreadFunctionInvocation> invocation = adoptPtr(new ThreadFunctionInvocation(entryPoint, data));
    pthread_t threadHandle;
    if (pthread_create(&threadHandle, 0, wtfThreadEntryPoint, invocation.get())) {
        LOG_ERROR("Failed to create pthread at entry point %p with data %p", wtfThreadEntryPoint, invocation.get());
        return 0;
    }

    // Balanced by adoptPtr() in wtfThreadEntryPoint.
    invocation.leakPtr();
    return establishIdentifierForPthreadHandle(threadHandle);
}

void initializeCurrentThreadInternal(const char* threadName)
{
#if HAVE(PTHREAD_SETNAME_NP)
    pthread_setname_np(threadName);
#elif OS(QNX)
    pthread_setname_np(pthread_self(), threadName);
#endif

#if OS(MAC_OS_X) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
    // All threads that potentially use APIs above the BSD layer must be registered with the Objective-C
    // garbage collector in case API implementations use garbage-collected memory.
    objc_registerThreadWithCollector();
#endif

#if OS(QNX) && CPU(ARM_THUMB2)
    enableIEEE754Denormal();
#endif

    ThreadIdentifier id = identifierByPthreadHandle(pthread_self());
    ASSERT(id);
    ThreadIdentifierData::initialize(id);
}

int waitForThreadCompletion(ThreadIdentifier threadID)
{
    pthread_t pthreadHandle;
    ASSERT(threadID);

    {
        // We don't want to lock across the call to join, since that can block our thread and cause deadlock.
        std::lock_guard<std::mutex> locker(threadMapMutex());
        pthreadHandle = pthreadHandleForIdentifierWithLockAlreadyHeld(threadID);
        ASSERT(pthreadHandle);
    }

    int joinResult = pthread_join(pthreadHandle, 0);

    if (joinResult == EDEADLK)
        LOG_ERROR("ThreadIdentifier %u was found to be deadlocked trying to quit", threadID);
    else if (joinResult)
        LOG_ERROR("ThreadIdentifier %u was unable to be joined.\n", threadID);

    std::lock_guard<std::mutex> locker(threadMapMutex());
    auto i = threadMap().find(threadID);
    ASSERT(i != threadMap().end());
    PthreadState* state = i->second.get();
    ASSERT(state);
    ASSERT(state->joinableState() == PthreadState::Joinable);

    // The thread has already exited, so clean up after it.
    if (state->hasExited())
        threadMap().erase(i);
    // The thread hasn't exited yet, so don't clean anything up. Just signal that we've already joined on it so that it will clean up after itself.
    else
        state->didJoin();

    return joinResult;
}

void detachThread(ThreadIdentifier threadID)
{
    ASSERT(threadID);

    std::lock_guard<std::mutex> locker(threadMapMutex());
    pthread_t pthreadHandle = pthreadHandleForIdentifierWithLockAlreadyHeld(threadID);
    ASSERT(pthreadHandle);

    int detachResult = pthread_detach(pthreadHandle);
    if (detachResult)
        LOG_ERROR("ThreadIdentifier %u was unable to be detached\n", threadID);

    auto i = threadMap().find(threadID);
    ASSERT(i != threadMap().end());
    PthreadState* state = i->second.get();
    ASSERT(state);
    if (state->hasExited())
        threadMap().erase(i);
    else
        i->second->didBecomeDetached();
}

void threadDidExit(ThreadIdentifier threadID)
{
    std::lock_guard<std::mutex> locker(threadMapMutex());
    auto i = threadMap().find(threadID);
    ASSERT(i != threadMap().end());
    PthreadState* state = i->second.get();
    ASSERT(state);
    
    state->didExit();

    if (state->joinableState() != PthreadState::Joinable)
        threadMap().erase(i);
}

void yield()
{
    sched_yield();
}

ThreadIdentifier currentThread()
{
    ThreadIdentifier id = ThreadIdentifierData::identifier();
    if (id)
        return id;

    // Not a WTF-created thread, ThreadIdentifier is not established yet.
    id = establishIdentifierForPthreadHandle(pthread_self());
    ThreadIdentifierData::initialize(id);
    
    printf("*** %d created thread \n", id);
    
    return id;
}


} // namespace WTF

#endif // USE(PTHREADS)

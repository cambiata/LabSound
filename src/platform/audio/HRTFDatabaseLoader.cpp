/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "HRTFDatabaseLoader.h"

#include "HRTFDatabase.h"
#include <wtf/MainThread.h>

#include <iostream>

namespace WebCore {

// Singleton
std::shared_ptr<HRTFDatabaseLoader> HRTFDatabaseLoader::s_loader;

std::shared_ptr<HRTFDatabaseLoader> HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(float sampleRate)
{
    if (!s_loader) {
        s_loader = std::make_shared<HRTFDatabaseLoader>(sampleRate);
        s_loader->loadAsynchronously();
    }
    return s_loader;
}

HRTFDatabaseLoader::HRTFDatabaseLoader(float sampleRate)
    : m_databaseLoaderThread(0)
    , m_databaseSampleRate(sampleRate)
{
}

HRTFDatabaseLoader::~HRTFDatabaseLoader()
{
    waitForLoaderThreadCompletion();
    m_hrtfDatabase.reset();
    
    // Clear out singleton.
    ASSERT(this == s_loader.get());
    s_loader = 0;
}


// Asynchronously load the database in this thread.
static void databaseLoaderEntry(void* threadData)
{
    HRTFDatabaseLoader* loader = reinterpret_cast<HRTFDatabaseLoader*>(threadData);
    ASSERT(loader);
    loader->load();
}

void HRTFDatabaseLoader::load()
{
    if (!m_hrtfDatabase.get()) {
        // Load the default HRTF database.
        m_hrtfDatabase = HRTFDatabase::create(m_databaseSampleRate);
    }
    if (!m_hrtfDatabase.get()) { // @Lab added error reporting
        std::cerr << "HRTF database not loaded" << std::endl;
    }
}

void HRTFDatabaseLoader::loadAsynchronously()
{
    std::lock_guard<std::mutex> locker(m_threadLock);
    
    if (!m_hrtfDatabase.get() && !m_databaseLoaderThread) {
        // Start the asynchronous database loading process.
        m_databaseLoaderThread = createThread(databaseLoaderEntry, this, "HRTF database loader");
    }
}

bool HRTFDatabaseLoader::isLoaded() const
{
    return m_hrtfDatabase.get();
}

void HRTFDatabaseLoader::waitForLoaderThreadCompletion()
{
    std::lock_guard<std::mutex> locker(m_threadLock);
    
    // waitForThreadCompletion() should not be called twice for the same thread.
    if (m_databaseLoaderThread)
        waitForThreadCompletion(m_databaseLoaderThread);
    m_databaseLoaderThread = 0;
}

HRTFDatabase* HRTFDatabaseLoader::defaultHRTFDatabase()
{
    if (!s_loader)
        return 0;
    
    return s_loader->database();
}

} // namespace WebCore

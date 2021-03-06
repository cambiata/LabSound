/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MediaStreamAudioSourceNode_h
#define MediaStreamAudioSourceNode_h

#include "AudioSourceNode.h"
#include "AudioSourceProvider.h"
#include "AudioSourceProviderClient.h"
#include "MediaStream.h"
#include "WTF/RefPtr.h"
#include "WTF/Threading.h"

namespace WebCore {

class AudioContext;
    

class MediaStreamAudioSourceNode : public AudioSourceNode, public AudioSourceProviderClient {
public:
    MediaStreamAudioSourceNode(std::shared_ptr<MediaStream>, AudioSourceProvider*, float sampleRate);
    virtual ~MediaStreamAudioSourceNode();

    MediaStream* mediaStream() { return m_mediaStream.get(); }

    // AudioNode
    virtual void process(ContextGraphLock& g, ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(std::shared_ptr<AudioContext>) override;

    // AudioSourceProviderClient
    virtual void setFormat(ContextGraphLock& g, ContextRenderLock& r, size_t numberOfChannels, float sampleRate) override;

    AudioSourceProvider* audioSourceProvider() const { return m_audioSourceProvider; }

private:
    // As an audio source, we will never propagate silence.
    virtual bool propagatesSilence(double now) const OVERRIDE { return false; }

    std::shared_ptr<MediaStream> m_mediaStream;
    AudioSourceProvider* m_audioSourceProvider;

    unsigned m_sourceNumberOfChannels;
};

} // namespace WebCore

#endif // MediaStreamAudioSourceNode_h

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

#ifndef MediaStreamAudioDestinationNode_h
#define MediaStreamAudioDestinationNode_h

#include "AudioBasicInspectorNode.h"
#include "AudioBus.h"
#include "MediaStream.h"
#include "WTF/RefPtr.h"

namespace WebCore {

class AudioContext;

class MediaStreamAudioDestinationNode : public AudioBasicInspectorNode {
public:
    MediaStreamAudioDestinationNode(size_t numberOfChannels, float sampleRate);
    virtual ~MediaStreamAudioDestinationNode();

    MediaStream* stream() { return m_stream.get(); }

    // AudioNode.
    virtual void process(ContextGraphLock& g, ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(std::shared_ptr<AudioContext>) override;
    
    MediaStreamSource* mediaStreamSource();

private:
    virtual double tailTime() const OVERRIDE { return 0; }
    virtual double latencyTime() const OVERRIDE { return 0; }

    // As an audio source, we will never propagate silence.
    virtual bool propagatesSilence(double now) const OVERRIDE { return false; }

    std::shared_ptr<MediaStream> m_stream;
    std::shared_ptr<MediaStreamSource> m_source;
    AudioBus m_mixBus;
};

} // namespace WebCore

#endif // MediaStreamAudioDestinationNode_h

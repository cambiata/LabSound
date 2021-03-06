/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
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

#ifndef ConvolverNode_h
#define ConvolverNode_h

#include "AudioNode.h"
#include "WTF/RefPtr.h"
#include "WTF/Threading.h"

namespace WebCore {

class AudioBuffer;
class Reverb;
    
class ConvolverNode : public AudioNode {
public:
    ConvolverNode(float sampleRate);
    virtual ~ConvolverNode();
    
    // AudioNode
    virtual void process(ContextGraphLock& g, ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(std::shared_ptr<AudioContext>) override;
    virtual void initialize();
    virtual void uninitialize();

    // Impulse responses
    void setBuffer(std::shared_ptr<AudioBuffer>);
    std::shared_ptr<AudioBuffer> buffer();

    bool normalize() const { return m_normalize; }
    void setNormalize(bool normalize) { m_normalize = normalize; }

private:
    virtual double tailTime() const OVERRIDE;
    virtual double latencyTime() const OVERRIDE;

    std::unique_ptr<Reverb> m_reverb;
    std::shared_ptr<AudioBuffer> m_buffer;

    // lock free swap on update
    bool m_swapOnRender;
    std::unique_ptr<Reverb> m_newReverb;
    std::shared_ptr<AudioBuffer> m_newBuffer;

    // Normalize the impulse response or not. Must default to true.
    bool m_normalize;
};

} // namespace WebCore

#endif // ConvolverNode_h

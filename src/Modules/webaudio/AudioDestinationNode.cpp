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

#include "LabSoundConfig.h"
#include "AudioDestinationNode.h"

#include "AudioContext.h"
#include "AudioContextLock.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioUtilities.h"
#include "DenormalDisabler.h"

namespace WebCore {
    
AudioDestinationNode::AudioDestinationNode(std::shared_ptr<AudioContext> c, float sampleRate)
    : AudioNode(sampleRate)
    , m_currentSampleFrame(0)
    , m_context(c)
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    setNodeType(NodeTypeDestination);
}

AudioDestinationNode::~AudioDestinationNode()
{
    uninitialize();
}

void AudioDestinationNode::render(AudioBus* sourceBus, AudioBus* destinationBus, size_t numberOfFrames)
{
    // The audio system might still be invoking callbacks during shutdown, so bail out if so.
    if (!m_context)
        return;
    
    // We don't want denormals slowing down any of the audio processing
    // since they can very seriously hurt performance.
    // This will take care of all AudioNodes because they all process within this scope.
    DenormalDisabler denormalDisabler;
    
    ContextRenderLock renderLock(m_context, "AudioDestinationNode::render");
    if (!renderLock.context())
        return;                     // return if couldn't acquire lock
    
    if (!m_context->isRunnable()) {
        destinationBus->zero();
        return;
    }

    ContextGraphLock graphLock(m_context, "AudioDestinationNode::render");
    
    if (!graphLock.context()) {
        destinationBus->zero();
        return;
    }

    // Let the context take care of any business at the start of each render quantum.
    m_context->handlePreRenderTasks(graphLock, renderLock);

    // Prepare the local audio input provider for this render quantum.
    if (sourceBus)
        m_localAudioInputProvider.set(sourceBus);

    // This will cause the node(s) connected to this destination node to process, which in turn will pull on their input(s),
    // all the way backwards through the rendering graph.
    AudioBus* renderedBus = input(0)->pull(graphLock, renderLock, destinationBus, numberOfFrames);
    
    if (!renderedBus)
        destinationBus->zero();
    else if (renderedBus != destinationBus) {
        // in-place processing was not possible - so copy
        destinationBus->copyFrom(*renderedBus);
    }

    // Process nodes which need a little extra help because they are not connected to anything, but still need to process.
    m_context->processAutomaticPullNodes(graphLock, renderLock, numberOfFrames);

    // Let the context take care of any business at the end of each render quantum.
    m_context->handlePostRenderTasks(graphLock, renderLock);
    
    // Advance current sample-frame.
    m_currentSampleFrame += numberOfFrames;
}

} // namespace WebCore


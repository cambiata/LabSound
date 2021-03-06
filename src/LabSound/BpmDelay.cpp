// Copyright (c) 2014 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound.h"
#include "BpmDelay.h"

#include "AudioBus.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioProcessor.h"
#include "ExceptionCodes.h"
#include "VectorMath.h"
#include "DelayNode.h"

#include <iostream>
#include <vector>

using namespace WebCore;
using namespace WebCore::VectorMath;
using namespace std;

namespace LabSound {

	class BpmDelay::NodeInternal : public WebCore::AudioProcessor{
	public:

        NodeInternal(float sampleRate) : AudioProcessor(sampleRate), channels(2) {
			m_delayTime = std::make_shared<AudioParam>("delayTime", 1, 0.1, 24);

            ExceptionCode ec;
            m_delayNode = std::make_shared<DelayNode>(44100, 24.0, ec);
		}

		virtual ~NodeInternal() {
		}

		// AudioProcessor interface
		virtual void initialize() { }
		virtual void uninitialize() { }

		// Processes the source to destination bus.  The number of channels must match in source and destination.
		void process(ContextGraphLock& g, ContextRenderLock& r, const WebCore::AudioBus* sourceBus, WebCore::AudioBus* destinationBus, size_t framesToProcess) {

			perryVerb.setT60(m_delayTime->value(r.contextPtr()));

			const float *source = sourceBus->channel(0)->data();

			float* dLeft = destinationBus->channel(0)->mutableData();
			float* dRight = destinationBus->channel(1)->mutableData();

			for (size_t i = 0; i < framesToProcess; ++i) {
				float dL = perryVerb.tick((double)source[i], 0);
				float dR = perryVerb.tick((double)source[i], 1);
				*dLeft++ = dL;
				*dRight++ = dR;
			}

		}

		// Resets filter state
		virtual void reset() { }

		virtual void setNumberOfChannels(unsigned i) {
			channels = i;
		}

		virtual double tailTime() const { return 0; }
		virtual double latencyTime() const { return 0; }

		int channels;

		std::shared_ptr<AudioParam> m_delayTime;

		std::shared_ptr<DelayNode> m_delayNode; 

		stk::NRev perryVerb;

	};

    BpmDelay::BpmDelay(float sampleRate)
		: WebCore::AudioBasicProcessorNode(sampleRate),
		  data(new NodeInternal(sampleRate)) {
		
		m_processor = std::move(std::unique_ptr<WebCore::AudioProcessor>(data));

		setNodeType((AudioNode::NodeType) LabSound::NodeTypeUnknown);

        addInput(std::unique_ptr<AudioNodeInput>(new WebCore::AudioNodeInput(this)));
		addOutput(std::unique_ptr<AudioNodeOutput>(new WebCore::AudioNodeOutput(this, 1))); // 2 stereo

		initialize(); 
	}

	std::shared_ptr<AudioParam> BpmDelay::delayTime() const {
		return data->m_delayTime;
	}

	BpmDelay::~BpmDelay() {

		uninitialize();

	}

} // LabSound


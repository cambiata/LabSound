//
//  STKNode.h
//
// Copyright (c) 2014 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef __LabSound__STKNode__
#define __LabSound__STKNode__

#include "AudioContext.h"
#include "AudioNode.h"
#include "AudioParam.h"
#include "AudioNodeOutput.h"

#include "ADSRNode.h"

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconstant_conversion"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wdeprecated-register"
#endif

#include "STK/STKIncludes.h"

#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

#if OS(WINDOWS)
#include <direct.h>
#endif

namespace LabSound {

	using namespace WebCore;

	template <class InternalSTKType>
	class STKNode : public AudioScheduledSourceNode {

	public:

		STKNode(float sampleRate) : AudioScheduledSourceNode(sampleRate)  {

            static std::once_flag init;
            std::call_once(init, [sampleRate](){
                // Oops, Windows platform for now...
                // char cwd[MAX_PATH];
                // _getcwd(cwd, MAX_PATH);
                
                std::string resourcePath = std::string("stkresources\\");
                stk::Stk::setRawwavePath(resourcePath);
                
                std::cout << "\nSTK Resource Path: " << resourcePath << std::endl;
                
                stk::Stk::showWarnings(true);
                stk::Stk::printErrors(true);
                stk::Stk::setSampleRate(sampleRate);
            });
            
			addOutput(adoptPtr(new AudioNodeOutput(this, 1)));
			setNodeType((AudioNode::NodeType) LabSound::NodeTypeSTK);
			initialize();
		}

		~STKNode() { }

		void process(ContextGraphLock& g, ContextRenderLock&, size_t framesToProcess) override {

			// First output bus 
			AudioBus* outputBus = output(0)->bus();

			if (!isInitialized() || !outputBus->numberOfChannels()) {
				outputBus->zero();
				return;
			}

			size_t quantumFrameOffset;
			size_t nonSilentFramesToProcess;

			updateSchedulingInfo(framesToProcess, outputBus, quantumFrameOffset, nonSilentFramesToProcess);

			if (!nonSilentFramesToProcess) {
				outputBus->zero();
				return;
			}

			float* monoChannel = outputBus->channel(0)->mutableData();

			stk::StkFrames synthFrames(framesToProcess, 1);
			synth.tick(synthFrames);

			for (uint32_t i = 0; i < framesToProcess; ++i) {
				monoChannel[i] = synthFrames(i, 0);
			}

			outputBus->clearSilentFlag();

		}

		InternalSTKType getSynth() { return synth; }

	private:

		// Satisfy the AudioNode interface
		virtual void reset(std::shared_ptr<AudioContext>) override { /*m_currentSampleFrame = 0;*/ }

		// virtual double tailTime() const OVERRIDE { return 0; }
		// virtual double latencyTime() const OVERRIDE { return 0; }

		virtual bool propagatesSilence(double now) const OVERRIDE{
			return false;
		}

		InternalSTKType synth;

		stk::FileWvOut* waveOut;

	};
}

#endif

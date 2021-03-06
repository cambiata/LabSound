#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;
using namespace std;

#define USE_LIVE

// Send live audio to a Dalek filter, constructed according to
// the recipe at http://webaudio.prototyping.bbc.co.uk/ring-modulator/
int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    float sampleRate = context->sampleRate();

#ifndef USE_LIVE
    SoundBuffer sample(context, "human-voice.mp4");
#endif
    
    shared_ptr<MediaStreamAudioSourceNode> input;
    
    shared_ptr<OscillatorNode> vIn;
    shared_ptr<GainNode> vInGain;
    shared_ptr<GainNode> vInInverter1;
    shared_ptr<GainNode> vInInverter2;
    shared_ptr<GainNode> vInInverter3;
    shared_ptr<DiodeNode> vInDiode1;
    shared_ptr<DiodeNode> vInDiode2;
    shared_ptr<GainNode> vcInverter1;
    shared_ptr<DiodeNode> vcDiode3;
    shared_ptr<DiodeNode> vcDiode4;
    shared_ptr<GainNode> outGain;
    shared_ptr<DynamicsCompressorNode> compressor;
    std::shared_ptr<AudioBufferSourceNode> player;
    {
        ContextGraphLock g(context);
        ContextRenderLock r(context);
        
        vIn = make_shared<OscillatorNode>(r, sampleRate);
        vIn->frequency()->setValue(30.0f);
        vIn->start(r, 0);
        
        vInGain = make_shared<GainNode>(sampleRate);
        vInGain->gain()->setValue(0.5f);
        
        // GainNodes can take negative gain which represents phase inversion
        vInInverter1 = make_shared<GainNode>(sampleRate);
        vInInverter1->gain()->setValue(-1.0f);
        vInInverter2 = make_shared<GainNode>(sampleRate);
        vInInverter2->gain()->setValue(-1.0f);
        
        vInDiode1 = make_shared<DiodeNode>(r, sampleRate);
        vInDiode2 = make_shared<DiodeNode>(r, sampleRate);
        
        vInInverter3 = make_shared<GainNode>(sampleRate);
        vInInverter3->gain()->setValue(-1.0f);
        
        // Now we create the objects on the Vc side of the graph
#ifndef USE_LIVE
        player = sample.create();
#endif
        
        vcInverter1 = make_shared<GainNode>(sampleRate);
        vcInverter1->gain()->setValue(-1.0f);
        
        vcDiode3 = make_shared<DiodeNode>(r, sampleRate);
        vcDiode4 = make_shared<DiodeNode>(r, sampleRate);
        
        // A gain node to control master output levels
        outGain = make_shared<GainNode>(sampleRate);
        outGain->gain()->setValue(4.0f);
        
        // A small addition to the graph given in Parker's paper is a compressor node
        // immediately before the output. This ensures that the user's volume remains
        // somewhat constant when the distortion is increased.
        compressor = make_shared<DynamicsCompressorNode>(sampleRate);
        compressor->threshold()->setValue(-12.0f);
        
        // Now we connect up the graph following the block diagram above (on the web page).
        // When working on complex graphs it helps to have a pen and paper handy!
        
        // First the Vc side
#ifdef USE_LIVE
        input = context->createMediaStreamSource(g, r, ec);
        input->connect(g, r, vcInverter1.get(), 0, 0, ec);
        input->connect(g, r, vcDiode4->node().get(), 0, 0, ec);
#else
        player->connect(g, r, vcInverter1.get(), 0, 0, ec);
        player->connect(g, r, vcDiode4->node().get(), 0, 0, ec);
#endif
        
        vcInverter1->connect(g,r, vcDiode3->node().get(), 0, 0, ec);
        
        // Then the Vin side
        vIn->connect(g,r, vInGain.get(), 0, 0, ec);
        vInGain->connect(g,r, vInInverter1.get(), 0, 0, ec);
        vInGain->connect(g,r, vcInverter1.get(), 0, 0, ec);
        vInGain->connect(g,r, vcDiode4->node().get(), 0, 0, ec);
        
        vInInverter1->connect(g,r, vInInverter2.get(), 0, 0, ec);
        vInInverter1->connect(g,r, vInDiode2->node().get(), 0, 0, ec);
        vInInverter2->connect(g,r, vInDiode1->node().get(), 0, 0, ec);
        
        // Finally connect the four diodes to the destination via the output-stage compressor and master gain node
        vInDiode1->node()->connect(g,r, vInInverter3.get(), 0, 0, ec);
        vInDiode2->node()->connect(g,r, vInInverter3.get(), 0, 0, ec);
        
        vInInverter3->connect(g,r, compressor.get(), 0, 0, ec);
        vcDiode3->node()->connect(g,r, compressor.get(), 0, 0, ec);
        vcDiode4->node()->connect(g,r, compressor.get(), 0, 0, ec);
        
        compressor->connect(g,r, outGain.get(), 0, 0, ec);
        outGain->connect(g,r, context->destination().get(), 0, 0, ec);
        
#ifndef USE_LIVE
        player->start(r, 0);
#endif
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(30));
    LabSound::finish(context);
    return 0;
}
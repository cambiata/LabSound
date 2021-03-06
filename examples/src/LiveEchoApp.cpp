#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;

int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    shared_ptr<MediaStreamAudioSourceNode> input;

    {
        ContextGraphLock g(context);
        ContextRenderLock r(context);
        input = context->createMediaStreamSource(ec);
        input->connect(g, r, context->destination().get(), 0, 0, ec);
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(10));
    LabSound::finish(context);
    
    return 0;
}

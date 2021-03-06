
/// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef LabSound_src_DiodeNode_h
#define LabSound_src_DiodeNode_h

#include "WaveShaperNode.h"

namespace LabSound {

    /// @TODO DiodeNode should subclass waveShaper, then the create method will work
    
    class DiodeNode
    {
    public:
        DiodeNode(ContextRenderLock& r, float sampleRate);
        void setDistortion(ContextRenderLock& r, float distortion);
        std::shared_ptr<WebCore::WaveShaperNode> node() const { return waveShaper; }

    private:
        void setCurve(ContextRenderLock&);

        std::shared_ptr<WebCore::WaveShaperNode> waveShaper;
        
        // parameters controlling the shape of the curve
        float vb;
        float vl;
        float h;
    };
    
}

#endif

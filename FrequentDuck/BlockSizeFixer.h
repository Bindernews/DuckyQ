#pragma once

#include <functional>
#include "heapbuf.h"
#include "IPlugConstants.h"
#include <vector>

using namespace iplug;

class BlockSizeFixer
{
public:
    using ProcessFixedT = std::function<void(sample** inputs, sample** outputs, int nFrames)>;

    BlockSizeFixer(int blockSize, int latency, int inputChannels, int outputChannels);
    ~BlockSizeFixer() {}

    void process(sample** inputs, sample** outputs, int inChans, int outChans, int nFrames);

    void reset();

    void setCallback(ProcessFixedT cb)
    {
        m_processFixed = cb;
    }


private:
    ProcessFixedT m_processFixed;
    std::vector<WDL_TypedBuf<sample>> m_ioBufs;
    std::vector<sample*> m_ioBufPtrs;
    int m_inChans;
    int m_outChans;
    int m_blockSize;
    int m_latency;
    int m_inPos;
    int m_outPos;
};


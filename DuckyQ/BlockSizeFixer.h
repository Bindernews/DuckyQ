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


    void setBlockSize(int blockSize);
    void setLatency(int latency);

    bool isNewBlock() const;
    

private:
    ProcessFixedT m_processFixed;
    std::vector<WDL_TypedBuf<sample>> m_ioBufs;
    std::vector<sample*> m_ioBufPtrs;
    /** Number of bytes in the full block, ignoring latency. */
    int m_blockBytes;
    int m_inChans;
    int m_outChans;
    int m_blockSize;
    int m_latency;
    int m_inPos;
    int m_outPos;
};


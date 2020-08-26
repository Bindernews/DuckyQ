#include "BlockSizeFixer.h"

BlockSizeFixer::BlockSizeFixer(int blockSize, int latency, int inputChannels, int outputChannels)
    : m_blockSize(blockSize), m_latency(latency), m_inChans(inputChannels), m_outChans(outputChannels)
{
    m_blockBytes = 0;
    m_inPos = 0;
    m_outPos = 0;

    for (int i = 0; i < m_inChans + m_outChans; i++) {
        WDL_TypedBuf<sample> tb;
        tb.Resize(m_blockSize);
        memset(tb.Get(), 0, tb.GetSize() * sizeof(sample));
        m_ioBufs.push_back(tb);
    }
}

void BlockSizeFixer::process(sample** inputs, sample** outputs, int inChans, int outChans, int nFrames)
{
    int inOffset = 0;
    int outOffset = 0;

    while (inOffset < nFrames) {

        // Copy output buffer to real output
        const size_t szOut = std::min(nFrames - outOffset, m_blockSize - m_outPos);
        for (int c = 0; c < outChans; c++) {
            memcpy(outputs[c] + outOffset, m_ioBufs[c + m_inChans].Get() + m_outPos, szOut * sizeof(sample));
        }
        outOffset += szOut;
        m_outPos += szOut;

        // Copy input buffer to real input
        const size_t szIn = std::min(nFrames - inOffset, m_blockSize - m_inPos);
        for (int c = 0; c < inChans; c++) {
            memcpy(m_ioBufs[c].Get() + m_inPos, inputs[c] + inOffset, szIn * sizeof(sample));
        }
        inOffset += szIn;
        m_inPos += szIn;
        m_blockBytes += szIn;

        // Check if we've reached our fixed block size.
        if (m_inPos == m_blockSize) {
            assert(m_outPos == m_blockSize);

            m_ioBufPtrs.clear();
            for (int i = 0; i < inChans; i++) {
                m_ioBufPtrs.push_back(m_ioBufs[i].Get());
            }
            for (int i = 0; i < outChans; i++) {
                m_ioBufPtrs.push_back(m_ioBufs[i + m_inChans].Get());
            }

            // Call the real processing function.
            m_processFixed(m_ioBufPtrs.data(), m_ioBufPtrs.data() + inChans, m_blockSize);

            // Move data backwards so we can decrease latency.
            const size_t moveSize = (m_blockSize - m_latency) * sizeof(sample);
            for (int c = 0; c < m_ioBufPtrs.size(); c++) {
                auto buf = m_ioBufPtrs[c];
                memmove(buf, buf + m_latency, moveSize);
            }

            m_inPos = m_blockSize - m_latency;
            m_outPos = m_inPos;
            
            // Update our block byte counter
            m_blockBytes = m_blockBytes % m_blockSize;
        }
    }
}

void BlockSizeFixer::reset()
{
    for (int i = 0; i < m_ioBufs.size(); i++) {
        memset(m_ioBufs[i].Get(), 0, m_ioBufs[i].GetSize() * sizeof(sample));
    }
    m_inPos = 0;
    m_outPos = 0;
}

bool BlockSizeFixer::isNewBlock() const
{
    return m_blockBytes >= m_blockSize;
}

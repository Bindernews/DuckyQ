#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "fft.h"

const int kNumPresets = 1;
const int kNumFFTBuffers = 4;
#define FFT_BLOCK_SIZE (1024)
#define MAX_INPUT_CHANS (4)
#define MAX_OUTPUT_CHANS (2)

enum EParams
{
    kGain = 0,
    kNumParams
};



using namespace iplug;
using namespace igraphics;

class FrequentDuck final : public Plugin
{
public:
    FrequentDuck(const InstanceInfo& info);

#if IPLUG_EDITOR
    void buildGui(IGraphics& ui);
#endif

    // Common
    void OnReset() override;
    //int GetLatency() const override;

#if IPLUG_DSP // http://bit.ly/2S64BDd
    void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
    void ProcessFFT();

    WDL_TypedBuf<WDL_FFT_COMPLEX> m_fftBuf[kNumFFTBuffers];
    WDL_TypedBuf<sample> m_outputBuf[MAX_OUTPUT_CHANS];
    int m_fftPos;
    /** Index in m_outputBuf corresponding to index 0 in outputs */
    int m_outPos;

    int m_latency;
#endif
};

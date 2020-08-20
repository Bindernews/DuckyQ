#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "BlockSizeFixer.h"
#include "fft.h"
#include "ISender.h"

const int kNumPresets = 1;

enum EControls
{
    kCtrlPanel = 0,
    kCtrlEqMain,
    kNumControls,
};

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
    void OnIdle() override;
#endif

    // Common
    void OnReset() override;
    //int GetLatency() const override;

#if IPLUG_DSP // http://bit.ly/2S64BDd
    void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
    void ProcessFFT(sample** inputs, sample** outputs, int nFrames);

    WDL_TypedBuf<WDL_FFT_COMPLEX> m_fftBuf[MAX_INPUT_CHANS];
    BlockSizeFixer m_blockFixer;

    using SendArrayT = std::array<WDL_FFT_COMPLEX, FFT_BLOCK_SIZE>;
    //using SendArrayT = std::array<float, GUI_FREQ_BUF_SIZE>;
    ISenderData<MAX_OUTPUT_CHANS, SendArrayT> m_senderData;
    ISender<MAX_OUTPUT_CHANS, 64, SendArrayT> m_sender;
#endif
};

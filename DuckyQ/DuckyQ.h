#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "BlockSizeFixer.h"
#include "fft.h"
#include "qmsg.h"

const int kNumPresets = 1;

enum EControls
{
    kCtrlPanel = 0,
    kCtrlEqMain,
    kCtrlBand0 = 100,
    kNumControls,
};

enum EMessage
{
    kMsgDSP = 100,
    kMsgdFFTSize,
    kMsgGUI = 200,
    kMsggSampleRate,
};

enum EParams
{
    kGain = 0,
    kNumParams
};

using namespace iplug;
using namespace igraphics;

class PlugGUI;

class DuckyQ final : public Plugin
{
public:
    DuckyQ(const InstanceInfo& info);
    ~DuckyQ();

    bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* data) override;

#if IPLUG_EDITOR
    PlugGUI* m_gui;
#endif

#if IPLUG_DSP // http://bit.ly/2S64BDd
public:
    void OnReset() override;
    void OnIdle() override;
    void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;

    void ProcessFFT(sample** inputs, sample** outputs, int nFrames);
    void setFFTSize(int fftSize);
    void copyToFFT(WDL_FFT_COMPLEX* dst, const sample* src, int count);

    WDL_TypedBuf<WDL_FFT_COMPLEX> m_fftBuf[MAX_INPUT_CHANS];
    BlockSizeFixer m_blockFixer;
    bool m_hasSidechain;
    int m_fftBlockSize;
    bn::QMsgMut m_graphMessage;
    bn::QMsgSender m_sender;
#endif
};

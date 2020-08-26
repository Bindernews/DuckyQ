#include "DuckyQ.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "EqGraph.h"


#define ZERO_WDL_BUF(buf, type_size) memset((buf).Get(), 0, (buf).GetSize() * (type_size))

DuckyQ::DuckyQ(const InstanceInfo& info)
    : Plugin(info, MakeConfig(kNumParams, kNumPresets))
#if IPLUG_DSP
    , m_blockFixer(FFT_BLOCK_SIZE, PLUG_LATENCY, MAX_INPUT_CHANS, MAX_OUTPUT_CHANS)
    , m_senderData(kCtrlEqMain, 2, 0)
#endif
{
    GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
    mMakeGraphicsFunc = [&]() {
        return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
    };
  
    mLayoutFunc = [&](IGraphics* pGraphics) { buildGui(*pGraphics); };
#endif

#if IPLUG_DSP
    WDL_fft_init();

    // Resize our buffers.
    for (int i = 0; i < MAX_INPUT_CHANS; i++) {
        m_fftBuf[i].Resize(FFT_BLOCK_SIZE);
    }

    m_blockFixer.setCallback([=](sample** inputs, sample** outputs, int nFrames) {
        this->ProcessFFT(inputs, outputs, nFrames);
    });
    
    m_hasSidechain = false;
#endif
}

#if IPLUG_EDITOR
void DuckyQ::buildGui(IGraphics& ui)
{
    ui.AttachCornerResizer(EUIResizerMode::Scale, false);
    ui.AttachPanelBackground(COLOR_GRAY);
    ui.LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = ui.GetBounds();
    ui.AttachControl(new ITextControl(b.GetMidVPadded(50), "Hello iPlug 2!", IText(50)));
    ui.AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain));
    //ui.AttachControl(new EqGraph<GUI_FREQ_BUF_SIZE>(IRECT(50, 400, b.R - 50, 580)), kCtrlEqMain);
    ui.AttachControl(new EqGraph<FFT_BLOCK_SIZE>(IRECT(50, 400, b.R - 50, 580)), kCtrlEqMain);
}
#endif

void DuckyQ::OnReset()
{
#if IPLUG_DSP
    for (int i = 0; i < MAX_INPUT_CHANS; i++) {
        ZERO_WDL_BUF(m_fftBuf[i], sizeof(WDL_FFT_COMPLEX));
    }
    m_blockFixer.reset();
    
    m_hasSidechain = NOutChansConnected() > NInChansConnected();
    
    //DBGMSG("IO Config: In: %d  Out: %d\n", NInChansConnected(), NOutChansConnected());
#endif
}

#if IPLUG_DSP
void DuckyQ::OnIdle()
{
    m_sender.TransmitData(*this);
}

void DuckyQ::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
    const int nChansIn = NInChansConnected();
    const int nChansOut = NOutChansConnected();

    m_blockFixer.process(inputs, outputs, nChansIn, nChansOut, nFrames);
}

inline static float signf(float v)
{
}

void DuckyQ::ProcessFFT(sample** inputs, sample** outputs, int nFrames)
{
    const int nChansOut = NOutChansConnected();

    // Reduce volume of main inputs by volume of side inputs
    // Index of the side input is main input index + nChansOut
    for (int c = 0; c < nChansOut; c++) {
        auto inMain = inputs[c];
        auto bufMain = m_fftBuf[c].Get();
        copyToFFT(bufMain, inMain, FFT_BLOCK_SIZE);
        WDL_fft(bufMain, FFT_BLOCK_SIZE, false);
        
        memcpy(m_senderData.vals[c].data(), bufMain, FFT_BLOCK_SIZE * sizeof(WDL_FFT_COMPLEX));
        
        // Only do sidechain if it's enabled
        if (m_hasSidechain) {
            auto inSide = inputs[c + nChansOut];
            auto bufSide = m_fftBuf[c + nChansOut].Get();
            copyToFFT(bufSide, inSide, FFT_BLOCK_SIZE);
            WDL_fft(bufSide, FFT_BLOCK_SIZE, false);
            
            #define TMP(m, s) (m) = signf(m) * (abs(m) - std::min(abs(s), abs(m)))
            for (int i = 0; i < FFT_BLOCK_SIZE; i++) {
                TMP((bufMain[i].re), (bufSide[i].re));
                TMP((bufMain[i].im), (bufSide[i].im));
            }
            #undef TMP
        }
        
        // Send freq data to the GUI
        //memcpy(m_senderData.vals[c].data(), bufMain, FFT_BLOCK_SIZE * sizeof(WDL_FFT_COMPLEX));

        // Reverse the process and write to output buf.
        auto bufOut = outputs[c];
        WDL_fft(bufMain, FFT_BLOCK_SIZE, true);
        for (int i = 0; i < FFT_BLOCK_SIZE; i++) {
            bufOut[i] = (sample)(bufMain[i].re);
        }
    }
    
    if (m_blockFixer.isNewBlock() || true) {
        m_sender.PushData(m_senderData);
    }
}

void DuckyQ::copyToFFT(WDL_FFT_COMPLEX *dst, const sample *src, int count)
{
    const float normVal = 1.f / (float)count;
    for (int i = 0; i < FFT_BLOCK_SIZE; i++) {
        dst[i].re = (float)(src[i] * normVal);
        dst[i].im = 0.f;
    }
}

#endif

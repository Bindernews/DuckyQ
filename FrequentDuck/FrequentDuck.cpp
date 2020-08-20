#include "FrequentDuck.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "EqGraph.h"

#define ZERO_WDL_BUF(buf, type_size) memset((buf).Get(), 0, (buf).GetSize() * (type_size))

FrequentDuck::FrequentDuck(const InstanceInfo& info)
    : Plugin(info, MakeConfig(kNumParams, kNumPresets)),
    m_blockFixer(FFT_BLOCK_SIZE, PLUG_LATENCY, MAX_INPUT_CHANS, MAX_OUTPUT_CHANS)
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
#endif
}

#if IPLUG_EDITOR
void FrequentDuck::buildGui(IGraphics& ui)
{
    ui.AttachCornerResizer(EUIResizerMode::Scale, false);
    ui.AttachPanelBackground(COLOR_GRAY);
    ui.LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = ui.GetBounds();
    ui.AttachControl(new ITextControl(b.GetMidVPadded(50), "Hello iPlug 2!", IText(50)));
    ui.AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain));
}
#endif

void FrequentDuck::OnReset()
{
#if IPLUG_DSP
    for (int i = 0; i < MAX_INPUT_CHANS; i++) {
        ZERO_WDL_BUF(m_fftBuf[i], sizeof(WDL_FFT_COMPLEX));
    }
    m_blockFixer.reset();
#endif

}

#if IPLUG_DSP
void FrequentDuck::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
    const int nChansIn = NInChansConnected();
    const int nChansOut = NOutChansConnected();

    m_blockFixer.process(inputs, outputs, nChansIn, nChansOut, nFrames);
}

float signf(float f)
{
    if (f < 0.f) { 
        return -1.f;
    }
    if (f > 0.f) {
        return  1.f;
    }
    return 0.f;
}

void FrequentDuck::ProcessFFT(sample** inputs, sample** outputs, int nFrames)
{
    const int nChansOut = NOutChansConnected();
    const float normVal = 1.0f / FFT_BLOCK_SIZE;

    // Reduce volume of main inputs by volume of side inputs
    // Index of the side input is main input index + nChansOut
    for (int c = 0; c < nChansOut; c++) {
        auto inMain = inputs[c];
        auto inSide = inputs[c + nChansOut];
        auto bufMain = m_fftBuf[c].Get();
        auto bufSide = m_fftBuf[c + nChansOut].Get();
        auto bufOut = outputs[c];

        for (int i = 0; i < FFT_BLOCK_SIZE; i++) {
            bufMain[i].re = (float)(inMain[i] * normVal);
            bufMain[i].im = 0.f;
            bufSide[i].re = (float)(inSide[i] * normVal);
            bufSide[i].im = 0.f;
        }

        WDL_fft(bufMain, FFT_BLOCK_SIZE, false);
        WDL_fft(bufSide, FFT_BLOCK_SIZE, false);
        
#define TMP(m, s) (m) = signf(m) * (abs(m) - std::min(abs(s), abs(m)))
//#define TMP(m, s) m = abs(m);
        for (int i = 0; i < FFT_BLOCK_SIZE; i++) {
            TMP((bufMain[i].re), (bufSide[i].re));
            TMP((bufMain[i].im), (bufSide[i].im));
            //bufMain[i].re *= 0.1;
            //bufMain[i].im *= 0.1;
        }
#undef TMP

        // Reverse the process and write to output buf.
        WDL_fft(bufMain, FFT_BLOCK_SIZE, true);
        for (int i = 0; i < FFT_BLOCK_SIZE; i++) {
            bufOut[i] = (sample)(bufMain[i].re);
        }
    }
}
#endif

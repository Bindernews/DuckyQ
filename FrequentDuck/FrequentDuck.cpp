#include "FrequentDuck.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

#define ZERO_WDL_BUF(buf, type_size) memset((buf).Get(), 0, (buf).GetSize() * (type_size))

FrequentDuck::FrequentDuck(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
    m_latency = PLUG_LATENCY;

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
    for (int i = 0; i < kNumFFTBuffers; i++) {
        m_fftBuf[i].Resize(FFT_BLOCK_SIZE);
    }
    for (int i = 0; i < MAX_OUTPUT_CHANS; i++) {
        m_outputBuf[i].Resize(FFT_BLOCK_SIZE);
    }
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
    for (int i = 0; i < kNumFFTBuffers; i++) {
        ZERO_WDL_BUF(m_fftBuf[i], sizeof(WDL_FFT_COMPLEX));
    }
    for (int i = 0; i < MAX_OUTPUT_CHANS; i++) {
        ZERO_WDL_BUF(m_outputBuf[i], sizeof(sample));
    }
    m_fftPos = FFT_BLOCK_SIZE - m_latency;
    m_outPos = FFT_BLOCK_SIZE - m_latency;
#endif

}

#if IPLUG_DSP
void FrequentDuck::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
    const int nChansIn = NInChansConnected();
    const int nChansOut = NOutChansConnected();
    const float normVal = 1.0f / FFT_BLOCK_SIZE;
    
    int inOffset = 0;
    int outOffset = 0;

    const auto copyOutput = [&]() {
        const int loopFrames = std::min(nFrames - outOffset, FFT_BLOCK_SIZE - m_outPos);
        for (int c = 0; c < nChansOut; c++) {
            auto dst = outputs[c] + outOffset;
            auto src = m_outputBuf[c].Get() + m_outPos;
            for (int i = 0; i < loopFrames; i++) {
                dst[i] = src[i];
            }
        }
        outOffset += loopFrames;
        m_outPos += loopFrames;
    };

    // Zero output
    for (int c = 0; c < nChansOut; c++) {
        for (int i = 0; i < nFrames; i++) {
            outputs[c][i] = 0;
        }
    }

    // Initial output copy
    copyOutput();

    // Copy inputs into FFT buffer and potentially process FFT samples
    while (inOffset < nFrames) {
        
        // Try to fill the FFT input buffers.
        const int loopFrames = std::min(nFrames - inOffset, FFT_BLOCK_SIZE - m_fftPos);
        for (int c = 0; c < nChansIn; c++) {
            auto buf = m_fftBuf[c].Get() + m_fftPos;
            for (int i = 0; i < loopFrames; i++) {
                buf[i].re = inputs[c][i] * normVal;
                buf[i].im = 0;
            }
        }
        
        // If we've filled the FFT buffer perform our FFT and do output.
        // This may happen multiple times if nFrames > FFT_BLOCK_SIZE.
        if (m_fftPos == FFT_BLOCK_SIZE) {
            ProcessFFT();
            copyOutput();
        }

        inOffset += loopFrames;
        m_fftPos += loopFrames;
    }

    outOffset++;
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

void FrequentDuck::ProcessFFT()
{
    const int nChansOut = NOutChansConnected();
    const int latencyInv = FFT_BLOCK_SIZE - m_latency;

    assert(m_outPos == FFT_BLOCK_SIZE);

    // Reduce volume of main inputs by volume of side inputs
    // Index of the side input is main input index + nChansOut
    for (int c = 0; c < nChansOut; c++) {
        auto bufMain = m_fftBuf[c].Get();
        auto bufSide = m_fftBuf[c + nChansOut].Get();
        auto bufOut = m_outputBuf[c].Get();

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

        memmove(bufMain, (void*)(bufMain + m_latency), latencyInv * sizeof(WDL_FFT_COMPLEX));
        memmove(bufSide, (void*)(bufSide + m_latency), latencyInv * sizeof(WDL_FFT_COMPLEX));
    }

    // Reset our output buffer positions and move the buffers backwards
    m_outPos = latencyInv;
    m_fftPos = latencyInv;
}
#endif

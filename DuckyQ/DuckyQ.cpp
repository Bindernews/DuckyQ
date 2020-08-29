#include "DuckyQ.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "gui.h"
#endif

#define ZERO_WDL_BUF(buf, type_size) memset((buf).Get(), 0, (buf).GetSize() * (type_size))

DuckyQ::DuckyQ(const InstanceInfo& info)
    : Plugin(info, MakeConfig(kNumParams, kNumPresets))
#if IPLUG_DSP
    , m_blockFixer(FFT_BLOCK_SIZE, PLUG_LATENCY, MAX_INPUT_CHANS, MAX_OUTPUT_CHANS)
    , m_hasSidechain(false)
    , m_fftBlockSize(0)
    , m_graphMessage(kCtrlEqMain, 0)
    , m_sender(64)
#endif
{
    GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
    m_gui = new PlugGUI(*this);

    mMakeGraphicsFunc = [&]() {
        return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
    };
  
    mLayoutFunc = [&](IGraphics* pGraphics) { m_gui->buildGui(*pGraphics); };
#endif

#if IPLUG_DSP
    // Setup any extra variables that we can't setup in the init section.

    // Init FFT data
    WDL_fft_init();
    // Setup FFT buffers
    setFFTSize(FFT_BLOCK_SIZE);

    m_blockFixer.setCallback([=](sample** inputs, sample** outputs, int nFrames) {
        this->ProcessFFT(inputs, outputs, nFrames);
    });
#endif
}

DuckyQ::~DuckyQ()
{
#if IPLUG_EDITOR
    delete m_gui;
#endif
}

bool DuckyQ::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* data)
{
    bn::QMsg msg{ data, dataSize };

#if IPLUG_DSP
    switch (msgTag) {
    case kMsgdFFTSize:
    {
        setFFTSize(msg.buffer<int>()[0]);
        return true;
    }
    }
#endif

#if IPLUG_EDITOR
    if (m_gui->OnMessage(msgTag, ctrlTag, msg)) {
        return true;
    }
#endif
    return false;
}

#if IPLUG_DSP
void DuckyQ::OnReset()
{
    for (int i = 0; i < MAX_INPUT_CHANS; i++) {
        m_fftBuf[i].Resize(m_fftBlockSize);
        ZERO_WDL_BUF(m_fftBuf[i], sizeof(WDL_FFT_COMPLEX));
    }
    m_blockFixer.reset();
    m_graphMessage.resize(sizeof(WDL_FFT_COMPLEX) * m_fftBlockSize * NOutChansConnected());

    m_hasSidechain = NOutChansConnected() > NInChansConnected();

    DBGMSG("IO Config: In: %d  Out: %d\n", NInChansConnected(), NOutChansConnected());
}

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
    return std::signbit(v) ? -1.f : 1.f;
}

void DuckyQ::ProcessFFT(sample** inputs, sample** outputs, int nFrames)
{
    const int nChansOut = NOutChansConnected();
    const int bufferSize = m_fftBlockSize * sizeof(WDL_FFT_COMPLEX);

    // Make sure we have enough space in our graph message buffer.
    // If the buffer is already large enough, nothing will happen.
    m_graphMessage.resize(bufferSize * nChansOut);

    // Reduce volume of main inputs by volume of side inputs
    // Index of the side input is main input index + nChansOut
    for (int c = 0; c < nChansOut; c++) {
        auto inMain = inputs[c];
        auto bufMain = m_fftBuf[c].Get();
        copyToFFT(bufMain, inMain, m_fftBlockSize);
        WDL_fft(bufMain, m_fftBlockSize, false);
        
        // Only do sidechain if it's enabled
        if (m_hasSidechain) {
            auto inSide = inputs[c + nChansOut];
            auto bufSide = m_fftBuf[c + nChansOut].Get();
            copyToFFT(bufSide, inSide, m_fftBlockSize);
            WDL_fft(bufSide, m_fftBlockSize, false);
            
            #define TMP(m, s) (m) = signf(m) * (abs(m) - std::min(abs(s), abs(m)))
            for (int i = 0; i < FFT_BLOCK_SIZE; i++) {
                TMP((bufMain[i].re), (bufSide[i].re));
                TMP((bufMain[i].im), (bufSide[i].im));
            }
            #undef TMP
        }
        
        // Send freq data to the GUI
        memcpy(m_graphMessage.buffer<uint8_t>() + (bufferSize * c), bufMain, bufferSize);

        // Reverse the process and write to output buf.
        auto bufOut = outputs[c];
        WDL_fft(bufMain, m_fftBlockSize, true);
        for (int i = 0; i < FFT_BLOCK_SIZE; i++) {
            bufOut[i] = (sample)(bufMain[i].re);
        }
    }
    
    if (m_blockFixer.isNewBlock() || true) {
        m_sender.PushData(m_graphMessage);
    }
}

void DuckyQ::setFFTSize(int fftSize)
{
    m_fftBlockSize = fftSize;
    m_blockFixer.setBlockSize(fftSize);
    m_graphMessage.resize(sizeof(WDL_FFT_COMPLEX) * m_fftBlockSize * NOutChansConnected());

    // Resize buffers.
    for (int i = 0; i < MAX_INPUT_CHANS; i++) {
        m_fftBuf[i].Resize(m_fftBlockSize);
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

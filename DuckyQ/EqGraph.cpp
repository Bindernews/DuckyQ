#include "EqGraph.h"
#include "ISender.h"

EqGraph::EqGraph(const IRECT& bounds, int baseTag, const IVStyle& style)
    : IControl(bounds, SplashClickActionFunc), IVectorBase(style)
{
    AttachIControl(this, "");
    mWidgetBounds = mRECT;
    m_sampleRate = 48000;
    m_fftSize = FFT_BLOCK_SIZE;
    mStyle.labelText.mSize = 12.f;
    mStyle.labelText.mAlign = EAlign::Near;
}

void EqGraph::Draw(IGraphics& g)
{
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
}

void EqGraph::DrawWidget(IGraphics& g)
{
    // Draw dark background
    g.DrawRect(COLOR_GRAY, mWidgetBounds);

    DrawVerticalLines(g);
    DrawGraph(g);
}

void EqGraph::OnMsgFromDelegate(int msgTag, int dataSize, const void* pData)
{
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
        m_buf.copyMsg(bn::QMsg(pData, dataSize));
        SetDirty(false);
    }
}

void EqGraph::DrawVerticalLines(IGraphics& g)
{
    // Draw vertical lines
    WDL_String text;

    const IRECT& b = mWidgetBounds;
    IRECT textSize;
    g.MeasureText(mStyle.labelText, "TESHz", textSize);
    IRECT tmpR;

    const double hsrate = m_sampleRate / 2.0;
    const float hzToFFTIndex = /* hz * */ ((float)m_fftSize / 2.f) / hsrate;
    double f = 20;
    double lx = mWidgetBounds.L;
    float nextTextX = b.L + textSize.W();

    while (f < hsrate) {
        float dx = Lerp(b.L, b.R, binToX(f * hzToFFTIndex));

        bool doText = (dx > nextTextX) && (f != 40) && (f != 4000) && (f != 15000)
            && (f < 400 || f >= 1000 || f == 500) && (f < 6000 || f >= 10000);

        if (dx > lx) {
            lx = dx + 3;
            g.PathLine(dx, b.T + (doText ? 0 : textSize.H() + 2.f), dx, b.B);
        }
        if (doText) {
            if (f >= 1000) {
                text.SetFormatted(10, "%dk", (int)(f * 0.001));
            }
            else {
                text.SetFormatted(10, "%d", (int)f);
            }
            nextTextX = dx + g.MeasureText(mStyle.labelText, text.Get(), tmpR) + 4.f;
            g.DrawText(mStyle.labelText, text.Get(), dx + 3, b.T);
        }

        // Increment f appropriately
        if (f < 100) {
            f += 10;
        }
        else if (f < 1000) {
            f += 100;
        }
        else if (f < 10000) {
            f += 1000;
        }
        else {
            f += 5000;
        }
    }
    g.PathStroke(IPattern(COLOR_WHITE), 1.0f);
    g.PathClear();
}

void EqGraph::DrawGraph(IGraphics& g)
{
    const int* permute_tab = WDL_fft_permute_tab(FFT_BLOCK_SIZE);
    const IRECT& b = mWidgetBounds;
    const IVec2 orig = { mWidgetBounds.L, mWidgetBounds.B };
    const float MIN_DB = -120.f;

    // hz = i * sample_rate / fft_size
    // https://stackoverflow.com/questions/4364823/how-do-i-obtain-the-frequencies-of-each-value-in-an-fft

    // To convert a complex FFT value into a value in the range [0,1] use the
    // following formula:
    // f(x) = 1 + (20 * log10( sqrt(square(x.real) + square(x.imag)) ) / (-MIN_DB)
    //   - MIN_DB = decibel value at 0
    //   - 20 * log10(x) is the formula to convert linear amplitude to decibels
    //   - By law of logs we can move the sqrt out of the log
    //     f(x) = 1 + (20 * (log10(...) * 0.5) ) / (-MIN_DB)
    //   - Adding 1 moves the result from range [-1, 0] to [0, 1]

    // For each channel, draw a line for the frequency
    for (int c = 0; c < 1; /*m_buf.nChans;*/ c++) {
        float* buf = m_buf.buffer<float>();

        g.PathMoveTo(orig.x, orig.y);
        for (int i = 0; i < m_fftSize / 2; i++) {
            int ix = permute_tab[i] * 2;
            float db = 1.f + (20.f * std::log10(sqr(buf[ix + 0]) + sqr(buf[ix + 1])) * 0.5f) / (-MIN_DB);
            float ty = Clip(0.f, 1.f, db) * b.H();
            //float tx = ((float)i / ((float)m_fftSize / 2.f)) * b.W();
            float tx = binToX((float)i) * b.W();

            g.PathLineTo(b.L + tx, b.B - ty);
        }
        g.PathStroke(IPattern(COLOR_RED), 1.f);
        g.PathClear();
    }
}

float EqGraph::binToX(float fftBin)
{
    static const float LOG_401 = std::log(401.f);

    const float xscale = 800.f / (m_fftSize - 4);
    return std::log(1.f + fftBin * xscale) / LOG_401;
}


EqBand::EqBand(const IRECT& bounds, int parentTag, int baseParam, IColor bandColor, const char* label)
    : IControl(bounds, { baseParam + 0, baseParam + 1, baseParam + 2, baseParam + 3, baseParam + 4 }, nullptr)
    , IVectorBase(DEFAULT_STYLE)
    , m_parentTag(parentTag)
    , m_baseParam(baseParam)
    , m_radius(1.f)
{
    AttachIControl(this, label);
    SetBandColor(bandColor);
    mWidgetBounds = mRECT.GetPadded(-2.f);
    
    mStyle.labelText = IText(12.f, EAlign::Center);
    mStyle.labelText.mVAlign = EVAlign::Middle;
}

void EqBand::Draw(IGraphics& g)
{
    const IRECT& b = mWidgetBounds;
    const IVec2 c{ b.MW(), b.MH() };
    const float hsize = b.W() / 2.f;
    const float rradius = m_radius * hsize;
    const float thickness = 2.f;

    //double vlevel = GetValue(0);
    //double vfreq = GetValue(1);
    double vwidth = GetValue(2);
    double vtype = GetValue(3);
    double vorder = GetValue(4);

    // Draw bg and outer circle
    g.FillCircle(m_colors[1], c.x, c.y, rradius, &GetBlend());
    g.DrawCircle(COLOR_BLACK, c.x, c.y, rradius, &GetBlend(), thickness);
    // Draw width arc. Half of the width to either side.
    float arcLen = vwidth * 360.f / 2.f;
    g.DrawArc(m_colors[0], c.x, c.y, rradius * 0.9f, arcLen, 360.f - arcLen, &GetBlend(), thickness);
    // Draw label
    g.DrawText(mStyle.labelText, mLabelStr.Get(), c.x, c.y, &GetBlend());
}

void EqBand::OnResize()
{
    mWidgetBounds = mRECT.GetPadded(-2.f);
}

void EqBand::SetBandColor(IColor color)
{
    m_colors[0] = color;
    m_colors[1] = color;
    m_colors[1].SetOpacity(0.6f);
    mStyle.labelText.mFGColor = color;
}

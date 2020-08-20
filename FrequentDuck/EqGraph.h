#pragma once

#include <array>
#include "IControl.h"
#include "ISender.h"
#include "config.h"
#include "fft.h"

using namespace iplug;
using namespace iplug::igraphics;

template<int BUFSIZE=128>
class EqGraph : public IControl,
                public IVectorBase
{
public:
    static const int MIN_HZ = 20;
    static const int MAX_HZ = 22000;

    EqGraph(const IRECT& bounds, const IVStyle& style = DEFAULT_STYLE)
        : IControl(bounds, SplashClickActionFunc), IVectorBase(style)
    {
        AttachIControl(this, "");
        mWidgetBounds = mRECT;
        m_sampleRate = 48000;
        mStyle.labelText.mSize = 12.f;
        mStyle.labelText.mAlign = EAlign::Near;
    }

    void Draw(IGraphics& g) override
    {
        DrawBackground(g, mRECT);
        DrawWidget(g);
        DrawLabel(g);
    }

    void DrawWidget(IGraphics& g) override
    {
        // Draw dark background
        g.DrawRect(COLOR_GRAY, mWidgetBounds);

        DrawVerticalLines(g);

        const IRECT& b = mWidgetBounds;
        const float slider2 = -120;
        const float incX = mWidgetBounds.W() / (float)BUFSIZE;
        const float amplitude = (mWidgetBounds.H() - 10.f) * 10;
        const IVec2 orig = { mWidgetBounds.L, mWidgetBounds.B };
        const float minY = powf(10, (-500.f / 20.f * 2.f));
        const double wsc = b.W() / logf(401.f);
        const double sc = (b.H() - 20) * 20 / (-slider2 * logf(10));
        const float xscale = 1.f;

        // hz = i * sample_rate / fft_size
        // https://stackoverflow.com/questions/4364823/how-do-i-obtain-the-frequencies-of-each-value-in-an-fft
        const float hzMul = m_sampleRate / BUFSIZE;
        // Float-version of BUFSIZE
        const float bufsizef = (float)BUFSIZE;

        // For each channel, draw a line for the frequency
        for (int c = 0; c < m_buf.nChans; c++) {
            float* buf = reinterpret_cast<float*>(m_buf.vals[c].data());
            g.PathMoveTo(orig.x, orig.y);

            for (int i = 0; i < BUFSIZE; i++) {
                float ty = logf(std::max(sqr(buf[0]) + sqr(buf[1]), minY));
                buf += 2;

                float hz = (float)i * hzMul;

                ty = ty * -0.5 * sc + 20;
                ty = std::min(ty, b.H());
                float tx = logf(1.f + i) * wsc;
                //float tx = Lerp(b.L, b.R, (float)i / bufsizef);

                g.PathLineTo(tx + b.L, b.T + ty);
            }

            //for (int i = 0; i < BUFSIZE; i++) {
            //    g.PathLineTo(orig.x + (incX * i), orig.y - (buf[i] * amplitude));
            //}
            g.PathStroke(IPattern(COLOR_RED), 2.f);
            g.PathClear();
        }
    }

    void DrawVerticalLines(IGraphics& g)
    {
        // Draw vertical lines
        WDL_String text;

        const IRECT& b = mWidgetBounds;
        IRECT textSize;
        g.MeasureText(mStyle.labelText, "TESHz", textSize);
        IRECT tmpR;

        double wsc = b.W() / logf(401.f);
        double f = 20;
        double lx = mWidgetBounds.L;
        float nextTextX = b.L + textSize.W();
        
        while (f < m_sampleRate * 0.5) {
            float dx = (logf(1.0 + (f / (m_sampleRate * 2.0) * 400)) * wsc) + b.L;

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
        g.PathStroke(IPattern(COLOR_WHITE), 2.0f);
        g.PathClear();
    }


    void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
    {
        if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
        {
            IByteStream stream(pData, dataSize);

            int pos = 0;
            pos = stream.Get(&m_buf, pos);

            SetDirty(false);
        }
    }

    inline static float sqr(float x)
    {
        return x * x;
    }

private:
    double m_sampleRate;
    //ISenderData<2, std::array<float, BUFSIZE>> m_buf;
    ISenderData<2, std::array<WDL_FFT_COMPLEX, BUFSIZE>> m_buf;
};

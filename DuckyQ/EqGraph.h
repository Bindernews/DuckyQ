#pragma once

#include <array>
#include "IControl.h"
#include "ISender.h"
#include "config.h"
#include "fft.h"
#include "wdl_base64.h"

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
        DrawGraph(g);
    }

    void DrawVerticalLines(IGraphics& g)
    {
        // Draw vertical lines
        WDL_String text;

        const IRECT& b = mWidgetBounds;
        IRECT textSize;
        g.MeasureText(mStyle.labelText, "TESHz", textSize);
        IRECT tmpR;

        const double hsrate = m_sampleRate / 2.0;
        double f = 20;
        double lx = mWidgetBounds.L;
        float nextTextX = b.L + textSize.W();
        
        
        while (f < hsrate) {
            //float dx = (logf(1.0 + (f / (m_sampleRate * 2.0) * 400)) * wsc) + b.L;
            float dx = Lerp(b.L, b.R, (float)(f / hsrate));

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
    
    void DrawGraph(IGraphics& g)
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
            float* buf = reinterpret_cast<float*>(m_buf.vals[c].data());
            
            g.PathMoveTo(orig.x, orig.y);
            for (int i = 0; i < BUFSIZE / 2; i++) {
                int ix = permute_tab[i] * 2;
                float db = 1.f + (20.f * std::log10( sqr(buf[ix + 0]) + sqr(buf[ix + 1]) ) * 0.5f) / (-MIN_DB);
                float ty = Clip(0.f, 1.f, db) * b.H();
                float tx = ((float)i / ((float)BUFSIZE/2.f)) * b.W();

                g.PathLineTo(b.L + tx, b.B - ty);
            }
            g.PathStroke(IPattern(COLOR_RED), 1.f);
            g.PathClear();
        }
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
    
    inline static float unlerpf(float a, float b, float x)
    {
        return (x - a) / (b - a);
    }

    inline static float sqr(float x)
    {
        return std::abs(x * x);
    }

private:
    double m_sampleRate;
    //ISenderData<2, std::array<float, BUFSIZE>> m_buf;
    ISenderData<2, std::array<WDL_FFT_REAL, BUFSIZE>> m_buf;
};

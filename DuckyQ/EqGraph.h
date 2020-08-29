#pragma once

#include <array>
#include "IControl.h"
#include "qmsg.h"
#include "config.h"
#include "fft.h"
#include "wdl_base64.h"

using namespace iplug;
using namespace iplug::igraphics;


class EqGraph;
class EqBand : public IControl
             , public IVectorBase
{
public:
    EqBand(const IRECT& bounds, int parentTag, int baseParam, IColor bandColor, const char* label="");

    void Draw(IGraphics& g) override;
    void OnResize() override;

    void SetBandColor(IColor color);

private:
    /** [0] = main color, [1] = partially transparent bg */
    IColor m_colors[3];
    /** Draw radius [0,1]. Used in animation. */
    float m_radius;
    int m_parentTag;
    int m_baseParam;
};

class EqGraph : public IControl
              , public IVectorBase
{
public:
    static const int MIN_HZ = 20;
    static const int MAX_HZ = 22000;

    EqGraph(const IRECT& bounds, int subTag, const IVStyle& style = DEFAULT_STYLE);

    void Draw(IGraphics& g) override;
    void DrawWidget(IGraphics& g) override;
    void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override;

    void DrawVerticalLines(IGraphics& g);
    void DrawGraph(IGraphics& g);

    EqBand* AddBand();

    void Reset(double sampleRate, int blockSize, int fftSize)
    {
        m_sampleRate = sampleRate;
        m_fftSize = fftSize;
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
    /** Takes an FFT bin index and converts it to an x-value in range [0,1] */
    float binToX(float fftBin);

private:
    /** Base tag for children. */
    int m_childTag;
    double m_sampleRate;
    int m_fftSize;
    bn::QMsgMut m_buf;
};

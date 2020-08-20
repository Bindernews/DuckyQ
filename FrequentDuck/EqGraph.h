#pragma once

#include "IControl.h"
#include "ISender.h"
#include "config.h"

using namespace iplug;
using namespace iplug::igraphics;

template<int BUFSIZE=128>
class EqGraph : public IControl,
                public IVectorBase
{
public:
    EqGraph(const IRECT& bounds, const IVStyle& style = DEFAULT_STYLE)
        : IControl(bounds, SplashClickActionFunc), IVectorBase(style)
    {
        AttachIControl(this, "");
    }

    void Draw(IGraphics& g) override
    {

    }

    void DrawWidget(IGraphics& g) override
    {
        for (int c = 0; c < m_buf.nChans; c++) {
            //std::array<float, FFT_ buf = m_buf.vals[c];
            //for (int i = 0; i < m_buf.vals
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

private:
    ISenderData<2, std::array<float, BUFSIZE>> m_buf;
};

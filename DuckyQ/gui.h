#pragma once
#include "IControls.h"
#include "DuckyQ.h"

struct GStyle
{
    IColor bandColors[MAX_EQ_BANDS];
};

class PlugGUI
{
public:
    PlugGUI(DuckyQ& owner);

    void buildGui(IGraphics& g);
    bool OnMessage(int msgTag, int ctrlTag, const bn::QMsg& msg);

    IGraphics* GetUI();

private:
    bool m_winSettingsOpen;
    int m_fftSize;
    DuckyQ& m_owner;
};



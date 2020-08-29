#include "gui.h"
#include "IControls.h"
#include "EqGraph.h"

#define CHECK_UI(retval) auto ui = GetUI(); if (!ui) { return retval; }

PlugGUI::PlugGUI(DuckyQ& owner)
    : m_owner(owner)
    , m_fftSize(FFT_BLOCK_SIZE)
    , m_winSettingsOpen(false)
{}

void PlugGUI::buildGui(IGraphics& ui)
{
    ui.AttachCornerResizer(EUIResizerMode::Scale, false);
    ui.AttachPanelBackground(COLOR_GRAY);
    ui.LoadFont("Roboto-Regular", ROBOTO_FN);

    const IRECT b = ui.GetBounds();
    const IRECT rEqGraph = IRECT(50, 400, b.R - 50, 580);

    ui.AttachControl(new ITextControl(b.GetMidVPadded(50), "Hello iPlug 2!", IText(50)));
    ui.AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain));
    ui.AttachControl(new EqGraph(rEqGraph, kCtrlBand0), kCtrlEqMain);
}

bool PlugGUI::OnMessage(int ctrlTag, int msgTag, const bn::QMsg& msg)
{
    CHECK_UI(false);

    switch (msgTag) {
    case kMsggSampleRate:
    {
        //ui->GetControlWithTag(kCtrlEqMain)->As<EqGraph>()->Reset()
        return true;
    }
    }

    return false;
}

IGraphics* PlugGUI::GetUI()
{
    return m_owner.GetUI();
}

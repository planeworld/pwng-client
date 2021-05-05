#include "ui_manager.hpp"

#include <vector>

#include "message_handler.hpp"
#include "network_manager.hpp"

void UIManager::displayHelp()
{
    ImGuiWindowFlags WindowFlags =  ImGuiWindowFlags_NoDecoration |
                                    ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoInputs |
                                    ImGuiWindowFlags_NoNav |
                                    ImGuiWindowFlags_NoMove;
    bool CloseButton{false};
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2, 40), ImGuiCond_Always, ImVec2(0.5f,0.5f));

    if (ShowHelp_)
    {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2,
                                        ImGui::GetIO().DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f,0.5f));
        ImGui::Begin("Help", &CloseButton, WindowFlags);
            ImGui::Text("Zoom in:         Mouse wheel forward");
            ImGui::Text("Zoom in (slow):  Mouse wheel forward + Left Shift");
            ImGui::Text("Zoom out:        Mouse wheel backward");
            ImGui::Text("Zoom out (slow): Mouse wheel backward + Left Shift");
        ImGui::End();
    }

}

void UIManager::displayObjectLabels(entt::entity _Cam)
{
    if (Labels_)
    {
        auto& HookPos = Reg_.get<SystemPositionComponent>(Reg_.get<HookComponent>(_Cam).e);
        auto& Pos = Reg_.get<SystemPositionComponent>(_Cam);
        auto& Zoom = Reg_.get<ZoomComponent>(_Cam);
        Reg_.view<MassComponent, SystemPositionComponent, NameComponent, RadiusComponent, StarDataComponent>(entt::exclude<entt::tag<"is_outside"_hs>>).each(
                [this, &HookPos, &Pos, &Zoom]
                (auto _e, const auto& _m, const auto& _p, const auto& _n, const auto& _r, const auto& _s)
        {
            ImGuiWindowFlags WindowFlags =  ImGuiWindowFlags_NoDecoration |
                                            ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_NoFocusOnAppearing |
                                            ImGuiWindowFlags_NoInputs |
                                            ImGuiWindowFlags_NoNav |
                                            ImGuiWindowFlags_NoMove;
            bool CloseButton{false};
            auto x = _p.x;
            auto y = _p.y;

            x -= Pos.x + HookPos.x;
            y += Pos.y - HookPos.y;
            x *= Zoom.z;
            y *= Zoom.z;

            double ScreenX = ImGui::GetIO().DisplaySize.x;
            double ScreenY = ImGui::GetIO().DisplaySize.y;
            ImGui::SetNextWindowPos(ImVec2(int( x+0.5*ScreenX),
                                           int(-y+0.5*ScreenY)));
            ImGui::Begin(_n.n.c_str(), &CloseButton, WindowFlags);
                ImGui::TextColored(ImVec4(0.5, 0.5, 1.0, 1.0), _n.n.c_str());
                ImGui::Separator();
                ImGui::Indent();
                    if (LabelsStarData_)
                    {
                        ImGui::Text("Spectral Class: %s", SpectralClassToStringMap[_s.SpectralClass].c_str());
                        ImGui::Text("Temperature:    %.0f K", _s.Temperature);
                        ImGui::Text("Radius:         %.2e km", _r.r*1.0e-3);
                    }
                    if (LabelsMass_ || LabelsStarData_) ImGui::Text("Mass:           %.2e kg", _m.m);
                    if (LabelsPosition_) ImGui::Text("Position (raw): (%.2e, %.2e) km", _p.x*1.0e-3, _p.y*1.0e-3);
                ImGui::Unindent();
            ImGui::End();
        });
    }
}

void UIManager::displayPerformance(PerformanceTimers& _Timers)
{
    ImGui::TextColored(ImVec4(1,1,0,1), "Performance");
    ImGui::Text("Client:");
    ImGui::Indent();
        ImGui::Text("Frame Time:  %.3f ms; (%.1f FPS)",
                    1000.0/double(ImGui::GetIO().Framerate), double(ImGui::GetIO().Framerate));
        ImGui::Text("Process Queue: %.2f ms", _Timers.QueueAvg.getAvg_ms());
        ImGui::Text("Render (CPU): %.2f ms", _Timers.RenderAvg.getAvg_ms());
        ImGui::Text("Viewport Test: %.2f ms", _Timers.ViewportTestAvg.getAvg_ms());
    ImGui::Unindent();
    ImGui::Text("Server:");
    ImGui::Indent();
        ImGui::Text("Sim:  %.2f ms", _Timers.ServerSimFrameTimeAvg.getAvg_ms());
        ImGui::Text("- Queue In:   %.2f ms", _Timers.ServerQueueInFrameTimeAvg.getAvg_ms());
        ImGui::Text("- Queue Out:  %.2f ms", _Timers.ServerQueueOutFrameTimeAvg.getAvg_ms());
        ImGui::Text("- Physics:  %.2f ms", _Timers.ServerPhysicsFrameTimeAvg.getAvg_ms());
    ImGui::Unindent();
}

void UIManager::displayScale(const int _Scale, const ScaleUnitE _ScaleUnit)
{
    ImGuiWindowFlags WindowFlags =  ImGuiWindowFlags_NoDecoration |
                                    ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoFocusOnAppearing |
                                    ImGuiWindowFlags_NoInputs |
                                    ImGuiWindowFlags_NoNav |
                                    ImGuiWindowFlags_NoMove;
    bool CloseButton{false};

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2, 40), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::Begin("Scale", &CloseButton, WindowFlags);
        int l = std::pow(10, _Scale);
        if (_ScaleUnit == ScaleUnitE::MLY)
            ImGui::Text("Scale: %d million ly", l);
        else if (_ScaleUnit == ScaleUnitE::LY)
            ImGui::Text("Scale: %d ly", l);
        else if (_ScaleUnit == ScaleUnitE::MKM)
            ImGui::Text("Scale: %d million km", l);
        else if (_ScaleUnit == ScaleUnitE::KM)
            ImGui::Text("Scale: %d km", l);
        else if (_ScaleUnit == ScaleUnitE::M)
            ImGui::Text("Scale: %d m", l);
    ImGui::End();
}

void UIManager::processCameraHooks(entt::entity _Cam)
{
    ImGui::TextColored(ImVec4(1,1,0,1), "Camera Hooks");

    std::vector<std::string> Names;
    std::vector<entt::entity> Entities;

    Names.push_back("None");
    Entities.push_back(Reg_.get<HookDummyComponent>(_Cam).e);

    Reg_.view<NameComponent>().each(
        [&Names, &Entities](auto _e, const auto& _n)
        {
            Names.push_back(_n.n);
            Entities.push_back(_e);
        });

    static int CamHook{0};
    if (ImGui::Combo("Select Hook", &CamHook, Names))
    {
        auto& Messages = Reg_.ctx<MessageHandler>();

        auto& h = Reg_.get<HookComponent>(_Cam);
        auto& p = Reg_.get<SystemPositionComponent>(_Cam);
        h.e = Entities[CamHook];
        p.x = 0.0;
        p.y = 0.0;
        DBLK(Messages.report("ui", "New camera hook on object " + Names[CamHook], MessageHandler::DEBUG_L1);)
    }
}

void UIManager::processConnections()
{
    auto& Network = Reg_.ctx<NetworkManager>();

    std::string Uri = "ws://localhost:9002/?id=1";
    if (Network.isConnected())
    {
        if (ImGui::Button("Disconnect")) Network.disconnect();
    }
    else
    {
        if (ImGui::Button("Connect")) Network.connect(Uri);
    }
}

void UIManager::processHelp()
{
    if (ImGui::Button("Help")) ShowHelp_ ^= true;

}

void UIManager::processObjectLabels()
{
    ImGui::Indent();
        ImGui::Checkbox("Object Labels", &Labels_);
        ImGui::Indent();
            ImGui::Checkbox("Mass", &LabelsMass_);
            ImGui::Checkbox("Position", &LabelsPosition_);
            ImGui::Checkbox("Velocity", &LabelsVelocity_);
            ImGui::Checkbox("Star Data", &LabelsStarData_);
        ImGui::Unindent();
    ImGui::Unindent();
}

void UIManager::processVerbosity()
{
    auto& Messages = Reg_.ctx<MessageHandler>();

    // If compiled debug, let the user choose debug level to avoid
    // excessive console spamming
    DBLK(
        static int DebugLevel = 4;
        ImGui::Text("Verbosity:");
        ImGui::Indent();
            ImGui::RadioButton("Info", &DebugLevel, 2);
            ImGui::RadioButton("Debug Level 1", &DebugLevel, 3);
            ImGui::RadioButton("Debug Level 2", &DebugLevel, 4);
            ImGui::RadioButton("Debug Level 3", &DebugLevel, 5);
        ImGui::Unindent();
        Messages.setLevel(MessageHandler::ReportLevelType(DebugLevel));
    )
}

namespace ImGui
{

static auto vector_getter = [](void* vec, int idx, const char** out_text)
{
    auto& vector = *static_cast<std::vector<std::string>*>(vec);
    if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
    *out_text = vector.at(idx).c_str();
    return true;
};

bool Combo(const char* label, int* currIndex, std::vector<std::string>& values)
{
    if (values.empty()) { return false; }
    return Combo(label, currIndex, vector_getter,
        static_cast<void*>(&values), values.size());
}

}

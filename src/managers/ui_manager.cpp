#include "ui_manager.hpp"

#include "json_manager.hpp"
#include "message_handler.hpp"
#include "network_manager.hpp"

void UIManager::addCamHook(entt::entity _e, const std::string& _n)
{
    CamHooks_.insert({_n, _e});
}

void UIManager::addSystem(entt::entity _e, const std::string& _n)
{
    NamesSubSystemsSet_.insert(_n);
}

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
            ImGui::Text("Move:        Mouse move + LCtrl");
            ImGui::Text("Zoom:        Mouse wheel");
            ImGui::Text("Zoom (slow): Mouse wheel + LCtrl");
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
        Reg_.view<MassComponent, SystemPositionComponent, NameComponent, RadiusComponent, StarDataComponent, InsideViewportTag>().each(
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
            ImGui::Begin(_n.Name, &CloseButton, WindowFlags);
                ImGui::TextColored(ImVec4(0.5, 0.5, 1.0, 1.0), _n.Name);
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

void UIManager::displayScaleAndTime(const int _Scale, const ScaleUnitE _ScaleUnit,
                                    const SimTimer& _SimTime)
{
    ImGuiWindowFlags WindowFlags =  ImGuiWindowFlags_NoDecoration |
                                    ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoFocusOnAppearing |
                                    ImGuiWindowFlags_NoInputs |
                                    ImGuiWindowFlags_NoNav |
                                    ImGuiWindowFlags_NoMove;
    bool CloseButton{false};

    auto UIStyle = &ImGui::GetStyle();
    UIStyle->WindowRounding = 0.0f;
    // UIStyle->WindowBorderSize = 0.0f;

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
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2,
                                   ImGui::GetIO().DisplaySize.y - 40), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::Begin("Time", &CloseButton, WindowFlags);
    ImGui::Text("Time: %dy %3.1dd %2.1dh %2.1dm %2.1ds",
                _SimTime.getYears(),
                _SimTime.getDaysFraction(),
                _SimTime.getHoursFraction(),
                _SimTime.getMinutesFraction(),
                _SimTime.getSecondsFraction());
    ImGui::End();

    UIStyle->WindowRounding = 5.0f;
    // UIStyle->WindowBorderSize = 1.0f;
}

void UIManager::finishSystemsTransfer()
{
    NamesSubSystems_.assign(NamesSubSystemsSet_.cbegin(), NamesSubSystemsSet_.cend());

    NamesCamHooks_.clear();
    NamesCamHooks_.reserve(CamHooks_.size());

    std::transform(CamHooks_.cbegin(), CamHooks_.cend(), std::back_inserter(NamesCamHooks_),
        [](decltype(CamHooks_)::value_type const& _Pair) {return _Pair.first;}
    );
}

void UIManager::processCameraHooks(entt::entity _Cam)
{
    static int CamHook{0};
    if (ImGui::Combo("Select Hook", &CamHook, NamesCamHooks_))
    {
        auto& Messages = Reg_.ctx<MessageHandler>();

        auto& h = Reg_.get<HookComponent>(_Cam);
        auto& p_s = Reg_.get<SystemPositionComponent>(_Cam);
        // h.e = EntitiesCamHook_[CamHook];
        h.e = CamHooks_[NamesCamHooks_[CamHook]];

        p_s = {0.0, 0.0};

        auto* p = Reg_.try_get<PositionComponent>(_Cam);
        if (p != nullptr) *p = {0.0, 0.0};

        DBLK(Messages.report("ui", "New camera hook on object " + std::string(NamesCamHooks_[CamHook]), MessageHandler::DEBUG_L1);)
    }
}

void UIManager::processClientControl()
{
    auto& Json = Reg_.ctx<JsonManager>();

    if (ImGui::Button("Subscribe: All"))
    {
        Json.createRequest("sub_galaxy_data").finalise();
        QueueOut_->enqueue(Json.getString());
        Json.createRequest("sub_dynamic_data").finalise();
        QueueOut_->enqueue(Json.getString());
        Json.createRequest("sub_perf_stats").finalise();
        QueueOut_->enqueue(Json.getString());
        Json.createRequest("sub_sim_stats").finalise();
        QueueOut_->enqueue(Json.getString());
    }
    if (ImGui::Button("Subscribe: Galaxy Data"))
    {
        Json.createRequest("sub_galaxy_data").finalise();
        QueueOut_->enqueue(Json.getString());
    }
    if (ImGui::Button("Subscribe: Dynamic Data"))
    {
        Json.createRequest("sub_dynamic_data").finalise();
        QueueOut_->enqueue(Json.getString());
    }
    if (ImGui::Button("Subscribe: Performance Stats"))
    {
        Json.createRequest("sub_perf_stats").finalise();
        QueueOut_->enqueue(Json.getString());
    }
    if (ImGui::Button("Subscribe: Simulation Stats"))
    {
        Json.createRequest("sub_sim_stats").finalise();
        QueueOut_->enqueue(Json.getString());
    }
}

void UIManager::processConnections()
{
    auto& Network = Reg_.ctx<NetworkManager>();

    static char Msg[128] ="ws://localhost:9002/?id=1";

    if (Network.isConnected())
    {
        if (ImGui::Button("Disconnect")) Network.disconnect();
    }
    else
    {
        if (ImGui::Button("Connect")) Network.connect(Msg);
        ImGui::SameLine();
        ImGui::InputText("##Server", Msg, IM_ARRAYSIZE(Msg));
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

void UIManager::processServerControl()
{
    auto& Json = Reg_.ctx<JsonManager>();

    static char Id[10] = "1";
    ImGui::Text("ID     ");
    ImGui::SameLine();
    ImGui::InputText("##ID", Id, IM_ARRAYSIZE(Id));
    static char Msg[128] = "Hello Server";
    ImGui::Text("Message");
    ImGui::SameLine();
    ImGui::InputText("##Message", Msg, IM_ARRAYSIZE(Msg));
    ImGui::SameLine();
    if (ImGui::Button("Send"))
    {
        Json.createRequest(Msg).finalise();
        QueueOut_->enqueue(Json.getString());
    }
    if (ImGui::Button("Accelerate Simulation"))
    {
        Json.createRequest("cmd_accelerate_simulation")
            .beginArray("params")
            .addValue(10000.0)
            .endArray()
            .finalise();
        QueueOut_->enqueue(std::string(Json.getString()));
    }
    if (ImGui::Button("Start Simulation"))
    {
        Json.createRequest("cmd_start_simulation").finalise();
        QueueOut_->enqueue(Json.getString());
    }
    if (ImGui::Button("Stop Simulation"))
    {
        Json.createRequest("cmd_stop_simulation").finalise();
        QueueOut_->enqueue(Json.getString());
    }
    if (ImGui::Button("Shutdown Server"))
    {
        Json.createRequest("cmd_shutdown").finalise();
        QueueOut_->enqueue(Json.getString());
    }
}

void UIManager::processSubscriptions()
{
    static int Subs{0};
    if (ImGui::Combo("Subscribe", &Subs, NamesSubs_))
    {
        auto& Json = Reg_.ctx<JsonManager>();

        std::string Name = NamesSubs_[Subs];

        NamesUnsubsSet_.insert(Name);
        NamesSubsSet_.erase(Name);

        NamesSubs_.assign(NamesSubsSet_.cbegin(), NamesSubsSet_.cend());
        NamesUnsubs_.assign(NamesUnsubsSet_.cbegin(), NamesUnsubsSet_.cend());

        Json.createRequest(Name)
            .finalise();
        QueueOut_->enqueue(Json.getString());

        Subs = 0;
    }

    static int Unsubs{0};
    if (ImGui::Combo("Unsubscribe", &Unsubs, NamesUnsubs_))
    {
        auto& Json = Reg_.ctx<JsonManager>();

        std::string Name = NamesUnsubs_[Unsubs];

        NamesSubsSet_.insert(Name);
        NamesUnsubsSet_.erase(Name);

        NamesSubs_.assign(NamesSubsSet_.cbegin(), NamesSubsSet_.cend());
        NamesUnsubs_.assign(NamesUnsubsSet_.cbegin(), NamesUnsubsSet_.cend());

        Json.createRequest("un"+Name)
            .finalise();
        QueueOut_->enqueue(Json.getString());

        Unsubs = 0;
    }
}

void UIManager::processStarSystems()
{
    static int StarSystemSub{0};
    if (ImGui::Combo("Subscribe System", &StarSystemSub, NamesSubSystems_))
    {
        auto& Json = Reg_.ctx<JsonManager>();

        std::string Name = NamesSubSystems_[StarSystemSub];

        NamesUnsubSystemsSet_.insert(Name);
        NamesSubSystemsSet_.erase(Name);

        NamesSubSystems_.assign(NamesSubSystemsSet_.cbegin(), NamesSubSystemsSet_.cend());
        NamesUnsubSystems_.assign(NamesUnsubSystemsSet_.cbegin(), NamesUnsubSystemsSet_.cend());

        Json.createRequest("sub_system")
            .addParam("name", Name)
            .finalise();
        QueueOut_->enqueue(Json.getString());

        StarSystemSub = 0;
    }
    static int StarSystemUnsub{0};
    if (ImGui::Combo("Unsubscribe System", &StarSystemUnsub, NamesUnsubSystems_))
    {
        auto& Json = Reg_.ctx<JsonManager>();

        std::string Name = NamesUnsubSystems_[StarSystemUnsub];

        NamesSubSystemsSet_.insert(Name);
        NamesUnsubSystemsSet_.erase(Name);

        NamesSubSystems_.assign(NamesSubSystemsSet_.cbegin(), NamesSubSystemsSet_.cend());
        NamesUnsubSystems_.assign(NamesUnsubSystemsSet_.cbegin(), NamesUnsubSystemsSet_.cend());

        Json.createRequest("unsub_system")
            .addParam("name", Name)
            .finalise();
        QueueOut_->enqueue(Json.getString());

        StarSystemUnsub = 0;
    }
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

void UIManager::initSubscriptions()
{
    NamesSubsSet_.insert("sub_dynamic_data");
    NamesSubsSet_.insert("sub_perf_stats_f0");
    NamesSubsSet_.insert("sub_perf_stats_f0.1");
    NamesSubsSet_.insert("sub_perf_stats_f0.5");
    NamesSubsSet_.insert("sub_perf_stats_f1");
    NamesSubsSet_.insert("sub_perf_stats_f5");
    NamesSubsSet_.insert("sub_perf_stats_f10");
    NamesSubsSet_.insert("sub_sim_stats_f0");
    NamesSubsSet_.insert("sub_sim_stats_f0.1");
    NamesSubsSet_.insert("sub_sim_stats_f0.5");
    NamesSubsSet_.insert("sub_sim_stats_f1");
    NamesSubsSet_.insert("sub_sim_stats_f5");
    NamesSubsSet_.insert("sub_sim_stats_f10");

    NamesSubs_.assign(NamesSubsSet_.cbegin(), NamesSubsSet_.cend());
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

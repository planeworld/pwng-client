#include "ui_manager.hpp"

#include <vector>

#include "message_handler.hpp"
#include "network_manager.hpp"

void UIManager::displayObjectLabels(entt::entity _Cam)
{
    auto& Hook = Reg_.get<HookComponent>(_Cam);
    auto& Pos = Reg_.get<PositionComponent>(_Cam);
    auto& Zoom = Reg_.get<ZoomComponent>(_Cam);
    Reg_.view<PositionComponent, CircleComponent, NameComponent>().each([this, &Hook, &Pos, &Zoom](auto _e, const auto& _p, const auto& _r, const auto& _n)
    {
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoFocusOnAppearing |
                                       ImGuiWindowFlags_NoInputs |
                                       ImGuiWindowFlags_NoNav |
                                       ImGuiWindowFlags_NoMove;
        bool CloseButton{false};
        auto x = _p.x;
        auto y = _p.y;

        x -= Pos.x + Hook.x;
        y += Pos.y - Hook.y;
        x *= Zoom.z;
        y *= Zoom.z;

        auto ScreenX = ImGui::GetIO().DisplaySize.x;
        auto ScreenY = ImGui::GetIO().DisplaySize.y;
        ImGui::SetNextWindowPos(ImVec2(int( x+0.5*ScreenX),
                                        int(-y+0.5*ScreenY)));
        ImGui::Begin(_n.n.c_str(), &CloseButton, WindowFlags);
            ImGui::Text(_n.n.c_str());
        ImGui::End();
    });
}

void UIManager::displayPerformance()
{
    ImGui::TextColored(ImVec4(1,1,0,1), "Performance");
    ImGui::Indent();
        ImGui::Text("Frame Time:  %.3f ms; (%.1f FPS)",
                    1000.0/double(ImGui::GetIO().Framerate), double(ImGui::GetIO().Framerate));
    ImGui::Unindent();
}

void UIManager::processCameraHooks(entt::entity _Cam)
{
    ImGui::TextColored(ImVec4(1,1,0,1), "Camera Hooks");

    std::vector<std::string> Names;
    std::vector<entt::entity> Entities;

    Names.push_back("None");
    Entities.push_back(entt::null);

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
        auto& p = Reg_.get<PositionComponent>(_Cam);
        h.e = Entities[CamHook];
        p.x = 0.0;
        p.y = 0.0;
        DBLK(Messages.report("prg", "New camera hook on object " + Names[CamHook], MessageHandler::DEBUG_L1);)
    }
}

void UIManager::processConnections()
{
    auto& Messages = Reg_.ctx<MessageHandler>();
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

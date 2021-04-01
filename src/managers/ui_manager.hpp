#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include <entt/entity/registry.hpp>
#include <Magnum/ImGuiIntegration/Context.hpp>

#include "components.hpp"

class UIManager
{

    public:

        UIManager(entt::registry& _Reg,
                  Magnum::ImGuiIntegration::Context& _ImGUI) :
                  Reg_(_Reg), ImGUI_(_ImGUI) {}

        void displayObjectLabels(entt::entity _Cam);
        void displayPerformance();
        void processCameraHooks(entt::entity _Cam);
        void processConnections();
        void processVerbosity();


    private:

        entt::registry& Reg_;

        Magnum::ImGuiIntegration::Context& ImGUI_;
};

#endif // UI_MANAGER_HPP

namespace ImGui
{

bool Combo(const char* label, int* currIndex, std::vector<std::string>& values);

}

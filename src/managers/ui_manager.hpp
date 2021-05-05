#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include <string>
#include <vector>

#include <entt/entity/registry.hpp>
#include <Magnum/ImGuiIntegration/Context.hpp>

#include "components.hpp"
#include "performance_timers.hpp"
#include "scale_unit.hpp"

class UIManager
{

    public:

        UIManager(entt::registry& _Reg,
                  Magnum::ImGuiIntegration::Context& _ImGUI) :
                  Reg_(_Reg), ImGUI_(_ImGUI) {}

        void displayHelp();
        void displayObjectLabels(entt::entity _Cam);
        void displayPerformance(PerformanceTimers& _Timers);
        void displayScale(const int _Scale, const ScaleUnitE _ScaleUnit);
        void processCameraHooks(entt::entity _Cam);
        void processConnections();
        void processHelp();
        void processObjectLabels();
        void processVerbosity();


    private:

        entt::registry& Reg_;
        Magnum::ImGuiIntegration::Context& ImGUI_;

        bool Labels_{false};
        bool LabelsMass_{false};
        bool LabelsPosition_{false};
        bool LabelsStarData_{false};
        bool LabelsVelocity_{false};
        bool ShowHelp_{false};
};

#endif // UI_MANAGER_HPP

namespace ImGui
{

bool Combo(const char* label, int* currIndex, std::vector<std::string>& values);

}

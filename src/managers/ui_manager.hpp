#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

#include <concurrentqueue/concurrentqueue.h>
#include <entt/entity/registry.hpp>
#include <Magnum/ImGuiIntegration/Context.hpp>

#include "components.hpp"
#include "performance_timers.hpp"
#include "scale_unit.hpp"
#include "sim_timer.hpp"

class UIManager
{

    public:

        explicit UIManager(entt::registry& _Reg,
                           Magnum::ImGuiIntegration::Context& _ImGUI,
                           moodycamel::ConcurrentQueue<std::string>* _QueueIn,
                           moodycamel::ConcurrentQueue<std::string>* _QueueOut) :
                Reg_(_Reg),
                QueueIn_(_QueueIn),
                QueueOut_(_QueueOut),
                ImGUI_(_ImGUI) {}

        void addCamHook(entt::entity _e, const std::string& _n);
        void addSystem(entt::entity _e, const std::string& _n);
        void displayHelp();
        void displayObjectLabels(entt::entity _Cam);
        void displayPerformance(PerformanceTimers& _Timers);
        void displayScaleAndTime(const int _Scale, const ScaleUnitE _ScaleUnit, const SimTimer& _SimTime);
        void finishSystemsTransfer();
        void processCameraHooks(entt::entity _Cam);
        void processClientControl();
        void processConnections();
        void processHelp();
        void processObjectLabels();
        void processServerControl();
        void processSubscriptions();
        void processVerbosity();


    private:

        entt::registry& Reg_;
        Magnum::ImGuiIntegration::Context& ImGUI_;

        moodycamel::ConcurrentQueue<std::string>* QueueIn_;
        moodycamel::ConcurrentQueue<std::string>* QueueOut_;

        std::map<std::string, entt::entity> CamHooks_;
        std::set<std::string> NamesSubSystemsSet_;
        std::set<std::string> NamesUnsubSystemsSet_;
        std::vector<std::string> NamesCamHooks_;
        std::vector<std::string> NamesSubSystems_;
        std::vector<std::string> NamesUnsubSystems_;
        // std::vector<entt::entity> EntitiesCamHook_;

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

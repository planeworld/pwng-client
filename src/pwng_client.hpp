#ifndef PWNG_CLIENT_HPP
#define PWNG_CLIENT_HPP

#include <string>
#include <unordered_map>

#include <entt/entity/entity.hpp>
#include <Magnum/Platform/GlfwApplication.h>

#include "color_palette.hpp"
#include "performance_timers.hpp"
#include "scale_unit.hpp"
#include "sim_timer.hpp"

using namespace Magnum;

class PwngClient : public Magnum::Platform::Application
{

    public:
    
        explicit PwngClient(const Arguments& arguments);

    private:

        entt::registry Reg_;
        moodycamel::ConcurrentQueue<std::string> InputQueue_;
        moodycamel::ConcurrentQueue<std::string> OutputQueue_;

        //--- Window Event Handling ---//
        void drawEvent() override;
        void keyPressEvent(KeyEvent& Event) override;
        void keyReleaseEvent(KeyEvent& Event) override;
        void mouseMoveEvent(MouseMoveEvent& Event) override;
        void mousePressEvent(MouseEvent& Event) override;
        void mouseReleaseEvent(MouseEvent& Event) override;
        void mouseScrollEvent(MouseScrollEvent& Event) override;
        void textInputEvent(TextInputEvent& Event) override;
        void viewportEvent(ViewportEvent& Event) override;

        void getObjectsFromQueue();
        void setupNetwork();
        void setupWindow();
        void updateUI();

        PerformanceTimers Timers_;
        SimTimer SimTime_;

        //--- Graphics ---//
        std::unordered_map<std::uint32_t, entt::entity> Id2EntityMap_;

        //--- UI ---//
        Magnum::ImGuiIntegration::Context ImGUI_{Magnum::NoCreate};
        ImGuiStyle* UIStyle_{nullptr};
        ImGuiStyle  UIStyleSubStats_;
        ImGuiStyle  UIStyleDefault_;

        // ImFont* UIFont_{nullptr};
};

#endif // PWNG_CLIENT_HPP

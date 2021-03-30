#ifndef PWNG_CLIENT_HPP
#define PWNG_CLIENT_HPP

#include <string>
#include <unordered_map>

#include <entt/entity/entity.hpp>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Shaders/Flat.h>

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
        void textInputEvent(TextInputEvent& Event) override;
        void viewportEvent(ViewportEvent& Event) override;

        void getObjectsFromQueue();
        void renderScene();
        void setupNetwork();
        void setupWindow();
        void updateUI();

        void sendJsonRpcMessage(const std::string& _Msg, const std::string& _ID);

        //--- Graphics ---//
        std::unordered_map<std::uint32_t, entt::entity> Id2EntityMap_;
        GL::Mesh CircleShape_{NoCreate};
        Matrix3 Projection_;
        Shaders::Flat2D Shader_{NoCreate};

        // --- Graphics - Camera ---//
        double CamX_{0.0};
        double CamY_{0.0};
        double CamZoom_{1.0};

        bool ShowRealObjectSizes_{false};

        //--- UI ---//
        Magnum::ImGuiIntegration::Context ImGUI_{Magnum::NoCreate};
        ImGuiStyle* UIStyle_{nullptr};
        ImGuiStyle  UIStyleSubStats_;
        ImGuiStyle  UIStyleDefault_;
};

#endif // PWNG_CLIENT_HPP
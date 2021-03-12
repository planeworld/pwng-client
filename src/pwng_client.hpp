#ifndef PWNG_CLIENT_HPP
#define PWNG_CLIENT_HPP

#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Platform/Sdl2Application.h>

class PwngClient : public Magnum::Platform::Application
{

    public:
    
        explicit PwngClient(const Arguments& arguments);

        void drawEvent() override {}

    private:

        void setupNetwork();
        void setupWindow();

        //-- UI --//
        Magnum::ImGuiIntegration::Context ImGUI_{Magnum::NoCreate};
        ImGuiStyle* UIStyle_{nullptr};
        ImGuiStyle  UIStyleSubStats_;
        ImGuiStyle  UIStyleDefault_;
};

#endif // PWNG_CLIENT_HPP

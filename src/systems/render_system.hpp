#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include <entt/entity/registry.hpp>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Shaders/Flat.h>

#include "color_palette.hpp"
#include "performance_timers.hpp"
#include "scale_unit.hpp"

// using namespace Magnum;

class RenderSystem
{

    public:

        explicit RenderSystem(entt::registry& _Reg);

        entt::entity getCamera() const {return Camera_;}

        void renderScale();
        void renderScene();
        void setupCamera();
        void setupGraphics();
        void setWindowSize(const double _x, const double _y);

    private:

        entt::registry& Reg_;

        double WindowSizeX_{1.0};
        double WindowSizeY_{1.0};

        GL::Mesh CircleShape_{NoCreate};
        GL::Mesh ScaleLineShapeH_{NoCreate};
        GL::Mesh ScaleLineShapeV_{NoCreate};
        Matrix3 Projection_;
        Shaders::Flat2D Shader_{NoCreate};

        ColorPalette TemperaturePalette_;
        int Scale_{0};

        ScaleUnitE ScaleUnit_{ScaleUnitE::LY};

        // --- Graphics - Camera ---//
        entt::entity Camera_;

        float StarsDisplaySizeMin_{0.5f};
        float StarsDisplayScaleFactor_{1.0f};

};

#endif // RENDER_SYSTEM_HPP

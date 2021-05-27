#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include <entt/entity/registry.hpp>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Shaders/Flat.h>

#include "blur_shader_5x1.hpp"
#include "color_palette.hpp"
#include "main_display_shader.hpp"
#include "performance_timers.hpp"
#include "scale_unit.hpp"

// using namespace Magnum;
//

class RenderSystem
{

    public:

        // Framebuffer for supersampling will be set to what the
        // GPU allows, but do not exceed 16K, since higher is most
        // likely not used
        static constexpr int TEXTURE_SIZE_MAX = 16384;

        explicit RenderSystem(entt::registry& _Reg);

        entt::entity getCamera() const {return Camera_;}
        int getScale() const {return Scale_;}
        ScaleUnitE getScaleUnit() const {return ScaleUnit_;}

        void renderScale();
        void renderScene();
        void setupCamera();
        void setupGraphics();
        void setupMainDisplayFBO();
        void setupMainDisplayMesh();
        void setWindowSize(const double _x, const double _y);

    private:

        entt::registry& Reg_;

        double RenderResFactor_{8.0};
        double RenderResFactorTarget_{4.0};
        int TextureSizeMax_{1024};
        int WindowSizeX_{1024};
        int WindowSizeY_{768};

        GL::Mesh CircleShape_{NoCreate};
        GL::Mesh ScaleLineShapeH_{NoCreate};
        GL::Mesh ScaleLineShapeV_{NoCreate};
        Matrix3 ProjectionScene_;
        Matrix3 ProjectionWindow_;
        Shaders::Flat2D Shader_{NoCreate};

        ColorPalette TemperaturePalette_;
        int Scale_{0};

        ScaleUnitE ScaleUnit_{ScaleUnitE::LY};

        //--- Framebuffer related ---//
        GL::Framebuffer* FBOMainDisplayFront_{nullptr};
        GL::Framebuffer* FBOMainDisplayBack_{nullptr};
        GL::Framebuffer FBOMainDisplay0_{NoCreate};
        GL::Framebuffer FBOMainDisplay1_{NoCreate};
        GL::Mesh MeshMainDisplay_{NoCreate};
        GL::Mesh MeshBlur5x1_{NoCreate};
        GL::Texture2D* TexMainDisplayFront_{nullptr};
        GL::Texture2D* TexMainDisplayBack_{nullptr};
        GL::Texture2D TexMainDisplay0_{NoCreate};
        GL::Texture2D TexMainDisplay1_{NoCreate};
        BlurShader5x1 ShaderBlur5x1_{NoCreate};
        MainDisplayShader ShaderMainDisplay_{NoCreate};

        // --- Graphics - Camera ---//
        entt::entity Camera_;

        float StarsDisplaySizeMin_{1.0f};
        float StarsDisplayScaleFactor_{1.0f};

};

#endif // RENDER_SYSTEM_HPP

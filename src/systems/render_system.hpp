#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include <array>
#include <vector>

#include <entt/entity/registry.hpp>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Shaders/VertexColor.h>

#include "blur_shader_5x1.hpp"
#include "color_palette.hpp"
#include "components.hpp"
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

        explicit RenderSystem(entt::registry& _Reg, PerformanceTimers& _Timers);

        entt::entity getCamera() const {return Camera_;}
        int getScale() const {return Scale_;}
        ScaleUnitE getScaleUnit() const {return ScaleUnit_;}

        void buildGalaxyMesh();
        void renderScale();
        void renderScene();
        void setRenderResFactor(const double _f) {RenderResFactorTarget_ = _f; this->updateRenderResFactor();}
        void setupCamera();
        void setupGraphics();
        void setWindowSize(const double _x, const double _y);

    private:

        static constexpr double TEXTURE_DECIMALS_MAX = std::log10(TEXTURE_SIZE_MAX);
        static constexpr int GALAXY_ZOOM_DECIMALS_MAX = int(7.225-TEXTURE_DECIMALS_MAX);
        static constexpr double GALAXY_ZOOM_MAX = ZoomComponent::CAMERA_ZOOM_DEFAULT * std::pow(10.0, GALAXY_ZOOM_DECIMALS_MAX);
        static constexpr std::array<double,5> GALAXY_SUB_LEVEL
            {1.0/8.0,
             1.0/16.0,
             1.0/32.0,
             1.0/64.0,
             1.0/128.0};

        void blur5x5(GL::Framebuffer* _FboFront, GL::Framebuffer* _FboBack,
                     GL::Texture2D* _TexFront, GL::Texture2D* _TexBack,
                     int _n, double _f);
        void blurSceneSSAA();
        void clampZoom();
        void renderGalaxy();
        void subSampleGalaxy();
        void testViewportGalaxy();
        void updateRenderResFactor();

        entt::registry& Reg_;
        PerformanceTimers& Timers_;

        double RenderResFactor_{2.0};
        double RenderResFactorTarget_{2.0};
        int TextureSizeMax_{1024};
        int WindowSizeX_{1024};
        int WindowSizeY_{768};

        bool IsSetup{false};

        GL::Mesh MeshGalaxy_{NoCreate};
        std::vector<GL::Mesh> CircleShapes_;
        GL::Mesh ScaleLineShapeH_{NoCreate};
        GL::Mesh ScaleLineShapeV_{NoCreate};
        Matrix3 ProjectionScene_;
        Matrix3 ProjectionWindow_;
        Shaders::VertexColor2D ShaderGalaxy_{NoCreate};
        Shaders::Flat2D Shader_{NoCreate};

        ColorPalette TemperaturePalette_;
        int Scale_{0};

        ScaleUnitE ScaleUnit_{ScaleUnitE::LY};

        //--- Framebuffer related ---//
        std::vector<GL::Framebuffer> FBOsGalaxySub0_;
        std::vector<GL::Framebuffer> FBOsGalaxySub1_;
        std::vector<GL::Framebuffer*> FBOsGalaxySubFront_;
        std::vector<GL::Framebuffer*> FBOsGalaxySubBack_;
        std::vector<GL::Texture2D> TexsGalaxySub0_;
        std::vector<GL::Texture2D> TexsGalaxySub1_;
        std::vector<GL::Texture2D*> TexsGalaxySubFront_;
        std::vector<GL::Texture2D*> TexsGalaxySubBack_;
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

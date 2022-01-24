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
#include "textures_weighted_avg_shader.hpp"

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

        DBLK(bool IsGalaxySubLevelsDisplayed{false};)

    private:

        static constexpr double TEXTURE_DECIMALS_MAX = std::log10(TEXTURE_SIZE_MAX);
        static constexpr int GALAXY_ZOOM_DECIMALS_MAX = int(7.225-TEXTURE_DECIMALS_MAX);
        static constexpr double GALAXY_ZOOM_MAX = ZoomComponent::CAMERA_ZOOM_DEFAULT * std::pow(10.0, GALAXY_ZOOM_DECIMALS_MAX);
        static constexpr int GALAXY_SUB_N{4};
        std::array<double, GALAXY_SUB_N> GALAXY_SUB_LEVEL
        {
             1.0/4.0,
             1.0/8.0,
             1.0/16.0,
             1.0/32.0,
             // 1.0/64.0,
        };
        static constexpr std::array<double, GALAXY_SUB_N> GALAXY_SUB_WEIGHTS
            {0.25,
             0.75,
             0.75,
             0.85};

        void blur5x5(GL::Framebuffer* _FboFront, GL::Framebuffer* _FboBack,
                     GL::Texture2D* _TexFront, GL::Texture2D* _TexBack,
                     int _Sx, int _Sy,
                     int _n, double _f);
        void blurSceneSSAA();
        void checkGalaxyTextureSizes();
        void clampZoom();
        void createFBOandTex(GL::Framebuffer* const _Fbo, GL::Texture2D* const _Tex, int _SizeX, int _SizeY);
        void renderGalaxy(double _Scale, bool _IsRenderResFactorConsidered = false);
        void subSampleGalaxy();
        void testViewportGalaxy();
        void updateRenderResFactor();

        entt::registry& Reg_;
        PerformanceTimers& Timers_;

        double RenderResFactor_{2.0};
        double RenderResFactorTarget_{2.0};
        int TextureSizeMax_{1024};
        int TextureSizeSubMax_{1024};
        int WindowSizeX_{1024};
        int WindowSizeY_{768};

        int GalaxyBlurIterations_{5};

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
        GL::Framebuffer* FBOGalaxyLevelCombinerFront_{nullptr};
        GL::Framebuffer* FBOGalaxyLevelCombinerBack_{nullptr};
        GL::Framebuffer FBOGalaxyLevelCombiner0_{NoCreate};
        GL::Framebuffer FBOGalaxyLevelCombiner1_{NoCreate};
        GL::Framebuffer* FBOGalaxyTemporalSmoothingFront_{nullptr};
        GL::Framebuffer* FBOGalaxyTemporalSmoothingBack_{nullptr};
        GL::Framebuffer FBOGalaxyTemporalSmoothing0_{NoCreate};
        GL::Framebuffer FBOGalaxyTemporalSmoothing1_{NoCreate};
        GL::Framebuffer* FBOMainDisplayFront_{nullptr};
        GL::Framebuffer* FBOMainDisplayBack_{nullptr};
        GL::Framebuffer FBOMainDisplay0_{NoCreate};
        GL::Framebuffer FBOMainDisplay1_{NoCreate};
        GL::Mesh MeshMainDisplay_{NoCreate};
        GL::Mesh MeshBlur5x1_{NoCreate};
        GL::Mesh MeshWeightedAvg_{NoCreate};
        GL::Texture2D* TexGalaxyLevelCombinerFront_{nullptr};
        GL::Texture2D* TexGalaxyLevelCombinerBack_{nullptr};
        GL::Texture2D TexGalaxyLevelCombiner0_{NoCreate};
        GL::Texture2D TexGalaxyLevelCombiner1_{NoCreate};
        GL::Texture2D* TexGalaxyTemporalSmoothingFront_{nullptr};
        GL::Texture2D* TexGalaxyTemporalSmoothingBack_{nullptr};
        GL::Texture2D TexGalaxyTemporalSmoothing0_{NoCreate};
        GL::Texture2D TexGalaxyTemporalSmoothing1_{NoCreate};
        GL::Texture2D* TexMainDisplayFront_{nullptr};
        GL::Texture2D* TexMainDisplayBack_{nullptr};
        GL::Texture2D TexMainDisplay0_{NoCreate};
        GL::Texture2D TexMainDisplay1_{NoCreate};
        BlurShader5x1 ShaderBlur5x1_{NoCreate};
        MainDisplayShader ShaderMainDisplay_{NoCreate};
        TexturesWeightedAvgShader ShaderWeightedAvg_{NoCreate};

        // --- Graphics - Camera ---//
        entt::entity Camera_;

        double StarsDisplaySizeMin_{1.0};
        double StarsDisplayScaleFactor_{1.0};

};

#endif // RENDER_SYSTEM_HPP

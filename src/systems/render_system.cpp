#include "render_system.hpp"

#include <Corrade/Containers/ArrayViewStl.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Primitives/Line.h>
#include <Magnum/Shaders/VertexColor.h>
#include <Magnum/Trade/MeshData.h>

#include "components.hpp"
#include "message_handler.hpp"

using namespace Magnum;

RenderSystem::RenderSystem(entt::registry& _Reg, PerformanceTimers& _Timers) :
    Reg_(_Reg),
    Timers_(_Timers),
    TemperaturePalette_(_Reg, 256, {0.95, 0.58, 0.26}, {0.47, 0.56, 1.0})
{}

void RenderSystem::buildGalaxyMesh()
{
    GL::Buffer Buffer{};

    MeshGalaxy_ = GL::Mesh{};
    std::vector<float> Pos;

    Reg_.view<SystemPositionComponent, RadiusComponent, StarDataComponent>().each(
        [&](auto _e, const auto& _p, const auto& _r, const auto& _s)
    {
        Pos.push_back(_p.x);
        Pos.push_back(_p.y);

        auto Pal = TemperaturePalette_.getColorClip((_s.Temperature-2000.0)/45000.0);
        for (auto i=0u; i<3u; ++i) Pos.push_back(Pal[i]);
    });

    Buffer.setData(Pos, GL::BufferUsage::StaticDraw);
    MeshGalaxy_.setCount(Pos.size()/5)
               .setPrimitive(GL::MeshPrimitive::Points)
               .addVertexBuffer(std::move(Buffer), 0, Shaders::VertexColor2D::Position{}, Shaders::VertexColor2D::Color3{});

    IsSetup = true;
}

void RenderSystem::renderScale()
{
    auto& Zoom = Reg_.get<ZoomComponent>(Camera_);

    constexpr double SCALE_BAR_SIZE_MAX = 0.2; // 20% of display width

    double BarLength{1.0};
    double Ratio = (SCALE_BAR_SIZE_MAX * WindowSizeX_) / Zoom.z;
    if (Ratio > 9.46e21)
    {
        BarLength = 9.46e21; // One million lightyears
        ScaleUnit_ = ScaleUnitE::MLY;
    }
    else if (Ratio > 9.46e15)
    {
        BarLength = 9.46e15; // One lightyear
        ScaleUnit_ = ScaleUnitE::LY;
    }
    else if (Ratio > 1.0e9)
    {
        BarLength = 1.0e9;
        ScaleUnit_ = ScaleUnitE::MKM;
    }
    else if (Ratio > 1.0e3)
    {
        BarLength = 1.0e3;
        ScaleUnit_ = ScaleUnitE::KM;
    }
    else
    {
        BarLength = 1.0;
        ScaleUnit_ = ScaleUnitE::M;
    }

    double Scale = WindowSizeX_ * SCALE_BAR_SIZE_MAX / (BarLength*Zoom.z);

    Scale_ = int(std::log10(Scale));

    Shader_.setTransformationProjectionMatrix(
        ProjectionWindow_ * Matrix3::scaling(
            Vector2(BarLength*0.5*std::pow(10, Scale_)*Zoom.z,
                    0.5 * WindowSizeY_ - 10)
    ));

    Shader_.setColor({0.8, 0.8, 1.0})
           .draw(ScaleLineShapeH_);

    Shader_.setTransformationProjectionMatrix(
        ProjectionWindow_ *
        Matrix3::translation(
            Vector2(- BarLength*0.5*std::pow(10, Scale_)*Zoom.z,
                    0.5 * WindowSizeY_ - 10)) *
        Matrix3::scaling(
            Vector2(1.0, 7.0))
    );
    Shader_.draw(ScaleLineShapeV_);

    Shader_.setTransformationProjectionMatrix(
        ProjectionWindow_ *
        Matrix3::translation(
            Vector2(BarLength*0.5*std::pow(10, Scale_)*Zoom.z,
                    0.5 * WindowSizeY_ - 10)) *
        Matrix3::scaling(
            Vector2(1.0, 7.0))
    );
    Shader_.draw(ScaleLineShapeV_);
}

void RenderSystem::renderScene()
{
    //----------------------------------
    // Adjust viewports and projections
    //----------------------------------
    // FBO viewport may not exceed maximum size of underlying texture. Above
    // this size, texture won't be pixel perfect but interpolated
    //
    GL::Renderer::setScissor({{0, 0}, {int(WindowSizeX_*RenderResFactor_), int(WindowSizeY_*RenderResFactor_)}});

    FBOMainDisplayFront_->clearColor(0, Color4(0.0f, 0.0f, 0.0f, 1.0f))
                         .setViewport({{0, 0}, {int(WindowSizeX_*RenderResFactor_), int(WindowSizeY_*RenderResFactor_)}})
                         .bind();

    this->clampZoom();
    this->testViewportGalaxy();

    Timers_.Render.start();

    this->renderGalaxy(1.0, true);
    this->subSampleGalaxy();
    this->blurSceneSSAA();

    double Weight = 0.75;

    if (Reg_.get<ZoomComponent>(Camera_).z >= 1.0e-13)
    {
        Weight = 0.75 * (1.0 - (Reg_.get<ZoomComponent>(Camera_).z - 1.0e-13));
    }

    std::swap(FBOMainDisplayFront_, FBOMainDisplayBack_);
    std::swap(TexMainDisplayFront_, TexMainDisplayBack_);

    FBOMainDisplayFront_->clearColor(0, Color4(0.0f, 0.0f, 0.0f, 1.0f))
                         .setViewport({{0, 0}, {int(WindowSizeX_*RenderResFactor_), int(WindowSizeY_*RenderResFactor_)}})
                         .bind();
    ShaderWeightedAvg_.bindTextures(*TexMainDisplayBack_, *TexGalaxyTemporalSmoothingBack_)
                      .setTexScale((RenderResFactor_*WindowSizeX_)/TextureSizeMax_,
                                   (RenderResFactor_*WindowSizeY_)/TextureSizeMax_)
                      .setSigma(GALAXY_SUB_LEVEL[0]*TextureSizeMax_/(TextureSizeSubMax_*RenderResFactor_))
                      .setWeight(Weight)
                      .draw(MeshWeightedAvg_);

    GL::Renderer::setScissor({{0, 0}, {WindowSizeX_, WindowSizeY_}});

    GL::defaultFramebuffer.clearColor(Color4(0.0f, 0.0f, 0.0f, 1.0f))
                          .setViewport({{}, {WindowSizeX_, WindowSizeY_}})
                          .bind();

    ShaderMainDisplay_.bindTexture(*TexMainDisplayFront_)
                      .setTexScale((RenderResFactor_*WindowSizeX_)/TextureSizeMax_,
                                   (RenderResFactor_*WindowSizeY_)/TextureSizeMax_)
                      .draw(MeshMainDisplay_);

    for (auto i=0; i<GALAXY_SUB_N; ++i)
    {
        GL::defaultFramebuffer.setViewport({{i*int(WindowSizeX_*1.0/GALAXY_SUB_N), 0},
                                           {(i+1)*int(WindowSizeX_*1.0/GALAXY_SUB_N), int(WindowSizeY_*1.0/GALAXY_SUB_N)}});

        ShaderMainDisplay_.bindTexture(*TexsGalaxySubFront_[i])
                          .setTexScale(double(WindowSizeX_)/TextureSizeSubMax_*GALAXY_SUB_LEVEL[i],
                                       double(WindowSizeY_)/TextureSizeSubMax_*GALAXY_SUB_LEVEL[i])
                          .draw(MeshMainDisplay_);
    }
    // GL::defaultFramebuffer.setViewport({{0, int(WindowSizeY_*1.0/GALAXY_SUB_N)},
    //                                    {int(WindowSizeX_*1.0/GALAXY_SUB_N), 2*int(WindowSizeY_*1.0/GALAXY_SUB_N)}});

    // ShaderMainDisplay_.bindTexture(*TexGalaxyLevelCombinerFront_)
    //                   .setTexScale(double(WindowSizeX_)/TextureSizeSubMax_*GALAXY_SUB_LEVEL[GALAXY_SUB_N-2],
    //                                double(WindowSizeY_)/TextureSizeSubMax_*GALAXY_SUB_LEVEL[GALAXY_SUB_N-2])
    //                   .draw(MeshMainDisplay_);

    GL::defaultFramebuffer.setViewport({{}, {WindowSizeX_, WindowSizeY_}});

    Timers_.Render.stop();
    Timers_.RenderAvg.addValue(Timers_.Render.elapsed());

    // Reg_.view<TireComponent, PositionComponent>().each(
        // [&](auto _e, const auto& _t, const auto& _p)
    // {
    //     auto x = _p.x;
    //     auto y = _p.y;

    //     x += CamPosSys.x - HookPosSys.x;
    //     y += CamPosSys.y - HookPosSys.y;

    //     x += CamPos.x;
    //     y += CamPos.y;

    //     if (HookPos != nullptr)
    //     {
    //         x -= HookPos->x;
    //         y -= HookPos->y;
    //     }

    //     x *= Zoom.z;
    //     y *= Zoom.z;

    //     auto r = _t.RimR;
    //     r *= Zoom.z;
    //     double RenderScale = 1.0;
    //     if (RenderResFactor_ < 1.0) RenderScale = 1.0/RenderResFactor_;
    //     if (r < RenderScale)
    //     {
    //         r=RenderScale;
    //     }

    //     Shader_.setTransformationProjectionMatrix(
    //         ProjectionScene_ *
    //         Matrix3::translation(Vector2(x, y)) *
    //         Matrix3::scaling(Vector2(r, r))
    //     );

    //     Shader_.setColor({1.0, 1.0, 1.0});
    //     if (r < 10)
    //         Shader_.draw(CircleShapes_[0]);
    //     else if (r < 300)
    //         Shader_.draw(CircleShapes_[1]);
    //     else
    //         Shader_.draw(CircleShapes_[2]);

    //     std::array<Vector2, TireComponent::SEGMENTS> VertsTire;
    //     for (auto i=0u; i<TireComponent::SEGMENTS; ++i)
    //     {
    //         auto x = _t.RubberX[i];
    //         auto y = _t.RubberY[i];

    //         x += CamPosSys.x - HookPosSys.x;
    //         y += CamPosSys.y - HookPosSys.y;

    //         x += CamPos.x;
    //         y += CamPos.y;

    //         if (HookPos != nullptr)
    //         {
    //             x -= HookPos->x;
    //             y -= HookPos->y;
    //         }

    //         x *= Zoom.z;
    //         y *= Zoom.z;

    //         auto r = 1.0;
    //         r *= Zoom.z;
    //         double RenderScale = 1.0;
    //         if (RenderResFactor_ < 1.0) RenderScale = 1.0/RenderResFactor_;
    //         if (r < RenderScale)
    //         {
    //             r=RenderScale;
    //         }
    //         VertsTire[i] = {float(x), float(y)};
    //     }

    //     GL::Mesh Mesh;
    //     GL::Buffer Buffer;
    //     Buffer.setData(VertsTire, GL::BufferUsage::DynamicDraw);
    //     Mesh.setCount(TireComponent::SEGMENTS)
    //         .setPrimitive(GL::MeshPrimitive::LineLoop)
    //         .addVertexBuffer(std::move(Buffer), 0, Shaders::VertexColor2D::Position{});

    //     Shader_.setTransformationProjectionMatrix(
    //         ProjectionScene_ *
    //         Matrix3::translation(Vector2(0.0, 0.0)) *
    //         Matrix3::scaling(Vector2(1.0, 1.0))
    //     );

    //     Magnum::GL::Renderer::setLineWidth(RenderScale*2.0);
    //     Shader_.draw(Mesh);
    //     Magnum::GL::Renderer::setLineWidth(1.0f);

    // }
    // );

}

void RenderSystem::setupCamera()
{
    Camera_ = Reg_.create();
    auto& HookDummy = Reg_.emplace<HookDummyComponent>(Camera_);
    HookDummy.e = Reg_.create();
    Reg_.emplace<PositionComponent>(HookDummy.e);
    Reg_.emplace<SystemPositionComponent>(HookDummy.e);

    auto& Hook = Reg_.emplace<HookComponent>(Camera_);
    Hook.e = HookDummy.e;

    Reg_.emplace<PositionComponent>(Camera_);
    Reg_.emplace<SystemPositionComponent>(Camera_);
    Reg_.emplace<ZoomComponent>(Camera_);
}

void RenderSystem::setupGraphics()
{
    GLint TextureSizeMax; glGetIntegerv (GL_MAX_TEXTURE_SIZE, &TextureSizeMax);
    DBLK(Reg_.ctx<MessageHandler>().report("gfx", "Maximum texture size: "
                                           + std::to_string(TextureSizeMax)
                                           +"x"
                                           + std::to_string(TextureSizeMax),
                                           MessageHandler::DEBUG_L1);)
    if (TextureSizeMax > TEXTURE_SIZE_MAX)
    {
        TextureSizeMax_ = TEXTURE_SIZE_MAX;
        DBLK(Reg_.ctx<MessageHandler>().report("gfx", "More than enough, using maximum texture size of "
                                            + std::to_string(TextureSizeMax_)
                                            +"x"
                                            + std::to_string(TextureSizeMax_),
                                            MessageHandler::DEBUG_L1);)
    }
    else
    {
        TextureSizeMax_ = TextureSizeMax;
    }

    if (TextureSizeMax < TextureSizeSubMax_)
    {
        TextureSizeSubMax_ = TextureSizeMax;
        Reg_.ctx<MessageHandler>().report("gfx", "Minimum resolution for galaxy rendering not met. Graphics might not be of intended quality", MessageHandler::WARNING);
    }

    ShaderGalaxy_ = Shaders::VertexColor2D{};
    Shader_ = Shaders::Flat2D{};
    CircleShapes_.push_back(MeshTools::compile(Primitives::circle2DSolid(10)));
    CircleShapes_.push_back(MeshTools::compile(Primitives::circle2DSolid(100)));
    CircleShapes_.push_back(MeshTools::compile(Primitives::circle2DSolid(1000)));
    ScaleLineShapeH_ = MeshTools::compile(Primitives::line2D({-1.0, 1.0},
                                                             { 1.0, 1.0}));
    ScaleLineShapeV_ = MeshTools::compile(Primitives::line2D({ 1.0, -1.0},
                                                             { 1.0,  1.0}));

    TemperaturePalette_.addSupportPoint(0.03, {1.0, 0.8, 0.21});
    TemperaturePalette_.addSupportPoint(0.11, {1.0, 0.93, 0.27});
    TemperaturePalette_.addSupportPoint(0.15, {1.0, 0.97, 0.7});
    TemperaturePalette_.addSupportPoint(0.19, {0.82, 0.92, 1.0});
    TemperaturePalette_.addSupportPoint(0.42, {0.4, 0.74, 1.0});
    TemperaturePalette_.buildLuT();

    //--- FBOs ---//
    this->createFBOandTex(&FBOMainDisplay0_, &TexMainDisplay0_, TextureSizeMax_, TextureSizeMax_);
    this->createFBOandTex(&FBOMainDisplay1_, &TexMainDisplay1_, TextureSizeMax_, TextureSizeMax_);

    FBOMainDisplayBack_ = &FBOMainDisplay0_;
    FBOMainDisplayFront_ = &FBOMainDisplay1_;

    TexMainDisplayBack_ = &TexMainDisplay0_;
    TexMainDisplayFront_ = &TexMainDisplay1_;

    //--- Shaders ---//
    ShaderMainDisplay_ = MainDisplayShader{};
    ShaderMainDisplay_.bindTexture(TexMainDisplay1_);

    ShaderBlur5x1_ = BlurShader5x1{};
    ShaderBlur5x1_.setHorizontal(true)
                  .bindTexture(TexMainDisplay0_);


    //--- Meshes ---//
    MeshMainDisplay_ = GL::Mesh{};
    MeshMainDisplay_.setCount(3)
                    .setPrimitive(GL::MeshPrimitive::Triangles);
    MeshBlur5x1_ = GL::Mesh{};
    MeshBlur5x1_.setCount(3)
                .setPrimitive(GL::MeshPrimitive::Triangles);
    MeshWeightedAvg_ = GL::Mesh{};
    MeshWeightedAvg_.setCount(3)
                    .setPrimitive(GL::MeshPrimitive::Triangles);

    // Setup FBOs and textures for galaxy subsampling
    for(auto i=0u; i<GALAXY_SUB_N; ++i)
    {
        FBOsGalaxySub0_.push_back(GL::Framebuffer{NoCreate});
        TexsGalaxySub0_.push_back(GL::Texture2D{NoCreate});
        FBOsGalaxySub1_.push_back(GL::Framebuffer{NoCreate});
        TexsGalaxySub1_.push_back(GL::Texture2D{NoCreate});
    }
    for (auto i=0u; i<GALAXY_SUB_N; ++i)
    {
        this->createFBOandTex(&FBOsGalaxySub0_[i], &TexsGalaxySub0_[i], TextureSizeSubMax_, TextureSizeSubMax_);
        this->createFBOandTex(&FBOsGalaxySub1_[i], &TexsGalaxySub1_[i], TextureSizeSubMax_, TextureSizeSubMax_);

    }
    for (auto i=0u; i<GALAXY_SUB_N; ++i)
    {
        FBOsGalaxySubFront_.push_back(&(FBOsGalaxySub0_[i]));
        FBOsGalaxySubBack_.push_back(&(FBOsGalaxySub1_[i]));
        TexsGalaxySubFront_.push_back(&(TexsGalaxySub0_[i]));
        TexsGalaxySubBack_.push_back(&(TexsGalaxySub1_[i]));
    }

    // Setup FBOs and textures for galaxy combiner
    this->createFBOandTex(&FBOGalaxyLevelCombiner0_, &TexGalaxyLevelCombiner0_, TextureSizeSubMax_, TextureSizeSubMax_);
    this->createFBOandTex(&FBOGalaxyLevelCombiner1_, &TexGalaxyLevelCombiner1_, TextureSizeSubMax_, TextureSizeSubMax_);

    FBOGalaxyLevelCombinerFront_ = &FBOGalaxyLevelCombiner0_;
    FBOGalaxyLevelCombinerBack_ = &FBOGalaxyLevelCombiner1_;
    TexGalaxyLevelCombinerFront_ = &TexGalaxyLevelCombiner0_;
    TexGalaxyLevelCombinerBack_ = &TexGalaxyLevelCombiner1_;

    // Setup FBOs and textures for temporal smoothing
    this->createFBOandTex(&FBOGalaxyTemporalSmoothing0_, &TexGalaxyTemporalSmoothing0_, TextureSizeSubMax_, TextureSizeSubMax_);
    this->createFBOandTex(&FBOGalaxyTemporalSmoothing1_, &TexGalaxyTemporalSmoothing1_, TextureSizeSubMax_, TextureSizeSubMax_);

    FBOGalaxyTemporalSmoothingFront_ = &FBOGalaxyTemporalSmoothing0_;
    FBOGalaxyTemporalSmoothingBack_ = &FBOGalaxyTemporalSmoothing1_;
    TexGalaxyTemporalSmoothingFront_ = &TexGalaxyTemporalSmoothing0_;
    TexGalaxyTemporalSmoothingBack_ = &TexGalaxyTemporalSmoothing1_;

    ShaderWeightedAvg_ = TexturesWeightedAvgShader{};
    ShaderWeightedAvg_.bindTextures(*TexsGalaxySubFront_[0], *TexsGalaxySubFront_[1])
                      .setSigma(GALAXY_SUB_LEVEL[1]);

    GL::Renderer::setScissor({{0, 0}, {int(WindowSizeX_*RenderResFactor_), int(WindowSizeY_*RenderResFactor_)}});
}

void RenderSystem::setWindowSize(const double _x, const double _y)
{
    WindowSizeX_ = _x;
    WindowSizeY_ = _y;

    this->updateRenderResFactor();

    ProjectionScene_= Magnum::Matrix3::projection(Magnum::Vector2(_x, _y));
    ProjectionWindow_= Magnum::Matrix3::projection(Magnum::Vector2(_x, _y));
}

void RenderSystem::blur5x5(GL::Framebuffer* _FboFront, GL::Framebuffer* _FboBack,
                           GL::Texture2D* _TexFront, GL::Texture2D* _TexBack,
                           int _Sx, int _Sy,
                           int _n, double _f)
{
    // Blur framebuffer 5x5 Gaussian kernel with _n iterations on subsampling factor _f
    // Viewport size is given by _Sx/_Sy times _f

    for (auto i=0; i<_n; ++i)
    {
        std::swap(_FboFront, _FboBack);
        std::swap(_TexFront, _TexBack);

        _FboFront->clearColor(0, Color4(0.0f, 0.0f, 0.0f, 1.0f))
                  .setViewport({{0, 0}, {int(_f * _Sx), int(_f * _Sy)}})
                  .bind();

        ShaderBlur5x1_.bindTexture(*_TexBack)
                      .setHorizontal(true)
                      .draw(MeshBlur5x1_);

        std::swap(_FboFront, _FboBack);
        std::swap(_TexFront, _TexBack);

        _FboFront->clearColor(0, Color4(0.0f, 0.0f, 0.0f, 1.0f))
                  .setViewport({{0, 0}, {int(_f * _Sx), int(_f * _Sy)}})
                  .bind();

        ShaderBlur5x1_.bindTexture(*_TexBack)
                      .setHorizontal(false)
                      .draw(MeshBlur5x1_);
    }

}

void RenderSystem::blurSceneSSAA()
{
    this->blur5x5(FBOMainDisplayFront_, FBOMainDisplayBack_,
                  TexMainDisplayFront_, TexMainDisplayBack_,
                  WindowSizeX_*RenderResFactor_, WindowSizeY_*RenderResFactor_,
                  int(RenderResFactor_+0.5)/2, 1.0);
}

void RenderSystem::clampZoom()
{
    auto& Zoom = Reg_.get<ZoomComponent>(Camera_);

    if (Zoom.c < Zoom.s && Zoom.t != Zoom.z)
    {
        Zoom.z += Zoom.i;
        Zoom.c++;
    }
    else
    {
        Zoom.c = 0;
        Zoom.t = Zoom.z;
    }
    if (Zoom.z < 1.0e-22) Zoom.z = 1.0e-22;
    else if (Zoom.z > 1000.0) Zoom.z = 1000.0;
}

void RenderSystem::createFBOandTex(GL::Framebuffer* const _Fbo,
                                   GL::Texture2D* const _Tex,
                                   int _SizeX, int _SizeY)
{
    *_Tex = GL::Texture2D{};
    _Tex->setMagnificationFilter(GL::SamplerFilter::Linear)
         .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear)
         .setWrapping(GL::SamplerWrapping::ClampToBorder)
         .setMaxAnisotropy(GL::Sampler::maxMaxAnisotropy())
         .setStorage(1, GL::TextureFormat::RGBA8, {_SizeX, _SizeY});

    *_Fbo = GL::Framebuffer{{{0, 0},{_SizeX, _SizeY}}};
    _Fbo->attachTexture(GL::Framebuffer::ColorAttachment{0}, *_Tex, 0)
         .clearColor(0, Color4(0.0f, 0.0f, 0.0f, 1.0f));
}

void RenderSystem::renderGalaxy(double _Scale, bool _IsRenderResFactorConsidered)
{
    auto& HookPosSys = Reg_.get<SystemPositionComponent>(Reg_.get<HookComponent>(Camera_).e);
    auto* HookPos    = Reg_.try_get<PositionComponent>(Reg_.get<HookComponent>(Camera_).e);
    auto& CamPosSys = Reg_.get<SystemPositionComponent>(Camera_);
    auto& CamPos = Reg_.get<PositionComponent>(Camera_);
    auto& Zoom = Reg_.get<ZoomComponent>(Camera_);

    if (IsSetup && Zoom.z < GALAXY_ZOOM_MAX)
    {
        auto x = CamPosSys.x-HookPosSys.x;
        auto y = CamPosSys.y-HookPosSys.y;
        x += CamPos.x;
        y += CamPos.y;
        if (HookPos != nullptr)
        {
            x-=HookPos->x;
            y-=HookPos->y;
        }

        if (_IsRenderResFactorConsidered)
            glPointSize(RenderResFactor_*1.5);
        else
            glPointSize(1.5);
        ShaderGalaxy_.setTransformationProjectionMatrix(
            ProjectionScene_ *
            Matrix3::translation(Vector2(x*Zoom.z, y*Zoom.z)) *
            Matrix3::scaling(Vector2(Zoom.z, Zoom.z))
        );

        ShaderGalaxy_.draw(MeshGalaxy_);
    }
    else
    {
        Reg_.view<SystemPositionComponent, RadiusComponent, InsideViewportTag>().each(
            [&](auto _e, const auto& _p, const auto& _r)
        {
            auto x = _p.x;
            auto y = _p.y;

            x += CamPosSys.x - HookPosSys.x;
            y += CamPosSys.y - HookPosSys.y;

            x += CamPos.x;
            y += CamPos.y;

            if (HookPos != nullptr)
            {
                x -= HookPos->x;
                y -= HookPos->y;
            }

            auto* p = Reg_.try_get<PositionComponent>(_e);
            if (p != nullptr)
            {
                x += p->x;
                y += p->y;
            }

            x *= Zoom.z;
            y *= Zoom.z;

            auto r = _r.r;
            r *= Zoom.z * StarsDisplayScaleFactor_;
            double RenderScale = 1.0;
            if (RenderResFactor_ < 1.0) RenderScale = 1.0/RenderResFactor_;
            if (r < StarsDisplaySizeMin_*RenderScale)
            {
                r=StarsDisplaySizeMin_*RenderScale;
            }
            r *= _Scale;

            Shader_.setTransformationProjectionMatrix(
                ProjectionScene_ *
                Matrix3::translation(Vector2(x, y)) *
                Matrix3::scaling(Vector2(r, r))
            );

            auto* s = Reg_.try_get<StarDataComponent>(_e);
            if (s != nullptr)
            {
                Shader_.setColor(TemperaturePalette_.getColorClip((s->Temperature-2000.0)/45000.0));
            }
            else
            {
                Shader_.setColor({0.0, 0.0, 1.0});
            }
            if (r < 10)
                Shader_.draw(CircleShapes_[0]);
            else if (r < 300)
                Shader_.draw(CircleShapes_[1]);
            else
                Shader_.draw(CircleShapes_[2]);
        }
        );
    }
}

void RenderSystem::subSampleGalaxy()
{
    for (auto i=0u; i<GALAXY_SUB_N; ++i)
    {
        FBOsGalaxySubFront_[i]->clearColor(0, Color4(0.0f, 0.0f, 0.0f, 1.0f))
                        .setViewport({{},{int(WindowSizeX_* GALAXY_SUB_LEVEL[i]),
                                          int(WindowSizeY_* GALAXY_SUB_LEVEL[i])}})
                        .bind();

        this->renderGalaxy(1.0/GALAXY_SUB_LEVEL[i]);
        this->blur5x5(FBOsGalaxySubFront_[i], FBOsGalaxySubBack_[i], TexsGalaxySubFront_[i], TexsGalaxySubBack_[i],
                      WindowSizeX_, WindowSizeY_, GalaxyBlurIterations_, GALAXY_SUB_LEVEL[i]);
    }

    FBOGalaxyLevelCombinerFront_->clearColor(0, Color4(0.0f, 0.0f, 0.0f, 1.0f))
                                 .setViewport({{},{int(WindowSizeX_ * GALAXY_SUB_LEVEL[GALAXY_SUB_N-2]),
                                                   int(WindowSizeY_ * GALAXY_SUB_LEVEL[GALAXY_SUB_N-2])}})
                                 .bind();

    ShaderWeightedAvg_.bindTextures(*TexsGalaxySubFront_[GALAXY_SUB_N-2], *TexsGalaxySubFront_[GALAXY_SUB_N-1])
                      .setTexScale(double(WindowSizeX_)/TextureSizeSubMax_*GALAXY_SUB_LEVEL[GALAXY_SUB_N-2],
                                   double(WindowSizeY_)/TextureSizeSubMax_*GALAXY_SUB_LEVEL[GALAXY_SUB_N-2])
                      .setSigma(GALAXY_SUB_LEVEL[GALAXY_SUB_N-1]/GALAXY_SUB_LEVEL[GALAXY_SUB_N-2])
                      .setWeight(GALAXY_SUB_WEIGHTS[GALAXY_SUB_N-2])
                      .draw(MeshWeightedAvg_);

    std::swap(FBOGalaxyLevelCombinerFront_, FBOGalaxyLevelCombinerBack_);
    std::swap(TexGalaxyLevelCombinerFront_, TexGalaxyLevelCombinerBack_);

    for (auto i=GALAXY_SUB_N-2; i>0; --i)
    {
        FBOGalaxyLevelCombinerFront_->clearColor(0, Color4(0.0f, 0.0f, 0.0f, 1.0f))
                                    .setViewport({{},{int(WindowSizeX_ * GALAXY_SUB_LEVEL[i-1]),
                                                      int(WindowSizeY_ * GALAXY_SUB_LEVEL[i-1])}})
                                    .bind();

        ShaderWeightedAvg_.bindTextures(*TexsGalaxySubFront_[i-1], *TexGalaxyLevelCombinerBack_)
                          .setTexScale(double(WindowSizeX_)/TextureSizeSubMax_*GALAXY_SUB_LEVEL[i-1],
                                       double(WindowSizeY_)/TextureSizeSubMax_*GALAXY_SUB_LEVEL[i-1])
                          .setSigma(GALAXY_SUB_LEVEL[i]/GALAXY_SUB_LEVEL[i-1])
                          .setWeight(GALAXY_SUB_WEIGHTS[i-1])
                          .draw(MeshWeightedAvg_);

        std::swap(FBOGalaxyLevelCombinerFront_, FBOGalaxyLevelCombinerBack_);
        std::swap(TexGalaxyLevelCombinerFront_, TexGalaxyLevelCombinerBack_);
    }

    // Temporal Smoothing

    FBOGalaxyTemporalSmoothingFront_->clearColor(0, Color4(0.0f, 0.0f, 0.0f, 1.0f))
                                     .setViewport({{},{int(WindowSizeX_ * GALAXY_SUB_LEVEL[0]),
                                                       int(WindowSizeY_ * GALAXY_SUB_LEVEL[0])}})
                                     .bind();

    ShaderWeightedAvg_.bindTextures(*TexGalaxyLevelCombinerBack_, *TexGalaxyTemporalSmoothingBack_)
                      .setTexScale(double(WindowSizeX_)/TextureSizeSubMax_*GALAXY_SUB_LEVEL[0],
                                   double(WindowSizeY_)/TextureSizeSubMax_*GALAXY_SUB_LEVEL[0])
                      .setSigma(1.0)
                      .setWeight(0.75)
                      .draw(MeshWeightedAvg_);

    GL::AbstractFramebuffer::blit(*FBOGalaxyTemporalSmoothingFront_, *FBOGalaxyTemporalSmoothingBack_,
                                  {{},{int(WindowSizeX_ * GALAXY_SUB_LEVEL[0]),
                                       int(WindowSizeY_ * GALAXY_SUB_LEVEL[0])}},
                                  GL::FramebufferBlit::Color
    );

    std::swap(FBOGalaxyTemporalSmoothingFront_, FBOGalaxyTemporalSmoothingBack_);
    std::swap(TexGalaxyTemporalSmoothingFront_, TexGalaxyTemporalSmoothingBack_);
}

void RenderSystem::testViewportGalaxy()
{
    // Remove all "inside" tags from objects
    // After that, test all objects for camera viewport and tag
    // appropriatly

    Timers_.ViewportTest.start();

    auto& HookPosSys = Reg_.get<SystemPositionComponent>(Reg_.get<HookComponent>(Camera_).e);
    auto* HookPos    = Reg_.try_get<PositionComponent>(Reg_.get<HookComponent>(Camera_).e);
    auto& CamPosSys = Reg_.get<SystemPositionComponent>(Camera_);
    auto& CamPos = Reg_.get<PositionComponent>(Camera_);
    auto& Zoom = Reg_.get<ZoomComponent>(Camera_);

    if (Zoom.z > GALAXY_ZOOM_MAX)
    {
        Reg_.clear<InsideViewportTag>();

        const double ScreenX = WindowSizeX_;
        const double ScreenY = WindowSizeY_;

        Reg_.view<SystemPositionComponent>().each(
            [&](auto _e, const auto& _p_s)
            {
                auto x = _p_s.x;
                auto y = _p_s.y;

                x += CamPosSys.x - HookPosSys.x;
                y += CamPosSys.y - HookPosSys.y;

                x += CamPos.x;
                y += CamPos.y;

                auto* p = Reg_.try_get<PositionComponent>(_e);
                if (p != nullptr)
                {
                    x += p->x;
                    y += p->y;
                }
                if (HookPos != nullptr)
                {
                    x -= HookPos->x;
                    y -= HookPos->y;
                }

                x *= Zoom.z;
                y *= Zoom.z;

                if ((x+0.5*ScreenX > 0) && (x <= 0.5*ScreenX) &&
                    (y+0.5*ScreenY > 0) && (y <= 0.5*ScreenY))
                {
                    Reg_.emplace<InsideViewportTag>(_e);
                }
            });
    }

    Timers_.ViewportTest.stop();
    Timers_.ViewportTestAvg.addValue(Timers_.ViewportTest.elapsed());
}

void RenderSystem::updateRenderResFactor()
{
    RenderResFactor_ = RenderResFactorTarget_;

    if (WindowSizeX_ >= WindowSizeY_)
    {
        if (RenderResFactor_ * WindowSizeX_ > TextureSizeMax_)
        {
            RenderResFactor_ = double(TextureSizeMax_) / WindowSizeX_;
        }
    }
    else
    {
        if (RenderResFactor_ * WindowSizeY_ > TextureSizeMax_)
        {
            RenderResFactor_ = double(TextureSizeMax_) / WindowSizeY_;
        }
    }
    DBLK(Reg_.ctx<MessageHandler>().report("gfx", "Setting render resolution factor to "
                                           + std::to_string(RenderResFactor_),
                                           MessageHandler::DEBUG_L1);)
}

#include "render_system.hpp"

#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Primitives/Line.h>
#include <Magnum/Trade/MeshData.h>

#include "components.hpp"

using namespace Magnum;

RenderSystem::RenderSystem(entt::registry& _Reg) :
    Reg_(_Reg),
    TemperaturePalette_(_Reg, 256, {0.95, 0.58, 0.26}, {0.47, 0.56, 1.0})
{}

void RenderSystem::renderScale()
{
    auto& Zoom = Reg_.get<ZoomComponent>(Camera_);

    constexpr double SCALE_BAR_SIZE_MAX = 0.2; // 20% of display width

    double BarLength{1.0};
    // double Ratio = WindowSizeX_ * SCALE_BAR_SIZE_MAX / (BarLength*Zoom.z);
    double Ratio = WindowSizeX_ * SCALE_BAR_SIZE_MAX / Zoom.z;
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
        Projection_ * Matrix3::scaling(
            Vector2(BarLength*0.5*std::pow(10, Scale_)*Zoom.z,
                    0.5 * WindowSizeY_ - 10)
    ));

    Shader_.setColor({0.8, 0.8, 1.0})
           .draw(ScaleLineShapeH_);

    Shader_.setTransformationProjectionMatrix(
        Projection_ *
        Matrix3::translation(
            Vector2(- BarLength*0.5*std::pow(10, Scale_)*Zoom.z,
                    0.5 * WindowSizeY_ - 10)) *
        Matrix3::scaling(
            Vector2(1.0, 7.0))
    );
    Shader_.draw(ScaleLineShapeV_);

    Shader_.setTransformationProjectionMatrix(
        Projection_ *
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
    GL::defaultFramebuffer.setViewport({{}, {WindowSizeX_, WindowSizeY_}});

    auto& HookPosSys = Reg_.get<SystemPositionComponent>(Reg_.get<HookComponent>(Camera_).e);
    auto* HookPos    = Reg_.try_get<PositionComponent>(Reg_.get<HookComponent>(Camera_).e);
    auto& CamPosSys = Reg_.get<SystemPositionComponent>(Camera_);
    auto& CamPos = Reg_.get<PositionComponent>(Camera_);
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
    else if (Zoom.z > 100.0) Zoom.z = 100.0;

    // Remove all "inside" tags from objects
    // After that, test all objects for camera viewport and tag
    // appropriatly
    // Timers_.ViewportTest.start();

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
    // Timers_.ViewportTest.stop();
    // Timers_.ViewportTestAvg.addValue(Timers_.ViewportTest.elapsed());

    // Timers_.Render.start();
    Reg_.view<SystemPositionComponent, RadiusComponent, StarDataComponent, InsideViewportTag>().each(
        [&](auto _e, const auto& _p, const auto& _r, const auto& _s)
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

        x *= Zoom.z;
        y *= Zoom.z;

        auto r = _r.r;
        r *= Zoom.z * StarsDisplayScaleFactor_;
        if (r < StarsDisplaySizeMin_) r=StarsDisplaySizeMin_;

        Shader_.setTransformationProjectionMatrix(
            Projection_ *
            Matrix3::translation(Vector2(x, y)) *
            Matrix3::scaling(Vector2(r, r))
        );

        Shader_.setColor(TemperaturePalette_.getColorClip((_s.Temperature-2000.0)/45000.0));
        Shader_.draw(CircleShape_);
    });

    Reg_.view<SystemPositionComponent, RadiusComponent, PositionComponent, InsideViewportTag>(entt::exclude<StarDataComponent>).each(
        [&](auto _e, const auto& _sp, const auto& _r, const auto& _p)
    {
        auto x = _sp.x;
        auto y = _sp.y;

        x += CamPosSys.x - HookPosSys.x;
        y += CamPosSys.y - HookPosSys.y;
        x += CamPos.x - HookPos->x;
        y += CamPos.y - HookPos->y;
        x += _p.x;
        y += _p.y;
        x *= Zoom.z;
        y *= Zoom.z;

        auto r = _r.r;
        r *= Zoom.z * StarsDisplayScaleFactor_;
        if (r < StarsDisplaySizeMin_) r=StarsDisplaySizeMin_;

        Shader_.setTransformationProjectionMatrix(
            Projection_ *
            Matrix3::translation(Vector2(x, y)) *
            Matrix3::scaling(Vector2(r, r))
        );

        Shader_.setColor({0.0, 0.0, 1.0});
        Shader_.draw(CircleShape_);
    });

    // Timers_.Render.stop();
    // Timers_.RenderAvg.addValue(Timers_.Render.elapsed());
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
    Shader_ = Shaders::Flat2D{};
    CircleShape_ = MeshTools::compile(Primitives::circle2DSolid(100));
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
}

void RenderSystem::setWindowSize(const double _x, const double _y)
{
    WindowSizeX_ = _x;
    WindowSizeY_ = _y;
    Projection_= Magnum::Matrix3::projection(Magnum::Vector2(_x, _y));
}

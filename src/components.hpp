#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

struct MassComponent
{
    double m{1.0};
};

struct CircleComponent
{
    double r{1.0};
};

struct HookComponent
{
    entt::entity e{entt::null};
    double x{0.0};
    double y{0.0};
};

struct NameComponent
{
    std::string n{"Jane Doe"};
};

struct PositionComponent
{
    double x{0.0};
    double y{0.0};
};

struct TemperatureComponent
{
    double t{0.0};
};

struct VelocityComponent
{
    double x{0.0};
    double y{0.0};
};

struct ZoomComponent
{
    double z{3.0e-9};
};

#endif // COMPONENTS_HPP

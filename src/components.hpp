#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include <string>
#include <unordered_map>

struct MassComponent
{
    double m{1.0};
};

struct RadiusComponent
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

enum class SpectralClassE : int
{
    M = 0,
    K = 1,
    G = 2,
    F = 3,
    A = 4,
    B = 5,
    O = 6
};
static std::unordered_map<SpectralClassE, std::string> SpectralClassToStringMap{
{SpectralClassE::M, "M"},
{SpectralClassE::K, "K"},
{SpectralClassE::G, "G"},
{SpectralClassE::F, "F"},
{SpectralClassE::A, "A"},
{SpectralClassE::B, "B"},
{SpectralClassE::O, "O"}
};

struct StarDataComponent
{
    SpectralClassE SpectralClass{SpectralClassE::M};
    double Temperature{0.0};
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

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

struct HookDummyComponent
{
    entt::entity e{entt::null};
};

struct HookComponent
{
    entt::entity e{entt::null};
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

struct SystemPositionComponent
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
    double z{1.0e-20}; // Zoom
    double t{1.0e-20}; // Zoom target
    double i{0.0};     // Zoom increment
    int    c{0};       // Zoom counter
    int    s{10};      // Zoom speed, steps (frames) to target
};

#endif // COMPONENTS_HPP

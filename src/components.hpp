#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include <cstring>
#include <unordered_map>

#include <entt/entity/registry.hpp>

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

// 31 characters and trailing delimiter (\0)
constexpr std::size_t NAME_SIZE_MAX = 32;

struct NameComponent
{
    // Use fixed length char[] to ensure memory is aligned and not dynamically
    // allocated (in comparison to std::string). Hence, a name-system is used
    // to set names via string copy and test for length
    char Name[NAME_SIZE_MAX]{"Unknown"};
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

struct TireComponent
{
    constexpr static int SEGMENTS = 64;

    double RimX{0.0};
    double RimY{0.0};
    double RimR{1.0};
    std::array<double, SEGMENTS> RubberX;
    std::array<double, SEGMENTS> RubberY;
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

struct DynamicObjectTag{};
struct InsideViewportTag{};
struct StarSystemTag{};
struct StaticObjectTag{};

#endif // COMPONENTS_HPP

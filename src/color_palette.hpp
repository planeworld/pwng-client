#ifndef COLOR_PALETTE_HPP
#define COLOR_PALETTE_HPP

#include <map>
#include <vector>

#include <entt/entity/registry.hpp>
#include <Magnum/Math/Color.h>

#include <message_handler.hpp>

using namespace Magnum;

class ColorPalette
{

    public:

        ColorPalette(entt::registry& _Reg, std::size_t _s,
                     const Color3& _First, const Color3& _Last) : Reg_(_Reg)
        {
            LuT_.resize(_s);
            LuT_.front() = _First;
            LuT_.back() = _Last;
            SupportPoints_[0.0] = _First;
            SupportPoints_[1.0] = _Last;
        }

        void addSupportPoint(double _Pos, const Color3& _Color)
        {
            DBLK(
                auto& Messages = Reg_.ctx<MessageHandler>();
                if (_Pos < 0.0 || _Pos > 1.0)
                {
                    Messages.report("col", "Support point out of range, zeroing", MessageHandler::WARNING);
                    _Pos = 0.0;
                }
            )
            SupportPoints_[_Pos] = _Color;
        }

        void addSupportPointClip(double _Pos, const Color3& _Color)
        {
            if (_Pos < 0.0) _Pos = 0.0;
            else if (_Pos > 1.0) _Pos = 1.0;
            SupportPoints_[_Pos] = _Color;
        }

        void buildLuT()
        {
            auto it0 = SupportPoints_.begin();
            auto it1 = it0; it1++;
            for (auto i=1u; i<LuT_.size()-1; ++i)
            {
                if (double(i)/LuT_.size() > it1->first)
                {
                    it0++;
                    it1++;
                }
                double f = (double(i)/LuT_.size() - it0->first) / (it1->first - it0->first);
                LuT_[i] = (1.0-f)*it0->second + f*it1->second;
                // std::cout << LuT_[i].g() << " ";
            }
        }

        const Color3& getColor(double _Pos) const
        {
            DBLK(
                auto& Messages = Reg_.ctx<MessageHandler>();
                if (_Pos < 0.0 || _Pos > 1.0)
                {
                    Messages.report("col", "Position out of range, zeroing", MessageHandler::WARNING);
                    _Pos = 0.0;
                }
            )
            return LuT_[int(_Pos * (LuT_.size()-1))];
        }

        const Color3& getColorClip(double _Pos) const
        {
            if (_Pos < 0.0) _Pos = 0.0;
            else if (_Pos > 1.0) _Pos = 1.0;

            return LuT_[int(_Pos * (LuT_.size()-1))];
        }

    private:

        entt::registry& Reg_;

        std::map<double, Color3> SupportPoints_;
        std::vector<Color3> LuT_;

};

#endif // COLOR_PALETTE_HPP

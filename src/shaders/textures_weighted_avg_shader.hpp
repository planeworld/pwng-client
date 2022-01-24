#ifndef TEXTURES_WEIGHTED_AVG_SHADER_H
#define TEXTURES_WEIGHTED_AVG_SHADER_H

#include <Corrade/Containers/Reference.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>

#include "shader_path.hpp"

using namespace Magnum;

class TexturesWeightedAvgShader : public GL::AbstractShaderProgram
{

    public:
        
        explicit TexturesWeightedAvgShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}
        
        explicit TexturesWeightedAvgShader()
        {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

            GL::Shader Vert{GL::Version::GL330, GL::Shader::Type::Vertex};
            GL::Shader Frag{GL::Version::GL330, GL::Shader::Type::Fragment};

            Vert.addFile(Path_+"texture_base_unit_shader.vert");
            Frag.addFile(Path_+"textures_weighted_avg_shader.frag");

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({Vert, Frag}));

            attachShaders({Vert, Frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            SigmaUniform_ = uniformLocation("u_sigma");
            TexScaleUniformX_ = uniformLocation("u_tex_scale_x");
            TexScaleUniformY_ = uniformLocation("u_tex_scale_y");
            WeightUniform_ = uniformLocation("u_weight");

            setUniform(uniformLocation("u_texture0"), TextureUnit0);
            setUniform(uniformLocation("u_texture1"), TextureUnit1);
            setUniform(TexScaleUniformX_, 1.0f);
            setUniform(TexScaleUniformY_, 1.0f);
            setUniform(SigmaUniform_, 1.0f);
        }

        TexturesWeightedAvgShader& setSigma(const float _Sigma)
        {
            setUniform(SigmaUniform_, _Sigma);
            return *this;
        }

        TexturesWeightedAvgShader& setTexScale(const float TexScaleX, const float TexScaleY)
        {
            setUniform(TexScaleUniformX_, TexScaleX);
            setUniform(TexScaleUniformY_, TexScaleY);
            return *this;
        }

        TexturesWeightedAvgShader& bindTextures(GL::Texture2D& _Texture0, GL::Texture2D& _Texture1)
        {
            _Texture0.bind(TextureUnit0);
            _Texture1.bind(TextureUnit1);
            return *this;
        }

        TexturesWeightedAvgShader& setWeight(const float _Weight)
        {
            setUniform(WeightUniform_, _Weight);
            return *this;
        }

    private:
        
        enum: Int { TextureUnit0 = 0, TextureUnit1 = 1 };

        Float SigmaUniform_{1.0f};
        Float TexScaleUniformX_ = 1.0f;
        Float TexScaleUniformY_ = 1.0f;
        Float WeightUniform_ = 0.5f;

        std::string Path_{SHADER_PATH};
};

#endif // TEXTURES_WEIGHTED_AVG_SHADER_H

#ifndef MAIN_DISPLAY_SHADER_H
#define MAIN_DISPLAY_SHADER_H

#include <Corrade/Containers/Reference.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>

#include "shader_path.hpp"

using namespace Magnum;

class MainDisplayShader : public GL::AbstractShaderProgram
{

    public:
        
        explicit MainDisplayShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}
        
        explicit MainDisplayShader()
        {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

            GL::Shader Vert{GL::Version::GL330, GL::Shader::Type::Vertex};
            GL::Shader Frag{GL::Version::GL330, GL::Shader::Type::Fragment};

            Vert.addFile(Path_+"texture_base_unit_shader.vert");
            Frag.addFile(Path_+"main_display_shader.frag");

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({Vert, Frag}));

            attachShaders({Vert, Frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            TexScaleUniformX_ = uniformLocation("u_tex_scale_x");
            TexScaleUniformY_ = uniformLocation("u_tex_scale_y");

            setUniform(uniformLocation("u_texture"), TextureUnit);
            setUniform(TexScaleUniformX_, 1.0f);
            setUniform(TexScaleUniformY_, 1.0f);
        }

        MainDisplayShader& setTexScale(const float TexScaleX, const float TexScaleY)
        {
            setUniform(TexScaleUniformX_, TexScaleX);
            setUniform(TexScaleUniformY_, TexScaleY);
            return *this;
        }

        MainDisplayShader& bindTexture(GL::Texture2D& Texture)
        {
            Texture.bind(TextureUnit);
            return *this;
        }

    private:
        
        enum: Int { TextureUnit = 0 };

        Float TexScaleUniformX_ = 1.0f;
        Float TexScaleUniformY_ = 1.0f;

        std::string Path_{SHADER_PATH};
};

#endif // MAIN_DISPLAY_SHADER_H

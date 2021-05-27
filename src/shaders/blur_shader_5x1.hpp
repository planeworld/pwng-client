#ifndef BLUR_SHADER_5X1_H
#define BLUR_SHADER_5X1_H

#include <Corrade/Containers/Reference.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>

#include "shader_path.hpp"

using namespace Magnum;

class BlurShader5x1 : public GL::AbstractShaderProgram
{

    public:
        
        explicit BlurShader5x1(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}
        
        explicit BlurShader5x1()
        {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

            GL::Shader Vert{GL::Version::GL330, GL::Shader::Type::Vertex};
            GL::Shader Frag{GL::Version::GL330, GL::Shader::Type::Fragment};

            Vert.addFile(Path_+"image_unit_shader.vert");
            Frag.addFile(Path_+"blur_shader_5x1.frag");

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({Vert, Frag}));

            attachShaders({Vert, Frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            HorizontalUniform_ = uniformLocation("u_horizontal");

            setUniform(uniformLocation("u_texture"), TextureUnit);
            setUniform(HorizontalUniform_, true);
        }

        BlurShader5x1& setHorizontal(bool _Hor)
        {
            setUniform(HorizontalUniform_, _Hor);
            return *this;
        }

        BlurShader5x1& bindTexture(GL::Texture2D& Texture)
        {
            Texture.bind(TextureUnit);
            return *this;
        }

    private:
        
        enum: Int { TextureUnit = 0 };

        bool HorizontalUniform_{true};

        std::string Path_{SHADER_PATH};
};

#endif // BLUR_SHADER_5X1_H

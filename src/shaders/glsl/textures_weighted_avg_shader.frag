uniform sampler2D u_texture0;
uniform sampler2D u_texture1;

uniform float u_sigma;
uniform float u_weight;

in vec2 v_tex;

out vec4 frag_color;

void main()
{
    frag_color = mix(texture(u_texture0, v_tex),
                     texture(u_texture1, v_tex*u_sigma),
                     u_weight);
}

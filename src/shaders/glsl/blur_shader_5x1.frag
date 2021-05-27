uniform sampler2D u_texture;
uniform bool u_horizontal;

out vec4 frag_color;

const float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    frag_color = texelFetch(u_texture, ivec2(gl_FragCoord.xy), 0) * weight[0];

    if (u_horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            frag_color += texelFetch(u_texture, ivec2(gl_FragCoord.xy) + ivec2(1,0) * i, 0) * weight[i];
            frag_color += texelFetch(u_texture, ivec2(gl_FragCoord.xy) - ivec2(1,0) * i, 0) * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            frag_color += texelFetch(u_texture, ivec2(gl_FragCoord.xy) + ivec2(0,1) * i, 0) * weight[i];
            frag_color += texelFetch(u_texture, ivec2(gl_FragCoord.xy) - ivec2(0,1) * i, 0) * weight[i];
        }
    }
}

uniform sampler2D u_texture;
uniform bool u_horizontal;

out vec4 frag_color;

// Normal distribution (mu=0, sigma=1) at x=0,1,2
// Hence, blurring uses all values within 2-sigma, which is
// 95.45% of the relevant information.
const float weight[3] = float[] (0.41, 0.24, 0.05);

void main()
{
    frag_color = texelFetch(u_texture, ivec2(gl_FragCoord.xy), 0) * weight[0];

    if (u_horizontal)
    {
        for(int i = 1; i < 3; ++i)
        {
            frag_color += texelFetch(u_texture, ivec2(gl_FragCoord.xy) + ivec2(1,0) * i, 0) * weight[i];
            frag_color += texelFetch(u_texture, ivec2(gl_FragCoord.xy) - ivec2(1,0) * i, 0) * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 3; ++i)
        {
            frag_color += texelFetch(u_texture, ivec2(gl_FragCoord.xy) + ivec2(0,1) * i, 0) * weight[i];
            frag_color += texelFetch(u_texture, ivec2(gl_FragCoord.xy) - ivec2(0,1) * i, 0) * weight[i];
        }
    }
}

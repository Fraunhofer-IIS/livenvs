#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

in vec2 tc[3];
in vec3 tgt_view_dir_ws[3];
uniform int layers;

out vec2 fs_tc;
out vec3 fs_tgt_view_dir_ws;

void main()
{
    for( int l = 0; l < 5; ++l ){
        gl_Layer = l;// built-in variable that specifies to which face we render.
        for (int i = 0; i < 3; ++i)// for each triangle's vertices
        {
            gl_Position = gl_in[i].gl_Position;
            fs_tc = tc[i];
            fs_tgt_view_dir_ws = tgt_view_dir_ws[i];
            EmitVertex();
        }
        EndPrimitive();
    }
}

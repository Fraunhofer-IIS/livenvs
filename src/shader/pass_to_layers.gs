#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

in vec2 tc[3];
uniform int layers;

out vec2 fs_tc;

void main()
{
    for( int l = 0; l < 5; ++l ){
        gl_Layer = l;// built-in variable that specifies to which face we render.
        for (int i = 0; i < 3; ++i)// for each triangle's vertices
        {
            gl_Position = gl_in[i].gl_Position;
            fs_tc = tc[i];
            EmitVertex();
        }
        EndPrimitive();
    }
}

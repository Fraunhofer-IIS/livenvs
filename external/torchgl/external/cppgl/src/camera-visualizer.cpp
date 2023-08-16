#include "camera-visualizer.h"
#include <glm/gtc/type_ptr.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>


CPPGL_NAMESPACE_BEGIN
using namespace CameraVisualization;

// // ------------------------------------------------
// // prototypes
// Drawelement Cube::prototype;


// // ------------------------------------------------
// // Cube


// class Cube {
// public:
//     Cube();
//     ~Cube();

//     void draw();
//     void update(const glm::mat4& trans);

//     // data
//     glm::mat4 trafo;
// 	static Drawelement prototype;
// };

// Cube::Cube() {
//     trafo = glm::mat4(1);// glm::scale(glm::mat4(1), glm::vec3(.2f,.2f,.2f));
// }

// void Cube::update(const glm::mat4& trans){
//     trafo = trans*glm::scale(glm::mat4(1), glm::vec3(.2f,.2f,.2f));;
// }

// void Cube::draw() {
//     prototype->model = trafo;
//     prototype->bind();
//     prototype->draw();
//     prototype->unbind();
// }

// Cube::~Cube() {
// }

static bool initialized = false;
static Mesh mesh;
static GLuint gl_shader;



bool CameraVisualization::lines = true;
float CameraVisualization::line_width = 3.f;
float CameraVisualization::clamp_far = 0.25f;
glm::vec3 CameraVisualization::uniform_color = glm::vec3(0.f);

bool CameraVisualization::display_got_called = false;

bool CameraVisualization::display_all_cameras;
std::map<std::string, bool> CameraVisualization::active_cameras;
static std::string get_log(GLuint object) {
    std::string error_string;
    GLint log_length = 0;
    if (glIsShader(object)) {
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
    } else if (glIsProgram(object)) {
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
    } else {
        error_string += "Not a shader or a program";
        return error_string;
    }
    if (log_length <= 1)
        // ignore empty string
        return error_string;
    char *log = (char *)malloc(log_length);
    if (glIsShader(object))
        glGetShaderInfoLog(object, log_length, NULL, log);
    else if (glIsProgram(object))
        glGetProgramInfoLog(object, log_length, NULL, log);
    error_string += log;
    free(log);
    return error_string;
}
// 
void init_shader(){

    gl_shader = glCreateProgram();

    // vertex shader 
    const char* v_src = 
        "#version 430 \n"
        "in vec3 in_pos; \n"

        "// uniform mat4 model; \n"
        "uniform mat4 inv_frustum_proj; \n"
        "uniform mat4 inv_frustum_view; \n"
        "uniform mat4 view; \n"
        "uniform mat4 proj; \n"

        "uniform vec3 uniform_color; \n"
        "uniform float clamp_far; \n"

        "out vec3 color; \n"
        "void main() { \n"
        "    // color = in_color; \n"
        "    vec4 pos_wc = vec4(in_pos,1); \n"
            
        "    pos_wc = inv_frustum_proj * pos_wc; \n"
        "    pos_wc /= pos_wc.w; \n"
        "    if(pos_wc.z < -1 && clamp_far > 0.) // vertex is (probably) at far plane \n"
        "        pos_wc = vec4(clamp_far*normalize(pos_wc.xyz),1); \n"

            
        "    float diff = length(inv_frustum_view[3] - inverse(view)[3]); \n"
        "    if (diff < .001){ \n"
        "        gl_Position = vec4(2,2,2,1); \n"
        "        return; \n"
        "    } \n"

        "    color = uniform_color; \n"
        "    if(dot(color, color) == 0) \n"
        "        color = vec3(0.25)+(1/clamp_far)*pos_wc.xyz; \n"
        "    pos_wc = inv_frustum_view * pos_wc; \n"
            
        "    gl_Position = proj * view * pos_wc; \n"
        "} \n";

    GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader, 1, &v_src, NULL);
    glCompileShader(v_shader);
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv(v_shader, GL_COMPILE_STATUS, &shaderCompiled);
    if (shaderCompiled != GL_TRUE) 
    {
    // std::cout << "vshader failed" <<std::endl;
    std::string log = get_log(v_shader);
        std::string error_msg = "ERROR: Failed to compile vshader shader\n";
        // get relevant lines
        std::string out;
        std::stringstream logstream(log);
        std::vector<int> lines;
        while (!logstream.eof()) {
            getline(logstream, out);
            try {
                int line = stoi(out.substr(2, out.find(":") - 3));
                lines.push_back(line);
            }
            catch (const std::exception& e) { (void) e; }
        }
        // print relevant lines
        std::stringstream stream(v_src);
        int line = 1;
        while (!stream.eof()) {
            getline(stream, out);
            if (std::find(lines.begin(), lines.end(), line) != lines.end())
                error_msg += "(" + std::to_string(line) + ")\t" + out + "\n";
            line++;
        }
        glDeleteShader(v_shader);
        std::cerr << error_msg << std::endl;
    }


    glAttachShader(gl_shader, v_shader);
    
    // fragment shader 
    const char *f_src = 
        "#version 430 \n"
        
        "in vec3 color; \n"
        "out vec4 out_col; \n"

        "void main() { \n"
        "    out_col = vec4(color,1); \n"
        "}";

    GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f_shader, 1, &f_src, NULL);
    glCompileShader(f_shader);
    shaderCompiled = GL_FALSE;
    glGetShaderiv(f_shader, GL_COMPILE_STATUS, &shaderCompiled);
    if (shaderCompiled != GL_TRUE) 
    {
    std::cout << "fshader failed" <<std::endl;
        std::string log = get_log(v_shader);
        std::string error_msg = "ERROR: Failed to compile fshader shader\n";
        // get relevant lines
        std::string out;
        std::stringstream logstream(log);
        std::vector<int> lines;
        while (!logstream.eof()) {
            getline(logstream, out);
            try {
                int line = stoi(out.substr(2, out.find(":") - 3));
                lines.push_back(line);
            }
            catch (const std::exception& e) { (void) e; }
        }
        // print relevant lines
        std::stringstream stream(v_src);
        int line = 1;
        while (!stream.eof()) {
            getline(stream, out);
            if (std::find(lines.begin(), lines.end(), line) != lines.end())
                error_msg += "(" + std::to_string(line) + ")\t" + out + "\n";
            line++;
        }
        glDeleteShader(v_shader);
        std::cerr << error_msg << std::endl;
    }

    glAttachShader(gl_shader, f_shader);
      
    // link program
    glLinkProgram(gl_shader);
    GLint link_ok = GL_FALSE;
    glGetProgramiv(gl_shader, GL_LINK_STATUS, &link_ok);
    if (link_ok != GL_TRUE) std::cout << "link failed" <<std::endl;

}


void init_cube(){
    { 
        
        mesh = Mesh("LocalGeometry");

        // add positions.
        std::vector<glm::vec3> positions = {
            // top
            glm::vec3(-1.,  1, -1),
            glm::vec3( 1.,  1, -1),
            glm::vec3( 1.,  1,  1),
            glm::vec3(-1.,  1,  1),

             // bottom
            glm::vec3(-1., -1, -1),
            glm::vec3( 1., -1, -1),
            glm::vec3( 1., -1,  1),
            glm::vec3(-1., -1,  1),

            // right
            glm::vec3( 1, -1., -1),
            glm::vec3( 1,  1., -1),
            glm::vec3( 1,  1.,  1),
            glm::vec3( 1, -1.,  1),

             // left
            glm::vec3( -1, -1., -1),
            glm::vec3( -1,  1., -1),
            glm::vec3( -1,  1.,  1),
            glm::vec3( -1, -1.,  1),

            // front
            glm::vec3(-1., -1,  1),
            glm::vec3( 1., -1,  1),
            glm::vec3( 1.,  1,  1),
            glm::vec3(-1.,  1,  1),

             // back
            glm::vec3(-1., -1, -1),
            glm::vec3( 1., -1, -1),
            glm::vec3( 1.,  1, -1),
            glm::vec3(-1.,  1, -1),

        };
        mesh->add_vertex_buffer(GL_FLOAT, 3, positions.size(), positions.data()); 
        
        mesh->set_primitive_type(GL_QUADS);            
        
	}
}


// excludes current camera
void CameraVisualization::display_all_active(){
    display_got_called = true;
    
    for (auto& [name, cam] : Camera::map) {
        // if (name == current_camera()->name) continue;
        if (active_cameras.count(name) == 0) active_cameras[name] = false;
        if (display_all_cameras || active_cameras[name]){
            display(cam);
        }
    }
}

// display single frustum
void CameraVisualization::display(const std::string& name){
    display(Camera::find(name));
}
void CameraVisualization::display(const Camera& camera){
    display(camera->view, camera->proj);
}
void CameraVisualization::display(const glm::mat4& view, const glm::mat4& proj){
    if (!initialized) {
        init_cube();
        init_shader();
        initialized = true;
    }

    bool culling = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    if (lines){
        glPolygonMode(GL_FRONT_AND_BACK, lines ? GL_LINE : GL_FILL);      
        glLineWidth(line_width);  
    }

    // std::cout << "bind shader" << std::endl; 
    glUseProgram(gl_shader); 
    // std::cout << "bind uniforms" << std::endl; 
    glUniformMatrix4fv(glGetUniformLocation(gl_shader, "inv_frustum_proj")
                        , 1, GL_FALSE, glm::value_ptr(glm::inverse(proj)));
    glUniformMatrix4fv(glGetUniformLocation(gl_shader, "inv_frustum_view")
                        , 1, GL_FALSE, glm::value_ptr(glm::inverse(view)));
    glUniformMatrix4fv(glGetUniformLocation(gl_shader, "view")
                        , 1, GL_FALSE, glm::value_ptr(current_camera()->view));
    glUniformMatrix4fv(glGetUniformLocation(gl_shader, "proj")
                        , 1, GL_FALSE, glm::value_ptr(current_camera()->proj));
    glUniform1f(glGetUniformLocation(gl_shader, "clamp_far"), clamp_far);
    glUniform3f(glGetUniformLocation(gl_shader, "uniform_color"), uniform_color.x, uniform_color.y, uniform_color.z);


    // std::cout << "bind vao" << std::endl; 
    glBindVertexArray(mesh->vao);
    // std::cout << "draw" << std::endl; 
    mesh->draw();
    glBindVertexArray(0);
    glUseProgram(0); 

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);   

    if(culling) 
        glEnable(GL_CULL_FACE);
}

void CameraVisualization::display_view(const Texture2D& tex){
    // TODO
}



CPPGL_NAMESPACE_END
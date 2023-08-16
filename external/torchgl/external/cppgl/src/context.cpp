#include "context.h"
#include "debug.h"
#include "camera.h"
#include "shader.h"
#include "texture.h"
#include "framebuffer.h"
#include "material.h"
#include "geometry.h"
#include "drawelement.h"
#include "anim.h"
#include "query.h"
#include "gui.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "image_load_store.h"
#include <glm/glm.hpp>
#include <iostream>

CPPGL_NAMESPACE_BEGIN

// -------------------------------------------
// helper funcs

static void glfw_error_func(int error, const char *description) {
    fprintf(stderr, "GLFW: Error %i: %s\n", error, description);
}

static void (*user_keyboard_callback)(int key, int scancode, int action, int mods) = 0;
static void (*user_mouse_callback)(double xpos, double ypos) = 0;
static void (*user_mouse_button_callback)(int button, int action, int mods) = 0;
static void (*user_mouse_scroll_callback)(double xoffset, double yoffset) = 0;
static void (*user_resize_callback)(int w, int h) = 0;

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
        Context::instance().show_gui = !Context::instance().show_gui;
    if (ImGui::GetIO().WantCaptureKeyboard) {
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        return;
    }
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, 1);
    if (user_keyboard_callback)
        user_keyboard_callback(key, scancode, action, mods);
}

static void glfw_mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (ImGui::GetIO().WantCaptureMouse)
        return;
    if (user_mouse_callback)
        user_mouse_callback(xpos, ypos);
}

static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse)
        return;
    if (user_mouse_button_callback)
        user_mouse_button_callback(button, action, mods);
}

static void glfw_mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (ImGui::GetIO().WantCaptureMouse)
        return;
    CameraImpl::default_camera_movement_speed += CameraImpl::default_camera_movement_speed * float(yoffset * 0.1);
    CameraImpl::default_camera_movement_speed = std::max(0.00001f, CameraImpl::default_camera_movement_speed);
    if (user_mouse_scroll_callback)
        user_mouse_scroll_callback(xoffset, yoffset);
}

static void glfw_resize_callback(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
    if (user_resize_callback)
        user_resize_callback(w, h);
}

static void glfw_char_callback(GLFWwindow* window, unsigned int c) {
    ImGui_ImplGlfw_CharCallback(window, c);
}

// -------------------------------------------
// Context

static ContextParameters parameters;

Context::Context() {
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed!");
    glfwSetErrorCallback(glfw_error_func);

    // some GL context settings
    if (parameters.gl_major > 0)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, parameters.gl_major);
    if (parameters.gl_minor > 0)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, parameters.gl_minor);
    glfwWindowHint(GLFW_RESIZABLE, parameters.resizable);
    glfwWindowHint(GLFW_VISIBLE, parameters.visible);
    glfwWindowHint(GLFW_DECORATED, parameters.decorated);
    glfwWindowHint(GLFW_FLOATING, parameters.floating);
    glfwWindowHint(GLFW_MAXIMIZED, parameters.maximised);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, parameters.gl_debug_context);

    // create window and context
    glfw_window = glfwCreateWindow(parameters.width, parameters.height, parameters.title.c_str(), 0, 0);
    if (!glfw_window) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateContext failed!");
    }
    glfwMakeContextCurrent(glfw_window);
    glfwSwapInterval(parameters.swap_interval);

    glewExperimental = GL_TRUE;
    const GLenum err = glewInit();
    if (err != GLEW_OK) {
        glfwDestroyWindow(glfw_window);
        glfwTerminate();
        throw std::runtime_error(std::string("GLEWInit failed: ") + (const char*)glewGetErrorString(err));
    }

    // output configuration
    std::cout << "GLFW: " << glfwGetVersionString() << std::endl;
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << ", " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // enable debugging output
    enable_strack_trace_on_crash();
    enable_gl_debug_output();

    // setup user ptr
    glfwSetWindowUserPointer(glfw_window, this);

    // install callbacks
    glfwSetKeyCallback(glfw_window, glfw_key_callback);
    glfwSetCursorPosCallback(glfw_window, glfw_mouse_callback);
    glfwSetMouseButtonCallback(glfw_window, glfw_mouse_button_callback);
    glfwSetScrollCallback(glfw_window, glfw_mouse_scroll_callback);
    glfwSetFramebufferSizeCallback(glfw_window, glfw_resize_callback);
    glfwSetCharCallback(glfw_window, glfw_char_callback);

    // set input mode
    glfwSetInputMode(glfw_window, GLFW_STICKY_KEYS, 1);
    glfwSetInputMode(glfw_window, GLFW_STICKY_MOUSE_BUTTONS, 1);

    // init imgui
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(glfw_window, false);
    ImGui_ImplOpenGL3_Init("#version 130");
    // ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // load custom font?
    if (fs::exists(parameters.font_ttf_filename)) {
        ImFontConfig config;
        config.OversampleH = 3;
        config.OversampleV = 3;
        std::cout << "Loading: " << parameters.font_ttf_filename << "..." << std::endl;
        ImGui::GetIO().FontDefault = ImGui::GetIO().Fonts->AddFontFromFileTTF(
                parameters.font_ttf_filename.string().c_str(), float(parameters.font_size_pixels), &config);
    }
    ImGui::GetIO().FontGlobalScale = parameters.global_font_scale;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // set some sane GL defaults
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glClearColor(0, 0, 0, 1);
    glClearDepth(1);

    // setup timer
    last_t = curr_t = glfwGetTime();
    cpu_timer = TimerQuery("CPU-time");
    frame_timer = TimerQuery("Frame-time");
    gpu_timer = TimerQueryGL("GPU-time");
    prim_count = PrimitiveQueryGL("#Primitives");
    frag_count = FragmentQueryGL("#Fragments");
    cpu_timer->begin();
    frame_timer->begin();
    gpu_timer->begin();
    prim_count->begin();
    frag_count->begin();
}

Context::~Context() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwDestroyWindow(glfw_window);
    glfwTerminate();
}

Context& Context::init(const ContextParameters& params) {
    parameters = params;
    // enforce setup of default camera
    current_camera();
    return instance();
}

Context& Context::instance() {
    static Context ctx;
    return ctx;
}

bool Context::running() { return !glfwWindowShouldClose(instance().glfw_window); }

void Context::swap_buffers() {
    if (instance().show_gui) gui_draw();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    instance().cpu_timer->end();
    instance().gpu_timer->end();
    instance().prim_count->end();
    instance().frag_count->end();
    glfwSwapBuffers(instance().glfw_window);
    instance().frame_timer->end();
    instance().frame_timer->begin();
    instance().cpu_timer->begin();
    instance().gpu_timer->begin();
    instance().prim_count->begin();
    instance().frag_count->begin();
    instance().last_t = instance().curr_t;
    instance().curr_t = glfwGetTime() * 1000; // s to ms
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

double Context::frame_time() { return instance().curr_t - instance().last_t; }

void Context::screenshot(const std::filesystem::path& path) {
    const glm::ivec2 size = resolution();
    std::vector<uint8_t> pixels(size_t(size.x) * size.y * 3);
    // glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
    // have allocated the exact size needed for the image so we have to use 1-byte alignment
    // (otherwise glReadPixels would write out of bounds)
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, size.x, size.y, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    // write ldr image to disk (async)
    image_store_ldr(path, pixels.data(), size.x, size.y, 3, true, true);
}

void Context::show() { glfwShowWindow(instance().glfw_window); }

void Context::hide() { glfwHideWindow(instance().glfw_window); }

glm::ivec2 Context::resolution() {
    int w, h;
    glfwGetFramebufferSize(instance().glfw_window, &w, &h);
    return glm::ivec2(w, h);
}

void Context::resize(int w, int h) {
    glfwSetWindowSize(instance().glfw_window, w, h);
    glViewport(0, 0, w, h);
}

void Context::set_title(const std::string& name) { glfwSetWindowTitle(instance().glfw_window, name.c_str()); }

void Context::set_swap_interval(uint32_t interval) { glfwSwapInterval(interval); }

void Context::capture_mouse(bool on) { glfwSetInputMode(instance().glfw_window, GLFW_CURSOR, on ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL); }

void Context::set_attribute(int attribute, bool value) { glfwSetWindowAttrib(instance().glfw_window, attribute, value ? GLFW_TRUE : GLFW_FALSE); }

glm::vec2 Context::mouse_pos() {
    double xpos, ypos;
    glfwGetCursorPos(instance().glfw_window, &xpos, &ypos);
    return glm::vec2(xpos, ypos);
}

bool Context::mouse_button_pressed(int button) { return glfwGetMouseButton(instance().glfw_window, button) == GLFW_PRESS; }

bool Context::key_pressed(int key) { return glfwGetKey(instance().glfw_window, key) == GLFW_PRESS; }

void Context::set_keyboard_callback(void (*fn)(int key, int scancode, int action, int mods)) { user_keyboard_callback = fn; }

void Context::set_mouse_callback(void (*fn)(double xpos, double ypos)) { user_mouse_callback = fn; }

void Context::set_mouse_button_callback(void (*fn)(int button, int action, int mods)) { user_mouse_button_callback = fn; }

void Context::set_mouse_scroll_callback(void (*fn)(double xoffset, double yoffset)) { user_mouse_scroll_callback = fn; }

void Context::set_resize_callback(void (*fn)(int w, int h)) { user_resize_callback = fn; }

// void Context::set_gui_callback(void (*fn)(void)) { user_gui_callback = fn; }

// -------------------------------------------
// GUI

// static void display_camera(Camera& cam) {
//     ImGui::Indent();
//     ImGui::Text("name: %s", cam->name.c_str());
//     ImGui::DragFloat3(("pos##" + cam->name).c_str(), &cam->pos.x, 0.001f);
//     ImGui::DragFloat3(("dir##" + cam->name).c_str(), &cam->dir.x, 0.001f);
//     ImGui::DragFloat3(("up##" + cam->name).c_str(), &cam->up.x, 0.001f);
//     ImGui::Checkbox(("fix_up_vector##" + cam->name).c_str(), &cam->fix_up_vector);
//     ImGui::Checkbox(("perspective##" + cam->name).c_str(), &cam->perspective);
//     if (cam->perspective) {
//         ImGui::DragFloat(("fov##" + cam->name).c_str(), &cam->fov_degree, 0.01f);
//         // ImGui::DragFloat(("near##" + cam->name).c_str(), &cam->near, 0.001f);
//         // ImGui::DragFloat(("far##" + cam->name).c_str(), &cam->far, 0.001f);
//     } else {
//         ImGui::DragFloat(("left##" + cam->name).c_str(), &cam->left, 0.001f);
//         ImGui::DragFloat(("right##" + cam->name).c_str(), &cam->right, 0.001f);
//         ImGui::DragFloat(("top##" + cam->name).c_str(), &cam->top, 0.001f);
//         ImGui::DragFloat(("bottom##" + cam->name).c_str(), &cam->bottom, 0.001f);
//     }
//     if (ImGui::Button(("Make current##" + cam->name).c_str())) make_camera_current(cam);
//     ImGui::Unindent();
// }

// static void display_shader(Shader& shader) {
//     ImGui::Indent();
//     ImGui::Text("name: %s", shader->name.c_str());
//     ImGui::Text("ID: %u", shader->id);
//     if (shader->source_files.count(GL_VERTEX_SHADER))
//         ImGui::Text("vertex source: %s", shader->source_files[GL_VERTEX_SHADER].c_str());
//     if (shader->source_files.count(GL_TESS_CONTROL_SHADER))
//         ImGui::Text("tess_control source: %s", shader->source_files[GL_TESS_CONTROL_SHADER].c_str());
//     if (shader->source_files.count(GL_TESS_EVALUATION_SHADER))
//         ImGui::Text("tess_eval source: %s", shader->source_files[GL_TESS_EVALUATION_SHADER].c_str());
//     if (shader->source_files.count(GL_GEOMETRY_SHADER))
//         ImGui::Text("geometry source: %s", shader->source_files[GL_GEOMETRY_SHADER].c_str());
//     if (shader->source_files.count(GL_FRAGMENT_SHADER))
//         ImGui::Text("fragment source: %s", shader->source_files[GL_FRAGMENT_SHADER].c_str());
//     if (shader->source_files.count(GL_COMPUTE_SHADER))
//         ImGui::Text("compute source: %s", shader->source_files[GL_COMPUTE_SHADER].c_str());
//     if (ImGui::Button("Compile"))
//         shader->compile();
//     ImGui::Unindent();
// }

// static void display_framebuffer(const Framebuffer& fbo) {
//     ImGui::Indent();
//     ImGui::Text("name: %s", fbo->name.c_str());
//     ImGui::Text("ID: %u", fbo->id);
//     ImGui::Text("size: %ux%u", fbo->w, fbo->h);
//     if (ImGui::CollapsingHeader(("depth attachment##" + fbo->name).c_str()) && fbo->depth_texture)
//         display_texture(fbo->depth_texture);
//     for (uint32_t i = 0; i < fbo->color_textures.size(); ++i)
//         if (ImGui::CollapsingHeader(std::string("color attachment " + std::to_string(i) + "##" + fbo->name).c_str()))
//             display_texture(fbo->color_textures[i]);
//     ImGui::Unindent();
// }

// static void display_material(const Material& mat) {
//     ImGui::Indent();
//     ImGui::Text("name: %s", mat->name.c_str());

//     ImGui::Text("int params: %lu", mat->int_map.size());
//     ImGui::Indent();
//     for (const auto& entry : mat->int_map)
//         ImGui::Text("%s: %i", entry.first.c_str(), entry.second);
//     ImGui::Unindent();

//     ImGui::Text("float params: %lu", mat->float_map.size());
//     ImGui::Indent();
//     for (const auto& entry : mat->float_map)
//         ImGui::Text("%s: %f", entry.first.c_str(), entry.second);
//     ImGui::Unindent();

//     ImGui::Text("vec2 params: %lu", mat->vec2_map.size());
//     ImGui::Indent();
//     for (const auto& entry : mat->vec2_map)
//         ImGui::Text("%s: (%f, %f)", entry.first.c_str(), entry.second.x, entry.second.y);
//     ImGui::Unindent();

//     ImGui::Text("vec3 params: %lu", mat->vec3_map.size());
//     ImGui::Indent();
//     for (const auto& entry : mat->vec3_map)
//         ImGui::Text("%s: (%f, %f, %f)", entry.first.c_str(), entry.second.x, entry.second.y, entry.second.z);
//     ImGui::Unindent();

//     ImGui::Text("vec4 params: %lu", mat->vec4_map.size());
//     ImGui::Indent();
//     for (const auto& entry : mat->vec4_map)
//         ImGui::Text("%s: (%f, %f, %f, %.f)", entry.first.c_str(), entry.second.x, entry.second.y, entry.second.z, entry.second.w);
//     ImGui::Unindent();

//     ImGui::Text("textures: %lu", mat->texture_map.size());
//     ImGui::Indent();
//     for (const auto& entry : mat->texture_map) {
//         ImGui::Text("%s:", entry.first.c_str());
//         display_texture(entry.second);
//     }
//     ImGui::Unindent();

//     ImGui::Unindent();
// }

// static void display_query_timer(const Query& query, const char* label="") {
//     const float avg = query.exp_avg;
//     const float lower = query.min();
//     const float upper = query.max();
//     ImGui::Text("avg: %.1fms, min: %.1fms, max: %.1fms", avg, lower, upper);
//     ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(.7, .7, 0, 1));
//     ImGui::PlotHistogram(label, query.data.data(), query.data.size(), query.curr, 0, 0.f, std::max(upper, 17.f), ImVec2(0, 30));
//     ImGui::PopStyleColor();
// }

// static void display_query_counter(const Query& query, const char* label="") {
//     const float avg = query.exp_avg;
//     const float lower = query.min();
//     const float upper = query.max();
//     ImGui::Text("avg: %uK, min: %uK, max: %uK", uint32_t(avg / 1000), uint32_t(lower / 1000), uint32_t(upper / 1000));
//     ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0, .7, .7, 1));
//     ImGui::PlotHistogram(label, query.data.data(), query.data.size(), query.curr, 0, 0.f, std::max(upper, 17.f), ImVec2(0, 30));
//     ImGui::PopStyleColor();
// }

// static void draw_gui() {
//     if (!Context::instance().show_gui) return;

//     // calc window size
//     static const int entry_length = 500/8;
//     int window_length = entry_length*TimerQuery::map.size();
//     window_length +=    entry_length*TimerQueryGL::map.size();
//     window_length +=    entry_length*PrimitiveQueryGL::map.size();
//     window_length +=    entry_length*FragmentQueryGL::map.size();


//     // TODO update GUI (add drawelements, geometry, meshes?)
//     static bool  gui_main_timers_only = true;
//     static bool gui_show_cameras = false;
//     static bool gui_show_textures = false;
//     static bool gui_show_fbos = false;
//     static bool gui_show_shaders = false;
//     static bool gui_show_materials = false;
//     static bool gui_show_performance = true;

//     if (ImGui::BeginMainMenuBar()) {
//         // camera menu
//         ImGui::Checkbox("main timers only", &gui_main_timers_only);
//         ImGui::Separator();
//         ImGui::Checkbox("cameras", &gui_show_cameras);
//         ImGui::Separator();
//         ImGui::Checkbox("textures", &gui_show_textures);
//         ImGui::Separator();
//         ImGui::Checkbox("fbos", &gui_show_fbos);
//         ImGui::Separator();
//         ImGui::Checkbox("shaders", &gui_show_shaders);
//         ImGui::Separator();
//         ImGui::Checkbox("materials", &gui_show_materials);
//         ImGui::Separator();
//         if (ImGui::Button("screenshot"))
//             Context::screenshot("screenshot.png");
//         ImGui::EndMainMenuBar();
//     }

//     if (gui_show_cameras) {
//         if (ImGui::Begin(std::string("Cameras (" + std::to_string(Camera::map.size()) + ")").c_str(), &gui_show_cameras)) {
//             ImGui::Text("Current: %s", current_camera()->name.c_str());
//             for (auto& entry : Camera::map) {
//                 if (ImGui::CollapsingHeader(entry.first.c_str()))
//                     display_camera(entry.second);
//             }
//         }
//         ImGui::End();
//     }

//     if (gui_show_textures) {
//         if (ImGui::Begin(std::string("Textures (" + std::to_string(Texture2D::map.size()) + ")").c_str(), &gui_show_textures)) {
//             for (auto& entry : Texture2D::map) {
//                 if (ImGui::CollapsingHeader(entry.first.c_str()))
//                     display_texture(entry.second, ImVec2(300, 300));
//             }
//         }
//         ImGui::End();
//     }

//     if (gui_show_shaders) {
//         if (ImGui::Begin(std::string("Shaders (" + std::to_string(Shader::map.size()) + ")").c_str(), &gui_show_shaders)) {
//             for (auto& entry : Shader::map)
//                 if (ImGui::CollapsingHeader(entry.first.c_str()))
//                     display_shader(entry.second);
//             if (ImGui::Button("Reload modified")) reload_modified_shaders();
//         }
//         ImGui::End();
//     }

//     if (gui_show_fbos) {
//         if (ImGui::Begin(std::string("Framebuffers (" + std::to_string(Framebuffer::map.size()) + ")").c_str(), &gui_show_fbos)) {
//             for (auto& entry : Framebuffer::map)
//                 if (ImGui::CollapsingHeader(entry.first.c_str()))
//                     display_framebuffer(entry.second);
//         }
//         ImGui::End();
//     }

//     if (gui_show_materials) {
//         if (ImGui::Begin(std::string("Materials (" + std::to_string(Material::map.size()) + ")").c_str(), &gui_show_materials)) {
//             for (auto& entry : Material::map)
//                 if (ImGui::CollapsingHeader(entry.first.c_str()))
//                     display_material(entry.second);
//         }
//         ImGui::End();
//     }


//     if (gui_show_performance){
//          // timers
//         ImGui::SetNextWindowPos(ImVec2(0, 20));
//         ImGui::SetNextWindowSize(ImVec2(350+200*(parameters.global_font_scale-1), window_length));
//         ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.,0.,0., 0.7));
//         ImGui::PushStyleVar(ImGuiStyleVar_Alpha, .9f);
//         if (ImGui::Begin("CPU/GPU Timer", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs)) {
//             if (gui_main_timers_only){
//                 ImGui::Separator();
//                 display_query_timer(*Context::instance().cpu_timer, Context::instance().cpu_timer->name.c_str());
//                 ImGui::Separator();
//                 display_query_timer(*Context::instance().gpu_timer, Context::instance().gpu_timer->name.c_str());
//                 ImGui::Separator();
//                 display_query_counter(*Context::instance().prim_count, Context::instance().prim_count->name.c_str());
//                 ImGui::Separator();
//                 display_query_counter(*Context::instance().frag_count, Context::instance().frag_count->name.c_str());
//             } else {
                
//                 for (const auto& entry : TimerQuery::map) {
//                     ImGui::Separator();
//                     display_query_timer(*entry.second, entry.second->name.c_str());
//                 }
//                 for (const auto& entry : TimerQueryGL::map) {
//                     ImGui::Separator();
//                     display_query_timer(*entry.second, entry.second->name.c_str());
//                 }
//                 for (const auto& entry : PrimitiveQueryGL::map) {
//                     ImGui::Separator();
//                     display_query_counter(*entry.second, entry.second->name.c_str());
//                 }
//                 for (const auto& entry : FragmentQueryGL::map) {
//                     ImGui::Separator();
//                     display_query_counter(*entry.second, entry.second->name.c_str());
//                 }
//             }
//         }
//         ImGui::PopStyleVar();
//         ImGui::PopStyleColor();
        
//         ImGui::End();
//     }

//     if (user_gui_callback)
//         user_gui_callback();
// }
CPPGL_NAMESPACE_END

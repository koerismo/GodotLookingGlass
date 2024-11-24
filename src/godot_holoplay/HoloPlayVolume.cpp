#include "HoloPlayVolume.h"

#include <windows.h>
#undef min
#undef max

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>

#include <HoloPlayCore.h>
#include <HoloPlayShadersOpen.h>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/Input.hpp>
#include <godot_cpp/classes/Input_Event.hpp>
#include <godot_cpp/classes/Input_Event_Mouse_Motion.hpp>
#include <godot_cpp/classes/Input_Event_Mouse_Button.hpp>
#include <godot_cpp/core/Math.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/Project_Settings.hpp>
#include <godot_cpp/variant/Rect2.hpp>
#include <godot_cpp/variant/RID.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/world3d.hpp>

#include <vector>
#include <string>

using namespace godot;

Shader* HoloPlayVolume::blit_shader = nullptr;
Shader* HoloPlayVolume::lightfield_shader = nullptr;
const GLfloat HoloPlayVolume::tri_verts[6] = {
        -1.0, -1.0,
        -1.0,  3.0,
        3.0, -1.0
};
HoloPlayVolume* HoloPlayVolume::grabbed_display = nullptr;

void HoloPlayVolume::compile_shaders() {
    // Compile shaders.
    static std::string gl_version_string = "#version 330 core\n";
    lightfield_shader = new Shader((gl_version_string + hpc_LightfieldVertShaderGLSL).c_str(), (gl_version_string + hpc_LightfieldFragShaderGLSL).c_str());
};

void HoloPlayVolume::free_shaders() {
    // Free shaders.
    delete blit_shader;
    delete lightfield_shader;
}

void HoloPlayVolume::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_process"), &HoloPlayVolume::_process);
    ClassDB::bind_method(D_METHOD("_input"), &HoloPlayVolume::_input);
    ClassDB::bind_method(D_METHOD("_notification"), &HoloPlayVolume::_notification);
    ClassDB::bind_method(D_METHOD("get_aspect"), &HoloPlayVolume::get_aspect);
    ClassDB::bind_method(D_METHOD("get_rect"), &HoloPlayVolume::get_rect);
    ClassDB::bind_method(D_METHOD("grab_mouse"), &HoloPlayVolume::grab_mouse);
    ClassDB::bind_method(D_METHOD("release_mouse"), &HoloPlayVolume::release_mouse);
    ClassDB::bind_method(D_METHOD("get_mouse_position"), &HoloPlayVolume::get_mouse_position);
    ClassDB::bind_method(D_METHOD("project_position"), &HoloPlayVolume::project_position);
    ClassDB::bind_method(D_METHOD("project_ray_origin"), &HoloPlayVolume::project_ray_origin);
    ClassDB::bind_method(D_METHOD("project_ray_normal"), &HoloPlayVolume::project_ray_normal);
    ClassDB::bind_method(D_METHOD("get_quilt_tex"), &HoloPlayVolume::get_quilt_tex);
    ClassDB::bind_method(D_METHOD("get_cull_mask"), &HoloPlayVolume::get_cull_mask);
    ClassDB::bind_method(D_METHOD("set_cull_mask", "p_cull_mask"), &HoloPlayVolume::set_cull_mask);
    ClassDB::bind_method(D_METHOD("get_environment"), &HoloPlayVolume::get_environment);
    ClassDB::bind_method(D_METHOD("get_device_index"), &HoloPlayVolume::get_device_index);
    ClassDB::bind_method(D_METHOD("set_device_index", "p_device_index"), &HoloPlayVolume::set_device_index);
    ClassDB::bind_method(D_METHOD("get_dummy"), &HoloPlayVolume::is_dummy);
    ClassDB::bind_method(D_METHOD("set_dummy", "p_dummy"), &HoloPlayVolume::set_dummy);
    ClassDB::bind_method(D_METHOD("get_debug_view"), &HoloPlayVolume::get_debug_view);
    ClassDB::bind_method(D_METHOD("set_debug_view", "p_debug_view"), &HoloPlayVolume::set_debug_view);
    ClassDB::bind_method(D_METHOD("get_near_clip"), &HoloPlayVolume::get_near_clip);
    ClassDB::bind_method(D_METHOD("set_near_clip", "p_near_clip"), &HoloPlayVolume::set_near_clip);
    ClassDB::bind_method(D_METHOD("get_far_clip"), &HoloPlayVolume::get_far_clip);
    ClassDB::bind_method(D_METHOD("set_far_clip", "p_far_clip"), &HoloPlayVolume::set_far_clip);
    ClassDB::bind_method(D_METHOD("get_view_dist"), &HoloPlayVolume::get_view_dist);
    ClassDB::bind_method(D_METHOD("set_view_dist", "p_view_dist"), &HoloPlayVolume::set_view_dist);
    ClassDB::bind_method(D_METHOD("get_view_cone"), &HoloPlayVolume::get_view_cone);
    ClassDB::bind_method(D_METHOD("set_view_cone", "p_view_cone"), &HoloPlayVolume::set_view_cone);
    ClassDB::bind_method(D_METHOD("get_size"), &HoloPlayVolume::get_size);
    ClassDB::bind_method(D_METHOD("set_size", "p_size"), &HoloPlayVolume::set_size);
    ClassDB::bind_method(D_METHOD("get_quilt_preset"), &HoloPlayVolume::get_quilt_preset);
    ClassDB::bind_method(D_METHOD("set_quilt_preset", "p_quilt_preset"), &HoloPlayVolume::set_quilt_preset);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "cull_mask", PROPERTY_HINT_FLAGS, ""), "set_cull_mask", "get_cull_mask");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "environment"), "set_environment", "get_environment");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "device_index"), "set_device_index", "get_device_index");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dummy"), "set_dummy", "get_dummy");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_view"), "set_debug_view", "get_debug_view");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "near_clip"), "set_near_clip", "get_near_clip");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "far_clip"), "set_far_clip", "get_far_clip");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "view_dist"), "set_view_dist", "get_view_dist");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "view_cone"), "set_view_cone", "get_view_cone");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "size"), "set_size", "get_size");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "quilt_preset", PROPERTY_HINT_ENUM, "Low,Medium,High,Very High"), "set_quilt_preset", "get_quilt_preset");

    // register_property("cull_mask", &HoloPlayVolume::set_cull_mask, &HoloPlayVolume::get_cull_mask, (uint32_t)0xffffff, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_LAYERS_3D_RENDER);
    // register_property("environment", &HoloPlayVolume::set_environment, &HoloPlayVolume::get_environment, Ref<Environment>(),  GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Environment");
    // register_property("device_index", &HoloPlayVolume::set_device_index, &HoloPlayVolume::get_device_index, 0);
    // register_property("dummy", &HoloPlayVolume::set_dummy, &HoloPlayVolume::is_dummy, false);
    // register_property("debug_view", &HoloPlayVolume::set_debug_view, &HoloPlayVolume::get_debug_view, false);
    // register_property("near_clip", &HoloPlayVolume::set_near_clip, &HoloPlayVolume::get_near_clip, 0.2f);
    // register_property("far_clip", &HoloPlayVolume::set_far_clip, &HoloPlayVolume::get_far_clip, 0.5f);
    // register_property("view_dist", &HoloPlayVolume::set_view_dist, &HoloPlayVolume::get_view_dist, 1.0f);
    // register_property("view_cone", &HoloPlayVolume::set_view_cone, &HoloPlayVolume::get_view_cone, 80.0f);
    // register_property("size", &HoloPlayVolume::set_size, &HoloPlayVolume::get_size, 1.0f);
    // register_property("quilt_preset", &HoloPlayVolume::set_quilt_preset, &HoloPlayVolume::get_quilt_preset, (int)QuiltPreset::MEDIUM_QUALITY,  GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "Low Quality, Medium Quality, High Quality, Very High Quality");
}

HoloPlayVolume::HoloPlayVolume() { }

HoloPlayVolume::~HoloPlayVolume() { }

float HoloPlayVolume::get_aspect() const {
    return aspect;
}

Rect2 HoloPlayVolume::get_rect() const {
    return Rect2(Vector2((real_t)win_x, (real_t)win_y), Vector2((real_t)screen_w, (real_t)screen_h));
}

Ref<Environment> HoloPlayVolume::get_environment() const {
    return environment;
};

void HoloPlayVolume::set_environment(const Ref<Environment> p_environment) {
    environment = p_environment;

    RenderingServer *vs = RenderingServer::get_singleton();
    for (int i = 0; i < cameras.size(); ++i) {
        RID camera = cameras[i];
        if (environment.is_valid()) {
            vs->camera_set_environment(camera, environment->get_rid());
        } else {
            vs->camera_set_environment(camera, RID());
        }
    }
};

int HoloPlayVolume::get_device_index() const {
    return device_index;
}

uint32_t HoloPlayVolume::get_cull_mask() const {
    return layers;
};

void HoloPlayVolume::set_cull_mask(uint32_t p_layers) {
    layers = p_layers;
    RenderingServer *vs = RenderingServer::get_singleton();
    for (int i = 0; i < cameras.size(); ++i) {
        RID camera = cameras[i];
        vs->camera_set_cull_mask(camera, layers);
    }
};

void HoloPlayVolume::set_cull_mask_bit(int p_layer, bool p_enable) {
    ERR_FAIL_INDEX(p_layer, 32);
    if (p_enable) {
        set_cull_mask(layers | (1 << p_layer));
    } else {
        set_cull_mask(layers & (~(1 << p_layer)));
    }
};

bool HoloPlayVolume::get_cull_mask_bit(int p_layer) const {
    ERR_FAIL_INDEX_V(p_layer, 32, false);
    return (layers & (1 << p_layer));
};

void HoloPlayVolume::set_device_index(int p_device_index) {
    device_index = p_device_index;
    update_device_properties();
}

bool HoloPlayVolume::is_dummy() const {
    return dummy;
}

void HoloPlayVolume::set_dummy(bool p_dummy) {
    dummy = p_dummy;
    update_device_properties();
}

bool HoloPlayVolume::get_debug_view() const {
    return debug_view;
}

void HoloPlayVolume::set_debug_view(bool p_debug_view) {
    debug_view = p_debug_view;
    lightfield_shader->use();
    lightfield_shader->setInt("debug", (int)debug_view);
}

float HoloPlayVolume::get_near_clip() const {
    return near_clip;
}

void HoloPlayVolume::set_near_clip(float p_near_clip) {
    near_clip = Math::max(p_near_clip, 0.0f);
    update_gizmos();
    update_cameras();
}

float HoloPlayVolume::get_far_clip() const {
    return far_clip;
}

void HoloPlayVolume::set_far_clip(float p_far_clip) {
    far_clip = Math::max(p_far_clip, 0.0f);
    update_gizmos();
    update_cameras(); 
}

float HoloPlayVolume::get_view_dist() const {
    return view_dist;
}

void HoloPlayVolume::set_view_dist(float p_view_dist) {
    view_dist = Math::max(p_view_dist, 0.0f);
    update_cameras();
}

float HoloPlayVolume::get_view_cone() const {
    return view_cone;
}

void HoloPlayVolume::set_view_cone(float p_view_cone) {
    view_cone = p_view_cone;
    update_cameras();
}

float HoloPlayVolume::get_size() const {
    return size;
}

void HoloPlayVolume::set_size(float p_size) {
    size = p_size;
    update_gizmos();
    update_cameras();
}

int HoloPlayVolume::get_quilt_preset() const {
    return quilt_preset;
}

void HoloPlayVolume::set_quilt_preset(int p_quilt_preset) { 
    quilt_preset = (QuiltPreset)p_quilt_preset;
    switch (quilt_preset) {
        case QuiltPreset::LOW_QUALITY:
            tex_width = 1024;
            tex_height = 1024;
            total_views = 21;
            num_cols = 3;
            num_rows = 7;
            break;
        case QuiltPreset::MEDIUM_QUALITY:
            tex_width = 2048;
            tex_height = 2048;
            total_views = 32;
            num_cols = 4;
            num_rows = 8;
            break;
        case QuiltPreset::HIGH_QUALITY:
            tex_width = 4096;
            tex_height = 4096;
            total_views = 45;
            num_cols = 5;
            num_rows = 9;
            break;
        case QuiltPreset::VERY_HIGH_QUALITY:
            tex_width = 8192;
            tex_height = 8192;
            total_views = 45;
            num_cols = 5;
            num_rows = 9;
            break;
    }
    create_viewports_and_cameras();

    // Resize quilt texture.
    update_quilt_viewport();

    lightfield_shader->use();
    lightfield_shader->setVec3("tile", (float)num_cols, (float)num_rows, (float)total_views);
}

void HoloPlayVolume::_init() {
        update_device_properties();

        set_notify_transform(true);
}

void HoloPlayVolume::_notification(int what) {
    if (what == Area3D::NOTIFICATION_ENTER_WORLD) {
        in_world = true;

        // Create our GLFW window without a context since we will be re-using
        // Godot's context.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        window = glfwCreateWindow(300, 400, "Godot HoloPlay Output", NULL, NULL);
        if (window == NULL)
        {
            ERR_PRINT("Could not create GLFW window!");
            return;
        }

        glfwGetWindowContentScale(window, &scale_x, &scale_y);
        glfwSetWindowContentScaleCallback(window, HoloPlayVolume::static_window_content_scale_callback);
        glfwSetWindowPos(window, (int)(scale_x*win_x)+1, (int)(scale_y*win_y)+1);
        glfwSetWindowSize(window, (int)(scale_x*screen_w), (int)(scale_y*screen_h));
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        glfwSetWindowUserPointer(window, (void *)this);
        glfwSetWindowFocusCallback(window, HoloPlayVolume::static_window_focus_callback);

        // Get the windows's device context.
        HWND hwnd = glfwGetWin32Window(window);
        hdc = GetDC(hwnd);
        // Not using async causes issues with Godot.
        ShowWindowAsync(hwnd, SW_SHOWNOACTIVATE);
        //SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_NOACTIVATE);
        if (!hdc) {
            ERR_PRINT("Could not retrieve device context!");
        }

        DisplayServer* dp = DisplayServer::get_singleton();
        int64_t id_opengl_ctx = dp->window_get_native_handle(DisplayServer::HandleType::OPENGL_CONTEXT);
        int64_t id_window_view = dp->window_get_native_handle(DisplayServer::HandleType::WINDOW_VIEW);

        // Set correct pixel format for the device context.
        HGLRC hglrc = (HGLRC)id_opengl_ctx;
        HDC hdc_gd = (HDC)id_window_view;

        PIXELFORMATDESCRIPTOR ppfd;
        int pxf = GetPixelFormat(hdc_gd);
        SetPixelFormat(hdc, pxf, &ppfd);

        // Create quilt canvas and viewport.
        RenderingServer *vs = RenderingServer::get_singleton();
        quilt_viewport_node = new Viewport();
        quilt_viewport = quilt_viewport_node->get_viewport_rid();
        vs->viewport_set_active(quilt_viewport, true);
        vs->viewport_set_update_mode(quilt_viewport, RenderingServer::VIEWPORT_UPDATE_ALWAYS);
        // vs->viewport_set_usage(quilt_viewport, VisualServer::VIEWPORT_USAGE_2D);
        quilt_canvas = vs->canvas_create();
        vs->viewport_attach_canvas(quilt_viewport, quilt_canvas);
        // vs->viewport_set_vflip(quilt_viewport, true);
        update_quilt_viewport();
        create_viewports_and_cameras();

        // Create VBO.
        glGenBuffers(1, &tri_vbo);
	    glBindBuffer(GL_ARRAY_BUFFER, tri_vbo);
	    glBufferData(GL_ARRAY_BUFFER, sizeof(tri_verts), tri_verts, GL_STATIC_DRAW);

    } else if (what == Area3D::NOTIFICATION_EXIT_WORLD) {
        in_world = false;
        glfwDestroyWindow(window);
        free_viewports_and_cameras();
        RenderingServer::get_singleton()->free_rid(quilt_canvas);
        quilt_viewport_node->queue_free();
        glDeleteBuffers(1, &tri_vbo);
    } else if (what == Area3D::NOTIFICATION_TRANSFORM_CHANGED) {
        // Update only the camera transforms but not the projections.
        RenderingServer *vs = RenderingServer::get_singleton();
        float s_view_dist = size * view_dist;

        for (int i = 0; i < cameras.size(); ++i) {
            RID camera = cameras[i];
            // Shift the camera horizontaly inside the view cone.
            float shift_angle = (i / (total_views - 1.0f) - 0.5f) * Math::deg_to_rad(view_cone);  
            float shift = s_view_dist * Math::tan(shift_angle);

            Transform3D t = get_global_transform(); // Should still be anchored to "unscaled" world
            t = t.translated(Vector3(shift, 0.0, s_view_dist));
            vs->camera_set_transform(camera, t);
        }
    }
}

void HoloPlayVolume::_process(float delta) {
    render_lightfield();

    if (wait_for_active) {
        DisplayServer* dp = DisplayServer::get_singleton();
        int64_t id_window_handle = dp->window_get_native_handle(DisplayServer::HandleType::WINDOW_HANDLE);

        HWND hwnd = (HWND)id_window_handle;
        SetActiveWindow(hwnd);
        if (hwnd == GetActiveWindow()) {
            wait_for_active = false;
        }
    }
}

void HoloPlayVolume::update_device_properties() {
    if (!dummy) {
        win_x = hpc_GetDevicePropertyWinX(device_index);
        win_y = hpc_GetDevicePropertyWinY(device_index);

        screen_w = hpc_GetDevicePropertyScreenW(device_index);
        screen_h = hpc_GetDevicePropertyScreenH(device_index);

        aspect = hpc_GetDevicePropertyDisplayAspect(device_index);

        inv_view = (bool)hpc_GetDevicePropertyInvView(device_index);

        red_index = hpc_GetDevicePropertyRi(device_index);
        blue_index = hpc_GetDevicePropertyBi(device_index);

        lent_pitch = hpc_GetDevicePropertyPitch(device_index);
        lent_offset = hpc_GetDevicePropertyCenter(device_index);
        lent_tilt = hpc_GetDevicePropertyTilt(device_index);

        fringe = hpc_GetDevicePropertyFringe(device_index);

        subpixel = hpc_GetDevicePropertySubp(device_index);
    } else {
        win_x = 0;
        win_y = 0;
        
        screen_w = 300;
        screen_h = 400;

        aspect = 0.75;

        inv_view = false;

        red_index = 0;
        blue_index = 2;

        lent_pitch = 0.0;
        lent_offset = 0.0;
        lent_tilt = 0.0;

        fringe = 0.0;
        subpixel = 0.0;
    }

    // Update uniforms.
    if (lightfield_shader) {
        lightfield_shader->use();

        lightfield_shader->setFloat("pitch", lent_pitch);
        lightfield_shader->setFloat("tilt", lent_tilt);
        lightfield_shader->setFloat("center", lent_offset);
        lightfield_shader->setInt("invView", (int)inv_view);
        lightfield_shader->setFloat("subp", subpixel);
        lightfield_shader->setFloat("displayAspect", aspect);
        lightfield_shader->setFloat("quiltAspect", aspect);
        lightfield_shader->setInt("ri", red_index);
        lightfield_shader->setInt("bi", blue_index);

        lightfield_shader->setInt("overscan", 0);
        lightfield_shader->setInt("quiltInvert", 0);
        lightfield_shader->setVec3("tile", (float)num_cols, (float)num_rows, (float)total_views);
        lightfield_shader->setVec2("viewPortion", 1.0, 1.0);

        lightfield_shader->setInt("screenTex", 0);
    }

    // Update the gizmo with the new data.
    update_gizmos();

    create_viewports_and_cameras();

    if (window) {
        glfwGetWindowContentScale(window, &scale_x, &scale_y);
        glfwSetWindowPos(window, (int)(scale_x*win_x)+1, (int)(scale_y*win_y)+1);
        glfwSetWindowSize(window, (int)(scale_x*screen_w), (int)(scale_y*screen_h));
    }
}

void HoloPlayVolume::create_viewports_and_cameras() {
    if (!in_world) return;

    free_viewports_and_cameras();

    // Projection matrices correlate to viewport size in Godot
    // so we need to find the smallest viewport size with correct
    // aspect ratio. This sadly renders some unnecessary pixels :(
    float view_width = (float)tex_width / num_cols;
    float view_height = (float)tex_height / num_rows;
    float view_aspect = view_width / view_height;

    RenderingServer *vs = RenderingServer::get_singleton();
    for (int i = 0; i < total_views; ++i) {
        RID viewport = vs->viewport_create();
        RID camera = vs->camera_create();
        RID canvas_item = vs->canvas_item_create();
        
        float x = (i % num_cols) * view_width;
        float y = (i / num_cols) * view_height;
        vs->canvas_item_add_texture_rect(canvas_item,
                                         Rect2(x, y, view_width, view_height),
                                         vs->viewport_get_texture(viewport));

        vs->canvas_item_set_parent(canvas_item, quilt_canvas);

        if (view_aspect > aspect) {
            vs->viewport_set_size(viewport, (int)view_width, (int)(view_width / aspect));
        } else {
            vs->viewport_set_size(viewport, (int)(view_height * aspect), (int)(view_height));
        }

        vs->viewport_attach_camera(viewport, camera);
        vs->viewport_set_active(viewport, true);
        vs->viewport_set_update_mode(viewport, RenderingServer::VIEWPORT_UPDATE_ALWAYS);
        vs->viewport_set_scenario(viewport, get_world_3d()->get_scenario());

        viewports.push_back(viewport);
        cameras.push_back(camera);
        canvas_items.push_back(canvas_item);
    }

    update_cameras();
}

void HoloPlayVolume::update_cameras() {
    RenderingServer *vs = RenderingServer::get_singleton();
    float s_view_dist = size * view_dist;
    float s_near_clip = size * near_clip;
    float s_far_clip = size * far_clip;

    float znear = s_view_dist - s_near_clip;
    float zfar = s_view_dist + s_far_clip;
    float near_size = size * znear / s_view_dist / aspect; // Width of near plane (in global units)

    for (int i = 0; i < cameras.size(); ++i) {
        RID camera = cameras[i];
        // Shift the camera horizontaly inside the view cone.
        float shift_angle = (i / (total_views - 1.0f) - 0.5f) * Math::deg_to_rad(view_cone);  
        float shift = s_view_dist * Math::tan(shift_angle);

        Transform3D t = get_global_transform(); // Should still be anchored to "unscaled" world
        t = t.translated(Vector3(shift, 0.0, s_view_dist));
        vs->camera_set_transform(camera, t);

        Vector2 offset = Vector2(-shift * znear / s_view_dist, 0.0); // Tilt frustum near plane opposite to shift.
        vs->camera_set_frustum(camera, near_size, offset, znear, zfar);

        vs->camera_set_cull_mask(camera, layers);
        if (environment.is_valid()) {
            vs->camera_set_environment(camera, environment->get_rid());
        } else {
            vs->camera_set_environment(camera, RID());
        }
    }
}

void HoloPlayVolume::free_viewports_and_cameras() {
    RenderingServer *vs = RenderingServer::get_singleton();
    for (int i = 0; i < viewports.size(); ++i) {
        RID viewport = viewports[i];
        RID camera = cameras[i];
        RID canvas_item = canvas_items[i];

        // TODO: What does this translate to in 4.x?
        // vs->viewport_detach(viewport);
        vs->free_rid(viewport);
        vs->free_rid(camera);
        vs->free_rid(canvas_item);
    }

    viewports.resize(0);
    cameras.resize(0);
    canvas_items.resize(0);
}

void HoloPlayVolume::render_lightfield() {
    if (!hdc) return;
    RenderingServer *vs = RenderingServer::get_singleton();

    DisplayServer* dp = DisplayServer::get_singleton();
    int64_t id_opengl_ctx = dp->window_get_native_handle(DisplayServer::HandleType::OPENGL_CONTEXT);
    int64_t id_window_view = dp->window_get_native_handle(DisplayServer::HandleType::WINDOW_VIEW);

    HGLRC hglrc = (HGLRC)id_opengl_ctx;
    HDC hdc_gd = (HDC)id_window_view;

    wglMakeCurrent(hdc, hglrc);
    
    glBindBuffer(GL_ARRAY_BUFFER, tri_vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, (int)(scale_x*screen_w), (int)(scale_y*screen_h));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)(vs->viewport_get_texture(quilt_viewport).get_id()));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    lightfield_shader->use();

    glDrawArrays(GL_TRIANGLES, 0, 3);

    SwapBuffers(hdc);
    
    wglMakeCurrent(hdc_gd, hglrc); // Move context back to main window.
}

void HoloPlayVolume::update_quilt_viewport() {
    if (quilt_viewport.is_valid()) {
        RenderingServer *vs = RenderingServer::get_singleton();
        vs->viewport_set_size(quilt_viewport, tex_width, tex_height);
    }
}

void HoloPlayVolume::static_window_focus_callback(GLFWwindow *window, int focused) {
    HoloPlayVolume *hpv = (HoloPlayVolume *)glfwGetWindowUserPointer(window);
    hpv->window_focus_callback((bool)focused); 
}

void HoloPlayVolume::static_window_content_scale_callback(GLFWwindow* window, float xscale, float yscale) {
    HoloPlayVolume *hpv = (HoloPlayVolume *)glfwGetWindowUserPointer(window);
    hpv->window_content_scale_callback(xscale, yscale); 
}

void HoloPlayVolume::window_focus_callback(bool focused) {
    if (focused) {
        DisplayServer* dp = DisplayServer::get_singleton();
        int64_t id_window_handle = dp->window_get_native_handle(DisplayServer::HandleType::WINDOW_HANDLE);

        HWND hwnd = (HWND)id_window_handle;
        SetActiveWindow(hwnd);
    }
}

void HoloPlayVolume::window_content_scale_callback(float xscale, float yscale) {
    scale_x = xscale;
    scale_y = yscale;
    glfwSetWindowPos(window, (int)(scale_x*win_x)+1, (int)(scale_y*win_y)+1);
    glfwSetWindowSize(window, (int)(scale_x*screen_w), (int)(scale_y*screen_h));
}

void HoloPlayVolume::grab_mouse() {
    if (!grabbed_display) { // if no display has grabbed the mouse, change mouse mode
        Input *input = Input::get_singleton();
        orig_mouse_mode = input->get_mouse_mode();
        input->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
    }
    grabbed_display = this;
}

void HoloPlayVolume::release_mouse() {
    if (grabbed_display == this) {
        Input::get_singleton()->set_mouse_mode(orig_mouse_mode);
        grabbed_display = nullptr;
    } else {
        ERR_PRINT("Window belonging to this HoloPlayVolume has not currently grabbed the mouse.");
    }
}

Vector2 HoloPlayVolume::get_mouse_position() const {
    return mouse_pos;
}

Vector3 HoloPlayVolume::project_position(Vector2 screen_point, float z_depth) const {
    real_t rel_x = (real_t)(screen_point.x / (real_t)screen_w - 0.5);
    real_t rel_y = (real_t)(0.5 - screen_point.y / (real_t)screen_h);
    Vector3 local_pos = size * Vector3(rel_x, rel_y / aspect, -z_depth);
    return (get_global_transform() * Transform3D().translated(local_pos)).origin;
}

Vector3 HoloPlayVolume::project_ray_normal(Vector2 screen_point) const {
    return -get_global_transform().basis.get_column(2);
}

Vector3 HoloPlayVolume::project_ray_origin(Vector2 screen_point) const {
    return project_position(screen_point, -near_clip);
}

Ref<ViewportTexture> HoloPlayVolume::get_quilt_tex() const {
    return quilt_viewport_node->get_texture();
};

void HoloPlayVolume::_input(const Ref<InputEvent> event) {
    if (grabbed_display == this) {
        InputEventMouseMotion *mm = Object::cast_to<InputEventMouseMotion>(event.ptr());
        if (mm) {
            mouse_pos += mm->get_relative();
            mouse_pos.x = Math::clamp(mouse_pos.x, 0.0f, (real_t)screen_w);
            mouse_pos.y = Math::clamp(mouse_pos.y, 0.0f, (real_t)screen_h);
        }
    }
}

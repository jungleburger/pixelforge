#include "editor_app.hpp"
#include "editor_context.hpp"
#include "panels/viewport_panel.hpp"
#include "panels/console_panel.hpp"
#include "panels/element_palette_panel.hpp"
#include "panels/performance_panel.hpp"

#include <pixelforge/core/logger.hpp>
#include <pixelforge/procgen/generator.hpp>
#include <pixelforge/procgen/biome.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <format>

namespace pf::editor {

EditorApp::EditorApp() = default;
EditorApp::~EditorApp() { shutdown(); }

bool EditorApp::init(int window_w, int window_h) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        PF_LOG_FATAL("SDL_Init failed: {}", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    m_window = SDL_CreateWindow("PixelForge Editor",
                                window_w, window_h,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        PF_LOG_FATAL("SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
    }

    m_gl_ctx = SDL_GL_CreateContext(m_window);
    if (!m_gl_ctx) {
        PF_LOG_FATAL("SDL_GL_CreateContext failed: {}", SDL_GetError());
        return false;
    }
    SDL_GL_MakeCurrent(m_window, m_gl_ctx);
    SDL_GL_SetSwapInterval(1); // vsync

    m_registry = std::make_unique<ElementRegistry>();

    WorldConfig cfg;
    cfg.seed   = 42;
    cfg.width  = 512;
    cfg.height = 256;
    m_world    = std::make_unique<World>(cfg, *m_registry);

    m_particles = std::make_unique<ParticleSystem>(*m_world);

    m_renderer = std::make_unique<GlRenderer>();
    if (!m_renderer->init(m_window, window_w, window_h)) {
        PF_LOG_FATAL("GlRenderer init failed");
        return false;
    }
    m_renderer->settled_layer().init(cfg.width, cfg.height);

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForOpenGL(m_window, m_gl_ctx);
    ImGui_ImplOpenGL3_Init("#version 430");

    m_running = true;
    m_last_time = static_cast<float>(SDL_GetTicks()) / 1000.f;
    PF_LOG_INFO("EditorApp: initialised {}x{}", window_w, window_h);
    return true;
}

void EditorApp::run() {
    while (m_running) {
        const float now = static_cast<float>(SDL_GetTicks()) / 1000.f;
        const float dt  = now - m_last_time;
        m_last_time     = now;

        process_events();
        update(dt);
        render();
    }
}

void EditorApp::process_events() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL3_ProcessEvent(&ev);
        if (ev.type == SDL_EVENT_QUIT) m_running = false;
        if (ev.type == SDL_EVENT_WINDOW_RESIZED) {
            m_renderer->resize(ev.window.data1, ev.window.data2);
        }
    }
}

void EditorApp::update(float dt) {
    m_particles->update(dt);
    m_renderer->settled_layer().upload(*m_world);
    m_renderer->dynamic_layer().upload(*m_world);
}

void EditorApp::render() {
    m_renderer->begin_frame();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    // Minimal menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Quit")) m_running = false;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(m_window);
    m_renderer->end_frame();
}

void EditorApp::shutdown() {
    if (m_gl_ctx) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    m_renderer.reset();
    m_particles.reset();
    m_world.reset();
    m_registry.reset();
    if (m_gl_ctx)  { SDL_GL_DeleteContext(m_gl_ctx);  m_gl_ctx = nullptr; }
    if (m_window)  { SDL_DestroyWindow(m_window);     m_window = nullptr; }
    SDL_Quit();
}

} // namespace pf::editor

#include "editor_app.hpp"
#include <pixelforge/core/logger.hpp>
#include <pixelforge/procgen/generator.hpp>
#include <pixelforge/procgen/biome.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <cstring>
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
    m_biomes   = std::make_unique<BiomeRegistry>();
    register_default_biomes(*m_biomes, *m_registry);

    WorldConfig cfg;
    cfg.seed   = 42;
    cfg.width  = 512;
    cfg.height = 256;
    m_world    = std::make_unique<World>(cfg, *m_registry);

    m_particles = std::make_unique<ParticleSystem>(*m_world);
    m_reactions = std::make_unique<ReactionSystem>(*m_world);

    // Wire EditorContext
    m_ctx.world     = m_world.get();
    m_ctx.registry  = m_registry.get();
    m_ctx.reactions = m_reactions.get();
    m_ctx.biomes    = m_biomes.get();
    m_ctx.particles = m_particles.get();

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
    m_reactions->apply_dynamic_heat(dt);
    m_reactions->tick(dt);
    m_perf_panel.record_frame(dt);
    m_renderer->settled_layer().upload(*m_world);
    m_renderer->dynamic_layer().upload(*m_world);
}

void EditorApp::render() {
    m_renderer->begin_frame();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    // Menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save World", "Ctrl+S")) {
                if (m_world) {
                    auto result = pf::WorldSerialiser::save(*m_world, m_ctx.save_path.c_str());
                    if (result)
                        m_console_panel.push(std::format("[Save] World saved to '{}'.", m_ctx.save_path));
                    else
                        m_console_panel.push(std::format("[Save] ERROR: {}", result.error().message));
                }
            }
            if (ImGui::MenuItem("Load World", "Ctrl+L")) {
                auto result = pf::WorldSerialiser::load(m_ctx.save_path.c_str(), *m_registry);
                if (result) {
                    // Reconstruct world + dependent simulation objects
                    m_world     = std::make_unique<World>(std::move(*result));
                    m_particles = std::make_unique<ParticleSystem>(*m_world);
                    m_reactions = std::make_unique<ReactionSystem>(*m_world);
                    m_renderer->settled_layer().init(
                        m_world->config().width, m_world->config().height);
                    m_ctx.world     = m_world.get();
                    m_ctx.particles = m_particles.get();
                    m_ctx.reactions = m_reactions.get();
                    m_console_panel.push(std::format("[Load] World loaded from '{}'.", m_ctx.save_path));
                } else {
                    m_console_panel.push(std::format("[Load] ERROR: {}", result.error().message));
                }
            }
            ImGui::Separator();
            {
                char path_buf[256]{};
                std::strncpy(path_buf, m_ctx.save_path.c_str(), sizeof(path_buf) - 1);
                ImGui::SetNextItemWidth(200.f);
                if (ImGui::InputText("##savepath", path_buf, sizeof(path_buf)))
                    m_ctx.save_path = path_buf;
                ImGui::SameLine(); ImGui::TextDisabled("Path");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4")) m_running = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Temperature Overlay", nullptr, &m_ctx.show_temp_overlay);
            ImGui::MenuItem("Show Grid",           nullptr, &m_ctx.show_grid);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Simulation")) {
            ImGui::SliderFloat("Ambient Temp (°C)", &m_reactions->ambient_temperature,
                               -100.f, 2000.f, "%.0f");
            ImGui::SliderFloat("Conduction Scale",  &m_reactions->conduction_scale,
                               0.f, 10.f, "%.2f");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // Draw all panels
    m_viewport_panel.draw(m_ctx);
    m_inspector_panel.draw(m_ctx);
    m_element_editor_panel.draw(m_ctx);
    m_palette_panel.draw(m_ctx);
    m_console_panel.draw(m_ctx);
    m_perf_panel.draw(m_ctx);
    m_hierarchy_panel.draw(m_ctx);
    m_worldgen_panel.draw(m_ctx);
    m_asset_browser_panel.draw(m_ctx);

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
    m_reactions.reset();
    m_particles.reset();
    m_world.reset();
    m_biomes.reset();
    m_registry.reset();
    if (m_gl_ctx)  { SDL_GL_DestroyContext(m_gl_ctx);  m_gl_ctx = nullptr; }
    if (m_window)  { SDL_DestroyWindow(m_window);     m_window = nullptr; }
    SDL_Quit();
}

} // namespace pf::editor

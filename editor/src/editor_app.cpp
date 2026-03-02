#include "editor_app.hpp"
#include <pixelforge/core/logger.hpp>
#include <pixelforge/procgen/generator.hpp>
#include <pixelforge/procgen/biome.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <sol/sol.hpp>
#include <cstring>
#include <filesystem>
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
    cfg.seed       = 42;
    cfg.width      = 512;
    cfg.height     = 256;
    cfg.infinite_x = false;   // bounded for now; enable once chunk streaming is in
    m_world    = std::make_unique<World>(cfg, *m_registry);

    m_particles = std::make_unique<ParticleSystem>(*m_world);
    m_reactions = std::make_unique<ReactionSystem>(*m_world);

    // Wire EditorContext
    m_ctx.world     = m_world.get();
    m_ctx.registry  = m_registry.get();
    m_ctx.reactions = m_reactions.get();
    m_ctx.particles = m_particles.get();
    m_ctx.world_width  = cfg.width;
    m_ctx.world_height = cfg.height;

    m_renderer = std::make_unique<GlRenderer>();
    if (!m_renderer->init(m_window, window_w, window_h)) {
        PF_LOG_FATAL("GlRenderer init failed");
        return false;
    }
    m_renderer->settled_layer().init(cfg.width, cfg.height);
    m_renderer->init_fbo(cfg.width, cfg.height);

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

    // Phase 8: Hook the engine logger into the console panel.
    pf::Logger::instance().add_sink(
        [this](pf::LogLevel level, std::string_view msg) {
            m_console_panel.push(level, std::string(msg));
        });

    // Phase 8: Initialise Lua + REPL.
    init_lua();

    // Load Lua element definitions so the palette has usable elements.
    {
        auto try_load = [&](const std::string& dir) -> bool {
            std::error_code ec;
            if (!std::filesystem::is_directory(dir, ec)) return false;
            for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
                if (entry.path().extension() == ".lua")
                    (void)m_lua_api->load_element_file(*m_lua, entry.path().string().c_str());
            }
            return true;
        };
        if (!try_load("assets/elements"))
            try_load("sandbox/assets/elements");
    }

    // Register biomes AFTER elements are loaded so look-ups resolve.
    m_biomes = std::make_unique<BiomeRegistry>();
    register_default_biomes(*m_biomes, *m_registry);
    m_ctx.biomes = m_biomes.get();

    // Default selected element to the first non-Air element (if any).
    if (m_registry->size() > 1)
        m_ctx.selected_element = 1;

    PF_LOG_INFO("EditorApp: initialised {}x{}", window_w, window_h);
    return true;
}

void EditorApp::init_lua() {
    m_lua     = std::make_unique<sol::state>();
    m_lua_api = std::make_unique<LuaApi>(*m_world, *m_registry);
    m_lua_api->bind(*m_lua);

    // Wire exec_lua so ConsolePanel can run REPL commands.
    m_ctx.exec_lua = [this](std::string_view code) -> std::string {
        auto result = m_lua->safe_script(
            std::string(code), sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            return std::format("[Lua Error] {}", err.what());
        }
        // Try to stringify the return value.
        if (result.return_count() > 0) {
            sol::object ret = result[0];
            if (ret.get_type() == sol::type::string)
                return ret.as<std::string>();
            if (ret.get_type() == sol::type::number)
                return std::to_string(ret.as<double>());
            if (ret.get_type() == sol::type::boolean)
                return ret.as<bool>() ? "true" : "false";
        }
        return "";
    };
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
        // Space = toggle simulation pause (only when ImGui is not capturing keyboard)
        if (ev.type == SDL_EVENT_KEY_DOWN &&
            ev.key.key == SDLK_SPACE &&
            !ImGui::GetIO().WantCaptureKeyboard) {
            m_ctx.simulation_paused = !m_ctx.simulation_paused;
        }
    }
}

void EditorApp::update(float dt) {
    m_renderer->settled_layer().set_lighting_enabled(m_ctx.show_lighting);
    m_renderer->settled_layer().set_occlusion_enabled(m_ctx.light_occlusion);
    m_renderer->settled_layer().set_occlusion_strength(m_ctx.light_occlusion_strength);

    using Clock = std::chrono::high_resolution_clock;

    if (!m_ctx.simulation_paused) {
        auto t0 = Clock::now();
        m_particles->update(dt);
        auto t1 = Clock::now();
        m_reactions->apply_dynamic_heat(dt);
        m_reactions->tick(dt);
        auto t2 = Clock::now();

        auto ms = [](auto a, auto b) -> float {
            return std::chrono::duration<float, std::milli>(b - a).count();
        };
        m_sys_times.physics_ms   = ms(t0, t1);
        m_sys_times.reactions_ms = ms(t1, t2);
    } else {
        m_sys_times.physics_ms   = 0.f;
        m_sys_times.reactions_ms = 0.f;
    }

    m_perf_panel.record_frame(dt);

    // Always upload so edits made while paused are visible.
    auto t3 = Clock::now();
    m_renderer->settled_layer().upload(*m_world);
    m_renderer->dynamic_layer().upload(*m_world);
    auto t4 = Clock::now();

    m_sys_times.renderer_upload_ms = std::chrono::duration<float, std::milli>(t4 - t3).count();
    m_perf_panel.record_subsystem_times(m_sys_times);
}

void EditorApp::render() {
    m_renderer->begin_frame();
    m_renderer->render_world();
    m_ctx.viewport_texture = m_renderer->world_texture();

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
                    m_renderer->init_fbo(
                        m_world->config().width, m_world->config().height);
                    m_ctx.world     = m_world.get();
                    m_ctx.particles = m_particles.get();
                    m_ctx.reactions = m_reactions.get();
                    m_ctx.world_width  = m_world->config().width;
                    m_ctx.world_height = m_world->config().height;
                    // Re-bind Lua to the new world
                    m_lua_api = std::make_unique<LuaApi>(*m_world, *m_registry);
                    m_lua_api->bind(*m_lua);
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
            ImGui::MenuItem("Lighting",            nullptr, &m_ctx.show_lighting);
            ImGui::MenuItem("Light Occlusion",     nullptr, &m_ctx.light_occlusion);
            ImGui::SliderFloat("Occlusion Strength", &m_ctx.light_occlusion_strength,
                               0.f, 0.9f, "%.2f");
            ImGui::MenuItem("Show Grid",           nullptr, &m_ctx.show_grid);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Simulation")) {
            ImGui::MenuItem("Pause / Resume", "Space", &m_ctx.simulation_paused);
            ImGui::Separator();
            ImGui::SliderFloat("Ambient Temp (\u00b0C)", &m_reactions->ambient_temperature,
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

    {
        using Clock = std::chrono::high_resolution_clock;
        auto imgui_t0 = Clock::now();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        auto imgui_t1 = Clock::now();
        m_sys_times.imgui_ms = std::chrono::duration<float, std::milli>
                               (imgui_t1 - imgui_t0).count();
        m_perf_panel.record_subsystem_times(m_sys_times);
    }

    SDL_GL_SwapWindow(m_window);
    m_renderer->end_frame();
}

void EditorApp::shutdown() {
    if (m_gl_ctx) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    // Lua must be destroyed before world/registry
    m_ctx.exec_lua = {};  // clear the lambda that holds 'this'
    m_lua_api.reset();
    m_lua.reset();
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

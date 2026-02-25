#include "asset_browser_panel.hpp"
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <format>
#include <sstream>

namespace pf::editor {

namespace fs = std::filesystem;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string to_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

void AssetBrowserPanel::scan_directory(const std::string& dir) {
    m_entries.clear();
    m_selected_idx = -1;
    m_current_dir  = dir;
    m_preview.clear();

    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return;

    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (entry.is_regular_file(ec)) {
            FileEntry fe;
            fe.path         = entry.path();
            fe.display_name = entry.path().filename().string();
            fe.ext          = to_lower(entry.path().extension().string());
            m_entries.push_back(std::move(fe));
        }
    }

    std::sort(m_entries.begin(), m_entries.end(),
        [](const FileEntry& a, const FileEntry& b) {
            return a.display_name < b.display_name;
        });
}

void AssetBrowserPanel::load_preview(const FileEntry& fe) {
    std::ifstream f(fe.path);
    if (!f) { m_preview = "(cannot open file)"; return; }

    std::ostringstream ss;
    std::string line;
    int n = 0;
    while (std::getline(f, line) && n < 24) {
        ss << line << '\n';
        ++n;
    }
    if (!f.eof()) ss << "...\n";
    m_preview = ss.str();
}

// ── Draw ──────────────────────────────────────────────────────────────────────

void AssetBrowserPanel::draw(EditorContext& ctx) {
    ImGui::Begin("Asset Browser");

    // ── Path bar ─────────────────────────────────────────────────────────────
    char root_buf[256]{};
    std::strncpy(root_buf, m_root_path.c_str(), sizeof(root_buf) - 1);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 70.f);
    if (ImGui::InputText("##root", root_buf, sizeof(root_buf)))
        m_root_path = root_buf;
    ImGui::SameLine();
    if (ImGui::Button("Browse")) {
        scan_directory(m_root_path);
        m_status_msg.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("\u21ba Refresh")) {
        scan_directory(m_current_dir.empty() ? m_root_path : m_current_dir);
        m_status_msg.clear();
    }

    // Quick-navigate to sub-folders
    ImGui::Separator();
    if (ImGui::SmallButton("elements/")) {
        scan_directory(m_root_path + "elements/");
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("worlds/")) {
        scan_directory(m_root_path + "worlds/");
    }
    if (!m_current_dir.empty())
        ImGui::TextDisabled("  %s", m_current_dir.c_str());

    // ── File list + preview split ─────────────────────────────────────────────
    const float panel_h = ImGui::GetContentRegionAvail().y - 36.f;
    if (panel_h < 50.f) { ImGui::End(); return; }

    ImGui::Columns(2, "##ab_cols", true);
    ImGui::SetColumnWidth(0, 180.f);

    // Left: file list
    ImGui::BeginChild("##filelist", {0.f, panel_h}, false);

    if (m_entries.empty()) {
        ImGui::TextDisabled("(empty — press Browse)");
    }

    for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
        const auto& fe = m_entries[i];
        // Colour by type
        ImVec4 col{0.8f, 0.8f, 0.8f, 1.f};
        if      (fe.ext == ".lua")  col = {0.6f, 0.9f, 1.f, 1.f};
        else if (fe.ext == ".toml") col = {1.f, 0.85f, 0.5f, 1.f};
        else if (fe.ext == ".pfw")  col = {0.6f, 1.f, 0.6f, 1.f};

        ImGui::PushStyleColor(ImGuiCol_Text, col);
        const bool sel = (i == m_selected_idx);
        if (ImGui::Selectable(fe.display_name.c_str(), sel)) {
            m_selected_idx = i;
            load_preview(fe);
            m_status_msg.clear();
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    // Right: preview + actions
    ImGui::BeginChild("##preview", {0.f, panel_h}, false);

    if (m_selected_idx >= 0 && m_selected_idx < static_cast<int>(m_entries.size())) {
        const auto& fe = m_entries[m_selected_idx];
        ImGui::TextColored({1.f, 1.f, 0.5f, 1.f}, "%s", fe.display_name.c_str());
        ImGui::TextDisabled("%s", fe.path.string().c_str());
        ImGui::Separator();

        // ── Actions by file type ──────────────────────────────────────────
        if (fe.ext == ".toml") {
            // World config file — shows generation settings
            ImGui::TextColored({1.f, 0.85f, 0.5f, 1.f}, "TOML world config");
            ImGui::TextDisabled("Configure generation params and use the");
            ImGui::TextDisabled("World Gen panel to regenerate.");
            ImGui::SameLine();
            if (ImGui::Button("Set as Save Path")) {
                ctx.save_path = fe.path.string();
                m_status_msg  = std::format("Save path set to '{}'", fe.display_name);
            }
        } else if (fe.ext == ".lua") {
            ImGui::TextColored({0.6f, 0.9f, 1.f, 1.f},
                "Lua element definition");
            ImGui::TextDisabled("Hot-reload: Phase 4 feature.");
            ImGui::TextDisabled("Use the Phase 4 file-watcher to reload at runtime.");
        } else if (fe.ext == ".pfw") {
            if (ImGui::Button("Load Save")) {
                if (ctx.world && ctx.registry) {
                    // Note: full world reconstruction must be done via EditorApp;
                    // the panel logs the intent and the File > Load World action
                    // handles it properly. We just set the save path here.
                    ctx.save_path = fe.path.string();
                    m_status_msg  = std::format("\u2714 Save path set to '{}' — use File > Load World",
                                                fe.display_name);
                }
            }
        }

        // ── Preview ───────────────────────────────────────────────────────
        ImGui::Spacing();
        const float preview_h = ImGui::GetContentRegionAvail().y - 24.f;
        if (preview_h > 40.f) {
            // std::string::data() returns char* in C++17; READ_ONLY prevents writes
            ImGui::InputTextMultiline("##preview_text",
                m_preview.data(),
                m_preview.size(),
                {-1.f, preview_h},
                ImGuiInputTextFlags_ReadOnly);
        }
    } else {
        ImGui::TextDisabled("Select a file to preview.");
    }

    ImGui::EndChild();
    ImGui::Columns(1);

    // ── Status bar ────────────────────────────────────────────────────────────
    if (!m_status_msg.empty()) {
        ImGui::Separator();
        // Treat messages starting with the success checkmark (UTF-8 0xE2...) as ok
        const bool ok = (m_status_msg[0] == '\xe2');
        ImGui::TextColored(ok ? ImVec4{0.4f, 1.f, 0.5f, 1.f}
                              : ImVec4{1.f, 0.4f, 0.4f, 1.f},
                           "%s", m_status_msg.c_str());
    }

    ImGui::End();
}

} // namespace pf::editor


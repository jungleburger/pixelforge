#pragma once
#include "../editor_context.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace pf::editor {

class AssetBrowserPanel {
public:
    void draw(EditorContext& ctx);

private:
    std::string m_root_path{"assets/"};

    // Directory listing (populated on demand)
    struct FileEntry {
        std::filesystem::path path;
        std::string           display_name;   // filename only
        std::string           ext;            // lowercase extension
    };
    std::vector<FileEntry>  m_entries;
    std::string             m_current_dir;   // directory currently shown
    int                     m_selected_idx{-1};
    std::string             m_preview;       // first ~20 lines of selected file
    std::string             m_status_msg;

    void scan_directory(const std::string& dir);
    void load_preview(const FileEntry& fe);
};

} // namespace pf::editor

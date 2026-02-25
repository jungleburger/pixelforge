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
        bool                  is_dir{false};  // subdirectory entry
    };
    std::vector<FileEntry>  m_entries;
    std::string             m_current_dir;   // directory currently shown
    int                     m_selected_idx{-1};
    std::string             m_preview;       // first ~24 lines of selected file
    std::string             m_status_msg;
    bool                    m_status_ok{false};

    // Navigation breadcrumb stack
    std::vector<std::string> m_dir_stack;   // history of parent dirs

    // Search / filter
    char   m_filter_buf[256]{};

    // New-element dialog state
    bool   m_show_new_element_dlg{false};
    char   m_new_element_name[128]{};

    void scan_directory(const std::string& dir, bool push_history = true);
    void navigate_up();
    void load_preview(const FileEntry& fe);
    void open_in_explorer(const FileEntry& fe) const;
    void create_element_file(const std::string& name) const;

    // Returns true if the entry passes current filter
    bool passes_filter(const FileEntry& fe) const;
};

} // namespace pf::editor

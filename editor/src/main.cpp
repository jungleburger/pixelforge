#include "editor_app.hpp"
#include <iostream>

int main(int /*argc*/, char* /*argv*/[]) {
    pf::editor::EditorApp app;
    if (!app.init()) {
        std::cerr << "EditorApp::init() failed\n";
        return 1;
    }
    app.run();
    return 0;
}

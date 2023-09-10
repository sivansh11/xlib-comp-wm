#include "window_manager.hpp"

int main(int argc, char **argv) {
    wm::window_manager_t window_manager{argc, argv};
    window_manager.run();
    return 0;
}
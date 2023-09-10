#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

namespace wm {

class _window_manager_t;

class window_manager_t {
public:
    window_manager_t(int argc, char **argv);
    ~window_manager_t();

    void run();

private:
    _window_manager_t *_window_manager;
};

} // namespace wm

#endif
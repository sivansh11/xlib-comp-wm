#include "window_manager.hpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <sstream>
#include <iostream>
#include <span>
#include <map>
#include <unordered_map>
#include <cassert>
#include <algorithm>

namespace wm {

#define THROW(msgs) \
std::stringstream error; \
error << msgs; \
throw std::runtime_error(error.str())    

#define LOG(msgs) \
std::stringstream msg; \
msg << msgs; \
std::cout << msg.str() << std::endl

class _window_manager_t {
public:
    _window_manager_t(int argc, char **argv) {
        // in future parse the args to get the actual connection
        _display = XOpenDisplay(":1");
        if (!_display) {
            THROW("Failed to open X display" << XDisplayName(":1"));
        }
        _root = DefaultRootWindow(_display);

        WM_PROTOCOLS = XInternAtom(_display, "WM_PROTOCOLS", false);
        WM_DELETE_WINDOW = XInternAtom(_display, "WM_DELETE_WINDOW", false);
    }

    ~_window_manager_t() {
        XCloseDisplay(_display);
    }

    void frame(const Window& w) {
        const uint32_t BORDER_WIDTH = 3;
        const unsigned long BORDER_COLOR = 0xff0000;
        const unsigned long BG_COLOR = 0x0000ff;

        XWindowAttributes x_window_attributes;
        XGetWindowAttributes(_display, w, &x_window_attributes);

        const Window frame = XCreateSimpleWindow(_display, _root, x_window_attributes.x, x_window_attributes.y, x_window_attributes.width, x_window_attributes.height, BORDER_WIDTH, BORDER_COLOR, BG_COLOR);

        XSelectInput(_display, frame, SubstructureRedirectMask | SubstructureNotifyMask);

        XAddToSaveSet(_display, w);

        XReparentWindow(_display, w, frame, 0, 0);

        XMapWindow(_display, frame);

        _clients[w] = frame;

        XGrabButton(_display, 
                    Button1, 
                    Mod1Mask, 
                    w, 
                    false, 
                    ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                    GrabModeAsync,
                    GrabModeAsync,
                    None,
                    None);
        
        XGrabButton(_display,
                    Button3,
                    Mod1Mask,
                    w,
                    false,
                    ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                    GrabModeAsync,
                    GrabModeAsync,
                    None,
                    None);

        XGrabKey(_display,
                 XKeysymToKeycode(_display, XK_F4),
                 Mod1Mask,
                 w,
                 false,
                 GrabModeAsync,
                 GrabModeAsync);

        XGrabKey(_display,
                 XKeysymToKeycode(_display, XK_Tab),
                 Mod1Mask,
                 w,
                 false,
                 GrabModeAsync,
                 GrabModeAsync);


        LOG("framed window " << w << "[" << frame << "]");
    }

    void unframe(const Window& w) {
        const Window frame = _clients[w];
        
        XUnmapWindow(_display, frame);

        XReparentWindow(_display, w, _root, 0, 0);

        XRemoveFromSaveSet(_display, w);

        XDestroyWindow(_display, frame);

        _clients.erase(w);

        LOG("Unframed window w " << w << " [" << frame << "]");

    }

    void run() {
        static bool wm_detected = false;
        XSetErrorHandler([](Display *display, XErrorEvent *e) -> int {
            if (e->error_code == BadAccess) {
                wm_detected = true;
            }
            return 0;
        });
        XSelectInput(_display, _root, SubstructureRedirectMask | SubstructureNotifyMask);
        XSync(_display, false);
        if (wm_detected) {
            THROW("Detected another window manager on display " << XDisplayString(_display));   
        }

        XSetErrorHandler([](Display *display, XErrorEvent *e) -> int {
            const int MAX_ERROR_TEXT_LENGTH = 1024;
            char error_text[MAX_ERROR_TEXT_LENGTH];
            XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
            LOG("ERROR: " << error_text);
            return 0;
        });

        for (;;) {
            XEvent e;
            XNextEvent(_display, &e);
            switch (e.type) {
                case CreateNotify: {
                    LOG("CreateNotify");
                }
                break;

                case DestroyNotify: {
                    LOG("DestroyNotify");
                }
                break;

                case ReparentNotify: {
                    LOG("ReparentNotify");
                }
                break;

                case MapNotify: {
                    LOG("MapNotify");
                }  
                break;

                case UnmapNotify: {
                    LOG("UnmapNotify");
                    [this](const XUnmapEvent& e) {
                        if (!_clients.count(e.window)) {
                            LOG("Ignore UnmapNotify for non-client window" << e.window);
                            return;
                        }
                        unframe(e.window);

                    }(e.xunmap);
                }
                break;

                case ConfigureNotify: {
                    LOG("ConfigureNotify");
                }   
                break;   

                case MapRequest: {
                    LOG("MapRequest");
                    [this](const XMapRequestEvent& e) {
                        frame(e.window);
                        XMapWindow(_display, e.window);
                    }(e.xmaprequest);
                }   
                break; 

                case ConfigureRequest: {
                    LOG("ConfigureRequest");
                    [this](const XConfigureRequestEvent& e) {
                        XWindowChanges changes{};
                        changes.x = e.x;
                        changes.y = e.y;
                        changes.width = e.width;
                        changes.height = e.height;
                        changes.border_width = e.border_width;
                        changes.sibling = e.above;
                        changes.stack_mode = e.detail;

                        if (_clients.count(e.window)) {
                            const Window frame = _clients[e.window];
                            XConfigureWindow(_display, frame, e.value_mask, &changes);
                            LOG("Resize [" << frame << "] to " << e.width << ' ' << e.height);
                        }

                        XConfigureWindow(_display, e.window, e.value_mask, &changes);
                        LOG("resize " << e.window << " to " << e.width << " " << e.height);

                    }(e.xconfigurerequest);
                }
                break;  

                case ButtonPress: {
                    LOG("ButtonPress");
                    [this](const XButtonEvent& e) {
                        assert(_clients.contains(e.window));
                        const Window frame = _clients[e.window];

                        _drag_start_position.x = e.x_root;
                        _drag_start_position.y = e.y_root;

                        Window returned_root;
                        int x, y;
                        unsigned int width, height, border_width, depth;

                        XGetGeometry(_display, frame, &returned_root, &x, &y, &width, &height, &border_width, &depth);
                    
                        _drag_start_frame_position.x = x;
                        _drag_start_frame_position.y = y;

                        _drag_start_frame_size.x = width;
                        _drag_start_frame_size.y = height;

                        XRaiseWindow(_display, frame);
                    }(e.xbutton);
                }
                break;

                case ButtonRelease: {
                    LOG("ButtonRelease");
                }
                break;

                case MotionNotify: {
                    LOG("MotionNotify");
                    while (XCheckTypedWindowEvent(_display, e.xmotion.window, MotionNotify, &e));
                    [this](const XMotionEvent& e) {
                        assert(_clients.contains(e.window));
                        const Window frame = _clients[e.window];
                        point_2D<int> drag_position;
                        drag_position.x = e.x_root;
                        drag_position.y = e.y_root;
                        point_2D<int> delta;
                        delta.x = drag_position.x - _drag_start_position.x;
                        delta.y = drag_position.y - _drag_start_position.y;

                        if (e.state & Button1Mask) {
                            point_2D<int> destination_frame_position;
                            destination_frame_position.x = _drag_start_frame_position.x + delta.x;
                            destination_frame_position.y = _drag_start_frame_position.y + delta.y;
                            XMoveWindow(_display, frame, destination_frame_position.x, destination_frame_position.y);
                        } else if (e.state & Button3Mask) {
                            point_2D<int> size_delta;
                            size_delta.x = std::max(delta.x, -_drag_start_frame_size.width);
                            size_delta.y = std::max(delta.y, -_drag_start_frame_size.height);
                            point_2D<int> destination_frame_size;
                            destination_frame_size.x = _drag_start_frame_size.x + size_delta.x;
                            destination_frame_size.y = _drag_start_frame_size.y + size_delta.y;
                            XResizeWindow(_display, frame, destination_frame_size.width, destination_frame_size.height);
                            XResizeWindow(_display, e.window, destination_frame_size.width, destination_frame_size.height);
                        }
                    }(e.xmotion);
                }
                break;

                case KeyPress: {
                    LOG("KeyPress");
                    [this](const XKeyEvent& e) {
                        if ((e.state & Mod1Mask) && (e.keycode == XKeysymToKeycode(_display, XK_F4))) {
                            Atom *supported_protocols;
                            int num_supported_protocols;
                            if (XGetWMProtocols(_display, e.window, &supported_protocols, &num_supported_protocols) && (std::find(supported_protocols, supported_protocols + num_supported_protocols, WM_DELETE_WINDOW) != supported_protocols + num_supported_protocols)) {
                                LOG("Deleting window " << e.window);
                                XEvent message{};
                                message.xclient.type = ClientMessage;
                                message.xclient.message_type = WM_PROTOCOLS;
                                message.xclient.window = e.window;
                                message.xclient.format = 32;
                                message.xclient.data.l[0] = WM_DELETE_WINDOW;
                                XSendEvent(_display, e.window, false, 0, &message);
                            } else {
                                LOG("Killing window " << e.window);
                                XKillClient(_display, e.window);
                            }
                        } else if ((e.state & Mod1Mask) && (e.keycode == XKeysymToKeycode(_display, XK_Tab))) {
                            auto i = _clients.find(e.window);
                            assert(i != _clients.end());
                            i++;
                            if (i == _clients.end()) {
                                i = _clients.begin();
                            }
                            XRaiseWindow(_display, i->second);
                            XSetInputFocus(_display, i->first, RevertToPointerRoot, CurrentTime);
                        }
                    }(e.xkey);
                }
                break;

                case KeyRelease: {

                }
                break;
            
                default: {
                    LOG("ignored event");
                }
                break;
            }
        }
    }

    Display *_display;
    Window _root;
    std::map<Window, Window> _clients;

    template <typename t>
    struct point_2D {
        union {
            t x, y;
            t data[2]; 
            t width, height;
        };

    };

    point_2D<int> _drag_start_position;
    point_2D<int> _drag_start_frame_position;
    point_2D<int> _drag_start_frame_size;

    Atom WM_PROTOCOLS;
    Atom WM_DELETE_WINDOW;
};

window_manager_t::window_manager_t(int argc, char **argv) {
    _window_manager = new _window_manager_t(argc, argv);
}

window_manager_t::~window_manager_t() {
    delete _window_manager;
}

void window_manager_t::run() {
    _window_manager->run();
}

} // namespace wm   

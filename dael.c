/*
 * file: dael.c
 * ------------
 * Author: Mason Armand
 * Date Created: Feb 27, 2025
 * Last Modified: Feb 27, 2025
 */
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

typedef struct {
        Cursor normal;
        Cursor resize;
        Cursor grab;
        Cursor plus;
} Dael_Cursors;

typedef struct Dael_Client Dael_Client;
typedef struct Dael_Workspace Dael_Workspace;

struct Dael_Client {
        Window win;
        int x;
        int y;
        int w;
        int h;
        bool is_fullscreen;
        Dael_Client* next;
        Dael_Client* prev;
};

struct Dael_Workspace {
        unsigned int id;
        Dael_Client* clients;
        Dael_Workspace* next;
        Dael_Workspace* prev;
};

typedef struct {
        unsigned int mod;
        KeySym key_sym;
        void (*func)(const char*);
        const void* arg;
} Dael_Keybinding;

typedef struct {
        Dael_Cursors curs;
        Dael_Workspace* workspaces;
        XFontStruct* font;
        Window root;
        Display* dpy;
        bool running;
} Dael_State;

typedef struct {
        int event_type;
        void (*handler)(XEvent*);
} Dael_EventHandler;

void launch_program(const char* program);
void quit(const char* args);

#include "config.h"


void grab_keys();

void handle_event(XEvent* e);
void handle_key_press(XEvent* e);
void handle_map_request(XEvent* e);

void Dael_State_init(Dael_State* wm);
void Dael_State_free(Dael_State* wm);

int xerror_handler(Display* display, XErrorEvent* error);

/* XEvent handler functions */
Dael_EventHandler event_handlers[] = {
    { KeyPress, handle_key_press },
    { MapRequest, handle_map_request },
    { 0, NULL }
};

/* global window manager state */
Dael_State wm = { 0 };


int main(void)
{
        XEvent e;

        XSetErrorHandler(xerror_handler);

        if (!(wm.dpy = XOpenDisplay(NULL))) {
                fprintf(stderr, "Failed to open display.\n");
                return 1;
        }

        wm.root = DefaultRootWindow(wm.dpy);

        XSelectInput(
                wm.dpy, wm.root,
                SubstructureRedirectMask |
                SubstructureNotifyMask |
                PropertyChangeMask |
                ButtonPressMask |
                ButtonReleaseMask |
                PointerMotionMask
        );

        grab_keys(&wm);

        XFlush(wm.dpy);

        /* XSetErrorHandler(xerror_handler); */

        XSync(wm.dpy, False);
        wm.running = true;

        while (wm.running) {
                XEvent e;
                XNextEvent(wm.dpy, &e);
                handle_event(&e);
        }

        Dael_State_free(&wm);
        return 0;
}


void Dael_State_init(Dael_State* wm)
{
        if (!(wm->dpy = XOpenDisplay(NULL)))
                fprintf(stderr, "Failed to open display.\n");
        wm->root = DefaultRootWindow(wm->dpy);

        XSelectInput(
                wm->dpy, wm->root,
                SubstructureRedirectMask |
                SubstructureNotifyMask |
                PropertyChangeMask |
                ButtonPressMask |
                ButtonReleaseMask |
                PointerMotionMask
        );

        wm->curs.normal = XCreateFontCursor(wm->dpy, XC_left_ptr);
        wm->curs.grab = XCreateFontCursor(wm->dpy, XC_fleur);
        wm->curs.resize = XCreateFontCursor(wm->dpy, XC_sizing);
        wm->curs.plus = XCreateFontCursor(wm->dpy, XC_plus);
        XDefineCursor(wm->dpy, wm->root, wm->curs.normal);

        wm->font = XLoadQueryFont(wm->dpy, FONT);
        if (!wm->font) {
                fprintf(stderr, "Failed to load font\n");
        }
}


void Dael_State_free(Dael_State* wm)
{
        XFreeFont(wm->dpy, wm->font);
        XFreeCursor(wm->dpy, wm->curs.normal);
        XFreeCursor(wm->dpy, wm->curs.resize);
        XFreeCursor(wm->dpy, wm->curs.grab);
        XFreeCursor(wm->dpy, wm->curs.plus);
        XCloseDisplay(wm->dpy);
}


void grab_keys()
{
        unsigned int i = 0;
        XUngrabKey(wm.dpy, AnyKey, AnyModifier, wm.root);

        while (config_keys[i].key_sym != NoSymbol) {
                KeySym key_sym = config_keys[i].key_sym;
                KeyCode keycode = XKeysymToKeycode(wm.dpy, key_sym);
                unsigned int mod = config_keys[i].mod;

                XGrabKey(wm.dpy, keycode, mod, wm.root, True,  GrabModeAsync, GrabModeAsync);
                i++;
        }
}


void launch_program(const char* program)
{
        pid_t pid = fork();

        if (pid == 0) {
                char* argv[] = {NULL, NULL};
                argv[0] = (char*) program;
                execvp(argv[0], argv);
                perror("execvp");
                exit(1);
        }
        else if (pid < 0) {
                perror("fork");
        }
}


void quit(const char* args)
{
        (void) args;
        wm.running = false;
}


void handle_event(XEvent* e)
{
        Dael_EventHandler* h;
        for (h = event_handlers; h->handler; h++) {
                if (e->type == h->event_type) {
                        h->handler(e);
                        return;
                }
        }
}


void handle_key_press(XEvent* e)
{
        unsigned int i = 0;
        XKeyEvent k = e->xkey;
        (void) wm;
        while (config_keys[i].key_sym != NoSymbol) {
                if ((k.state & config_keys[i].mod)
                && (k.keycode == XKeysymToKeycode(wm.dpy, config_keys[i].key_sym))
                && (config_keys[i].func)) {
                        config_keys[i].func(config_keys[i].arg);

                }
                i++;
        }
}

void handle_map_request(XEvent* e)
{
        XMapRequestEvent* req = &e->xmaprequest;
        XMapWindow(wm.dpy, req->window);
}


int xerror_handler(Display* display, XErrorEvent* error)
{
        char error_msg[120];
        XGetErrorText(display, error->error_code, error_msg, sizeof(error_msg));
        fprintf(stderr, "X Error: %s\n", error_msg);
        /*exit(1);*/
        return 0;
}

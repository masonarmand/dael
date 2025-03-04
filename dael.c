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
        Dael_Client* focused;
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
        Dael_Workspace* current_workspace;
        XFontStruct* font;
        Window root;
        Display* dpy;
        bool running;
} Dael_State;

typedef struct {
        int event_type;
        void (*handler)(XEvent*);
} Dael_EventHandler;

/* config.h / user key-bindable functions */
void launch_program(const char* program);
void quit(const char* args);
void focus_next(const char* args);
void focus_prev(const char* args);
void append_workspace(const char* args);
void next_workspace(const char* args);
void prev_workspace(const char* args);


#include "config.h"

Dael_Client* add_client(Window win);
void remove_client(Window win);
void hide_workspace(Dael_Workspace* ws);
void show_workspace(Dael_Workspace* ws);

void grab_keys();
void set_window_border(Dael_Client* client);
void handle_event(XEvent* e);
void handle_key_press(XEvent* e);
void handle_map_request(XEvent* e);

void Dael_State_init(Dael_State* state);
void Dael_State_free(Dael_State* state);

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
        Dael_State_init(&wm);
        grab_keys(&wm);
        XFlush(wm.dpy);
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


Dael_Client* add_client(Window win)
{
        Dael_Client* new_c = malloc(sizeof(Dael_Client));
        Dael_Client* last;

        if (!wm.current_workspace) {
                append_workspace(NULL);
        }
        new_c->win = win;
        new_c->x = 0;
        new_c->y = 0;
        new_c->w = 800;
        new_c->h = 600;
        new_c->is_fullscreen = false;
        new_c->next = NULL;
        new_c->prev = NULL;

        last = wm.current_workspace->clients;
        if (!last) {
                wm.current_workspace->clients = new_c;
        }
        else {
                while (last->next)
                        last = last->next;
                last->next = new_c;
                new_c->prev = last;
        }
        set_window_border(new_c);
        return new_c;
}


void remove_client(Window win)
{
        Dael_Workspace* ws = wm.current_workspace;
        Dael_Client* c;
        if (!ws)
                return;
        c = ws->clients;
        while (c) {
                if (c->win == win) {
                        if (c->prev) {
                                c->prev->next = c->next;
                        }
                        else {
                                ws->clients = c->next;
                        }
                        if (c->next) {
                                c->next->prev = c->prev;
                        }
                        free(c);
                        return;
                }
                c = c->next;
        }
}


void append_workspace(const char* args)
{
        Dael_Workspace* new_ws = malloc(sizeof(Dael_Workspace));
        new_ws->id = (wm.current_workspace) ? wm.current_workspace->id + 1 : 1;
        new_ws->clients = NULL;
        new_ws->next = NULL;
        new_ws->prev = NULL;

        if (!wm.current_workspace) {
                wm.current_workspace = new_ws;
        }
        else {
                Dael_Workspace* last = wm.current_workspace;
                while (last->next)
                        last = last->next;
                last->next = new_ws;
                new_ws->prev = last;
        }

        wm.current_workspace = new_ws;
}


void next_workspace(const char* args)
{
        if (wm.current_workspace && wm.current_workspace->next) {
                hide_workspace(wm.current_workspace);
                wm.current_workspace = wm.current_workspace->next;
                show_workspace(wm.current_workspace);
        }
}


void prev_workspace(const char* args)
{
        if (wm.current_workspace && wm.current_workspace->prev) {
                hide_workspace(wm.current_workspace);
                wm.current_workspace = wm.current_workspace->prev;
                show_workspace(wm.current_workspace);
        }
}


void hide_workspace(Dael_Workspace* ws)
{
        Dael_Client* client;
        if (!ws)
                return;
        client = ws->clients;
        while (client) {
                XUnmapWindow(wm.dpy, client->win);
                client = client->next;
        }
}

void show_workspace(Dael_Workspace* ws)
{
        Dael_Client* client;
        if (!ws)
                return;
        client = ws->clients;
        while (client) {
                XMapWindow(wm.dpy, client->win);
                client = client->next;
        }
}


void Dael_State_init(Dael_State* state)
{
        if (!(state->dpy = XOpenDisplay(NULL))) {
                fprintf(stderr, "Failed to open display.\n");
                exit(1);
        }
        state->root = DefaultRootWindow(state->dpy);

        XSelectInput(
                state->dpy, state->root,
                SubstructureRedirectMask |
                SubstructureNotifyMask |
                PropertyChangeMask |
                ButtonPressMask |
                ButtonReleaseMask |
                PointerMotionMask
        );

        state->curs.normal = XCreateFontCursor(state->dpy, XC_left_ptr);
        state->curs.grab = XCreateFontCursor(state->dpy, XC_fleur);
        state->curs.resize = XCreateFontCursor(state->dpy, XC_sizing);
        state->curs.plus = XCreateFontCursor(state->dpy, XC_plus);
        XDefineCursor(state->dpy, state->root, state->curs.normal);

        state->font = XLoadQueryFont(state->dpy, FONT);
        if (!state->font) {
                fprintf(stderr, "Failed to load font\n");
        }
}


void Dael_State_free(Dael_State* state)
{
        XFreeFont(state->dpy, state->font);
        XFreeCursor(state->dpy, state->curs.normal);
        XFreeCursor(state->dpy, state->curs.resize);
        XFreeCursor(state->dpy, state->curs.grab);
        XFreeCursor(state->dpy, state->curs.plus);
        XCloseDisplay(state->dpy);
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


void set_window_border(Dael_Client* client)
{
        unsigned long color;
        if (!client || !wm.current_workspace)
                return;
        color = (wm.current_workspace->focused == client)
                ? BORDER_FOCUSED : BORDER_UNFOCUSED;
        XSetWindowBorderWidth(wm.dpy, client->win, BORDER_SIZE);
        XSetWindowBorder(wm.dpy, client->win, color);
}


void apply_layout(void)
{
        int screen_w;
        int screen_h;
        int mx = 0;
        int my = 0;
        int mh = 0;
        int mw = 0;
        Dael_Client* m;
        Dael_Client* client;
        int num_slaves = 0;

        if (!wm.current_workspace || !wm.current_workspace->clients)
                return;

        screen_w = DisplayWidth(wm.dpy, DefaultScreen(wm.dpy));
        screen_h = DisplayHeight(wm.dpy, DefaultScreen(wm.dpy));
        m = wm.current_workspace->clients;
        client = m->next;

        while (client) {
                num_slaves ++;
                client = client->next;
        }

        /* master window takes left half of display */
        mw = screen_w / 2 - BORDER_SIZE * 2;
        mh = screen_h - BORDER_SIZE * 2;

        set_window_border(m);
        XMoveResizeWindow(wm.dpy, m->win, mx, my, mw, mh);

        if (num_slaves > 0) {
                int cw = screen_w / 2 - BORDER_SIZE * 2;
                int ch = screen_h / num_slaves - BORDER_SIZE * 2;
                int cx = screen_w / 2;
                int cy = 0;
                client = m->next;

                while (client) {
                        set_window_border(client);
                        XMoveResizeWindow(wm.dpy, client->win, cx, cy, cw, ch);
                        cy += ch;
                        client = client->next;
                }
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


void focus_next(const char* args)
{
        Dael_Client* client;
        Dael_Client* prev_focused;
        (void) args;

        if (!wm.current_workspace || !wm.current_workspace->clients)
                return;

        prev_focused = wm.current_workspace->focused;
        client = prev_focused->next;

        if (client) {
                wm.current_workspace->focused = client;
        }
        else {
                wm.current_workspace->focused = wm.current_workspace->clients;
        }

        set_window_border(prev_focused);
        set_window_border(wm.current_workspace->focused);

        XRaiseWindow(wm.dpy, wm.current_workspace->focused->win);
}


void focus_prev(const char* args)
{
        Dael_Client* client;
        Dael_Client* prev = NULL;
        (void) args;
        if (!wm.current_workspace || !wm.current_workspace->clients)
                return;

        while (client) {
                if (client == wm.current_workspace->focused) {
                        wm.current_workspace->focused = prev ? prev : client;
                        XRaiseWindow(wm.dpy, wm.current_workspace->focused->win);
                        return;
                }
                prev = client;
                client = client->next;
        }
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
        Dael_Client* client = add_client(req->window);
        XMapWindow(wm.dpy, req->window);

        wm.current_workspace->focused = client;
        XRaiseWindow(wm.dpy, client->win);

        apply_layout();
}


int xerror_handler(Display* display, XErrorEvent* error)
{
        char error_msg[120];
        XGetErrorText(display, error->error_code, error_msg, sizeof(error_msg));
        fprintf(stderr, "X Error: %s\n", error_msg);
        /*exit(1);*/
        return 0;
}

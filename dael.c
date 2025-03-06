/*
 * file: dael.c
 * ------------
 * Author: Mason Armand
 * Date Created: Feb 27, 2025
 * Last Modified: Feb 27, 2025
 */

/*
 * TODO
 * floating
 *
 * Handlers:
 * - ConfigureRequest
 * - ConfigureNotify
 * - EnterNotify
 * - Expose
 * - FocusIn
 * - MappingNotify
 * - PropertyNotify
 * - UnmapNotify
 *
 * Floating if:
 * - NetWMTypeDialog
 */
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum {
        NORMAL,
        MONOCLE,

        MODE_COUNT /* not a tiling mode, just designates size of enum */
} Dael_TilingMode;

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
        bool is_fullscreen;
        bool is_floating;
        Dael_Client* next;
        Dael_Client* prev;
};

struct Dael_Workspace {
        unsigned int id;
        unsigned int master_size;
        Dael_TilingMode mode;
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
        Dael_Workspace* workspaces;
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

/* handy macro from dwm */
#define CLEANMASK(mask) (mask & ~(numlockmask | LockMask))

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* config.h / user key-bindable functions */
void launch_program(const char* program);
void quit(const char* args);
void swap_master(const char* args);
void focus_next(const char* args);
void focus_prev(const char* args);
void increase_size(const char* args);
void decrease_size(const char* args);
void append_workspace(const char* args);
void next_workspace(const char* args);
void prev_workspace(const char* args);
void cycle_tiling_mode(const char* args);
void kill_window(const char* args);


#include "config.h"

void update_numlockmask(void);
Dael_Client* add_client(Window win);
void remove_client(Dael_Workspace* ws, Dael_Client* c);
Dael_Client* get_client(Window win);
void hide_workspace(Dael_Workspace* ws);
void show_workspace(Dael_Workspace* ws);
Dael_Workspace* get_workspace_for_client(Dael_Client* client);
void grab_change_cursor(Window window, Cursor cursor);

void get_screen_center(int* x, int* y);
void change_master_size(int amount);
void apply_layout(void);
void manage_floating_windows(void);
void tile_normal(int w, int h);
void tile_monocle(int w, int h);
void grab_keys(void);
int send_event(Dael_Client* c, Atom proto);
Atom get_window_atom_property(Dael_Client* c, Atom prop);
void set_window_focus(Dael_Client* client);
void update_window_type(Dael_Client* c);
void set_window_border(Dael_Client* client);
void remove_window_border(Dael_Client* client);
void handle_event(XEvent* e);
void handle_property_notify(XEvent* e);
void handle_configure_request(XEvent *e);
void handle_key_press(XEvent* e);
void handle_map_request(XEvent* e);
void handle_destroy_notify(XEvent* e);

void Dael_State_init(Dael_State* state);
void Dael_State_free(Dael_State* state);

int xerror_handler(Display* display, XErrorEvent* error);
int xerror_ignore(Display* display, XErrorEvent* error);
void die(const char* e);

/* XEvent handler functions */
Dael_EventHandler event_handlers[] = {
    { KeyPress, handle_key_press },
    { MapRequest, handle_map_request },
    { PropertyNotify, handle_property_notify },
    { DestroyNotify, handle_destroy_notify },
    { ConfigureRequest, handle_configure_request },
    { 0, NULL }
};

/* global window manager state */
Dael_State wm = { 0 };
unsigned int numlockmask;


int main(void)
{
        struct sigaction sa;
        XSetErrorHandler(xerror_handler);
        Dael_State_init(&wm);
        grab_keys();
        XFlush(wm.dpy);
        XSync(wm.dpy, False);
        wm.running = true;

        update_numlockmask();

        /* zombie handling from dwm */
        /* do not transform children into zombies */
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
        sa.sa_handler = SIG_IGN;
        sigaction(SIGCHLD, &sa, NULL);

        /* cleanup zombies */
        while (waitpid(-1, NULL, WNOHANG) > 0);


        while (wm.running) {
                XEvent e;
                XNextEvent(wm.dpy, &e);
                handle_event(&e);
        }

        Dael_State_free(&wm);
        return 0;
}


void update_numlockmask(void)
{
        XModifierKeymap* modmap = XGetModifierMapping(wm.dpy);
        KeyCode numlock_keycode = XKeysymToKeycode(wm.dpy, XK_Num_Lock);
        unsigned int k;
        int j;

        numlockmask = 0;

        for (k = 0; k < 8; k++) {
                for (j = 0; j < modmap->max_keypermod; j++) {
                        unsigned int index = modmap->max_keypermod * k + j;
                        KeyCode keycode = modmap->modifiermap[index];

                        if (keycode == numlock_keycode) {
                                numlockmask = (1 << k);
                        }
                }
        }

        XFreeModifiermap(modmap);
}


Dael_Client* add_client(Window win)
{
        Dael_Client* new_c = malloc(sizeof(Dael_Client));
        Dael_Client* last;

        if (!wm.current_workspace) {
                append_workspace(NULL);
                wm.workspaces = wm.current_workspace;
        }
        new_c->win = win;
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
        /*set_window_border(new_c);*/
        return new_c;
}


void remove_client(Dael_Workspace* ws, Dael_Client* c)
{
        if (!ws || !c)
                return;

        if (c->prev)
                c->prev->next = c->next;
        if (c->next)
                c->next->prev = c->prev;

        if (ws->clients == c)
                ws->clients = c->next;

        if (ws->focused == c) {
                ws->focused = (c->next) ? c->next : c->prev;
                if (ws->focused)
                        set_window_focus(ws->focused);
        }

        free(c);
}


Dael_Client* get_client(Window win)
{
        Dael_Client* c;
        Dael_Workspace* ws;
        for (ws = wm.workspaces; ws; ws = ws->next)
                for (c = ws->clients; c; c = c->next)
                        if (c->win == win)
                                return c;
        return NULL;
}


void append_workspace(const char* args)
{
        Dael_Workspace* new_ws = malloc(sizeof(Dael_Workspace));
        new_ws->id = (wm.current_workspace) ? wm.current_workspace->id + 1 : 1;
        new_ws->clients = NULL;
        new_ws->next = NULL;
        new_ws->prev = NULL;
        new_ws->mode = NORMAL;
        new_ws->master_size = MASTER_DEFAULT;

        (void) args;

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

        next_workspace("");
}


void next_workspace(const char* args)
{
        (void) args;
        if (wm.current_workspace && wm.current_workspace->next) {
                hide_workspace(wm.current_workspace);
                wm.current_workspace = wm.current_workspace->next;
                show_workspace(wm.current_workspace);
        }
}


void prev_workspace(const char* args)
{
        (void) args;
        if (wm.current_workspace && wm.current_workspace->prev) {
                hide_workspace(wm.current_workspace);
                wm.current_workspace = wm.current_workspace->prev;
                show_workspace(wm.current_workspace);
        }
}


void cycle_tiling_mode(const char* args)
{
        Dael_TilingMode cur = wm.current_workspace->mode;
        (void) args;

        wm.current_workspace->mode = (cur + 1) % MODE_COUNT;
        apply_layout();
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


Dael_Workspace* get_workspace_for_client(Dael_Client* client)
{
        Dael_Workspace* ws = wm.workspaces;

        while (ws) {
                Dael_Client* c = ws->clients;
                while (c) {
                        if (c == client) {
                                return ws;
                        }
                        c = c->next;
                }
                ws = ws->next;
        }
        return NULL;
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


void grab_keys(void)
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


void remove_window_border(Dael_Client* client)
{
        if (!client)
                return;
        XSetWindowBorderWidth(wm.dpy, client->win, 0);
        XFlush(wm.dpy);
}


void apply_layout(void)
{
        int screen_w = DisplayWidth(wm.dpy, DefaultScreen(wm.dpy));
        int screen_h = DisplayHeight(wm.dpy, DefaultScreen(wm.dpy));

        if (!wm.current_workspace || !wm.current_workspace->clients)
                return;

        switch (wm.current_workspace->mode) {
        case NORMAL:
                tile_normal(screen_w, screen_h);
                break;
        case MONOCLE:
                tile_monocle(screen_w, screen_h);
                break;
        default:
                tile_normal(screen_w, screen_h);
        }

        manage_floating_windows();
}

/* for dialog windows */
void manage_floating_windows(void)
{
        Dael_Client* client = wm.current_workspace->clients;
        while (client) {
                int x;
                int y;
                unsigned int w = 640;
                unsigned int h = 480;

                if (!client->is_floating) {
                        client = client->next;
                        continue;
                }

                get_screen_center(&x, &y);
                x -= w / 2;
                y -= h / 2;

                XMoveResizeWindow(wm.dpy, client->win, x, y, w, h);
                XRaiseWindow(wm.dpy, client->win);
                set_window_border(client);

                client = client->next;
        }
}


void tile_normal(int w, int h)
{
        int mh = 0;
        int mw = 0;
        Dael_Client* m;
        Dael_Client* client;
        int num_slaves = 0;
        int msize = wm.current_workspace->master_size;

        m = wm.current_workspace->clients;
        client = m->next;

        while (client) {
                if (!client->is_floating)
                        num_slaves++;
                client = client->next;
        }

        if (num_slaves == 0) {
                tile_monocle(w, h);
                return;
        }

        /* master window takes left half of display */
        mw = (w * msize) / 100;
        mh = h;
        mw -= BORDER_SIZE * 2;
        mh -= BORDER_SIZE * 2;

        set_window_border(m);
        XMoveResizeWindow(wm.dpy, m->win, 0, 0, mw, mh);

        if (num_slaves > 0) {
                int cw = (w - mw) - BORDER_SIZE * 2;
                int total_height = h - (num_slaves * BORDER_SIZE * 2);
                int ch = total_height / num_slaves;
                int extra_space = total_height % num_slaves;
                int cx = mw;
                int cy = 0;
                client = m->next;

                while (client) {
                        int height = ch;

                        if (client->is_floating) {
                                client = client->next;
                                continue;
                        }

                        set_window_border(client);

                        if (extra_space > 0) {
                                height += 1;
                                extra_space--;
                        }
                        XMoveResizeWindow(wm.dpy, client->win, cx, cy, cw, height);
                        cy += height + (BORDER_SIZE * 2);
                        client = client->next;
                }
        }
}


/* in monocle mode, the focused window is the master window
 * and fills the entirety of the screen */
void tile_monocle(int w, int h)
{
        Dael_Client* c = wm.current_workspace->clients;

        while (c) {
                if (!c->is_floating) {
                        remove_window_border(c);
                        XMoveResizeWindow(wm.dpy, c->win, 0, 0, w, h);
                }
                c = c->next;
        }
}


/* adopted from dwm.c from suckless */
int send_event(Dael_Client* c, Atom proto)
{
        int n;
        Atom* protocols;
        Atom wm_protocols = XInternAtom(wm.dpy, "WM_PROTOCOLS", False);
        int exists = 0;
        XEvent e;

        if (XGetWMProtocols(wm.dpy, c->win, &protocols, &n)) {
                while (!exists && n--)
                        exists = protocols[n] == proto;
                XFree(protocols);
        }
        if (exists) {
                e.type = ClientMessage;
                e.xclient.window = c->win;
                e.xclient.message_type = wm_protocols;
                e.xclient.format = 32;
                e.xclient.data.l[0] = proto;
		e.xclient.data.l[1] = CurrentTime;
                XSendEvent(wm.dpy, c->win, False, NoEventMask, &e);
        }
        return exists;
}


Atom get_window_atom_property(Dael_Client* c, Atom prop)
{
        int dummy_i;
        unsigned long dummy_l;
        unsigned char* p = NULL;
        Atom dummy_a;
        Atom atom = None;

        if (XGetWindowProperty(
                wm.dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
                &dummy_a, &dummy_i, &dummy_l, &dummy_l, &p) == Success && p) {
                atom = *(Atom *)p;
                XFree(p);
        }
        return atom;
}


void kill_window(const char* args)
{
        Atom wm_delete = XInternAtom(wm.dpy, "WM_DELETE_WINDOW", False);
        Dael_Client* client = wm.current_workspace->focused;
        (void) args;
        if (!send_event(client, wm_delete)) {
                XGrabServer(wm.dpy);
                XSetErrorHandler(xerror_ignore);
                XSetCloseDownMode(wm.dpy, DestroyAll);
                XKillClient(wm.dpy, client->win);
                XSync(wm.dpy, False);
                XSetErrorHandler(xerror_handler);
                XUngrabServer(wm.dpy);
        }
}


void launch_program(const char* program)
{
        struct sigaction sa;
        pid_t pid = fork();

        if (pid == 0) {
                char* argv[] = {NULL, NULL};

                if (wm.dpy)
			close(ConnectionNumber(wm.dpy));
		setsid();

		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;
		sigaction(SIGCHLD, &sa, NULL);

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


void swap_master(const char* args)
{
        Window tmp;
        Dael_Client* master = wm.current_workspace->clients;
        Dael_Client* focused = wm.current_workspace->focused;
        (void) args;

        if (!master || !focused || focused == master)
                return;

        tmp = master->win;
        master->win = focused->win;
        focused->win = tmp;

        set_window_focus(wm.current_workspace->focused);
        apply_layout();
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

        set_window_focus(wm.current_workspace->focused);
        apply_layout();
}


void focus_prev(const char* args)
{
        Dael_Client* prev_focused;
        (void) args;

        if (!wm.current_workspace || !wm.current_workspace->clients)
                return;

        prev_focused = wm.current_workspace->focused;

        if (prev_focused->prev) {
                wm.current_workspace->focused = prev_focused->prev;
        } else {
                wm.current_workspace->focused = wm.current_workspace->clients;
                while (wm.current_workspace->focused->next) {
                        wm.current_workspace->focused = wm.current_workspace->focused->next;
                }
        }

        set_window_border(prev_focused);
        set_window_border(wm.current_workspace->focused);

        set_window_focus(wm.current_workspace->focused);
        apply_layout();
}


void increase_size(const char* args)
{
        (void) args;
        change_master_size(SIZE_INCREMENT);
}


void decrease_size(const char* args)
{
        (void) args;
        change_master_size(-SIZE_INCREMENT);
}


void get_screen_center(int* x, int* y)
{
        int screen_w = DisplayWidth(wm.dpy, DefaultScreen(wm.dpy));
        int screen_h = DisplayHeight(wm.dpy, DefaultScreen(wm.dpy));
        *x = screen_w / 2;
        *y = screen_h / 2;
}


void change_master_size(int amount)
{
        int new_size;
        int clamped_size;

        if (wm.current_workspace->mode == MONOCLE)
                return;

        new_size = wm.current_workspace->master_size + amount;
        clamped_size = MIN(MASTER_MAX, MAX(MASTER_MIN, new_size));
        wm.current_workspace->master_size = clamped_size;

        apply_layout();
}


void set_window_focus(Dael_Client* client)
{
        XSetInputFocus(wm.dpy, client->win, RevertToPointerRoot, CurrentTime);
        XRaiseWindow(wm.dpy, client->win);
}


void update_window_type(Dael_Client* c)
{
        Atom type = XInternAtom(wm.dpy, "_NET_WM_WINDOW_TYPE", False);
        Atom dialog = XInternAtom(wm.dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
        Atom atom = get_window_atom_property(c, type);
        c->is_floating = (atom == dialog);
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


void handle_property_notify(XEvent* e)
{
        Dael_Client* c;
        XPropertyEvent* ev = &e->xproperty;
        Atom type = XInternAtom(wm.dpy, "_NET_WM_WINDOW_TYPE", False);

        if ((c = get_client(ev->window))) {
                if (ev->atom == type)
                        update_window_type(c);
        }
        printf("propnotif\n");
}


void handle_configure_request(XEvent* e)
{
        (void) e;
        /*
        XConfigureRequestEvent *ev = &e->xconfigurerequest;
        XWindowChanges wc;
        Dael_Client* c = get_client(ev->window);

        if (!c || !c->is_floating)
                return;

        wc.x = ev->x;
        wc.y = ev->y;
        wc.width = ev->width;
        wc.height = ev->height;
        wc.border_width = ev->border_width;
        wc.sibling = ev->above;
        wc.stack_mode = ev->detail;
        XConfigureWindow(wm.dpy, ev->window, ev->value_mask, &wc);
        */
}


void handle_key_press(XEvent* e)
{
        unsigned int i = 0;
        XKeyEvent k = e->xkey;
        unsigned int modmask = CLEANMASK(k.state);

        while (config_keys[i].key_sym != NoSymbol) {
                if ((modmask == config_keys[i].mod)
                && (k.keycode == XKeysymToKeycode(wm.dpy, config_keys[i].key_sym))
                && (config_keys[i].func)) {
                        config_keys[i].func(config_keys[i].arg);

                }
                i++;
        }
}


void handle_map_request(XEvent* e)
{
        static XWindowAttributes wa;
        XMapRequestEvent* req = &e->xmaprequest;
        Dael_Client* client;

	if (!XGetWindowAttributes(wm.dpy, req->window, &wa) || wa.override_redirect)
		return;

        if (get_client(req->window))
                return;

        client = add_client(req->window);
        XMapWindow(wm.dpy, req->window);
        update_window_type(client);

        wm.current_workspace->focused = client;
        set_window_focus(client);

        apply_layout();
}


void handle_destroy_notify(XEvent* e)
{
        Dael_Client* c;
        XDestroyWindowEvent* ev = &e->xdestroywindow;
        if ((c = get_client(ev->window))) {
                Dael_Workspace* ws;
                if ((ws = get_workspace_for_client(c))) {
                        remove_client(ws, c);
                }
                apply_layout();
        }
}


int xerror_handler(Display* display, XErrorEvent* error)
{
        char error_msg[120];
        XGetErrorText(display, error->error_code, error_msg, sizeof(error_msg));
        fprintf(stderr, "X Error: %s\n", error_msg);
        /*exit(1);*/
        return 0;
}


int xerror_ignore(Display* display, XErrorEvent* error)
{
        (void) display;
        (void) error;
        return 0;
}


void die(const char* e)
{
        fprintf(stdout,"dael: %s\n",e);
        exit(1);
}


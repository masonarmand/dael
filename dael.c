/*
 * file: dael.c
 * ------------
 * Author: Mason Armand
 * Date Created: Feb 27, 2025
 * Last Modified: Feb 27, 2025
 */
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

typedef struct {
        unsigned int temp;
} Dael_Config;

typedef struct {
        Dael_Config conf;
        Window root;
        Display* dpy;
} Dael_State;

typedef struct Dael_Client {
        Window win;
        struct Dael_Client* prev;
        struct Dael_Client* next;
} Dael_Client;

typedef struct {
        unsigned int mod;
        KeySym key_sym;
        void (*func)(const char*);
        const void* arg;
} Dael_Keybinding;

void grab_keys(Dael_State* state);
void launch_program(const char* program);
void handle_key_press(Dael_State* state, XEvent* e);

#include "config.h"


int main(void)
{
        return 0;
}


void grab_keys(Dael_State* state)
{
        unsigned int i = 0;
        XUngrabKey(state->dpy, AnyKey, AnyModifier, state->root);

        while (keys[i].key_sym != NoSymbol) {
                KeySym key_sym = keys[i].key_sym;
                KeyCode keycode = XKeysymToKeycode(state->dpy, key_sym);
                unsigned int mod = keys[i].mod;

                XGrabKey(state->dpy, keycode, mod, state->root, True,  GrabModeAsync, GrabModeAsync);
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


void handle_key_press(Dael_State* state, XEvent* e)
{
        /*
        unsigned int i;
        XKeyEvent* key_ev = &e->xkey;
        KeySym key_sym = XKeycodeToKeysym(state->dpy, (KeyCode)key_ev->keycode, 0);

        while (keys[i].key_sym != NoSymbol) {
                if (key_sym == keys[i].key_sym)
                i++;
        }*/
}

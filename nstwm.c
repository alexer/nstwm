/* NotSoTinyWM is written by Aleksi Torhamo <aleksi@torhamo.net>.
 * It is based on TinyWM, which is written by Nick Welch <nick@incise.org>.
 *
 * This software is in the public domain and is provided AS IS, with NO WARRANTY. */

#include <X11/Xlib.h>
#include "util.h"
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void decorate(Display *dpy, XAssocTable *windows, Window win)
{
    XWindowAttributes attr;
    XSetWindowAttributes values;
    XWindowChanges changes;
    Window frame;
    Window *data;

    values.background_pixel = WhitePixel(dpy, DefaultScreen(dpy));
    values.event_mask = ButtonPressMask|SubstructureNotifyMask;

    changes.sibling = win;
    changes.stack_mode = Above;

    XAddToSaveSet(dpy, win);
    XSetWindowBorderWidth(dpy, win, 0);
    XGetWindowAttributes(dpy, win, &attr);
    frame = XCreateWindow(dpy, DefaultRootWindow(dpy), attr.x - 5, attr.y - 25, attr.width + 10, attr.height + 30,
        0, CopyFromParent, CopyFromParent, CopyFromParent, CWBackPixel|CWEventMask, &values);
    XConfigureWindow(dpy, frame, CWSibling|CWStackMode, &changes);
    XReparentWindow(dpy, win, frame, 5, 25);
    XMapWindow(dpy, frame);

    data = (Window *)malloc(2 * sizeof(Window));
    data[0] = frame;
    data[1] = win;
    XMakeAssoc(dpy, windows, frame, (char *)data);
    XMakeAssoc(dpy, windows, win, (char *)data);
}

int main(void)
{
    Display * dpy;
    XSetWindowAttributes winattrs;
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;
    int xdiff, ydiff, width, height;
    Window junkwin;
    Window *children;
    unsigned int num_children, i;
    XAssocTable *windows;

    if(!(dpy = XOpenDisplay(0x0))) {
        return 1;
    }

    winattrs.event_mask = SubstructureNotifyMask;
    XChangeWindowAttributes(dpy, DefaultRootWindow(dpy), CWEventMask, &winattrs);

    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask,
        DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);

    windows = XCreateAssocTable(32);

    XQueryTree(dpy, DefaultRootWindow(dpy), &junkwin, &junkwin, &children, &num_children);
    for(i = 0; i < num_children; i++) {
        XGetWindowAttributes(dpy, children[i], &attr);
        if(attr.override_redirect == True || attr.map_state == IsUnmapped) {
            continue;
        }
        decorate(dpy, windows, children[i]);
    }
    XFree(children);

    for(;;) {
        XNextEvent(dpy, &ev);
        if(ev.type == KeyPress && ev.xkey.subwindow != None) {
            XRaiseWindow(dpy, ev.xkey.subwindow);
        } else if(ev.type == ButtonPress) {
            XGrabPointer(dpy, ev.xbutton.window, 1, PointerMotionMask|ButtonReleaseMask,
                GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
            XGetWindowAttributes(dpy, ev.xbutton.window, &attr);
            start = ev.xbutton;
        } else if(ev.type == MotionNotify) {
            while(XCheckTypedEvent(dpy, MotionNotify, &ev));
            xdiff = ev.xbutton.x_root - start.x_root;
            ydiff = ev.xbutton.y_root - start.y_root;
            if(start.button == 1) {
                XMoveWindow(dpy, start.window,
                    attr.x + xdiff,
                    attr.y + ydiff
                );
            } else if(start.button == 3) {
                width = MAX(11, attr.width + xdiff);
                height = MAX(31, attr.height + ydiff);
                children = (Window *)XLookUpAssoc(dpy, windows, start.window);
                XResizeWindow(dpy, start.window, width, height);
                XResizeWindow(dpy, children[1], width - 10, height - 30);
            }
        } else if(ev.type == ButtonRelease) {
            XUngrabPointer(dpy, CurrentTime);
        } else if(ev.type == MapNotify) {
            children = (Window *)XLookUpAssoc(dpy, windows, ev.xmap.window);
            if(children == NULL) {
                decorate(dpy, windows, ev.xmap.window);
            } else if(ev.xmap.window == children[1]) {
                XMapWindow(dpy, children[0]);
            }
        } else if(ev.type == UnmapNotify) {
            children = (Window *)XLookUpAssoc(dpy, windows, ev.xunmap.window);
            if(children != NULL && ev.xunmap.window == children[1]) {
                XUnmapWindow(dpy, children[0]);
            }
        } else if(ev.type == DestroyNotify) {
            children = (Window *)XLookUpAssoc(dpy, windows, ev.xdestroywindow.window);
            if(children != NULL && ev.xdestroywindow.window == children[1]) {
                XDeleteAssoc(dpy, windows, children[0]);
                XDeleteAssoc(dpy, windows, children[1]);
                XDestroyWindow(dpy, children[0]);
                free(children);
            }
        }
    }
    XDestroyAssocTable(windows);
}


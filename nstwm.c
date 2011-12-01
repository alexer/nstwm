/* NotSoTinyWM is written by Aleksi Torhamo <aleksi@torhamo.net>.
 * It is based on TinyWM, which is written by Nick Welch <nick@incise.org>.
 *
 * This software is in the public domain and is provided AS IS, with NO WARRANTY. */

#include <X11/Xlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void decorate(Display *dpy, Window win)
{
    XWindowAttributes attr;
    XSetWindowAttributes values;
    XWindowChanges changes;
    Window frame;

    values.background_pixel = WhitePixel(dpy, DefaultScreen(dpy));
    values.event_mask = ButtonPressMask|ButtonReleaseMask;

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
}

int main(void)
{
    Display * dpy;
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;
    int xdiff, ydiff;
    Window junkwin;
    Window *children;
    unsigned int num_children, i;

    if(!(dpy = XOpenDisplay(0x0))) {
        return 1;
    }

    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask,
        DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);

    XQueryTree(dpy, DefaultRootWindow(dpy), &junkwin, &junkwin, &children, &num_children);
    for(i = 0; i < num_children; i++) {
        XGetWindowAttributes(dpy, children[i], &attr);
        if(attr.override_redirect == True || attr.map_state == IsUnmapped) {
            continue;
        }
        decorate(dpy, children[i]);
    }
    XFree(children);

    for(;;) {
        XNextEvent(dpy, &ev);
        if(ev.type == KeyPress && ev.xkey.subwindow != None) {
            XRaiseWindow(dpy, ev.xkey.subwindow);
        } else if(ev.type == ButtonPress) {
            XGrabPointer(dpy, ev.xbutton.window, 1, PointerMotionMask,
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
                XResizeWindow(dpy, start.window,
                    MAX(1, attr.width + xdiff),
                    MAX(1, attr.height + ydiff)
                );
            }
        } else if(ev.type == ButtonRelease) {
            XUngrabPointer(dpy, CurrentTime);
        }
    }
}


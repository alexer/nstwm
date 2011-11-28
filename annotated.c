/* NotSoTinyWM is written by Aleksi Torhamo <aleksi@torhamo.net>.
 * It is based on TinyWM, which is written by Nick Welch <nick@incise.org>.
 *
 * This software is in the public domain and is provided AS IS, with NO WARRANTY. */

/* this is a commented version of nstwm.c, since uncommented code is much
 * easier to edit (when you're familiar with the code).
 */

/* most X stuff will be included with Xlib.h, but a few things require other
 * headers, like Xmd.h, keysym.h, etc.
 */
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
    values.event_mask = ButtonPressMask;

    changes.sibling = win;
    changes.stack_mode = Above;

    /* normally windows are destroyed if their parent is destroyed. since we
     * reparent windows to out decorations, they would normally be destroyed
     * when the window manager dies. adding windows to our save-set prevents
     * them being destroyed when we die.
     */
    XAddToSaveSet(dpy, win);
    /* remove any borders the window might have, since it's gonna get decorations now..
     * XXX: I don't know if the X11 borders are purely a visual thing, of if
     * they can actually be used for something..
     */
    XSetWindowBorderWidth(dpy, win, 0);
    XGetWindowAttributes(dpy, win, &attr);
    /* create the window, 5px borders plus a 20px bar at the top. setting the
     * background pixel means we don't have to do any drawing and stuff ourselves.
     * we set the event mask for the window so that we get clicks only from the
     * frame, clicks to the window still go to the application as they're supposed to.
     */
    frame = XCreateWindow(dpy, DefaultRootWindow(dpy), attr.x - 5, attr.y - 25, attr.width + 10, attr.height + 30,
        0, CopyFromParent, CopyFromParent, CopyFromParent, CWBackPixel|CWEventMask, &values);
    /* make sure window is over the frame. XXX: is this really needed? */
    XConfigureWindow(dpy, frame, CWSibling|CWStackMode, &changes);
    /* reparent the window to our decorations, take borders and top bar into
     * account for the position.
     */
    XReparentWindow(dpy, win, frame, 5, 25);
    /* make our frame (and the window) visible. */
    XMapWindow(dpy, frame);

    /* keep track which frame contains which window */
    data = (Window *)malloc(sizeof(Window));
    *data = win;
    XMakeAssoc(dpy, windows, frame, (char *)data);
}

int main(void)
{
    Display * dpy;
    XWindowAttributes attr;

    /* we use this to save the pointer's state at the beginning of the
     * move/resize.
     */
    XButtonEvent start;

    XEvent ev;
    int xdiff, ydiff, width, height;
    Window junkwin;
    Window *children;
    unsigned int num_children, i;
    XAssocTable *windows;

    /* return failure status if we can't connect */
    if(!(dpy = XOpenDisplay(0x0))) {
        return 1;
    }

    /* we use DefaultRootWindow to get the root window, which is a somewhat
     * naive approach that will only work on the default screen.  most people
     * only have one screen, but not everyone.  if you run multi-head without
     * xinerama then you quite possibly have multiple screens. (i'm not sure
     * about vendor-specific implementations, like nvidia's)
     *
     * many, probably most window managers only handle one screen, so in
     * reality this isn't really *that* naive.
     *
     * if you wanted to get the root window of a specific screen you'd use
     * RootWindow(), but the user can also control which screen is our default:
     * if they set $DISPLAY to ":0.foo", then our default screen number is
     * whatever they specify "foo" as.
     */

    /* you could also include keysym.h and use the XK_F1 constant instead of
     * the call to XStringToKeysym, but this method is more "dynamic."  imagine
     * you have config files which specify key bindings.  instead of parsing
     * the key names and having a huge table or whatever to map strings to XK_*
     * constants, you can just take the user-specified string and hand it off
     * to XStringToKeysym.  XStringToKeysym will give you back the appropriate
     * keysym or tell you if it's an invalid key name.
     *
     * a keysym is basically a platform-independent numeric representation of a
     * key, like "F1", "a", "b", "L", "5", "Shift", etc.  a keycode is a
     * numeric representation of a key on the keyboard sent by the keyboard
     * driver (or something along those lines -- i'm no hardware/driver expert)
     * to X.  so we never want to hard-code keycodes, because they can and will
     * differ between systems.
     */
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask,
        DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);

    /* initialize hashtable for keeping track of windows */
    windows = XCreateAssocTable(32);

    /* in the next loop, we add decorations to existing windows */

    /* get all children of the root window */
    XQueryTree(dpy, DefaultRootWindow(dpy), &junkwin, &junkwin, &children, &num_children);
    for(i = 0; i < num_children; i++) {
        /* skip adding decorations to this window if it's override_redirect
         * or unmapped.
         * XXX: why these two conditions, i have no idea. every window manager
         * seems to use something along these lines. somebodyplease explain. :(
         * apparently, override_redirect is meant for "temporary pop-up windows
         * that should not be reparented or affected by the window manager's
         * layout policy", whatever that means. something tells me i have never
         * seen such a window, ever..
         */
        XGetWindowAttributes(dpy, children[i], &attr);
        if(attr.override_redirect == True || attr.map_state == IsUnmapped) {
            continue;
        }
        /* the window seems sane, add decorations to it */
        decorate(dpy, windows, children[i]);
    }
    XFree(children);

    for(;;) {
        /* this is the most basic way of looping through X events; you can be
         * more flexible by using XPending(), or ConnectionNumber() along with
         * select() (or poll() or whatever floats your boat).
         */
        XNextEvent(dpy, &ev);

        /* this is our keybinding for raising windows.  as i saw someone
         * mention on the ratpoison wiki, it is pretty stupid; however, i
         * wanted to fit some sort of keyboard binding in here somewhere, and
         * this was the best fit for it.
         *
         * i was a little confused about .window vs. .subwindow for a while,
         * but a little RTFMing took care of that.  our passive grabs above
         * grabbed on the root window, so since we're only interested in events
         * for its child windows, we look at .subwindow.  when subwindow ==
         * None, that means that the window the event happened in was the same
         * window that was grabbed on -- in this case, the root window.
         */
        if(ev.type == KeyPress && ev.xkey.subwindow != None) {
            XRaiseWindow(dpy, ev.xkey.subwindow);
        } else if(ev.type == ButtonPress) {
            /* ask for motion events so we can track the drag. We ask for
             * button release events here instead of in the frame event mask
             * since otherwise we miss the button release if the cursor is
             * outside the window.
             */
            XGrabPointer(dpy, ev.xbutton.window, 1, PointerMotionMask|ButtonReleaseMask,
                GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
            /* we "remember" the position of the pointer at the beginning of
             * our move/resize, and the size/position of the window.  that way,
             * when the pointer moves, we can compare it to our initial data
             * and move/resize accordingly.
             */
            XGetWindowAttributes(dpy, ev.xbutton.window, &attr);
            start = ev.xbutton;
        /* we only get motion events when a button is being pressed,
         * but we still have to check that the drag started on a window */
        } else if(ev.type == MotionNotify) {
            /* here we "compress" motion notify events.
             *
             * if there are 10 of them waiting, it makes no sense to look at
             * any of them but the most recent.  in some cases -- if the window
             * is really big or things are just acting slowly in general --
             * failing to do this can result in a lot of "drag lag," especially
             * if your wm does a lot of drawing and whatnot that causes it to
             * lag.
             *
             * for window managers with things like desktop switching, it can
             * also be useful to compress EnterNotify events, so that you don't
             * get "focus flicker" as windows shuffle around underneath the
             * pointer.
             *
             * TODO: i think in theory we should stop if we see any button
             * events, since currently we can process them out of order.
             * i doubt it makes any difference in practice, however.
             */
            while(XCheckTypedEvent(dpy, MotionNotify, &ev));

            /* now we use the stuff we saved at the beginning of the
             * move/resize and compare it to the pointer's current position to
             * determine what the window's new size or position should be.
             *
             * if the initial button press was button 1, then we're moving.
             * otherwise it was 3 and we're resizing.
             *
             * we also make sure not to go negative with the window's
             * dimensions, resulting in "wrapping" which will make our window
             * something ridiculous like 65000 pixels wide (often accompanied
             * by lots of swapping and slowdown).
             *
             * even worse is if we get "lucky" and hit a width or height of
             * exactly zero, triggering an X error.  so we specify a minimum
             * width/height of 1 pixel.
             */
            xdiff = ev.xbutton.x_root - start.x_root;
            ydiff = ev.xbutton.y_root - start.y_root;
            if(start.button == 1) {
                XMoveWindow(dpy, start.window,
                    attr.x + xdiff,
                    attr.y + ydiff
                );
            } else if(start.button == 3) {
                /* do not resize frame or window below 1 pixel width or height */
                width = MAX(11, attr.width + xdiff);
                height = MAX(31, attr.height + ydiff);
                /* find out which window is associated with this frame */
                junkwin = *(Window *)XLookUpAssoc(dpy, windows, start.window);
                /* resize frame */
                XResizeWindow(dpy, start.window, width, height);
                /* resize window */
                XResizeWindow(dpy, junkwin, width - 10, height - 30);
            }
        } else if(ev.type == ButtonRelease) {
            /* stop receiving motion events */
            XUngrabPointer(dpy, CurrentTime);
        }
    }
    XDestroyAssocTable(windows);
}


# NotSoTinyWM is written by Aleksi Torhamo <aleksi@torhamo.net>.
# It is based on TinyWM, which is written by Nick Welch <nick@incise.org>.
#
# This software is in the public domain and is provided AS IS, with NO WARRANTY.

# This is a lightly (in comparison to annotated.c) commented version of nstwm.py.
# For more in-depth explanations, refer to annotated.c.
# Why make code changes once, when you could make them two.. I mean four times instead!

from Xlib.display import Display
from Xlib import X, XK

dpy = Display()
root = dpy.screen().root

event_mask = X.ButtonPressMask|X.ButtonReleaseMask|X.PointerMotionMask

# Grab the alt+F1 key combination on the root window, which means it'll only be reported to us
root.grab_key(dpy.keysym_to_keycode(XK.string_to_keysym("F1")),
    X.Mod1Mask, 1, X.GrabModeAsync, X.GrabModeAsync)
# Grab alt+left-click and alt+right-click, respectively. We ask to receive notifications when
# either combination is pressed or released, and when the mouse is moved while a button is kept pressed.
root.grab_button(1, X.Mod1Mask, 1, event_mask,
    X.GrabModeAsync, X.GrabModeAsync, X.NONE, X.NONE)
root.grab_button(3, X.Mod1Mask, 1, event_mask,
    X.GrabModeAsync, X.GrabModeAsync, X.NONE, X.NONE)

start = None
while 1:
    # Get the next event from the queue
    ev = dpy.next_event()

    # ev.window is the window we asked for events, and ev.child is the window that received the event.

    # In our case, if ev.child == X.NONE, the event happened on the root window,
    # since that's where we asked for events. We ignore those, since we're not
    # really interested on clicks on the root window. And raising the root window
    # wouldn't make sense either..

    # We have only asked key press events for one key, so we can be sure any
    # key press event is for alt+F1 without explicitly checking for it.
    if ev.type == X.KeyPress and ev.child != X.NONE:
        # Raise the window in question to the top
        ev.child.configure(stack_mode = X.Above)
    # We got a mouse button press, start dragging.
    elif ev.type == X.ButtonPress and ev.child != X.NONE:
        # Save the original position and size of the window
        attr = ev.child.get_geometry()
        # Save the start position of the drag
        start = ev
    # We have to check that start is not empty to make sure we've started
    # dragging on a real window, since we get events from the root window too.
    elif ev.type == X.MotionNotify and start:
        # See how the pointer has moved relative to the root window.
        xdiff = ev.root_x - start.root_x
        ydiff = ev.root_y - start.root_y
        # Move or resize the window depending on which button was pressed.
        if start.detail == 1:
            start.child.configure(
                x = attr.x + xdiff,
                y = attr.y + ydiff
            )
        elif start.detail == 3:
            start.child.configure(
                width = max(1, attr.width + xdiff),
                height = max(1, attr.height + ydiff)
            )
    # We got a mouse button release, stop dragging.
    elif ev.type == X.ButtonRelease:
        start = None


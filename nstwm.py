# NotSoTinyWM is written by Aleksi Torhamo <aleksi@torhamo.net>.
# It is based on TinyWM, which is written by Nick Welch <nick@incise.org>.
#
# This software is in the public domain and is provided AS IS, with NO WARRANTY.

from Xlib.display import Display
from Xlib import X, XK

dpy = Display()
root = dpy.screen().root

event_mask = X.ButtonPressMask|X.ButtonReleaseMask|X.PointerMotionMask

root.grab_key(dpy.keysym_to_keycode(XK.string_to_keysym("F1")),
    X.Mod1Mask, 1, X.GrabModeAsync, X.GrabModeAsync)
root.grab_button(1, X.Mod1Mask, 1, event_mask,
    X.GrabModeAsync, X.GrabModeAsync, X.NONE, X.NONE)
root.grab_button(3, X.Mod1Mask, 1, event_mask,
    X.GrabModeAsync, X.GrabModeAsync, X.NONE, X.NONE)

start = None
while 1:
    ev = dpy.next_event()
    if ev.type == X.KeyPress and ev.child != X.NONE:
        ev.child.configure(stack_mode = X.Above)
    elif ev.type == X.ButtonPress and ev.child != X.NONE:
        attr = ev.child.get_geometry()
        start = ev
    elif ev.type == X.MotionNotify and start:
        xdiff = ev.root_x - start.root_x
        ydiff = ev.root_y - start.root_y
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
    elif ev.type == X.ButtonRelease:
        start = None


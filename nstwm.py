# NotSoTinyWM is written by Aleksi Torhamo <aleksi@torhamo.net>.
# It is based on TinyWM, which is written by Nick Welch <nick@incise.org>.
#
# This software is in the public domain and is provided AS IS, with NO WARRANTY.

from Xlib.display import Display
from Xlib import X, XK
from util import *

windows = {}
def decorate(win):
    win.change_save_set(X.SetModeInsert)
    win.configure(border_width = 0)
    geom = win.get_geometry()
    frame = root.create_window(geom.x - 5, geom.y - 25, geom.width + 10, geom.height + 30,
        0, scr.root_depth, X.CopyFromParent, scr.root_visual, background_pixel = scr.white_pixel, event_mask = X.ButtonPressMask|X.SubstructureNotifyMask)
    frame.configure(sibling = win, stack_mode = X.Above)
    win.reparent(frame, 5, 25)
    frame.map()
    windows[frame] = windows[win] = (frame, win)

dpy = Display()
scr = dpy.screen()
root = scr.root

root.change_attributes(event_mask = X.SubstructureNotifyMask)
root.grab_key(dpy.keysym_to_keycode(XK.string_to_keysym("F1")),
    X.Mod1Mask, 1, X.GrabModeAsync, X.GrabModeAsync)

for win in root.query_tree().children:
    attr = win.get_attributes()
    if attr.override_redirect or attr.map_state == X.IsUnmapped:
        continue
    decorate(win)

while 1:
    ev = dpy.next_event()
    if ev.type == X.KeyPress and ev.child != X.NONE:
        ev.child.configure(stack_mode = X.Above)
    elif ev.type == X.ButtonPress:
        ev.window.grab_pointer(1, X.PointerMotionMask|X.ButtonReleaseMask,
            X.GrabModeAsync, X.GrabModeAsync, X.NONE, X.NONE, X.CurrentTime)
        attr = ev.window.get_geometry()
        start = ev
    elif ev.type == X.MotionNotify:
        ev = compress_motion(dpy, ev)
        xdiff = ev.root_x - start.root_x
        ydiff = ev.root_y - start.root_y
        if start.detail == 1:
            start.window.configure(
                x = attr.x + xdiff,
                y = attr.y + ydiff
            )
        elif start.detail == 3:
            width, height = max(11, attr.width + xdiff), max(31, attr.height + ydiff)
            start.window.configure(width = width, height = height)
            windows[start.window][1].configure(width = width - 10, height = height - 30)
    elif ev.type == X.ButtonRelease:
        dpy.ungrab_pointer(X.CurrentTime)
    elif ev.type == X.MapNotify:
        frame, win = windows.get(ev.window, (None, None))
        if frame is None:
            decorate(ev.window)
    elif ev.type == X.DestroyNotify:
        frame, win = windows.get(ev.window, (None, None))
        if ev.window == win:
            del windows[frame]
            del windows[win]
            frame.destroy()


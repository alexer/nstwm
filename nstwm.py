# NotSoTinyWM is written by Aleksi Torhamo <aleksi@torhamo.net>.
# It is based on TinyWM, which is written by Nick Welch <nick@incise.org>.
#
# This software is in the public domain and is provided AS IS, with NO WARRANTY.

from Xlib.display import Display
from Xlib import X, XK
from util import *

windows = {}
reparenting = set()
def decorate(win):
    attr = win.get_attributes()
    if attr.override_redirect or attr.map_state == X.IsUnmapped:
        return
    win.change_save_set(X.SetModeInsert)
    win.configure(border_width = 0)
    geom = win.get_geometry()
    depth = 32
    visual = match_visual_info(scr, depth, X.TrueColor).visual_id
    colormap = root.create_colormap(visual, X.AllocNone)
    frame = root.create_window(max(0, geom.x - 5), max(0, geom.y - 25), geom.width + 10, geom.height + 30,
        0, depth, X.CopyFromParent, visual, colormap = colormap, background_pixel = 0xffffffff, border_pixel = 0, event_mask = X.ButtonPressMask|X.SubstructureNotifyMask)
    frame.configure(sibling = win, stack_mode = X.Above)
    win.reparent(frame, 5, 25)
    frame.map()
    windows[frame] = windows[win] = (frame, win)
    reparenting.add(win)

dpy = Display()
scr = dpy.screen()
root = scr.root

root.change_attributes(event_mask = X.SubstructureNotifyMask)
root.grab_key(dpy.keysym_to_keycode(XK.string_to_keysym("F1")),
    X.Mod1Mask, 1, X.GrabModeAsync, X.GrabModeAsync)

for win in root.query_tree().children:
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
    elif ev.type == X.ReparentNotify:
        frame, win = windows.get(ev.window, (None, None))
        if win in reparenting:
            reparenting.remove(win)
    elif ev.type == X.MapNotify:
        frame, win = windows.get(ev.window, (None, None))
        if frame is None:
            decorate(ev.window)
    elif ev.type == X.UnmapNotify:
        frame, win = windows.get(ev.window, (None, None))
        if ev.window == win and win not in reparenting:
            win.reparent(root, 0, 0)
            win.change_save_set(X.SetModeDelete)
            del windows[frame]
            del windows[win]
            frame.destroy()
    elif ev.type == X.DestroyNotify:
        frame, win = windows.get(ev.window, (None, None))
        if ev.window == win:
            del windows[frame]
            del windows[win]
            frame.destroy()


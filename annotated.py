# NotSoTinyWM is written by Aleksi Torhamo <aleksi@torhamo.net>.
# It is based on TinyWM, which is written by Nick Welch <nick@incise.org>.
#
# This software is in the public domain and is provided AS IS, with NO WARRANTY.

# This is a lightly (in comparison to annotated.c) commented version of nstwm.py.
# For more in-depth explanations, refer to annotated.c.
# Why make code changes once, when you could make them two.. I mean four times instead!

from Xlib.display import Display
from Xlib import X, XK
from util import *

windows = {}
def decorate(win):
    # Normally windows are destroyed if their parent is destroyed.
    # Since we reparent windows to out decorations, they would normally be destroyed when the window manager dies.
    # Adding windows to our save-set prevents them being destroyed when we die.
    win.change_save_set(X.SetModeInsert)
    # Remove any borders the window might have, since it's gonna have decorations now..
    # XXX: I don't know if the X11 borders are purely a visual thing, of if they can actually be used for something..
    win.configure(border_width = 0)
    # Get window geometry so that we know how big to make the decorations
    geom = win.get_geometry()
    # Create the window, 5px borders plus a 20px bar at the top.
    # Setting the background pixel means we don't have to do any drawing and stuff ourselves.
    # We set ButtonPressMask in the event mask for the window so that we get clicks only from the frame,
    # clicks to the window still go to the application as they're supposed to.
    # We set SubstructureNotifyMask so that we're notified when the contained window is destroyed,
    # so that we know when to clean up.
    frame = root.create_window(geom.x - 5, geom.y - 25, geom.width + 10, geom.height + 30,
        0, scr.root_depth, X.CopyFromParent, scr.root_visual, background_pixel = scr.white_pixel, event_mask = X.ButtonPressMask|X.SubstructureNotifyMask)
    # Make sure window is over the frame. XXX: Is this really needed?
    frame.configure(sibling = win, stack_mode = X.Above)
    # Reparent the window to our decorations, take borders and top bar into account for the position.
    win.reparent(frame, 5, 25)
    # Make our frame (and the window) visible.
    frame.map()
    # Keep track which frame contains which window
    windows[frame] = windows[win] = (frame, win)

dpy = Display()
scr = dpy.screen()
root = scr.root

# We set SubstructureNotifyMask on the root window so that we're notified when top-level windows are mapped,
# so that we can decorate them if they're undecorated.
root.change_attributes(event_mask = X.SubstructureNotifyMask)
# Grab the alt+F1 key combination on the root window, which means it'll only be reported to us
root.grab_key(dpy.keysym_to_keycode(XK.string_to_keysym("F1")),
    X.Mod1Mask, 1, X.GrabModeAsync, X.GrabModeAsync)

# Add decorations to existing windows
for win in root.query_tree().children:
    attr = win.get_attributes()
    # Skip adding decorations to this window if it's override_redirect or unmapped.
    # We ignore unmapped windows, since programs seem to use lots of windows that
    # are never mapped during their lifetime. So why bother? Just decorate them
    # if/when they're mapped.
    # XXX: Why windows with override_redirect are skipped, I have no idea. Every
    # window manager seems to do it. Somebody please explain. :(
    # Apparently, override_redirect is meant for "temporary pop-up windows that
    # should not be reparented or affected by the window manager's layout policy",
    # whatever that means. Something tells me I have never seen such a window, ever..
    if attr.override_redirect or attr.map_state == X.IsUnmapped:
        continue
    # The window seems sane, add decorations to it
    decorate(win)

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
    # We got a mouse button press, start dragging. We only get clicks from frames
    # of windows we have decorated.
    elif ev.type == X.ButtonPress:
        # Ask for motion events so we can track the drag. We ask for button
        # release events here instead of in the frame event mask since otherwise
        # we miss the button release if the cursor is outside the window.
        ev.window.grab_pointer(1, X.PointerMotionMask|X.ButtonReleaseMask,
            X.GrabModeAsync, X.GrabModeAsync, X.NONE, X.NONE, X.CurrentTime)
        # Save the original position and size of the window
        attr = ev.window.get_geometry()
        # Save the start position of the drag
        start = ev
    # We only get motion events after we've started dragging, so no special
    # checking necessary.
    elif ev.type == X.MotionNotify:
        # Compress motion notify events, since it doesn't make any sense to
        # look at anything besides the most recent one. This speeds up resizing
        # considerably.
        # TODO: I think in theory we should stop if we see any button events,
        # since doing it like this can process them out of order. I doubt it
        # makes any difference in practice, however.
        ev = compress_motion(dpy, ev)
        # See how the pointer has moved relative to the root window.
        xdiff = ev.root_x - start.root_x
        ydiff = ev.root_y - start.root_y
        # Move or resize the window depending on which button was pressed.
        if start.detail == 1:
            start.window.configure(
                x = attr.x + xdiff,
                y = attr.y + ydiff
            )
        elif start.detail == 3:
            # Do not resize frame or window below 1 pixel width or height
            width, height = max(11, attr.width + xdiff), max(31, attr.height + ydiff)
            # Resize frame
            start.window.configure(width = width, height = height)
            # Resize window
            windows[start.window][1].configure(width = width - 10, height = height - 30)
    # We got a mouse button release, stop dragging.
    elif ev.type == X.ButtonRelease:
        # Stop receiving motion events
        dpy.ungrab_pointer(X.CurrentTime)
    # Since we have asked for SubstructureNotify events for the root window and all
    # our decorations, we have to do a little checking in those events.
    # We can ignore any events on our decorations, since we're the ones who caused
    # them in the first place.

    # A window was mapped
    elif ev.type == X.MapNotify:
        frame, win = windows.get(ev.window, (None, None))
        # We don't know this window yet, so it's a top-level window being mapped
        # for the first time.
        if frame is None:
            decorate(ev.window)
    # A window was destroyed
    elif ev.type == X.DestroyNotify:
        frame, win = windows.get(ev.window, (None, None))
        # It's a decorated window, destroy the decorations too
        if ev.window == win:
            del windows[frame]
            del windows[win]
            frame.destroy()


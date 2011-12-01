"""
This file implements functions which should have been implemented in the X library.
(More or less)
"""
from Xlib import X

def compress_motion(dpy, default = None):
    """Compresses motion notify events

    Does the same thing as this C code:
    while(XCheckTypedEvent(dpy, MotionNotify, &ev));
    """
    dpy = dpy.display

    # Lock the event queue so we can operate on it safely
    dpy.event_queue_read_lock.acquire()
    dpy.event_queue_write_lock.acquire()

    # Extract the events we're interested in
    motion_events = [event for event in dpy.event_queue if event.type == X.MotionNotify]
    # If we didn't get any events, there's obviously nothing to remove, so don't bother
    if motion_events:
        # Remove the events from the queue
        dpy.event_queue[:] = [event for event in dpy.event_queue if event.type != X.MotionNotify]

    # Unlock the event queue
    dpy.event_queue_write_lock.release()
    dpy.event_queue_read_lock.release()

    # Return the latest event, or a default value if we didn't get any events
    return motion_events[-1] if motion_events else default

def match_visual_info(screen, depth, visual_class):
    """Returns the visual information for a visual that matches the
    specified depth and class for a screen.

    Does the same thing as the C Xlib function XMatchVisualInfo().
    """
    for depth_info in screen.allowed_depths:
        if depth_info.depth != depth:
            continue
        for visual_info in depth_info.visuals:
            if visual_info.visual_class != visual_class:
                continue
            return visual_info


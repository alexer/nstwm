/* XAssoc functions */
/* Copyright    Massachusetts Institute of Technology    1985   */

/* emacs_insque & emacs_remque */
/*
Copyright (C) 1993, 1994, 1995, 1996, 1997, 1998, 2001, 2002, 2003,
  2004, 2005, 2006, 2007, 2008, 2009  Free Software Foundation, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include "util.h"
#include <stdlib.h>
#include <errno.h>

extern int errno;

/* Insert ELEM into a doubly-linked list, after PREV.  */

void emacs_insque(struct qelem *elem, struct qelem *prev)
{
    struct qelem *next = prev->q_forw;
    prev->q_forw = elem;
    if (next)
        next->q_back = elem;
    elem->q_forw = next;
    elem->q_back = prev;
}

/* Unlink ELEM from the doubly-linked list that it is in.  */

void emacs_remque(struct qelem *elem)
{
    struct qelem *next = elem->q_forw;
    struct qelem *prev = elem->q_back;
    if (next)
        next->q_back = prev;
    if (prev)
        prev->q_forw = next;
}

/*
 * XCreateAssocTable - Create an XAssocTable.  The size argument should be
 * a power of two for efficiency reasons.  Some size suggestions: use 32
 * buckets per 100 objects;  a reasonable maximum number of object per
 * buckets is 8.  If there is an error creating the XAssocTable, a NULL
 * pointer is returned.
 */
XAssocTable *XCreateAssocTable(size)
    register int size;		/* Desired size of the table. */
{
    register XAssocTable *table;	/* XAssocTable to be initialized. */
    register XAssoc *buckets;	/* Pointer to the first bucket in */
                    /* the bucket array. */

    /* Malloc the XAssocTable. */
    if ((table = (XAssocTable *)malloc(sizeof(XAssocTable))) == NULL) {
        /* malloc call failed! */
        errno = ENOMEM;
        return(NULL);
    }

    /* calloc the buckets (actually just their headers). */
    buckets = (XAssoc *)calloc((unsigned)size, (unsigned)sizeof(XAssoc));
    if (buckets == NULL) {
        /* calloc call failed! */
        errno = ENOMEM;
        return(NULL);
    }

    /* Insert table data into the XAssocTable structure. */
    table->buckets = buckets;
    table->size = size;

    while (--size >= 0) {
        /* Initialize each bucket. */
        buckets->prev = buckets;
        buckets->next = buckets;
        buckets++;
    }

    return(table);
}

/*
 * XDestroyAssocTable - Destroy (free the memory associated with)
 * an XAssocTable.
 */
void XDestroyAssocTable(table)
    register XAssocTable *table;
{
    register int i;
    register XAssoc *bucket;
    register XAssoc *Entry, *entry_next;

    /* Free the buckets. */
    for (i = 0; i < table->size; i++) {
        bucket = &table->buckets[i];
        for (
                Entry = bucket->next;
            Entry != bucket;
            Entry = entry_next
        ) {
                entry_next = Entry->next;
            free((char *)Entry);
        }
    }

    /* Free the bucket array. */
    free((char *)table->buckets);

    /* Free the table. */
    free((char *)table);
}

/*
 * XMakeAssoc - Insert data into an XAssocTable keyed on an XId.
 * Data is inserted into the table only once.  Redundant inserts are
 * meaningless (but cause no problems).  The queue in each association
 * bucket is sorted (lowest XId to highest XId).
 */
void XMakeAssoc(dpy, table, x_id, data)
    register Display *dpy;
    register XAssocTable *table;
    register XID x_id;
    register char *data;
{
    int hash;
    register XAssoc *bucket;
    register XAssoc *Entry;
    register XAssoc *new_entry;

    /* Hash the XId to get the bucket number. */
    hash = x_id & (table->size - 1);
    /* Look up the bucket to get the entries in that bucket. */
    bucket = &table->buckets[hash];
    /* Get the first entry in the bucket. */
    Entry = bucket->next;

    /* If (Entry != bucket), the bucket is empty so make */
    /* the new entry the first entry in the bucket. */
    /* if (Entry == bucket), the we have to search the */
    /* bucket. */
    if (Entry != bucket) {
        /* The bucket isn't empty, begin searching. */
        /* If we leave the for loop then we have either passed */
        /* where the entry should be or hit the end of the bucket. */
        /* In either case we should then insert the new entry */
        /* before the current value of "Entry". */
        for (; Entry != bucket; Entry = Entry->next) {
            if (Entry->x_id == x_id) {
                /* Entry has the same XId... */
                if (Entry->display == dpy) {
                    /* Entry has the same Display... */
                    /* Therefore there is already an */
                    /* entry with this XId and Display, */
                    /* reset its data value and return. */
                    Entry->data = data;
                    return;
                }
                /* We found an association with the right */
                /* id but the wrong display! */
                continue;
            }
            /* If the current entry's XId is greater than the */
            /* XId of the entry to be inserted then we have */
            /* passed the location where the new XId should */
            /* be inserted. */
            if (Entry->x_id > x_id) break;
        }
        }

    /* If we are here then the new entry should be inserted just */
    /* before the current value of "Entry". */
    /* Create a new XAssoc and load it with new provided data. */
    new_entry = (XAssoc *) malloc(sizeof(XAssoc));
    new_entry->display = dpy;
    new_entry->x_id = x_id;
    new_entry->data = data;

    /* Insert the new entry. */
    emacs_insque((struct qelem *)new_entry, (struct qelem *)Entry->prev);
}

/*
 * XLookUpAssoc - Retrieve the data stored in an XAssocTable by its XId.
 * If an appropriately matching XId can be found in the table the routine will
 * return apointer to the data associated with it. If the XId can not be found
 * in the table the routine will return a NULL pointer.  All XId's are relative
 * to the currently active Display.
 */
char *XLookUpAssoc(dpy, table, x_id)
        register Display *dpy;
    register XAssocTable *table;	/* XAssocTable to search in. */
    register XID x_id;			/* XId to search for. */
{
    int hash;
    register XAssoc *bucket;
    register XAssoc *Entry;

    /* Hash the XId to get the bucket number. */
    hash = x_id & (table->size - 1);
    /* Look up the bucket to get the entries in that bucket. */
    bucket = &table->buckets[hash];
    /* Get the first entry in the bucket. */
    Entry = bucket->next;

    /* Scan through the entries in the bucket for the right XId. */
    for (; Entry != bucket; Entry = Entry->next) {
        if (Entry->x_id == x_id) {
            /* We have the right XId. */
            if (Entry->display == dpy) {
                /* We have the right display. */
                /* We have the right entry! */
                return(Entry->data);
            }
            /* Oops, identical XId's on different displays! */
            continue;
        }
        if (Entry->x_id > x_id) {
            /* We have gone past where it should be. */
            /* It is apparently not in the table. */
            return(NULL);
        }
    }
    /* It is apparently not in the table. */
    return(NULL);
}

/*
 * XDeleteAssoc - Delete an association in an XAssocTable keyed on
 * an XId.  An association may be removed only once.  Redundant
 * deletes are meaningless (but cause no problems).
 */
void XDeleteAssoc(dpy, table, x_id)
        register Display *dpy;
    register XAssocTable *table;
    register XID x_id;
{
    int hash;
    register XAssoc *bucket;
    register XAssoc *Entry;

    /* Hash the XId to get the bucket number. */
    hash = x_id & (table->size - 1);
    /* Look up the bucket to get the entries in that bucket. */
    bucket = &table->buckets[hash];
    /* Get the first entry in the bucket. */
    Entry = bucket->next;

    /* Scan through the entries in the bucket for the right XId. */
    for (; Entry != bucket; Entry = Entry->next) {
        if (Entry->x_id == x_id) {
            /* We have the right XId. */
            if (Entry->display == dpy) {
                /* We have the right display. */
                /* We have the right entry! */
                /* Remove it from the queue and */
                /* free the entry. */
                emacs_remque((struct qelem *)Entry);
                free((char *)Entry);
                return;
            }
            /* Oops, identical XId's on different displays! */
            continue;
        }
        if (Entry->x_id > x_id) {
            /* We have gone past where it should be. */
            /* It is apparently not in the table. */
            return;
        }
    }
    /* It is apparently not in the table. */
    return;
}


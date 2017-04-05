//
// Created by ghost on 2/23/17.
//

#ifndef NAUTILUS_SHARE_H
#define NAUTILUS_SHARE_H

G_BEGIN_DECLS

/* Declarations for the Share extension object.  This object will be
 * instantiated by nautilus.  It implements the GInterfaces
 * exported by libnautilus. */

typedef struct _NautilusShare      NautilusShare;
typedef struct _NautilusShareClass NautilusShareClass;

struct _NautilusShare {
    GObject parent_slot;
};

struct _NautilusShareClass {
    GObjectClass parent_slot;

    /* No extra class members */
};


typedef struct _NautilusShareData      NautilusShareData;

struct _NautilusShareData {
    gchar		*fullpath;
    gchar		*section;
    NautilusFileInfo *fileinfo;
};

#endif //SHARE_NAUTILUS_EXTENSION_H

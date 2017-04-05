//
// Created by ghost on 2/24/17.
//

#ifndef SHARE_NAUTILUS_PERMISSIONS_H
#define SHARE_NAUTILUS_PERMISSIONS_H

typedef enum {
    CONFIRM_CANCEL_OR_ERROR,
    CONFIRM_NO_MODIFICATIONS,
    CONFIRM_MODIFIED
} ConfirmPermissionsStatus;

void
restore_write_permissions (const char *path);

void
restore_saved_permissions (const char *path);

ConfirmPermissionsStatus
confirm_sharing_permissions (GtkWidget *widget, const char *path, gboolean is_shared,
                             char * guest_ok, gboolean is_writable);


#endif //SHARE_NAUTILUS_PERMISSIONS_H

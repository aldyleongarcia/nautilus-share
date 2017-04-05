//
// Created by ghost on 2/23/17.
//
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "shares.h"
#include "property-page-class.h"

#include "nautilus-info-provider.h"

#include <glib/gi18n-lib.h>

#include <stdlib.h>


/*----------------Direccion completa del directorio a compartir-------------*/
gchar *
get_fullpath_from_fileinfo(NautilusFileInfo *fileinfo) {
    GFile *file;
    gchar *fullpath;

    g_assert (fileinfo != NULL);

    file = nautilus_file_info_get_location(fileinfo);
    fullpath = g_file_get_path(file);

    g_assert (fullpath != NULL && g_file_is_native(file));
    /* In the beginning we checked that this was a local URI inside root directory / */
    g_object_unref(file);

    return (fullpath);
}

/* Implementation of the NautilusInfoProvider interface */
/* nautilus_info_provider_update_file_info
 * This function is called by Nautilus when it wants the extension to
 * fill in data about the file.  It passes a NautilusFileInfo object,
 * which the extension can use to read data from the file, and which
 * the extension should add data to.
 *
 * If the data can be added immediately (without doing blocking IO),
 * the extension can do so, and return NAUTILUS_OPERATION_COMPLETE.
 * In this case the 'update_complete' and 'handle' parameters can be
 * ignored.
 *
 * If waiting for the data would block the UI, the extension should
 * perform the task asynchronously, and return
 * NAUTILUS_OPERATION_IN_PROGRESS.  The function must also set the
 * 'handle' pointer to a value unique to the object, and invoke the
 * 'update_complete' closure when the update is done.
 *
 * If the extension encounters an error, it should return
 * NAUTILUS_OPERATION_FAILED.
 */

typedef struct {
    gboolean cancelled;
    NautilusInfoProvider *provider;
    NautilusFileInfo *file;
    GClosure *update_complete;
} NautilusShareHandle;

NautilusShareStatus
get_share_status_and_free_share_info(ShareInfo *share_info) {
    NautilusShareStatus result;

    if (!share_info)
        result = NAUTILUS_SHARE_NOT_SHARED;
    else {
        if (share_info->for_all_groups)
            result = NAUTILUS_SHARE_SHARED_RW;
        else
            result = NAUTILUS_SHARE_SHARED_RO;

        shares_free_share_info(share_info);
    }

    return result;
}

void
get_share_info_for_file_info(NautilusFileInfo *file, ShareInfo **share_info, gboolean *is_shareable) {
    char *uri;
    char *local_path = NULL;
    GFile *f;

    *share_info = NULL;
    *is_shareable = FALSE;

    uri = nautilus_file_info_get_uri(file);
    f = nautilus_file_info_get_location(file);
    if (!uri)
        goto out;

#define NETWORK_SHARE_PREFIX "network:///share-"

    if (g_str_has_prefix(uri, NETWORK_SHARE_PREFIX)) {
        const char *share_name;

        share_name = uri + strlen(NETWORK_SHARE_PREFIX);

        /* FIXME: NULL GError */
        if (!shares_get_share_info_for_share_name(share_name, share_info, NULL)) {
            *share_info = NULL;
            *is_shareable = TRUE;
            /* it *has* the prefix, anyway... we are just unsynchronized with what gnome-vfs thinks */
        } else {
            *is_shareable = TRUE;
        }

        goto out;
    }

    if (!nautilus_file_info_is_directory(file))
        goto out;

    local_path = g_file_get_path(f);
    if (!local_path || !g_file_is_native(f))
        goto out;

   // g_message ("Info for File() start");
    /* FIXME: NULL GError */
    if (!shares_get_share_info_for_path(local_path, share_info, NULL))
        goto out;

    *is_shareable = TRUE;

    out:

    g_object_unref(f);
    g_free(uri);
    g_free(local_path);

   // g_message ("Info for File go out() start");
}

/*--------------------------------------------------------------------------*/
NautilusShareStatus
file_get_share_status_file(NautilusFileInfo *file) {
    ShareInfo *share_info;
    gboolean is_shareable;
   // g_message ("Info for File Status() start");
    get_share_info_for_file_info(file, &share_info, &is_shareable);

    if (!is_shareable)
        return NAUTILUS_SHARE_NOT_SHARED;
    //g_message ("Info for File Status() ok");
    return get_share_status_and_free_share_info(share_info);
}

NautilusOperationResult
nautilus_share_update_file_info(NautilusInfoProvider *provider,
                                NautilusFileInfo *file,
                                GClosure *update_complete,
                                NautilusOperationHandle **handle) {
/*   gchar *share_status = NULL; */

    switch (file_get_share_status_file(file)) {

        case NAUTILUS_SHARE_SHARED_RO:
            nautilus_file_info_add_emblem(file, "shared");
/*     share_status = _("shared (read only)"); */
            break;

        case NAUTILUS_SHARE_SHARED_RW:
            nautilus_file_info_add_emblem(file, "shared");
/*     share_status = _("shared (read and write)"); */
            break;

        case NAUTILUS_SHARE_NOT_SHARED:
/*     share_status = _("not shared"); */
            break;

        default:
            g_assert_not_reached ();
            break;
    }

/*   nautilus_file_info_add_string_attribute (file, */
/* 					   "NautilusShare::share_status", */
/* 					   share_status); */
    return NAUTILUS_OPERATION_COMPLETE;
}


void
nautilus_share_cancel_update(NautilusInfoProvider *provider,
                             NautilusOperationHandle *handle) {
    NautilusShareHandle *share_handle;

    share_handle = (NautilusShareHandle *) handle;
    share_handle->cancelled = TRUE;
}


//
// Created by ghost on 2/23/17.
//

#ifndef SHARE_NAUTILUS_INFO_PROVIDER_H
#define SHARE_NAUTILUS_INFO_PROVIDER_H


typedef enum {
    NAUTILUS_SHARE_NOT_SHARED,
    NAUTILUS_SHARE_SHARED_RO,
    NAUTILUS_SHARE_SHARED_RW
} NautilusShareStatus;



gchar *
get_fullpath_from_fileinfo(NautilusFileInfo *fileinfo);

NautilusShareStatus
get_share_status_and_free_share_info (ShareInfo *share_info);

void
get_share_info_for_file_info (NautilusFileInfo *file, ShareInfo **share_info, gboolean *is_shareable);


NautilusShareStatus
file_get_share_status_file(NautilusFileInfo *file);


NautilusOperationResult
nautilus_share_update_file_info (NautilusInfoProvider *provider,
                                 NautilusFileInfo *file,
                                 GClosure *update_complete,
                                 NautilusOperationHandle **handle);

void
nautilus_share_cancel_update (NautilusInfoProvider *provider,
                              NautilusOperationHandle *handle);



#endif //SHARE_NAUTILUS_INFO_PROVIDER_H

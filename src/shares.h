#ifndef SHARES_H
#define SHARES_H

#include <glib.h>

typedef struct {

    char *path;  //La dirección del directorio
    char *share_name; //Nombre del directorio
    char *comment; //Comentario del directorio

    char *guest_ok; //Acceso a invitado con permisos  not,read,full
    GSList *rules; //Lista de usuarios con permisos.

    gboolean for_all_groups;

} ShareInfo;

#define SHARES_ERROR (shares_error_quark ())
#define GUEST_USER (get_guest_user())

typedef enum {
    SHARES_ERROR_FAILED,
    SHARES_ERROR_NONEXISTENT
} SharesError;

char *
get_guest_user(void);

GQuark shares_error_quark(void);

void shares_free_share_info(ShareInfo *info);

gboolean shares_get_path_is_shared(const char *path, gboolean *ret_is_shared, GError **error);

gboolean shares_get_share_info_for_path(const char *path, ShareInfo **ret_share_info, GError **error);

gboolean shares_get_share_name_exists(const char *share_name, gboolean *ret_exists, GError **error);

gboolean shares_get_share_info_for_share_name(const char *share_name, ShareInfo **ret_share_info, GError **error);

gboolean shares_modify_share(const char *old_path, ShareInfo *info, GError **error);

gboolean shares_get_share_info_list(GSList **ret_info_list, GError **error);

void shares_free_share_info_list(GSList *list);

gboolean shares_supports_guest_ok(gboolean *supports_guest_ok_ret,
                                  GError **error);

void shares_set_debug(gboolean error_on_refresh,
                      gboolean error_on_add,
                      gboolean error_on_modify,
                      gboolean error_on_remove);

#endif

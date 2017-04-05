//
// Created by ghost on 2/24/17.
//

#ifndef SHARE_PROPERTY_PAGE_CLASS_H
#define SHARE_PROPERTY_PAGE_CLASS_H

#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-info-provider.h>
#include <libnautilus-extension/nautilus-menu-provider.h>
#include <libnautilus-extension/nautilus-property-page-provider.h>
#include <libnautilus-extension/nautilus-column-provider.h>
#include <libnautilus-extension/nautilus-extension-types.h>

#include <gtk/gtk.h>

typedef struct {

    char *path; /* Full path which is being shared */
    NautilusFileInfo *fileinfo; /* Nautilus file to which this page refers */

    GtkBuilder *xml;
    GtkWidget *main;

    /* Widget that holds all the rest.
     * Its "PropertyPage" GObject-data points to this PropertyPage structure
     * Se colocan todos los elmentos que pertenecen a la vista definida en GLADE*/

    GtkWidget *switchbutton_share_folder;

    GtkWidget *logo_image;

    GtkWidget *share_name;
    GtkWidget *entry_share_name;

    GtkWidget *users_section_gtk_box;

    GtkWidget *users_section_combobox;
    GtkWidget *tree_view_users_section;
    GtkWidget *users_section;
    GtkWidget *add_user_toolbutton;
    GtkWidget *remove_user_toolbutton;
    GtkWidget *info_user_toolbutton;

    GtkWidget *comment_expander;
    GtkWidget *buffer_share_comment;

    GtkWidget *label_status;

    GtkWidget *button_cancel;
    GtkWidget *button_apply;

    GtkWidget *standalone_window;

    /*Variables para ver el estado de la carpeta compartida*/
    gboolean local_user_share_ok;

    GSList *share_users;

    gboolean was_initially_shared; //si ha sido compartida una vez anterior

    gboolean was_writable_for_all; //Si nautilus le ha aplicado permisos para el grupo otros.

    gboolean is_modified;  //si los parámetros de la ventana de propiedades cambian se activa

} PropertyPage;
#endif //SHARE_PROPERTY_PAGE_CLASS_H

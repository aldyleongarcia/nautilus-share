/* nautilus-share -- Nautilus File Sharing Extension
 *
 * Sebastien Estienne <sebastien.estienne@gmail.com>
 * Aldy Leon Garcia <aldyl@nauta.cu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * (C) Copyright 2005 Ethium, Inc.
 * (C) Copyright 2017 UCI.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "property-page-class.h"
#include "shares.h"
#include "nautilus-info-provider.h"
#include "nautilus-permissions.h"
#include "rule_user.h"

#include "property-page.h"
#include "installSamba.h"

#include <glib/gi18n-lib.h>
#include <stdlib.h>

const gboolean DEBUG = FALSE;

enum {
    COLUMN_ITEM_NAME,
    COLUMN_ITEM_WRITE,
    NUM_ITEM_COLUMNS
};

//Eliminar datos de la pagina de propiedades
static void
free_property_page_cb(gpointer data) {

    PropertyPage *page;

    page = data;

    unwait_samba_installation(page);

    g_free(page->path);

    g_object_unref(page->fileinfo);

    g_object_unref(page->xml);

    g_free(page);
}


static void
alert_samba_user(PropertyPage *page, gchar *user_name) {

    GtkWidget *dialog_samba;

    dialog_samba = gtk_message_dialog_new(
            GTK_WINDOW(page->standalone_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_CLOSE,
            _("The user %s has not registered to the Samba Server."),
            user_name);

    gtk_message_dialog_format_secondary_markup(
            GTK_MESSAGE_DIALOG (dialog_samba),
            _(" Ask your administrator to add new user to the Samba Server. \n \n "
                      "<b> Suggestion: </b> sudo smbpasswd -a <i><b>%s</b></i>   \n \n %s"), user_name,
            _("<b> Always restart the user session after performing one of these actions.</b> "));


    gtk_dialog_run(GTK_DIALOG (dialog_samba));

    gtk_widget_destroy(dialog_samba);


}

static void
alert_sambashare_user(PropertyPage *page) {


    GtkWidget *dialog_sambashare;

    dialog_sambashare = gtk_message_dialog_new(
            GTK_WINDOW(page->standalone_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_CLOSE,
            _("Permission denied for %s."),
            g_get_user_name());

    gtk_message_dialog_format_secondary_markup(
            GTK_MESSAGE_DIALOG (dialog_sambashare),
            _("You do not have permission to create a usershare. \n\n %s \n\n %s %s sambashare \n\n %s"),
            _("Ask your administrator to grant you permissions to create a shared folder."),
            _("<b>Suggestion:</b> \n sudo adduser"), g_get_user_name(),
            _("<b> Always restart the user session after performing one of these actions.</b> "));

    gtk_dialog_run(GTK_DIALOG (dialog_sambashare));

    gtk_widget_destroy(dialog_sambashare);

}

static void
notify_add_samba_users(PropertyPage *page) {

    GtkWidget *dialog_sambashare;

    dialog_sambashare = gtk_message_dialog_new(
            GTK_WINDOW(page->standalone_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_CLOSE,
            _("Ask your administrator to grant you permissions to create a shared folder."));

    gtk_message_dialog_format_secondary_markup(

            GTK_MESSAGE_DIALOG (dialog_sambashare),
            _("Instructions for sharing directories with Nautilus-Share."
                      "\n\n%s \n%s \n\n%s \n%s sambashare \n\n%s \n%s"),
            _("<b>Add new user to SAMBA server:</b>"),
            _("    sudo smbpasswd -a <i>your_username</i>"),
            _("<b>Add new user to sambashare group:</b>"),
            _("    sudo adduser <i>your_username</i> "),
            _("<b> Always restart the user session after performing one of these actions.</b> "),
            _("Sharing with guest users without a password is available \n if specified in the Samba configuration. "));

    gtk_dialog_run(GTK_DIALOG (dialog_sambashare));

    gtk_widget_destroy(dialog_sambashare);

}


static void
info_add_user(GtkWidget *button, gpointer data) {

    PropertyPage *page = ((PropertyPage *) data);
    notify_add_samba_users(page);

}


static GtkTreeModel *
create_items_model_users(PropertyPage *page, ShareInfo *info) {

    GtkListStore *model;

    GtkTreeIter iter;

    /* create list store */
    model = gtk_list_store_new(NUM_ITEM_COLUMNS,
                               G_TYPE_STRING,
                               G_TYPE_BOOLEAN);

    /* add items */
    if (DEBUG) g_message("Carpeta Compartida %s", info ? "si" : "no");

    gchar *user_name = NULL;

    gchar *write = NULL;


    if (info) {

        gboolean write_b = FALSE;

        gint cant = rule_length(info->rules);

        if (DEBUG) g_message("Info Carpeta Compartida cantidad elementos %d", cant);

        for (guint i = 0; i < cant; i++) {

            rule_get_data(&user_name, &write, i, info->rules);

            if (!strcmp(write, "d") == 0) {

                if (strcmp(write, "f") == 0) write_b = TRUE;

                if (DEBUG) g_message("Lista de Usuario: %s, Permiso: %s", user_name, write);

                gtk_list_store_append(model,
                                      &iter);

                gtk_list_store_set(model,
                                   &iter,
                                   COLUMN_ITEM_NAME,
                                   user_name,
                                   COLUMN_ITEM_WRITE,
                                   write_b,
                                   -1);
            }

            write_b = FALSE;
        }

    } else {

        //Default user logged as first Account only read.

        user_name = strdup(g_get_user_name());

        gtk_list_store_append(model,
                              &iter);

        gtk_list_store_set(model, &iter,
                           COLUMN_ITEM_NAME,
                           user_name,
                           COLUMN_ITEM_WRITE,
                           TRUE,
                           -1);

    }

    return GTK_TREE_MODEL (model);
}


static gboolean
check_system_samba_user(PropertyPage *page, gchar *name_user) {

    char sAux[24] = "";

    gboolean exito = FALSE;

    FILE *pfsArchivo;

    if (g_strcmp0(name_user, GUEST_USER) != 0) {

        char comando[] = "sudo pdbedit -L | cut -d \":\" -f1";

        pfsArchivo = popen(comando, "r");

        while (fscanf(pfsArchivo, "%24s", sAux) != EOF && g_strcmp0(sAux, "") != 0) {


            if (DEBUG) g_message("Local Samba Server User authentication  %24s", sAux);

            if (g_strcmp0(sAux, name_user) == 0) {
                exito = TRUE;
                break;
            }


        }


        pclose(pfsArchivo);


    } else {

        return TRUE;

    }

    return exito;
}


static gboolean
check_system_samba_status(PropertyPage *page) {

    char sAux[24] = "";

    FILE *pfsArchivo;

    char comando[] = "nc -vz localhost 445 2>&1";

    pfsArchivo = popen(comando, "r");

    while (fscanf(pfsArchivo, "%24s", sAux) != EOF && strcmp(sAux, "") != 0) {


        if (DEBUG) g_message("Samba Status %24s", sAux);

        if (strcmp(sAux, "succeeded!") == 0) {

            pclose(pfsArchivo);

            return TRUE;

        }


    }

    pclose(pfsArchivo);


    return FALSE;
}

static gboolean
check_system_sambausershare_user(PropertyPage *page, gchar *name_user) {

    char sAux[24] = "";

    FILE *pfsArchivo;

    char comando[] = "cat /etc/group | grep sambashare | cut -d \":\" -f4 | sed -e \"s/,/\\n/g\"";

    pfsArchivo = popen(comando, "r");

    while (fscanf(pfsArchivo, "%24s", sAux) != EOF && strcmp(sAux, "") != 0) {


        if (DEBUG) g_message("User system group Sambashare %24s", sAux);

        if (strcmp(sAux, name_user) == 0)

            return TRUE;

    }

    pclose(pfsArchivo);

    return FALSE;
}


static void
create_system_users_list(PropertyPage *page) {

    FILE *pfsArchivo;

    char sAux[24] = "";

    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT (page->users_section_combobox));

    //Usuarios con autentificacion

    char comando[] = "cat /etc/passwd | grep  x:10[0-9][0-9]: | cut -d\":\" -f1 ";

    pfsArchivo = popen(comando, "r");

    while (fscanf(pfsArchivo, "%24s", sAux) != EOF && strcmp(sAux, "") != 0) {


        if (DEBUG) g_message("User system local authentication %24s", sAux);

        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (page->users_section_combobox), "user", sAux);

    }

    pclose(pfsArchivo);

    gboolean guest_ok_allowed;

    shares_supports_guest_ok(&guest_ok_allowed, NULL);

    if (guest_ok_allowed) {

        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (page->users_section_combobox), "user", GUEST_USER);

    }

}

gboolean
foreach_func(GtkTreeModel *model, GtkTreePath *path,
             GtkTreeIter *iter, gpointer user_data) {

    PropertyPage *page = ((PropertyPage *) user_data);

    gchar *user_name;

    gboolean permission_write = FALSE;

    /* Note: here we use 'iter' and not '&iter', because we did not allocate
     *  the iter on the stack and are already getting the pointer to a tree iter */

    gtk_tree_model_get(model, iter,
                       COLUMN_ITEM_NAME,
                       &user_name,
                       COLUMN_ITEM_WRITE,
                       &permission_write,
                       -1);

    if (user_name == NULL)
        return TRUE;

    if (DEBUG) g_message("Datos recuperados %s %s", user_name, permission_write ? "f" : "r");

    page->share_users = rule_add_data(strdup(user_name), permission_write ? "f" : "r", page->share_users);

    g_free(user_name);

    return FALSE; /* do not stop walking the store, call us with next row */
}

static void
get_user_permission(GtkTreeModel *model, PropertyPage *page) {

    if (DEBUG) g_message("BEGIN get user permission");

    rule_delete(page->share_users);

    page->share_users = NULL;

    gtk_tree_model_foreach(model, foreach_func, page);

    if (DEBUG) g_message("Longitud de shareuser  %d", rule_length(page->share_users));
    if (DEBUG) g_message("END get user permission");

}

static void
toolbutoons_action_update(PropertyPage *page) {

    //Actualiza la lista de usuarios.
    GtkTreeModel *model;
    GtkTreeView *treeview = (GtkTreeView *) page->tree_view_users_section;
    model = gtk_tree_view_get_model(treeview);
    get_user_permission(model, page);

    page->old_user = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(page->users_section_combobox));

    if (!rule_find(page->share_users,
                   page->old_user,
                   NULL)) {

        gtk_widget_set_sensitive(page->add_user_toolbutton,
                                 gtk_true());

        if (DEBUG) g_message("Este usuario %s no esta en la Lista", page->old_user);

    } else {

        gtk_widget_set_sensitive(page->add_user_toolbutton,
                                 FALSE);

        if (DEBUG) g_message("Este usuario %s esta presente en la Lista", page->old_user);

    }

}



static void
add_item(GtkWidget *button, gpointer data) {


    PropertyPage *page = ((PropertyPage *) data);

    gchar *user;

    GtkTreeIter current, iter;

    GtkTreePath *path;

    GtkTreeModel *model;

    GtkTreeViewColumn *column;


    GtkTreeView *treeview = (GtkTreeView *) page->tree_view_users_section;

    /* Insert a new row below the current one */

    gtk_tree_view_get_cursor(treeview,
                             &path,
                             NULL);

    model = gtk_tree_view_get_model(treeview);

    //Actualiza la lista de usuarios.
    get_user_permission(model, page);

    user = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(page->users_section_combobox));

    if (check_system_samba_user(page,
                                user)) {

        if (!rule_find(page->share_users,
                       user,
                       NULL)) {


            if (path) {

                gtk_tree_model_get_iter(model,
                                        &current,
                                        path);

                gtk_tree_path_free(path);

                gtk_list_store_insert_after(GTK_LIST_STORE (model),
                                            &iter,
                                            &current);


            } else {

                gtk_list_store_insert(GTK_LIST_STORE (model),
                                      &iter,
                                      -1);

            }

            /* Set the data for the new row */
            gtk_list_store_set(GTK_LIST_STORE (model),
                               &iter,
                               COLUMN_ITEM_NAME,
                               user,
                               COLUMN_ITEM_WRITE,
                               FALSE,
                               -1);

            /* Move focus to the new row */
            path = gtk_tree_model_get_path(model,
                                           &iter);

            column = gtk_tree_view_get_column(treeview,
                                              0);

            gtk_tree_view_set_cursor(treeview,
                                     path,
                                     column,
                                     FALSE);

            gtk_widget_set_sensitive(page->add_user_toolbutton, FALSE);

        }

    } else {

        alert_samba_user(page, user);

    }

    // Definir el estado del boton eliminar
    if (gtk_tree_model_iter_n_children(model, NULL) > 1) {

        gtk_widget_set_sensitive(page->remove_user_toolbutton, TRUE);

    } else {

        gtk_widget_set_sensitive(page->remove_user_toolbutton, FALSE);

    }


    gtk_tree_path_free(path);

}

static void
remove_item(GtkWidget *widget, gpointer data) {

    PropertyPage *page = ((PropertyPage *) data);

    GtkTreeIter iter;

    GtkTreeView *treeview = (GtkTreeView *) page->tree_view_users_section;

    GtkTreeModel *model = gtk_tree_view_get_model(treeview);

    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);


    if (gtk_tree_selection_get_selected(selection,
                                        NULL,
                                        &iter)) {

        GtkTreePath *path;

        path = gtk_tree_model_get_path(model,
                                       &iter);

        if (gtk_tree_model_iter_n_children(model, NULL) <= 1) {

            gtk_widget_set_sensitive(page->remove_user_toolbutton, FALSE);

        }

        gtk_list_store_remove(GTK_LIST_STORE (model),
                              &iter);

        gtk_tree_path_free(path);

        toolbutoons_action_update(page);

    }

}

static void
write_toggled(GtkCellRendererToggle *cell,
              gchar *path_str,
              gpointer data) {

    GtkTreeModel *model = (GtkTreeModel *) data;
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean fixed;

    /* get toggled iter */

    gtk_tree_model_get_iter(model,
                            &iter,
                            path);

    gtk_tree_model_get(model,
                       &iter,
                       COLUMN_ITEM_WRITE,
                       &fixed,
                       -1);

    /* do something with the value */

    fixed ^= 1;

    /* set new value */

    gtk_list_store_set(GTK_LIST_STORE (model),
                       &iter,
                       COLUMN_ITEM_WRITE,
                       fixed,
                       -1);

    /* clean up */

    gtk_tree_path_free(path);
}

static void
add_columns(GtkTreeView *treeview,
            GtkTreeModel *items_model) {

    GtkCellRenderer *renderer;

    /* User column */
    renderer = gtk_cell_renderer_text_new();

    g_object_set(renderer,
                 "editable",
                 FALSE,
                 NULL);

    g_object_set_data(G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_ITEM_NAME));

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (treeview),
                                                -1,
                                                _("Share users"),
                                                renderer,
                                                "text",
                                                COLUMN_ITEM_NAME,
                                                NULL);

    /* Write column */
    renderer = gtk_cell_renderer_toggle_new();

    g_signal_connect (renderer,
                      "toggled",
                      G_CALLBACK(write_toggled),
                      items_model);

    g_object_set_data(G_OBJECT (renderer),
                      "column",
                      GINT_TO_POINTER (COLUMN_ITEM_WRITE));

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (treeview),
                                                -1,
                                                _("Can write?"),
                                                renderer,
                                                "active",
                                                COLUMN_ITEM_WRITE,
                                                NULL);

}

void
modified_table(GtkTreeView *tree_view,
               gpointer user_data) {

    PropertyPage *page = (PropertyPage *) user_data;

    page->is_modified = gtk_true();

    property_page_check_sensitivity(page);


    GtkTreeView *treeview = (GtkTreeView *) page->tree_view_users_section;

    GtkTreeModel *model = gtk_tree_view_get_model(treeview);

    if (gtk_tree_model_iter_n_children(model, NULL) > 1) {

        gtk_widget_set_sensitive(page->remove_user_toolbutton, TRUE);

    } else {

        gtk_widget_set_sensitive(page->remove_user_toolbutton, FALSE);

    }


}


static void
add_user_privileges_section(PropertyPage *page, ShareInfo *info) {

    GtkWidget *widget;

    GtkWidget *sw;

    GtkTreeModel *items_model;


    create_system_users_list(page);

    widget = page->users_section;

    /*Scroll Windows*/

    sw = gtk_scrolled_window_new(NULL, NULL);

    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (sw),
                                        GTK_SHADOW_ETCHED_IN);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (sw),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);

    gtk_box_pack_start(GTK_BOX (widget),
                       sw,
                       TRUE,
                       TRUE,
                       0);

    /*Create models */
    items_model = create_items_model_users(page,
                                           info);

    /*Create tree view */
    page->tree_view_users_section = gtk_tree_view_new_with_model(items_model);

    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(
            GTK_TREE_VIEW (page->tree_view_users_section)),
                                GTK_SELECTION_SINGLE);
    /*Add Columns*/
    add_columns(GTK_TREE_VIEW (page->tree_view_users_section),
                items_model);

    /*Actions Buttons*/
    gtk_widget_set_sensitive(page->add_user_toolbutton, gtk_false());

    gtk_widget_set_sensitive(page->remove_user_toolbutton, gtk_false());

    g_signal_connect (GTK_TOOL_BUTTON(page->add_user_toolbutton),
                      "clicked",
                      G_CALLBACK(add_item),
                      page);

    g_signal_connect (GTK_TOOL_BUTTON(page->remove_user_toolbutton),
                      "clicked",
                      G_CALLBACK(remove_item),
                      page);

    g_signal_connect (GTK_TOOL_BUTTON(page->info_user_toolbutton),
                      "clicked",
                      G_CALLBACK(info_add_user),
                      page);

    g_signal_connect (GTK_TREE_VIEW(page->tree_view_users_section),
                      "cursor-changed",
                      G_CALLBACK(modified_table),
                      page);


    //Close
    g_object_unref(items_model);

    gtk_container_add(GTK_CONTAINER (sw), page->tree_view_users_section);

    gtk_widget_show_all(widget);
}

static ShareInfo get_user_privileges(PropertyPage *page, ShareInfo share_info) {

    GtkWidget *widget;
    GtkWidget *sw;
    GtkWidget *treeview;

    GtkTreeModel *model;

    widget = GTK_WIDGET (gtk_builder_get_object(page->xml, "users_section"));

    sw = g_list_nth_data(gtk_container_get_children(GTK_CONTAINER (widget)), 0);

    treeview = g_list_nth_data(gtk_container_get_children(GTK_CONTAINER (sw)), 0);

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

    get_user_permission(model, page);

    //Compartir con usuario Invitado
    gboolean guest_ok_allowed;

    shares_supports_guest_ok(&guest_ok_allowed, NULL);

    share_info.guest_ok = "not";

    if (rule_find(page->share_users, strdup(GUEST_USER), "r"))

        share_info.guest_ok = "read";

    if (rule_find(page->share_users, strdup(GUEST_USER), "f"))

        share_info.guest_ok = "full";

    if (DEBUG) g_message("Access guest %s", share_info.guest_ok);

    share_info.rules = page->share_users;

    return share_info;
}

static void
property_page_set_controls_sensitivity(PropertyPage *page,
                                       gboolean sensitive) {


    gtk_widget_set_sensitive(page->users_section_gtk_box,
                             sensitive);

    gtk_widget_set_sensitive(page->users_section,
                             sensitive);

    gtk_widget_set_sensitive(page->comment_expander,
                             sensitive);

    gtk_widget_set_sensitive(page->share_name,
                             sensitive);

    gtk_widget_set_sensitive(page->logo_image,
                             sensitive);

    gtk_widget_set_sensitive(page->info_user_toolbutton,
                             sensitive);


}

static void
property_page_label_button_apply(PropertyPage *page,
                                 gboolean enabled) {
    gboolean apply_is_sensitive;

    if (enabled)

        apply_is_sensitive = page->is_modified ||
                             !page->was_initially_shared;

    else

        apply_is_sensitive = page->was_initially_shared;


    gtk_widget_set_sensitive(page->button_apply,
                             apply_is_sensitive);

    gtk_button_set_label(GTK_BUTTON(page->button_apply),
                         page->was_initially_shared ? _("Modify _Share")
                                                    : _("Create _Share"));

}


void
property_page_check_sensitivity(PropertyPage *page) {

    gboolean enabled;

    enabled = gtk_switch_get_state(GTK_SWITCH(page->switchbutton_share_folder));

    if (DEBUG) g_message("Switch Butoon State %s", enabled ? "true" : "false");

    //enable controls to share
    if (DEBUG) g_message("Control Action");

    property_page_set_controls_sensitivity(page,
                                           enabled);

    if (DEBUG) g_message("End Control Action");

    property_page_label_button_apply(page,
                                     enabled);

    if (DEBUG) g_message("Butoon Apply Changed");

    char *name;
    name = g_strdup(g_get_user_name());

    if (DEBUG) g_message("El usuario en esta sesion %s", name);

    //Comprobar que  local  que se van a compartir estan en  Dominio Samba.
    if (DEBUG) g_message("Pregunta si el usuario esta en sambashare o Samba");

    if (enabled) {

        if (!check_system_sambausershare_user(page, name)) {

            gtk_switch_set_state(GTK_SWITCH(page->switchbutton_share_folder), FALSE);

            alert_sambashare_user(page);

            if (DEBUG) g_message("Not Sambashare");

        } else if (!check_system_samba_user(page, name)) {

            gtk_switch_set_state(GTK_SWITCH(page->switchbutton_share_folder), FALSE);

            alert_samba_user(page, name);

            if (DEBUG) g_message("Not Samba user");

        }
        if (DEBUG) g_message("All conditions");
    }

    g_free(name);

    if (DEBUG) g_message("Switch Butoon Good");

}


/*---------------------------Label Status-----------------------------------*/
/* This function does simple validation on the share name and sets the error
   * label; just let it run and ignore the result value.
   */
/*---------------------------Label Status-----------------------------------------------*/
static void
property_page_set_warning_name(PropertyPage *page, char *message) {

    gtk_style_context_add_class(gtk_widget_get_style_context(page->entry_share_name), GTK_STYLE_CLASS_WARNING);

    gtk_label_set_text(GTK_LABEL (page->label_status), message);

}

static void
property_page_set_error_name(PropertyPage *page, char *message) {

    gtk_style_context_add_class(gtk_widget_get_style_context(page->entry_share_name), GTK_STYLE_CLASS_ERROR);

    gtk_label_set_text(GTK_LABEL (page->label_status), message);

}

static void
property_page_unset_warning_name(PropertyPage *page) {

    gtk_style_context_remove_class(gtk_widget_get_style_context(page->entry_share_name), GTK_STYLE_CLASS_ERROR);
    gtk_style_context_remove_class(gtk_widget_get_style_context(page->entry_share_name), GTK_STYLE_CLASS_WARNING);

    gtk_label_set_text(GTK_LABEL (page->label_status), "");

}

static void
property_page_set_error(PropertyPage *page, const char *message) {

    gtk_label_set_text(GTK_LABEL (page->label_status), message);

}

/*---------------------------Label Status-----------------------------------*/


static gboolean
property_page_share_name_is_valid(PropertyPage *page) {

    const char *newname;

    newname = gtk_entry_get_text(GTK_ENTRY (page->entry_share_name));

    if (strlen(newname) == 0) {

        property_page_set_error_name(page,
                                     _("The share name cannot be empty"));

        return FALSE;

    }


    if (g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY (page->entry_share_name)), -1) > 24) {

        property_page_set_warning_name(page,
                                       _("Very long name to maintain compatibility  between systems."));

        return TRUE;

    }


    if (!page->was_initially_shared) {


        GError *error;

        gboolean exists;

        error = NULL;

        if (!shares_get_share_name_exists(newname,
                                          &exists,
                                          &error)) {

            char *str;

            str = g_strdup_printf(_("Error while getting share information: %s"),
                                  error->message);

            g_message("%s", str);

            g_free(str);

            g_error_free(error);

        }


        if (exists) {

            property_page_set_error_name(page,
                                         _("Another share has the same name"));

            return FALSE;
        }

    }

    property_page_unset_warning_name(page);

    return TRUE;

}

static gboolean
change_status_samba_server(PropertyPage *page) {

    if (check_system_samba_status(page)) {

        gtk_image_set_from_file(GTK_IMAGE(page->logo_image), INTERFACES_DIR
        "/ok.png");
        if (DEBUG) g_message("Local Samba Server Status  Active ");

        return TRUE;

    } else {

        gtk_image_set_from_file(GTK_IMAGE(page->logo_image), INTERFACES_DIR
        "/bad.png");

        property_page_set_error(page,
                                _("The Samba server service is not active \n or is being blocked by a firewall."));

        if (DEBUG) g_message("Local Samba Server Status  Inactive ");

        return FALSE;

    }

}

//Directory name control
static void
modify_share_name_text_entry(GtkEditable *editable,
                             gpointer user_data) {


    PropertyPage *page = ((PropertyPage *) user_data);

    page->is_modified = gtk_true();

    property_page_check_sensitivity(page);

    property_page_share_name_is_valid(page);

}

//Directory comment control
static void
modify_share_comment_text_view(GtkContainer *container,
                               GtkWidget *widget,
                               gpointer user_data) {


    PropertyPage *page = (PropertyPage *) user_data;

    page->is_modified = gtk_true();

    property_page_check_sensitivity(page);

}


/*Signal handler for the "active" signal of the Switch*/
static void
on_switchbutton_share_folder_state_set(GObject *switcher,
                                       GParamSpec *pspec,
                                       gpointer user_data) {

    PropertyPage *page = ((PropertyPage *) user_data);


    if (check_samba_installed()) {

        if (DEBUG) g_message("Notify active Switch Butoon");

        property_page_check_sensitivity(page);


        if (DEBUG) g_message(" END Notify active Switch Butoon");

    } else if (gtk_switch_get_state(GTK_SWITCH(page->switchbutton_share_folder)))

        wait_samba_installation(page);

    else

        unwait_samba_installation(page);

}


static gboolean
property_page_commit(PropertyPage *page) {

    gboolean is_shared;

    ShareInfo share_info;

    ConfirmPermissionsStatus status;

    GError *error;

    gboolean retval;

    //Create sambashare list
    create_system_users_list(page);

    //share or not
    is_shared = gtk_switch_get_state(GTK_SWITCH(page->switchbutton_share_folder));

    /**********Rellenar el Formulario share_info **********/

    //Directory path
    share_info.path = page->path;

    //Validate share name

    if (!property_page_share_name_is_valid(page)) return FALSE;

    //Server Samba is Active

    if (!change_status_samba_server(page)) return FALSE;

    //Directory name
    share_info.share_name = (char *) gtk_entry_get_text(GTK_ENTRY (page->entry_share_name));

    //Comment
    GtkTextBuffer *buffer;

    GtkTextIter start, end;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (page->buffer_share_comment));

    gtk_text_buffer_get_start_iter(buffer, &start);

    gtk_text_buffer_get_end_iter(buffer, &end);

    share_info.comment = strdup(gtk_text_buffer_get_text(buffer, &start, &end, FALSE));


    //Guardar usuarios
    GSList *rules = NULL;

    share_info.rules = rules;

    share_info = get_user_privileges(page, share_info);

    /********************Fin de Rellenar el Formulario share_info ***************/

    //Definir máscara para directorios
    char *name;
    name = g_strdup(g_get_user_name());

    if (rule_find(page->share_users, name, NULL) && rule_length(share_info.rules) == 1)

        share_info.for_all_groups = gtk_false();

    else

        share_info.for_all_groups = gtk_true();

    g_free(name);


    /* Do we need to unset the write permissions that we added in the past? */

    if (is_shared && page->was_writable_for_all && !share_info.for_all_groups)

        restore_write_permissions(page->path);

    status = confirm_sharing_permissions(page->main,
                                         page->path,
                                         is_shared,
                                         share_info.guest_ok,
                                         share_info.for_all_groups);

    if (status == CONFIRM_CANCEL_OR_ERROR)
        return FALSE;
    /* the user didn't want us to change his folder's permissions */


    error = NULL;

    retval = shares_modify_share(share_info.path,
                                 is_shared ? &share_info : NULL, &error);

    if (!retval) {

        property_page_set_error(page,
                                error->message);

        g_error_free(error);

        /* Since the operation failed, we restore things to the way they were */
        if (status == CONFIRM_MODIFIED)
            restore_saved_permissions(page->path);

    } else {

        nautilus_file_info_invalidate_extension_info(page->fileinfo);

        /* update initially shared state, so that we may undo later on */
        page->was_initially_shared = is_shared;

        page->is_modified = FALSE;

    }

    if (!is_shared)

        restore_saved_permissions(page->path);

    return retval;
}


//Aplicar los cambios realizados pasando todos los valores
static void
button_apply_clicked_cb(GtkButton *button,
                        gpointer data) {

    PropertyPage *page = ((PropertyPage *) data);


    if (property_page_commit(page)) {

        if (page->standalone_window)

            gtk_widget_destroy(page->standalone_window);

        else

            property_page_check_sensitivity(page);

    }
}


void
button_cancel_clicked_cb(GtkButton *button, gpointer data) {

    GtkWidget *window;

    window = GTK_WIDGET (data);

    gtk_widget_destroy(window);

}


static void
users_section_combobox_changed(GtkBox *widget,
                               gpointer user_data) {

    PropertyPage *page = ((PropertyPage *) user_data);


    toolbutoons_action_update(page);

    page->is_modified = TRUE;

    property_page_check_sensitivity(page);

}

static void
comment_share(PropertyPage *page) {

    GtkWidget *view;
    GtkWidget *sw;

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (sw),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(sw),
                                               80);

    gtk_container_add(GTK_CONTAINER (page->comment_expander), sw);

    gtk_widget_show(sw);

    view = gtk_text_view_new();

    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW (view), GTK_WRAP_WORD);

    gtk_text_view_set_left_margin(GTK_TEXT_VIEW (view), 20);

    gtk_text_view_set_right_margin(GTK_TEXT_VIEW (view), 20);

    page->buffer_share_comment = view;

    gtk_container_add(GTK_CONTAINER (sw), view);

    g_assert(page->buffer_share_comment != NULL);

    gtk_widget_show(view);

}

void get_share_info_path(const PropertyPage *page, GError **error, ShareInfo **share_info) {

    if (!shares_get_share_info_for_path(page->path, share_info, error)) {
        /* We'll assume that there is no share for that path, but we'll still
         * bring up an error dialog.
         */
        GtkWidget *message;

        message = gtk_message_dialog_new(GTK_WINDOW(page->standalone_window),
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         _("There was an error while getting the sharing information"));

        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (message), "%s", (*error)->message);

        gtk_widget_show(message);

        (*share_info) = NULL;

        g_error_free((*error));

        (*error) = NULL;
    }

}

void integrate_with_glade(PropertyPage *page, GError **error) {
    /*****************
    *nautilus-share.ui
    *
    *****************/

    //Integración con Glade

    page->xml = gtk_builder_new();

    gtk_builder_set_translation_domain(page->xml, "nautilus-share");

    //Dirección de la interfaz Glade

    g_assert (gtk_builder_add_from_file(page->xml,
                                        INTERFACES_DIR
                      "/share-dialog.ui",
                              error));


    //Vista principal y validación
    page->main = GTK_WIDGET (gtk_builder_get_object(page->xml,
                                                    "start"));
    g_assert (page->main != NULL);

    g_object_set_data_full(G_OBJECT (page->main),
                           "PropertyPage",
                           page,
                           free_property_page_cb);

    //Declarar todos los widget en la vista de GLADE

    page->switchbutton_share_folder = GTK_WIDGET (gtk_builder_get_object(page->xml, "switchbutton_share_folder"));
    page->logo_image = GTK_WIDGET (gtk_builder_get_object(page->xml, "logo_image"));
    page->share_name = GTK_WIDGET (gtk_builder_get_object(page->xml, "share_name"));
    page->entry_share_name = GTK_WIDGET (gtk_builder_get_object(page->xml, "entry_share_name"));

    page->users_section_gtk_box = GTK_WIDGET (gtk_builder_get_object(page->xml, "users_section_gtk_box"));
    page->users_section_combobox = GTK_WIDGET (gtk_builder_get_object(page->xml, "users_section_combobox"));
    page->users_section = GTK_WIDGET (gtk_builder_get_object(page->xml, "users_section"));

    page->add_user_toolbutton = GTK_WIDGET (gtk_builder_get_object(page->xml, "add_user_toolbutton"));
    page->remove_user_toolbutton = GTK_WIDGET (gtk_builder_get_object(page->xml, "remove_user_toolbutton"));
    page->info_user_toolbutton = GTK_WIDGET (gtk_builder_get_object(page->xml, "info_user_toolbutton"));

    page->comment_expander = GTK_WIDGET (gtk_builder_get_object(page->xml, "comment_expander"));

    page->label_status = GTK_WIDGET (gtk_builder_get_object(page->xml, "label_status"));
    page->button_cancel = GTK_WIDGET (gtk_builder_get_object(page->xml, "button_cancel"));
    page->button_apply = GTK_WIDGET (gtk_builder_get_object(page->xml, "button_apply"));


    /* Sanity check so that we don't screw up the Glade file *
      Chequear que todos los elmentos estan presentes.
      */

    g_assert (page->switchbutton_share_folder != NULL);

    g_assert (page->logo_image != NULL);

    g_assert (page->share_name != NULL);

    g_assert (page->entry_share_name != NULL);

    g_assert (page->users_section_gtk_box != NULL);

    g_assert (page->users_section_combobox != NULL);

    g_assert (page->users_section != NULL);

    g_assert (page->add_user_toolbutton != NULL);

    g_assert (page->remove_user_toolbutton != NULL);

    g_assert (page->info_user_toolbutton != NULL);

    g_assert (page->comment_expander != NULL);

    g_assert (page->label_status != NULL);

    g_assert (page->button_cancel != NULL);

    g_assert (page->button_apply != NULL);

}


void get_share_info_values(PropertyPage *page, const ShareInfo *share_info, char **share_name, const char **comment,
                           const char **apply_button_label) {
    if (share_info) {

        /* Es Compartida */
        page->was_initially_shared = TRUE;

        //Aplicar permisos necesarios
        page->was_writable_for_all = share_info->for_all_groups;

        /* Nombre */
        (*share_name) = share_info->share_name;

        /* Comentario */
        if (strcmp(share_info->comment, "") != 0) {

            (*comment) = share_info->comment;

            gtk_expander_set_expanded(GTK_EXPANDER(page->comment_expander),
                                      TRUE);

        } else {

            gtk_expander_set_expanded(GTK_EXPANDER(page->comment_expander),
                                      FALSE);

            (*comment) = "";

        }

        /* Share toggle */
        gtk_switch_set_state(GTK_SWITCH(page->switchbutton_share_folder),
                             TRUE);

        /* Apply button */
        (*apply_button_label) = _("Modify _Share");

    } else {

        /* Nombre */
        (*share_name) = g_filename_display_basename(page->path);

        /* Comentario */
        (*comment) = "";

        /* Share toggle */
        gtk_switch_set_state(GTK_SWITCH(page->switchbutton_share_folder),
                             FALSE);

        /* Apply button */
        (*apply_button_label) = _("Create _Share");

    }
}


PropertyPage *
create_property_page(NautilusFileInfo *fileinfo) {
    //  g_message ("Property Page() start");

    PropertyPage *page;
    GError *error;
    ShareInfo *share_info;
    char *share_name;
    const char *comment;
    const char *apply_button_label;

    page = g_new0 (PropertyPage, 1);

    page->share_users = NULL;

    page->path = get_fullpath_from_fileinfo(fileinfo);

    page->fileinfo = g_object_ref(fileinfo);

    error = NULL;

    get_share_info_path(page,
                        &error,
                        &share_info);

    integrate_with_glade(page,
                         &error);

    add_user_privileges_section(page,
                                share_info);

    comment_share(page);

    get_share_info_values(page,
                          share_info,
                          &share_name,
                          &comment,
                          &apply_button_label);

    //Obtener el estado del servidor Samba
    change_status_samba_server(page);

    /*Llenar los formularios de property page*/
    gtk_entry_set_text(GTK_ENTRY (page->entry_share_name),
                       share_name);

    ///////////////////////////////////////////////////////////////////////////////
    GtkTextBuffer *buffer;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (page->buffer_share_comment));

    gint lenght = (gint) strlen(comment);

    gtk_text_buffer_set_text(buffer,
                             comment,
                             lenght);

    gtk_text_view_set_buffer(GTK_TEXT_VIEW (page->buffer_share_comment),
                             buffer);

    //////////////////////////////////////////////////////////////////////////
    //Propiedades de button_apply
    gtk_button_set_label(GTK_BUTTON (page->button_apply),
                         apply_button_label);

    gtk_button_set_use_underline(GTK_BUTTON (page->button_apply),
                                 TRUE);

    gtk_button_set_image(GTK_BUTTON (page->button_apply),
                         gtk_image_new_from_file(INTERFACES_DIR
    "/save.png"));

    gtk_widget_set_sensitive(page->button_apply, FALSE);


    /******Visibilidad****
   * ***************************/


    property_page_check_sensitivity(page);

    property_page_share_name_is_valid(page);

    /*****************************************
     * Signal handlers Manejadores de eventos*
     * ***************************************/

    /*Connecting the clicked signal to the callback function*/

    g_signal_connect (page->users_section_combobox, "changed",
                      G_CALLBACK(users_section_combobox_changed),
                      page);

    g_signal_connect (GTK_SWITCH(page->switchbutton_share_folder),
                      "notify::active",
                      G_CALLBACK(on_switchbutton_share_folder_state_set),
                      page);

    g_signal_connect (page->entry_share_name, "changed",
                      G_CALLBACK(modify_share_name_text_entry),
                      page);

    g_signal_connect (page->comment_expander, "set-focus-child",
                      G_CALLBACK(modify_share_comment_text_view),
                      page);


    g_signal_connect (page->button_apply, "clicked",
                      G_CALLBACK(button_apply_clicked_cb),
                      page);

    if (share_info != NULL)

        shares_free_share_info(share_info);

    return page;
}




/*--------------------------------------------------------------------------*/
/* nautilus_property_page_provider_get_pages
 *
 * This function is called by Nautilus when it wants property page
 * items from the extension.
 *
 * This function is called in the main thread before a property page
 * is shown, so it should return quickly.
 *
 * The function should return a GList of allocated NautilusPropertyPage
 * items.
 */
static GList *
nautilus_share_get_property_pages(NautilusPropertyPageProvider *provider,
                                  GList *files) {
    PropertyPage *page;

    GList *pages;

    NautilusPropertyPage *np_page;

    NautilusFileInfo *fileinfo;

    ShareInfo *share_info;

    gboolean is_shareable;

    /* Only show the property page if 1 file is selected */
    if (!files || files->next != NULL) {

        return NULL;

    }

    fileinfo = NAUTILUS_FILE_INFO (files->data);

    //g_message ("Info for File property page() start");

    get_share_info_for_file_info(fileinfo,
                                 &share_info,
                                 &is_shareable);

    if (!is_shareable)

        return NULL;

    page = create_property_page(fileinfo);

    gtk_widget_destroy(page->button_cancel);

    if (share_info)
        shares_free_share_info(share_info);

    pages = NULL;

    np_page = nautilus_property_page_new
            ("NautilusShare::property_page",
             gtk_label_new(_("Local Network Share")),
             page->main);

    pages = g_list_append(pages, np_page);

    //  g_message ("Info for File property page() ok");

    return pages;
}


void
nautilus_share_property_page_provider_iface_init(NautilusPropertyPageProviderIface *iface) {
    iface->get_pages = nautilus_share_get_property_pages;
}

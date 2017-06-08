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

#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-info-provider.h>
#include <libnautilus-extension/nautilus-menu-provider.h>
#include <libnautilus-extension/nautilus-property-page-provider.h>

#include "property-page-class.h"
#include <glib/gi18n-lib.h>
#include <stdlib.h>

//Proyect all class
#include "shares.h"
#include "property-page.h"

#include "nautilus-extension.h"
#include "nautilus-info-provider.h"

GObjectClass *parent_class;

static void
share_this_folder_callback(NautilusMenuItem *item,
                           gpointer user_data) {
    NautilusFileInfo *fileinfo;
    PropertyPage *page;
    GtkWidget *window;

    fileinfo = NAUTILUS_FILE_INFO (user_data);
    g_assert (fileinfo != NULL);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (window), _("Folder Sharing"));
    page = create_property_page(fileinfo);
    page->standalone_window = window;
    g_signal_connect (page->button_cancel, "clicked",
                      G_CALLBACK(button_cancel_clicked_cb), window);

    gtk_container_add(GTK_CONTAINER (window), page->main);
    gtk_widget_show(window);
}

static GList *
nautilus_share_get_file_items(NautilusMenuProvider *provider,
                              GtkWidget *window,
                              GList *files) {
    GList *items;
    NautilusMenuItem *item;
    NautilusFileInfo *fileinfo;
    ShareInfo *share_info;
    gboolean is_shareable;

    /* Only show the property page if 1 file is selected */
    if (!files || files->next != NULL) {
        return NULL;
    }

    fileinfo = NAUTILUS_FILE_INFO (files->data);

   // g_message ("Info for File Items() start");
    get_share_info_for_file_info(fileinfo, &share_info, &is_shareable);
    //g_message ("Info for File Items() ok");
    if (!is_shareable)
        return NULL;

    if (share_info)
        shares_free_share_info(share_info);

    /* We don't own a reference to the file info to keep it around, so acquire one */
    g_object_ref(fileinfo);


    item = nautilus_menu_item_new("NautilusShare::share",
                                  _("Share with Windows users"),
                                  _("Share this Folder"),
                                  "folder-remote");

    g_signal_connect (item, "activate",
                      G_CALLBACK(share_this_folder_callback),
                      fileinfo);
    g_object_set_data_full(G_OBJECT (item),
                           "files",
                           fileinfo,
                           g_object_unref);

    /* Release our reference when the menu item goes away */
    // g_message ("Info for File Items List() start");
    items = g_list_append(NULL, item);
    //g_message ("Info for File Items List() ok");
    return items;
}


static void
nautilus_share_menu_provider_iface_init(NautilusMenuProviderIface *iface) {
    iface->get_file_items = nautilus_share_get_file_items;
}


static void
nautilus_share_info_provider_iface_init(NautilusInfoProviderIface *iface) {
    iface->update_file_info = nautilus_share_update_file_info;
    iface->cancel_update = nautilus_share_cancel_update;
}


/*--------------------------------------------------------------------------*/
/* Type registration.  Because this type is implemented in a module
 * that can be unloaded, we separate type registration from get_type().
 * the type_register() function will be called by the module's
 * initialization function. */

GType share_type = 0;

#define NAUTILUS_TYPE_SHARE  (nautilus_share_get_type ())

static GType
nautilus_share_get_type(void) {
    return share_type;
}


static void
nautilus_share_instance_init(NautilusShare *share) {
}


static void
nautilus_share_class_init(NautilusShareClass *class) {
    parent_class = g_type_class_peek_parent(class);
}


static void
nautilus_share_register_type(GTypeModule *module) {
    const GTypeInfo info = {
            sizeof(NautilusShareClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) nautilus_share_class_init,
            NULL,
            NULL,
            sizeof(NautilusShare),
            0,
            (GInstanceInitFunc) nautilus_share_instance_init,
    };

    share_type = g_type_module_register_type(module,
                                             G_TYPE_OBJECT,
                                             "NautilusShare",
                                             &info, 0);

    /* En el menu de propiedades es la primera llamada a propertypage */
    const GInterfaceInfo property_page_provider_iface_info = {
            (GInterfaceInitFunc) nautilus_share_property_page_provider_iface_init,
            NULL,
            NULL
    };

    g_type_module_add_interface(module,
                                share_type,
                                NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER,
                                &property_page_provider_iface_info);


    /* Directory Info? */
    const GInterfaceInfo info_provider_iface_info = {
            (GInterfaceInitFunc) nautilus_share_info_provider_iface_init,
            NULL,
            NULL
    };

    g_type_module_add_interface(module,
                                share_type,
                                NAUTILUS_TYPE_INFO_PROVIDER,
                                &info_provider_iface_info);

    /* Menu right clik */
    const GInterfaceInfo menu_provider_iface_info = {
            (GInterfaceInitFunc) nautilus_share_menu_provider_iface_init,
            NULL,
            NULL
    };

    g_type_module_add_interface(module,
                                share_type,
                                NAUTILUS_TYPE_MENU_PROVIDER,
                                &menu_provider_iface_info);

}


/*
 * Extension module functions.  These functions are defined in
 * nautilus-extensions-types.h, and must be implemented by all
 * extensions.
   Initialization function.  In addition to any module-specific
 * initialization, any types implemented by the module should
 * be registered here. */

void
nautilus_module_initialize(GTypeModule *module) {
    //  g_print ("Initializing nautilus-share extension\n");
    bindtextdomain("nautilus-share", NAUTILUS_SHARE_LOCALEDIR);
    bind_textdomain_codeset("nautilus-share", "UTF-8");

    nautilus_share_register_type(module);
}

/* Perform module-specific shutdown.
 * Cerrar el módulo de forma específica
 * */
void
nautilus_module_shutdown(void) {
    /*g_print ("Shutting down nautilus-share extension\n");*/
    /*FIXME  Is necessary special shutdown?*/
}

/* List all the extension types.  */

void
nautilus_module_list_types(const GType **types,
                           int *num_types) {
    static GType type_list[1];

    type_list[0] = NAUTILUS_TYPE_SHARE;

    *types = type_list;
    *num_types = 1;
}

/* nautilus-share -- Nautilus File Sharing Extension
 *
 * Sebastien Estienne <sebest@ethium.net>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-column-provider.h>
#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-info-provider.h>
#include <libnautilus-extension/nautilus-menu-provider.h>
#include <libnautilus-extension/nautilus-property-page-provider.h>

#include <libgnomevfs/gnome-vfs-utils.h>
#include "nautilus-share.h"

#include <eel/eel-vfs-extensions.h>

#include <glib/gi18n-lib.h>

#include <gtk/gtktable.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtk.h>
#include <gtk/gtkentry.h>

#include <glade/glade.h>

#include <string.h>
#include <time.h>

#include <unistd.h>
#include <stdlib.h>

#include "shares.h"


static GObjectClass *parent_class;

/* Structure to hold all the information for a share's property page.  If
 * you add stuff to this, add it to free_property_page_cb() as well.
 */
typedef struct {
  char *path; /* Full path which is being shared */
  NautilusFileInfo *fileinfo; /* Nautilus file to which this page refers */

  GladeXML *xml;

  GtkWidget *main; /* Widget that holds all the rest.  Its "PropertyPage" GObject-data points to this PropertyPage structure */
  
  GtkWidget *checkbutton_share_folder;
  GtkWidget *hbox_share_name;
  GtkWidget *hbox_share_comment;
  GtkWidget *entry_share_name;
  GtkWidget *checkbutton_share_rw_ro;
  GtkWidget *entry_share_comment;
  GtkWidget *label_status;
} PropertyPage;

static void property_page_set_warning (PropertyPage *page);
static void property_page_set_error (PropertyPage *page, const char *message);
static void property_page_set_normal (PropertyPage *page);

#if 0
/*--------------------------------------------------------------------------*/
/* Share Editor */
enum
  {
    SHARENAME_COL,
/*     USER_COL, */
    PATH_COL,
    N_COLUMNS
  };

typedef struct {
  GtkListStore *list_store;
  GtkTreeIter iter;
} MyList;

static void
dump(char *path, gpointer user_data)
{
  MyList *mylist = (MyList *)user_data;

  gtk_list_store_append(mylist->list_store, &mylist->iter);
  gtk_list_store_set(mylist->list_store,&mylist->iter,
		     SHARENAME_COL,smbparser_dbus_share_get_name(g_dbus,path),
/* 		     USER_COL,"sebest", */
		     PATH_COL,path,
		     -1);

}

static int
shareeditor (void)
{
  GSList *paths;

  GtkTreeView *treeview_share_list;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkListStore *list_store;
  GtkTreeIter iter;
  
  glade_init();

  MyList *mylist = (MyList *)malloc(sizeof(MyList)); 

  treeview_share_list = (GtkTreeView *)glade_xml_get_widget(xml,"treeview_share_list");


  list_store = gtk_list_store_new (N_COLUMNS,
				   G_TYPE_STRING, /* sharename */
				   G_TYPE_STRING /* path */
				   );
  
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview_share_list), GTK_TREE_MODEL (list_store));

/* share name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("share name",
						     renderer,
						     "text",
						     SHARENAME_COL,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview_share_list), column);

/* path */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("path",
						     renderer,
						     "text",
						     PATH_COL,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview_share_list), column);

  mylist->list_store = list_store;
  mylist->iter = iter;
  
  paths = smbparser_dbus_get_sharepaths(g_dbus); 
  g_slist_foreach(paths,(GFunc)dump,mylist);

  return 0;
}
#endif

static void
property_page_validate_fields (PropertyPage *page)
{
  const char *name;

  name = gtk_entry_get_text (GTK_ENTRY (page->entry_share_name));
  
  if (g_utf8_strlen (name, -1) <= 12)
    property_page_set_normal (page);
  else
    property_page_set_warning (page);
}

static void
property_page_commit (PropertyPage *page)
{
  ShareInfo share_info;
  GError *error;

  share_info.path = page->path;
  share_info.share_name = (char *) gtk_entry_get_text (GTK_ENTRY (page->entry_share_name));
  share_info.comment = (char *) gtk_entry_get_text (GTK_ENTRY (page->entry_share_comment));
  share_info.is_writable = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (page->checkbutton_share_rw_ro));

  error = NULL;
  if (!shares_modify_share (share_info.path, &share_info, &error))
    {
      property_page_set_error (page, error->message);
      g_error_free (error);
    }
  else
    {
      property_page_validate_fields (page);
      nautilus_file_info_invalidate_extension_info (page->fileinfo);
    }
}

/*--------------------------------------------------------------------------*/
static gchar *
get_fullpath_from_fileinfo(NautilusFileInfo *fileinfo)
{
  gchar *uri;
  gchar *fullpath;

  g_assert (fileinfo != NULL);
  
  uri = nautilus_file_info_get_uri(fileinfo);
  fullpath = gnome_vfs_get_local_path_from_uri(uri);
  g_assert (fullpath != NULL); /* In the beginning we checked that this was a local URI */
  g_free(uri);

  return(fullpath);
}

/*--------------------------------------------------------------------------*/
#if 0
/* FMQ: FIXME: remove this function? */
/* FMQ: this is a focus-out handler; the event type is wrong */
static gboolean
left_share_comment_text_entry  (GtkWidget *widget,
				GdkEventCrossing *event,
				gpointer user_data)
{
  /* FMQ: this now gets passed a PropertyPage! */
  gchar *fullpath = (gchar *)user_data;
  char *comment = (char *)gtk_entry_get_text((GtkEntry *)widget);

  if(fullpath && comment)
    {
      /* check that the comment field is not empty */
      if(strcmp(comment, ""))
	smbparser_dbus_share_set_key(g_dbus,fullpath,"comment",comment);
      else
	smbparser_dbus_share_remove_key(g_dbus,fullpath,"comment");
    }
  smbparser_dbus_write(g_dbus);
  return FALSE;
}
#endif

static void
modify_share_comment_text_entry  (GtkEditable *editable,
				  gpointer user_data)
{
  PropertyPage *page;

  page = user_data;

  property_page_commit (page);
}

/*--------------------------------------------------------------------------*/
static void
property_page_set_warning (PropertyPage *page)
{
  GdkColor  colorYellow;

  gtk_label_set_text (GTK_LABEL (page->label_status), _("Share name is too long"));

  gdk_color_parse ("#ECDF62", &colorYellow);
  gtk_widget_modify_base (page->entry_share_name, GTK_STATE_NORMAL, &colorYellow);
}


static void
property_page_set_error (PropertyPage *page, const char *message)
{
  GdkColor colorRed;

  gtk_label_set_text (GTK_LABEL (page->label_status), message);

  gdk_color_parse ("#C1665A", &colorRed);
  gtk_widget_modify_base (page->entry_share_name, GTK_STATE_NORMAL, &colorRed);
}

static void
property_page_set_normal (PropertyPage *page)
{
  gtk_label_set_text (GTK_LABEL (page->label_status), "");
  gtk_widget_modify_base (page->entry_share_name, GTK_STATE_NORMAL, NULL);
}

static gboolean
property_page_share_name_is_valid (PropertyPage *page)
{
  const char *newname;

  newname = gtk_entry_get_text (GTK_ENTRY (page->entry_share_name));

  if (strlen (newname) == 0)
    {
      property_page_set_error (page, _("The share name cannot be empty"));
      return FALSE;
    }
  else
    {
      GError *error;
      gboolean exists;

      error = NULL;
      if (!shares_get_share_name_exists (newname, &exists, &error))
	{
	  char *str;

	  str = g_strdup_printf (_("Error while getting share information: %s"), error->message);
	  property_page_set_error (page, str);
	  g_free (str);
	  g_error_free (error);

	  return FALSE;
	}

      if (exists)
	{
	  property_page_set_error (page, _("Another share has the same name"));
	  return FALSE;
	}
      else
	return TRUE;
    }
}

static void
modify_share_name_text_entry  (GtkEditable *editable,
			       gpointer user_data)
{
  PropertyPage *page;

  page = user_data;

  if (property_page_share_name_is_valid (page))
    property_page_commit (page);
}

static void
property_page_set_sensitivity (PropertyPage *page,
			       gboolean      sensitive)
{
  gtk_widget_set_sensitive (page->entry_share_name, sensitive);
  gtk_widget_set_sensitive (page->entry_share_comment, sensitive);
  gtk_widget_set_sensitive (page->hbox_share_comment, sensitive);
  gtk_widget_set_sensitive (page->hbox_share_name, sensitive);
  gtk_widget_set_sensitive (page->checkbutton_share_rw_ro, sensitive);
}

/*--------------------------------------------------------------------------*/
static void
on_checkbutton_share_folder_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  PropertyPage *page;

  page = user_data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (page->checkbutton_share_folder)))
    {
      property_page_set_sensitivity (page, TRUE);
      property_page_commit (page);
    }
  else
    {
      GError *error;

      /*  sharing button is inactive */
      property_page_set_sensitivity (page, FALSE);

      error = NULL;
      if (!shares_modify_share (page->path, NULL, &error))
	{
	  property_page_set_error (page, error->message);
	  g_error_free (error);
	}

      /* ask nautilus to update emblem */
      nautilus_file_info_invalidate_extension_info (page->fileinfo);
    }
}


/*--------------------------------------------------------------------------*/
static gboolean
left_share_name_text_entry  (GtkWidget *widget,
			     GdkEventFocus *event,
			     gpointer user_data)
{
  PropertyPage *page;

  page = user_data;

  if (property_page_share_name_is_valid (page))
    property_page_commit (page);

  return FALSE;
}

/*--------------------------------------------------------------------------*/
static void
on_checkbutton_share_rw_ro_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  PropertyPage *page;

  page = user_data;
  property_page_commit (page);
}

static void
free_property_page_cb (gpointer data)
{
  PropertyPage *page;

  page = data;

  g_free (page->path);
  g_object_unref (page->fileinfo);
  g_object_unref (page->xml);

  g_free (page);
}

/*--------------------------------------------------------------------------*/
static PropertyPage *
create_property_page (NautilusFileInfo *fileinfo)
{
  PropertyPage *page;
  GError *error;
  ShareInfo *share_info;
  char *share_name;
  gboolean free_share_name;
  const char *comment;

  page = g_new0 (PropertyPage, 1);

  page->path = get_fullpath_from_fileinfo(fileinfo);
  page->fileinfo = g_object_ref (fileinfo);

  error = NULL;
  if (!shares_get_share_info_for_path (page->path, &share_info, &error))
    {
      /* We'll assume that there is no share for that path, but we'll still
       * bring up an error dialog.
       */
      GtkWidget *message;

      message = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					_("There was an error while getting the sharing information"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message), "%s", error->message);
      gtk_widget_show (message);

      share_info = NULL;
      g_error_free (error);
    }

  page->xml = glade_xml_new(INTERFACES_DIR"/share-dialog.glade","vbox1",GETTEXT_PACKAGE);
  page->main = glade_xml_get_widget(page->xml,"vbox1");
  g_object_set_data_full (G_OBJECT (page->main),
			  "PropertyPage",
			  page,
			  free_property_page_cb);

  page->checkbutton_share_folder = glade_xml_get_widget(page->xml,"checkbutton_share_folder");
  page->hbox_share_comment = glade_xml_get_widget(page->xml,"hbox_share_comment");
  page->hbox_share_name = glade_xml_get_widget(page->xml,"hbox_share_name");
  page->checkbutton_share_rw_ro = glade_xml_get_widget(page->xml,"checkbutton_share_rw_ro");
  page->entry_share_name = glade_xml_get_widget(page->xml,"entry_share_name");
  page->entry_share_comment = glade_xml_get_widget(page->xml,"entry_share_comment");
  page->label_status = glade_xml_get_widget(page->xml,"label_status");

  /* Sanity check so that we don't screw up the Glade file */
  g_assert (page->checkbutton_share_folder != NULL
	    && page->hbox_share_comment != NULL
	    && page->hbox_share_name != NULL
	    && page->checkbutton_share_rw_ro != NULL
	    && page->entry_share_name != NULL
	    && page->entry_share_comment != NULL
	    && page->label_status != NULL);

  /* Share name */

  if (share_info)
    {
      share_name = share_info->share_name;
      free_share_name = FALSE;
    }
  else
    {
      share_name = g_filename_display_basename (page->path);
      free_share_name = TRUE;
    }

  gtk_entry_set_text (GTK_ENTRY (page->entry_share_name), share_name);

  if (free_share_name)
    g_free (share_name);

  /* Comment */

  if (share_info == NULL || share_info->comment == NULL)
    comment = "";
  else
    comment = share_info->comment;

  gtk_entry_set_text (GTK_ENTRY (page->entry_share_comment), comment);

  /* Share toggle */

  if (share_info)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (page->checkbutton_share_folder), TRUE);
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (page->checkbutton_share_folder), FALSE);
      property_page_set_sensitivity (page, FALSE);
    }

  /* Share name */

  if (g_utf8_strlen(gtk_entry_get_text (GTK_ENTRY (page->entry_share_name)), -1) > 12)
    property_page_set_warning (page);

  /* Permissions */
  if (share_info != NULL && share_info->is_writable)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (page->checkbutton_share_rw_ro), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (page->checkbutton_share_rw_ro), FALSE);

  /* Signal handlers */
#if 0
  /* FMQ: FIXME: remove these? */
  g_signal_connect (page->entry_share_comment,"focus-out-event",
                    G_CALLBACK (left_share_comment_text_entry),
                    page);

  g_signal_connect (page->entry_share_name, "focus-out-event",
                    G_CALLBACK (left_share_name_text_entry),
                    page);
#endif
  g_signal_connect (page->entry_share_name, "changed",
                    G_CALLBACK (modify_share_name_text_entry),
                    page);

  g_signal_connect (page->entry_share_comment, "changed",
		    G_CALLBACK (modify_share_comment_text_entry),
		    page);

  g_signal_connect (page->checkbutton_share_folder, "toggled",
                    G_CALLBACK (on_checkbutton_share_folder_toggled),
                    page);

  g_signal_connect (page->checkbutton_share_rw_ro, "toggled",
                    G_CALLBACK (on_checkbutton_share_rw_ro_toggled),
                    page);

  if (share_info != NULL)
    shares_free_share_info (share_info);

  return page;
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
 * If waiting for the deata would block the UI, the extension should
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



/*--------------------------------------------------------------------------*/
static NautilusShareStatus 
file_get_share_status (gchar *fullpath)
{
  NautilusShareStatus res;
  ShareInfo *share_info;

  /* FIXME: NULL GError */
  if (!shares_get_share_info_for_path (fullpath, &share_info, NULL))
    return NAUTILUS_SHARE_NOT_SHARED;

  if (!share_info)
    res = NAUTILUS_SHARE_NOT_SHARED;
  else
    {
      if (share_info->is_writable)
	res = NAUTILUS_SHARE_SHARED_RW;
      else
	res = NAUTILUS_SHARE_SHARED_RO;

      shares_free_share_info (share_info);
    }

  return res;
}

/*--------------------------------------------------------------------------*/
static NautilusShareStatus 
file_get_share_status_file(NautilusFileInfo *file)
{
  char		*uri = NULL;
  char		*local_path = NULL;
  NautilusShareStatus result;

  if (!nautilus_file_info_is_directory(file) ||
      !(uri =  nautilus_file_info_get_uri(file)))
    {
      return NAUTILUS_SHARE_NOT_SHARED;
    }

  if(!(local_path = gnome_vfs_get_local_path_from_uri(uri)))
    {
      g_free(uri);
      return NAUTILUS_SHARE_NOT_SHARED;
    }

  result = file_get_share_status (local_path);

  g_free (uri);
  g_free (local_path);

  return result;
}

static NautilusOperationResult
nautilus_share_update_file_info (NautilusInfoProvider *provider,
				 NautilusFileInfo *file,
				 GClosure *update_complete,
				 NautilusOperationHandle **handle)
{
/*   gchar *share_status = NULL; */
  
  switch (file_get_share_status_file (file)) {

  case NAUTILUS_SHARE_SHARED_RO:
    nautilus_file_info_add_emblem (file, "shared");
/*     share_status = _("shared (read only)"); */
    break;

  case NAUTILUS_SHARE_SHARED_RW:
    nautilus_file_info_add_emblem (file, "shared");
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


static void
nautilus_share_cancel_update (NautilusInfoProvider *provider,
			      NautilusOperationHandle *handle)
{
  NautilusShareHandle *share_handle;
	
  share_handle = (NautilusShareHandle*)handle;
  share_handle->cancelled = TRUE;
}

static void 
nautilus_share_info_provider_iface_init (NautilusInfoProviderIface *iface)
{
  iface->update_file_info = nautilus_share_update_file_info;
  iface->cancel_update = nautilus_share_cancel_update;
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
nautilus_share_get_property_pages (NautilusPropertyPageProvider *provider,
				   GList *files)
{
  PropertyPage *page;
  GList *pages;
  NautilusPropertyPage *np_page;
  char *tmp;
  NautilusFileInfo *fileinfo;

  /* Only show the property page if 1 file is selected */
  if (!files || files->next != NULL) {
    return NULL;
  }

  fileinfo = NAUTILUS_FILE_INFO (files->data);
  /* if it's not a directory it can't be shared */
  if (!nautilus_file_info_is_directory(fileinfo))
    return NULL;
  
  tmp = nautilus_file_info_get_uri_scheme(fileinfo);
  if (strncmp(tmp,"file",4) != 0)
    {
      g_free(tmp);
      return NULL;
    }
  g_free(tmp);

  page = create_property_page (fileinfo);
  
  pages = NULL;
  np_page = nautilus_property_page_new
    ("NautilusShare::property_page",
     gtk_label_new (_("Share")),
     page->main);
  pages = g_list_append (pages, np_page);

  return pages;
}

/*--------------------------------------------------------------------------*/
static void 
nautilus_share_property_page_provider_iface_init (NautilusPropertyPageProviderIface *iface)
{
  iface->get_pages = nautilus_share_get_property_pages;
}

/*--------------------------------------------------------------------------*/
static void
nautilus_share_instance_init (NautilusShare *share)
{
}

/*--------------------------------------------------------------------------*/
static void
nautilus_share_class_init (NautilusShareClass *class)
{
  parent_class = g_type_class_peek_parent (class);
}

/* nautilus_menu_provider_get_file_items
 *  
 * This function is called by Nautilus when it wants context menu
 * items from the extension.
 *
 * This function is called in the main thread before a context menu
 * is shown, so it should return quickly.
 * 
 * The function should return a GList of allocated NautilusMenuItem
 * items.
 */
static void button_callback( GtkWidget *widget,
                      gpointer   data )
{
  gtk_widget_destroy((GtkWidget *)data);
}

static void
share_this_folder_callback (NautilusMenuItem *item,
			    gpointer user_data)
{
  NautilusFileInfo *fileinfo;
  PropertyPage *page;
  GtkWidget * window;
  GtkWidget * button;

  fileinfo = NAUTILUS_FILE_INFO (user_data);
  g_assert (fileinfo != NULL);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  page = create_property_page (fileinfo);
  button = glade_xml_get_widget(page->xml,"button_close");
  gtk_container_add (GTK_CONTAINER (window), page->main);
  gtk_widget_show (button);
  gtk_widget_show (window);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (button_callback), window);
}

static GList *
nautilus_share_get_file_items (NautilusMenuProvider *provider,
			     GtkWidget *window,
			     GList *files)
{
  GList *items;
  NautilusMenuItem *item;
  char *tmp;
  NautilusFileInfo *fileinfo;

  /* Only show the property page if 1 file is selected */
  if (!files || files->next != NULL) {
    return NULL;
  }

  fileinfo = NAUTILUS_FILE_INFO (files->data);
  /* if it's not a directory it can't be shared */
  if (!nautilus_file_info_is_directory(fileinfo))
    return NULL;

  /* FMQ: add the "my-shares" scheme here */
  tmp = nautilus_file_info_get_uri_scheme(fileinfo);
  if (strncmp(tmp,"file",4) != 0)
    {
      g_free(tmp);
      return NULL;
    }
  g_free(tmp);

  /* We don't own a reference to the file info to keep it around, so acquire one */
  g_object_ref (fileinfo);
  
  tmp = nautilus_file_info_get_uri(fileinfo);

  /* FMQ: change the label to "Share with Windows users"? */
  item = nautilus_menu_item_new ("NautilusShare::share",
				 _("Share"),
				 _("Share this Folder"),
				 "stock_shared-by-me");
  g_signal_connect (item, "activate",
		    G_CALLBACK (share_this_folder_callback),
		    fileinfo);
  g_object_set_data_full (G_OBJECT (item), 
			  "files",
			  fileinfo,
			  g_object_unref); /* Release our reference when the menu item goes away */

  items = g_list_append (NULL, item);
  return items;
}

/*--------------------------------------------------------------------------*/
static void 
nautilus_share_menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
	iface->get_file_items = nautilus_share_get_file_items;
}

/*--------------------------------------------------------------------------*/
/* Type registration.  Because this type is implemented in a module
 * that can be unloaded, we separate type registration from get_type().
 * the type_register() function will be called by the module's
 * initialization function. */
static GType share_type = 0;

#define NAUTILUS_TYPE_SHARE  (nautilus_share_get_type ())

static GType
nautilus_share_get_type (void) 
{
  return share_type;
}

static void
nautilus_share_register_type (GTypeModule *module)
{
  static const GTypeInfo info = {
    sizeof (NautilusShareClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) nautilus_share_class_init,
    NULL, 
    NULL,
    sizeof (NautilusShare),
    0,
    (GInstanceInitFunc) nautilus_share_instance_init,
  };

  share_type = g_type_module_register_type (module,
					    G_TYPE_OBJECT,
					    "NautilusShare",
					    &info, 0);

  /* onglet share propri�t� */
  static const GInterfaceInfo property_page_provider_iface_info = {
    (GInterfaceInitFunc) nautilus_share_property_page_provider_iface_init,
    NULL,
    NULL
  };
	
  g_type_module_add_interface (module,
			       share_type,
			       NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER,
			       &property_page_provider_iface_info);


  /* premier page propri�t� ? */
  static const GInterfaceInfo info_provider_iface_info = {
    (GInterfaceInitFunc) nautilus_share_info_provider_iface_init,
    NULL,
    NULL
  };

  g_type_module_add_interface (module,
			       share_type,
			       NAUTILUS_TYPE_INFO_PROVIDER,
			       &info_provider_iface_info);

  /* Menu right clik */
  static const GInterfaceInfo menu_provider_iface_info = {
    (GInterfaceInitFunc) nautilus_share_menu_provider_iface_init,
    NULL,
    NULL
  };
  
  g_type_module_add_interface (module,
			       share_type,
			       NAUTILUS_TYPE_MENU_PROVIDER,
			       &menu_provider_iface_info);
  
}

/* Extension module functions.  These functions are defined in 
 * nautilus-extensions-types.h, and must be implemented by all 
 * extensions. */

/* Initialization function.  In addition to any module-specific 
 * initialization, any types implemented by the module should 
 * be registered here. */
void
nautilus_module_initialize (GTypeModule  *module)
{
  g_print ("Initializing nautilus-share extension\n");

  bindtextdomain("nautilus-share", NAUTILUS_SHARE_LOCALEDIR);
  bind_textdomain_codeset("nautilus-share", "UTF-8");

  nautilus_share_register_type (module);
}

/* Perform module-specific shutdown. */
void
nautilus_module_shutdown   (void)
{
  g_print ("Shutting down nautilus-share extension\n");
  /* FIXME freeing */
}

/* List all the extension types.  */
void 
nautilus_module_list_types (const GType **types,
			    int          *num_types)
{
  static GType type_list[1];
	
  type_list[0] = NAUTILUS_TYPE_SHARE;

  *types = type_list;
  *num_types = 1;
}

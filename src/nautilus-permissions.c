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

#include <glib/gi18n-lib.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "property-page-class.h"
#include "nautilus-permissions.h"

/* need go+rx for guest enabled usershares */
#define NEED_IF_GUESTOK_MASK  (S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

/* writable usershares need go+w additionally*/
#define NEED_IF_WRITABLE_MASK (S_IWGRP | S_IWOTH)

#define NEED_ALL_MASK         (NEED_IF_GUESTOK_MASK | NEED_IF_WRITABLE_MASK)


static void
save_key_file (const char *filename, GKeyFile *key_file)
{
    char *contents;
    gsize length;

    //FIXME NULL GError
    contents = g_key_file_to_data (key_file, &length, NULL);
    if (!contents)
        return;

    //FIXME NULL GError
    g_file_set_contents (filename, contents, length, NULL);

    g_free (contents);
}

static void
remove_permissions (const char *path, mode_t need_mask)
{
    struct stat st;
    mode_t new_mode;

    if (need_mask == 0)
        return;

    if (stat (path, &st) != 0)
        return;

    new_mode = st.st_mode & ~need_mask;

     /*FixMe*/
    /* Bleah, no error checking */
    chmod (path, new_mode);
}


static char *
get_key_file_path (void)
{
    return g_build_filename (g_get_home_dir (), ".gnome2", "nautilus-share-modified-permissions", NULL);
}


static void
remove_from_saved_permissions (const char *path, mode_t remove_mask)
{
    GKeyFile *key_file;
    char *key_file_path;

    if (remove_mask == 0)
        return;

    key_file = g_key_file_new ();
    key_file_path = get_key_file_path ();

    if (g_key_file_load_from_file (key_file, key_file_path, 0, NULL))
    {
        mode_t need_mask;
        mode_t remove_from_current_mask;
        char *str;

        need_mask = 0;

        /* NULL GError */
        str = g_key_file_get_string (key_file, path, "need_mask", NULL);

        if (str)
        {
            guint i;

            if (sscanf (str, "%o", &i) == 1) /* octal */
                need_mask = i;

            g_free (str);
        }

        remove_from_current_mask = need_mask & remove_mask;
        remove_permissions (path, remove_from_current_mask);

        need_mask &= ~remove_mask;

        if (need_mask == 0)
        {
            /* NULL GError */
            g_key_file_remove_group (key_file, path, NULL);
        }
        else
        {
            char buf[50];

            g_snprintf (buf, sizeof (buf), "%o", (guint) need_mask); /* octal */
            g_key_file_set_string (key_file, path, "need_mask", buf);
        }

        save_key_file (key_file_path, key_file);
    }

    g_key_file_free (key_file);
    g_free (key_file_path);
}


void
restore_write_permissions (const char *path)
{
    remove_from_saved_permissions (path, NEED_IF_WRITABLE_MASK);
}


void
restore_saved_permissions (const char *path)
{
    remove_from_saved_permissions (path, NEED_ALL_MASK);
}


static void
save_permissions_changed(const char *path, mode_t need_mask)
{
    GKeyFile *key_file;
    char *key_file_path;
    char str[50];

    key_file = g_key_file_new ();
    key_file_path = get_key_file_path ();

    /* FIXME NULL GError
     *
     * We don't check the return value of this.  If the file doesn't exist, we'll
     * simply want to create it.
     */

    g_key_file_load_from_file (key_file, key_file_path, 0, NULL);

    g_snprintf (str, sizeof (str), "%o", (guint) need_mask); /* octal, baby */
    g_key_file_set_string (key_file, path, "need_mask", str);

    save_key_file (key_file_path, key_file);

    g_key_file_free (key_file);
    g_free (key_file_path);
}


static void
error_when_changing_permissions (GtkWidget *widget, const char *path)
{
    GtkWidget *toplevel;
    GtkWidget *dialog;
    char *display_name;

    toplevel = gtk_widget_get_toplevel (widget);
    if (!GTK_IS_WINDOW (toplevel))
        toplevel = NULL;

    display_name = g_filename_display_basename (path);

    dialog = gtk_message_dialog_new (toplevel ? GTK_WINDOW (toplevel) : NULL,
                                     0,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_OK,
                                     _("Could not change the permissions of folder \"%s\""),
                                     display_name);

    g_free (display_name);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}


static gboolean
message_confirm_missing_permissions (GtkWidget *widget, const char *path, mode_t need_mask)
{
    GtkWidget *toplevel;
    GtkWidget *dialog;
    char *display_name;
    gboolean result;

    toplevel = gtk_widget_get_toplevel (widget);
    if (!GTK_IS_WINDOW (toplevel))
        toplevel = NULL;

    display_name = g_filename_display_basename (path);

    dialog = gtk_message_dialog_new (toplevel ? GTK_WINDOW (toplevel) : NULL,
                                     0,
                                     GTK_MESSAGE_QUESTION,
                                     GTK_BUTTONS_NONE,
                                     _("Nautilus needs to add some permissions to your folder "
                                               "\"%s\" in order to share it"),
                                     display_name);


    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              _("The folder \"%s\" needs the following extra permissions for sharing to work:\n"
                                                        "%s%s%s"
                                                        "Do you want Nautilus to add these permissions to the folder automatically?"),
                                              display_name,
                                              (need_mask & (S_IRGRP | S_IROTH)) ? _(
                                                      "  - read permission by group/other\n") : "",
                                              (need_mask & (S_IWGRP | S_IWOTH)) ? _(
                                                      "  - write permission by group/other\n") : "",
                                              (need_mask & (S_IXGRP | S_IXOTH)) ? _(
                                                      "  - execute permission by group/other\n") : "");
    g_free (display_name);

    gtk_dialog_add_button (GTK_DIALOG (dialog), "gtk-cancel", GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button (GTK_DIALOG (dialog), _("Add the permissions automatically"), GTK_RESPONSE_ACCEPT);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

    result = gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT;
    gtk_widget_destroy (dialog);

    return result;
}


ConfirmPermissionsStatus
confirm_sharing_permissions (GtkWidget *widget, const char *path, gboolean is_shared,
                             char *guest_ok, gboolean is_writable)
{
    struct stat st;

    mode_t mode, new_mode, need_mask;

    if (!is_shared | (stat (path, &st) != 0))
        return CONFIRM_NO_MODIFICATIONS;

   /* We'll just let "net usershare" give back an error if the file disappears */

    new_mode = mode = st.st_mode;

    if (strcmp(guest_ok,"read")==0)
        new_mode |= NEED_IF_GUESTOK_MASK;

    if (strcmp(guest_ok,"full")==0)
        new_mode |= NEED_IF_WRITABLE_MASK;

    if (is_writable)
        new_mode |= NEED_IF_WRITABLE_MASK;

    need_mask = new_mode & ~mode;

    if (need_mask != 0)
    {
        g_assert (mode != new_mode);

        if (!message_confirm_missing_permissions (widget, path, need_mask))
            return CONFIRM_CANCEL_OR_ERROR;

        if (chmod (path, new_mode) != 0)
        {
            error_when_changing_permissions (widget, path);
            return CONFIRM_CANCEL_OR_ERROR;
        }

        save_permissions_changed(path, need_mask);

        return CONFIRM_MODIFIED;
    }
    else
    {
        g_assert (mode == new_mode);
        return CONFIRM_NO_MODIFICATIONS;
    }

}

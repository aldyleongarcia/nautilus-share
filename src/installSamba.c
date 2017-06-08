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
#include <sys/wait.h>

#include <glib-object.h>
#include "property-page-class.h"
#include "property-page.h"

#include "installSamba.h"

gboolean
check_samba_installed() {
    return g_file_test("/usr/sbin/smbd", G_FILE_TEST_IS_EXECUTABLE);
}

/* a linked list of PropertyPages waiting for samba to be installed */
GList *pages_waiting_for_samba = NULL;

/* TRUE if currently installing samba, FALSE otherwise */
gboolean is_installing_samba = FALSE;

void
finish_samba_installation(gboolean success) {
    gint response;
    GtkWidget *dialog_ask_restart;

    is_installing_samba = FALSE;

    while (pages_waiting_for_samba) {
        PropertyPage *current = pages_waiting_for_samba->data;
        gtk_switch_set_state(GTK_SWITCH(current->switchbutton_share_folder), success);
        property_page_check_sensitivity(current);
        pages_waiting_for_samba = g_list_remove(pages_waiting_for_samba, current);
    }

    if (!success)
        return;

    /* if we're successful, then ask to restart to make libpam-smbpass create
       NTLM password hash */

    dialog_ask_restart = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_CLOSE,
            _("Restart your session"));
    gtk_message_dialog_format_secondary_markup(
            GTK_MESSAGE_DIALOG (dialog_ask_restart),
            _("You need to restart your session in order to enable sharing."));
    gtk_dialog_add_button(GTK_DIALOG (dialog_ask_restart), _("Restart session"),
                          GTK_RESPONSE_OK);
    response = gtk_dialog_run(GTK_DIALOG (dialog_ask_restart));
    gtk_widget_destroy(dialog_ask_restart);

    if (response == GTK_RESPONSE_OK) {
        g_spawn_command_line_async("gnome-session-save --logout", NULL);
    }
}


static void
set_environment(gpointer display) {
    g_setenv("DISPLAY", display, TRUE);
}

static gboolean
spawn_apturl(GtkWidget *window, const gchar *packages, gint *child_pid) {
    GError *error = NULL;
    gboolean retval = TRUE;
    gchar **argv;
    gchar *display;

    { /* generate argv */
        gint i = 0;

        argv = g_new0 (gchar*, 3);
        argv[i++] = g_find_program_in_path("apturl");
        argv[i++] = g_strdup_printf("apt:%s", packages);
        argv[i++] = NULL;
    }

    display =
            gdk_screen_make_display_name(gtk_window_get_screen(GTK_WINDOW (window)));

    if (!g_spawn_async(NULL,     /* working directory */
                       argv,
                       NULL,     /* envp */
                       G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                       set_environment, display, child_pid, &error)) {
        fprintf(stderr, "apturl spawn failed; %s", (error) ? error->message : "");
        g_error_free(error);
        retval = FALSE;
    }

    g_strfreev(argv);
    g_free(display);

    return retval;

}

static gboolean
on_wait_timeout(gpointer data) {
    gint pid, status;

    pid = GPOINTER_TO_INT (data);

    if (waitpid(pid, &status, WNOHANG) > 0) {
        gtk_main_quit();
        return FALSE;
    }

    return TRUE;
}

static gboolean
wait_for_apturl(GtkWindow *window, pid_t pid) {
    gint timer;
    GdkDisplay *display = gtk_widget_get_display(gtk_window_get_default_widget(window));

    /* set the cursor to a watch */
    GdkCursor *cursor = gdk_cursor_new_from_name(display, "wait");

    gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET (window)), cursor);
    gtk_widget_set_sensitive(GTK_WIDGET (window), FALSE);

    /* wait until apturl is done */
    timer = g_timeout_add(500, on_wait_timeout, GINT_TO_POINTER (pid));
    gtk_main();
    g_source_remove(timer);

    /* set the cursor to nothing */
    gtk_widget_set_sensitive(GTK_WIDGET (window), FALSE);
    gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET (window)), NULL);

    g_object_unref(cursor);

    return check_samba_installed();
}

static void
run_apturl(GtkWidget *window) {
    GtkWidget *dialog_ask_retry;
    gboolean success = FALSE;

    /* generate list of packages for installing */
    const gchar *packages = "samba,libpam-smbpass";

    /* generate dialog_ask_retry */
    dialog_ask_retry = gtk_message_dialog_new(NULL,
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_WARNING,
                                              GTK_BUTTONS_CLOSE,
                                              _("Sharing service installation failed"));

    gtk_message_dialog_format_secondary_markup(
            GTK_MESSAGE_DIALOG (dialog_ask_retry),
            _("Sharing service installation has failed. Would you like to retry the "
                      "installation?"));

    gtk_dialog_add_button(GTK_DIALOG (dialog_ask_retry),
                          _("Retry"),
                          GTK_RESPONSE_OK);

    /* loop until either installed successfully, or user cancels */
    for (;;) {
        gint response; /* response from dialog */
        pid_t pid; /* apturl's pid */

        if (spawn_apturl(window, packages, &pid))
            if ((success = wait_for_apturl(GTK_WINDOW (window), pid)))
                break;

        response = gtk_dialog_run(GTK_DIALOG (dialog_ask_retry));
        gtk_widget_hide(dialog_ask_retry);
        if (response != GTK_RESPONSE_OK) {
            success = FALSE;
            break;
        }
    }

    /* clean up */
    gtk_widget_destroy(dialog_ask_retry);
    finish_samba_installation(success);
}


void
start_samba_installation() {
    gint response; /* response from dialog */
    GtkWidget *dialog;

    /* ask to install samba */
    dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_WARNING,
                                    GTK_BUTTONS_CLOSE,
                                    _("Sharing service is not installed"));
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG (dialog),
                                               _("You need to install the Windows networks "
                                                         "sharing service in order to share your folders."));
    gtk_dialog_add_button(GTK_DIALOG (dialog), _("Install service"), GTK_RESPONSE_OK);
    response = gtk_dialog_run(GTK_DIALOG (dialog));
    gtk_widget_hide(dialog);

    if (response != GTK_RESPONSE_OK)
        finish_samba_installation(FALSE);
    else {
        is_installing_samba = TRUE;
        run_apturl(dialog);
    }
    gtk_widget_destroy(dialog);
}


void
wait_samba_installation(PropertyPage *page) {
    /* if installed, do nothing */
    if (check_samba_installed())
        return;

    pages_waiting_for_samba = g_list_prepend(pages_waiting_for_samba, page);

    /* start samba installation if not already started */
    if (!is_installing_samba)
        start_samba_installation();
}

/*---------------------end samba auto-install code--------------------------*/
void
unwait_samba_installation(PropertyPage *page) {
    pages_waiting_for_samba = g_list_remove(pages_waiting_for_samba, page);
}


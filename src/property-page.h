//
// Created by ghost on 2/23/17.
//

#ifndef SHARE_PROPERTY_PAGE_H
#define SHARE_PROPERTY_PAGE_H

/* Structure to hold all the information for a share's property page.  If
 * you add stuff to this, add it to free_property_page_cb() as well.
 */


void
property_page_check_sensitivity (PropertyPage *page);

void
nautilus_share_property_page_provider_iface_init (NautilusPropertyPageProviderIface *iface);

PropertyPage *
create_property_page (NautilusFileInfo *fileinfo);

void
button_cancel_clicked_cb (GtkButton *button, gpointer data);

#endif //SHARE_PROPERTY_PAGE_H

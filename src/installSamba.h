//
// Created by ghost on 2/22/17.
//

#ifndef SHARE_INSTALLSAMBA_H
#define SHARE_INSTALLSAMBA_H

gboolean
check_samba_installed ();

void
finish_samba_installation (gboolean success);

void
start_samba_installation ();

void
wait_samba_installation (PropertyPage *page);

void
unwait_samba_installation (PropertyPage *page);

#endif //SHARE_INSTALLSAMBA_H

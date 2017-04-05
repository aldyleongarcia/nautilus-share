//
// Created by ghost on 2/25/17.
//

#ifndef SHARE_PROPERTY_PAGE_USERS_H
#define SHARE_PROPERTY_PAGE_USERS_H


typedef struct {
    gchar *name;
    gchar *permission;
} Rule;

GSList *rule_copy_deep(GSList *rules);
void rule_delete(GSList *rule);
GSList* rule_add_data(gchar *name, gchar *permission, GSList *rules);
void rule_get_data(gchar **name, gchar **privilege, guint pos, GSList *rules);
guint rule_length(GSList *rules);
gboolean rule_find(GSList *rules, gchar *name, gchar *permission);

#endif //SHARE_PROPERTY_PAGE_USERS_H

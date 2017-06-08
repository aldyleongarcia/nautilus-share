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
 * (C) Copyright 2017 Cesol, UCI.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include "rule_user.h"

void
rule_delete(GSList *rule) {
    g_slist_free(rule);
}

gpointer
copy_rule(gconstpointer src,
          gpointer data) {

    Rule *rule = ((Rule *) src);

    Rule *aux = g_new(Rule, 1);

    aux->name = strdup(rule->name);
    //  printf("Copy-deep %s N \n", aux->name);
    aux->permission = strdup(rule->permission);
    //  printf("Copy-deep %s P \n", aux->permission);

    return aux;

}

GSList *rule_copy_deep(GSList *rules) {

    return g_slist_copy_deep(rules, copy_rule, NULL);

}

GSList *
rule_add_data(gchar *name, gchar *permission, GSList *rules) {
    Rule *new = g_new(Rule, 1);
    new->name = name;
    new->permission = permission;

    // Validar datos
    //printf("Rule add data Nombre:  %s \n", new->name);
    // printf("Rule add data Permiso:  %s \n", new->permission);


    return g_slist_append(rules, new);
}

void
rule_get_data(gchar **name, gchar **privilege, guint pos, GSList *rules) {

    gint cant = g_slist_length(rules);

    if (pos < cant && pos >= 0) {

        //printf("Cantidad y pos %d %d \n", cant, pos);

        Rule *auxiliar;

        auxiliar = ((Rule *) g_slist_nth_data(rules, pos));

        g_assert(auxiliar->name);
        g_assert(auxiliar->permission);

        // printf("Rule Nombre:  %s  \n", auxiliar->name);
        // printf("Permission:  %s   \n", auxiliar->permission);

        *name = auxiliar->name;
        *privilege = auxiliar->permission;

    }
}

guint rule_length(GSList *rules) {
    guint cant = g_slist_length(rules);
    return cant;
}

gint
rule_compare(gconstpointer pa,
             gconstpointer pb) {

    const Rule *a = pa, *b = pb;

    /* Compares by title, but also you can compare by index or price */
    if (strcmp(a->name, b->name) == 0 && strcmp(a->permission, b->permission) == 0)
        return 0;

    return -1;
}

gboolean rule_find(GSList *rules, gchar *name, gchar *permission) {

    Rule *new = g_new(Rule, 1);

    if (permission != NULL) {
        new->name = name;
        new->permission = permission;

        if (g_slist_find_custom(rules, new, rule_compare) == NULL) {

            return FALSE;
        } else {

            return TRUE;
        }

    }

    if (name != NULL) {

        new->name = name;
        new->permission = "r";

        // printf("A buscar %s %s \n", new->name, new->permission);

        if (g_slist_find_custom(rules, new, rule_compare) == NULL) {
            new->name = name;
            new->permission = "f";

            if (g_slist_find_custom(rules, new, rule_compare) == NULL) {

                return FALSE;
            } else {

                return TRUE;
            }
        } else {

            return TRUE;
        }

    }

    return FALSE;

}


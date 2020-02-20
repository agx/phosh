/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <glib.h>


/**
 * BAD_PROP:
 * @instance: the object to act on
 * @type_name: the symbol prefix for the type of @instance
 * @TypeName: the type name of @instance
 *
 * Attempt to get/set a non existant (invalid) property on an instance
 *
 * Used to check the default case of property getters/setters
 */
#define BAD_PROP(instance, type_name, TypeName)                               \
  g_test_expect_message (G_LOG_DOMAIN,                                        \
                         G_LOG_LEVEL_WARNING,                                 \
                         "* invalid property id 2000000 for \"bad-prop\" of type 'GParamString' in '"#TypeName"'"); \
  type_name##_get_property (G_OBJECT (instance),                              \
                            2000000,                                          \
                            NULL,                                             \
                            g_param_spec_string ("bad-prop",                  \
                                                 "Bad Prop",                  \
                                                 "Non-Existant",              \
                                                 NULL,                        \
                                                 G_PARAM_READWRITE));         \
                                                                              \
  g_test_expect_message (G_LOG_DOMAIN,                                        \
                         G_LOG_LEVEL_WARNING,                                 \
                         "* invalid property id 2000000 for \"bad-prop\" of type 'GParamString' in '"#TypeName"'"); \
  type_name##_set_property (G_OBJECT (instance),                              \
                            2000000,                                          \
                            NULL,                                             \
                            g_param_spec_string ("bad-prop",                  \
                                                 "Bad Prop",                  \
                                                 "Non-Existant",              \
                                                 NULL,                        \
                                                 G_PARAM_READWRITE));

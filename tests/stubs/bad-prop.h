/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <glib.h>


/**
 * BAD_PROP_GET:
 * @instance: the object to act on
 * @type_name: the symbol prefix for the type of @instance
 * @TypeName: the type name of @instance
 *
 * Attempt to get a non existant (invalid) property on an instance
 *
 * Used to check the default case of property getters/setters
 */
#define BAD_PROP_GET(instance, type_name, TypeName)                           \
  if (g_test_subprocess ()) {                                           \
    type_name##_get_property (G_OBJECT (instance),                      \
                              2000000,                                  \
                              NULL,                                     \
                              g_param_spec_string ("bad-prop",          \
                                                   "Bad Prop",          \
                                                   "Non-Existant",      \
                                                   NULL,                \
                                                   G_PARAM_READWRITE)); \
   }                                                                    \
   g_test_trap_subprocess (NULL, 0, 0);                                 \
   g_test_trap_assert_failed ();                                        \
   g_test_trap_assert_stderr ("* invalid property id 2000000 for \"bad-prop\" of type 'GParamString' in '"#TypeName"'\n")


/**
 * BAD_PROP_SET:
 * @instance: the object to act on
 * @type_name: the symbol prefix for the type of @instance
 * @TypeName: the type name of @instance
 *
 * Attempt to get a non existant (invalid) property on an instance
 *
 * Used to check the default case of property getters/setters
 */
#define BAD_PROP_SET(instance, type_name, TypeName)                     \
  if (g_test_subprocess ()) {                                           \
    type_name##_set_property (G_OBJECT (instance),                      \
                              2000000,                                  \
                              NULL,                                     \
                              g_param_spec_string ("bad-prop",          \
                                                   "Bad Prop",          \
                                                   "Non-Existant",      \
                                                   NULL,                \
                                                   G_PARAM_READWRITE)); \
   }                                                                    \
   g_test_trap_subprocess (NULL, 0, 0);                                 \
   g_test_trap_assert_failed ();                                        \
   g_test_trap_assert_stderr ("* invalid property id 2000000 for \"bad-prop\" of type 'GParamString' in '"#TypeName"'\n")

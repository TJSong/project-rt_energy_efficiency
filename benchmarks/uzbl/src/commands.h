#ifndef UZBL_COMMANDS_H
#define UZBL_COMMANDS_H

#include <glib.h>

struct _UzblCommand;
typedef struct _UzblCommand UzblCommand;

GArray *
uzbl_commands_args_new ();
void
uzbl_commands_args_append (GArray *argv, const gchar *arg);
void
uzbl_commands_args_free (GArray *argv);

const UzblCommand *
uzbl_commands_parse (const gchar *cmd, GArray *argv);
void
uzbl_commands_run_parsed (const UzblCommand *info, GArray *argv, GString *result);
void
uzbl_commands_run_argv (const gchar *cmd, GArray *argv, GString *result);
void
uzbl_commands_run (const gchar *cmd, GString *result);

void
uzbl_commands_load_file (const gchar *path);

#endif

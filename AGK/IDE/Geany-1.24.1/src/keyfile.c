/*
 *      keyfile.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2005-2012 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
 *      Copyright 2006-2012 Nick Treleaven <nick(dot)treleaven(at)btinternet(dot)com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * geany.conf preferences file loading and saving.
 */

/*
 * Session file format:
 * filename_xx=pos;filetype UID;read only;Eencoding;use_tabs;auto_indent;line_wrapping;filename
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "geany.h"

#ifdef HAVE_VTE
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "support.h"
#include "keyfile.h"
#include "prefs.h"
#include "ui_utils.h"
#include "utils.h"
#include "document.h"
#include "filetypes.h"
#include "sciwrappers.h"
#include "encodings.h"
#include "vte.h"
#include "main.h"
#include "msgwindow.h"
#include "search.h"
#include "project.h"
#include "editor.h"
#include "printing.h"
#include "templates.h"
#include "toolbar.h"
#include "stash.h"
#include "callbacks.h"
#include "sidebar.h"

/* some default settings which are used at the very first start of Geany to fill
 * the configuration file */
#define GEANY_MAX_SYMBOLLIST_HEIGHT		10
#define GEANY_MIN_SYMBOLLIST_CHARS		4
#define GEANY_MSGWIN_HEIGHT				208
#define GEANY_DISK_CHECK_TIMEOUT		30
#define GEANY_DEFAULT_TOOLS_MAKE		"make"
#ifdef G_OS_WIN32
#define GEANY_DEFAULT_TOOLS_TERMINAL	"cmd.exe /Q /C %c"
#else
#define GEANY_DEFAULT_TOOLS_TERMINAL	"xterm -e \"/bin/sh %c\""
#endif
#ifdef __arm__
	#define GEANY_DEFAULT_TOOLS_BROWSER		"epiphany-browser"
#else
	#define GEANY_DEFAULT_TOOLS_BROWSER		"firefox"
#endif
#define GEANY_DEFAULT_TOOLS_PRINTCMD	"lpr"
#define GEANY_DEFAULT_TOOLS_GREP		"grep"
#define GEANY_DEFAULT_MRU_LENGTH		10
#ifdef __APPLE__
    #define GEANY_DEFAULT_FONT_SYMBOL_LIST	"Helvetica 12"
    #define GEANY_DEFAULT_FONT_MSG_WINDOW	"Helvetica 12"
    #define GEANY_DEFAULT_FONT_EDITOR		"Courier 12"
#else
    #define GEANY_DEFAULT_FONT_SYMBOL_LIST	"Sans 9"
    #define GEANY_DEFAULT_FONT_MSG_WINDOW	"Sans 9"
    #define GEANY_DEFAULT_FONT_EDITOR		"Monospace 10"
#endif
#define GEANY_TOGGLE_MARK				"~ "
#define GEANY_MAX_AUTOCOMPLETE_WORDS	30
#define GEANY_MAX_SYMBOLS_UPDATE_FREQ	250
#define GEANY_DEFAULT_FILETYPE_REGEX    "-\\*-\\s*([^\\s]+)\\s*-\\*-"


static gchar *scribble_text = NULL;
static gint scribble_pos = -1;
static GPtrArray *session_files = NULL;
static gint session_notebook_page;
static gint hpan_position;
static gint vpan_position;
static const gchar atomic_file_saving_key[] = "use_atomic_file_saving";

static GPtrArray *keyfile_groups = NULL;

GPtrArray *pref_groups = NULL;

static struct
{
	gint number_ft_menu_items;
	gint number_non_ft_menu_items;
	gint number_exec_menu_items;
}
build_menu_prefs;


/* The group will be free'd on quitting.
 * @param for_prefs_dialog is whether the group also has Prefs dialog items. */
void configuration_add_pref_group(struct StashGroup *group, gboolean for_prefs_dialog)
{
	g_ptr_array_add(keyfile_groups, group);

	if (for_prefs_dialog)
		g_ptr_array_add(pref_groups, group);
}


/* The group will be free'd on quitting. */
void configuration_add_various_pref_group(struct StashGroup *group)
{
	configuration_add_pref_group(group, TRUE);
	stash_group_set_various(group, TRUE);
}


static void init_pref_groups(void)
{
	StashGroup *group;

	group = stash_group_new(PACKAGE);
	configuration_add_pref_group(group, TRUE);
	stash_group_add_entry(group, &prefs.default_open_path,
		"default_open_path", "", "startup_path_entry");

	stash_group_add_toggle_button(group, &file_prefs.cmdline_new_files,
		"cmdline_new_files", TRUE, "check_cmdline_new_files");

	stash_group_add_toggle_button(group, &interface_prefs.notebook_double_click_hides_widgets,
		"notebook_double_click_hides_widgets", FALSE, "check_double_click_hides_widgets");
	stash_group_add_toggle_button(group, &file_prefs.tab_close_switch_to_mru,
		"tab_close_switch_to_mru", FALSE, "check_tab_close_switch_to_mru");
	stash_group_add_integer(group, &interface_prefs.tab_pos_sidebar, "tab_pos_sidebar", GTK_POS_TOP);
	stash_group_add_radio_buttons(group, &interface_prefs.sidebar_pos,
		"sidebar_pos", GTK_POS_LEFT,
		"radio_sidebar_left", GTK_POS_LEFT,
		"radio_sidebar_right", GTK_POS_RIGHT,
		NULL);
	stash_group_add_radio_buttons(group, &interface_prefs.msgwin_orientation,
		"msgwin_orientation", GTK_ORIENTATION_VERTICAL,
		"radio_msgwin_vertical", GTK_ORIENTATION_VERTICAL,
		"radio_msgwin_horizontal", GTK_ORIENTATION_HORIZONTAL,
		NULL);

	/* editor display */
	stash_group_add_toggle_button(group, &interface_prefs.highlighting_invert_all,
		"highlighting_invert_all", FALSE, "check_highlighting_invert");

	stash_group_add_toggle_button(group, &search_prefs.use_current_word,
		"pref_main_search_use_current_word", TRUE, "check_search_use_current_word");

	/* editor */
	//stash_group_add_toggle_button(group, &editor_prefs.indentation->detect_type, "check_detect_indent", FALSE, "check_detect_indent_type");
	//stash_group_add_toggle_button(group, &editor_prefs.indentation->detect_width, "detect_indent_width", FALSE, "check_detect_indent_width");
	stash_group_add_toggle_button(group, &editor_prefs.use_tab_to_indent,
		"use_tab_to_indent", TRUE, "check_tab_key_indents");
	stash_group_add_spin_button_integer(group, &editor_prefs.indentation->width,
		"pref_editor_tab_width", 4, "spin_indent_width");
	stash_group_add_combo_box(group, (gint*)(void*)&editor_prefs.indentation->auto_indent_mode,
		"indent_mode", GEANY_AUTOINDENT_CURRENTCHARS, "combo_auto_indent_mode");
	stash_group_add_radio_buttons(group, (gint*)(void*)&editor_prefs.indentation->type,
		"indent_type", GEANY_INDENT_TYPE_TABS,
		"radio_indent_spaces", GEANY_INDENT_TYPE_SPACES,
		"radio_indent_tabs", GEANY_INDENT_TYPE_TABS,
		"radio_indent_both", GEANY_INDENT_TYPE_BOTH,
		NULL);
	stash_group_add_radio_buttons(group, (gint*)(void*)&editor_prefs.show_virtual_space,
		"virtualspace", GEANY_VIRTUAL_SPACE_SELECTION,
		"radio_virtualspace_disabled", GEANY_VIRTUAL_SPACE_DISABLED,
		"radio_virtualspace_selection", GEANY_VIRTUAL_SPACE_SELECTION,
		"radio_virtualspace_always", GEANY_VIRTUAL_SPACE_ALWAYS,
		NULL);
	stash_group_add_toggle_button(group, &editor_prefs.autocomplete_doc_words,
		"autocomplete_doc_words", FALSE, "check_autocomplete_doc_words");
	stash_group_add_toggle_button(group, &editor_prefs.completion_drops_rest_of_word,
		"completion_drops_rest_of_word", FALSE, "check_completion_drops_rest_of_word");
	stash_group_add_spin_button_integer(group, (gint*)&editor_prefs.autocompletion_max_entries,
		"autocompletion_max_entries", GEANY_MAX_AUTOCOMPLETE_WORDS,
		"spin_autocompletion_max_entries");
	stash_group_add_spin_button_integer(group, (gint*)&editor_prefs.autocompletion_update_freq,
		"autocompletion_update_freq", GEANY_MAX_SYMBOLS_UPDATE_FREQ, "spin_symbol_update_freq");
	stash_group_add_string(group, &editor_prefs.color_scheme,
		"color_scheme", NULL);

	/* files */
	stash_group_add_spin_button_integer(group, (gint*)&file_prefs.mru_length,
		"mru_length", GEANY_DEFAULT_MRU_LENGTH, "spin_mru");
	stash_group_add_spin_button_integer(group, &file_prefs.disk_check_timeout,
		"disk_check_timeout", GEANY_DISK_CHECK_TIMEOUT, "spin_disk_check");

	/* various geany prefs */
	group = stash_group_new(PACKAGE);
	configuration_add_various_pref_group(group);

	stash_group_add_boolean(group, &editor_prefs.show_scrollbars,
		"show_editor_scrollbars", TRUE);
	stash_group_add_boolean(group, &editor_prefs.brace_match_ltgt,
		"brace_match_ltgt", FALSE);
	stash_group_add_boolean(group, &editor_prefs.use_gtk_word_boundaries,
		"use_gtk_word_boundaries", TRUE);
	stash_group_add_boolean(group, &editor_prefs.complete_snippets_whilst_editing,
		"complete_snippets_whilst_editing", FALSE);
	stash_group_add_boolean(group, &file_prefs.use_safe_file_saving,
		atomic_file_saving_key, FALSE);
	stash_group_add_boolean(group, &file_prefs.gio_unsafe_save_backup,
		"gio_unsafe_save_backup", FALSE);
	stash_group_add_boolean(group, &file_prefs.use_gio_unsafe_file_saving,
		"use_gio_unsafe_file_saving", TRUE);
	/* for backwards-compatibility */
	stash_group_add_integer(group, &editor_prefs.indentation->hard_tab_width,
		"indent_hard_tab_width", 8);
	stash_group_add_integer(group, (gint*)&search_prefs.find_selection_type,
		"find_selection_type", GEANY_FIND_SEL_CURRENT_WORD);
	stash_group_add_string(group, &file_prefs.extract_filetype_regex,
		"extract_filetype_regex", GEANY_DEFAULT_FILETYPE_REGEX);

	/* Note: Interface-related various prefs are in ui_init_prefs() */

	/* various build-menu prefs */
	group = stash_group_new("build-menu");
	configuration_add_various_pref_group(group);

	stash_group_add_integer(group, &build_menu_prefs.number_ft_menu_items,
		"number_ft_menu_items", 0);
	stash_group_add_integer(group, &build_menu_prefs.number_non_ft_menu_items,
		"number_non_ft_menu_items", 0);
	stash_group_add_integer(group, &build_menu_prefs.number_exec_menu_items,
		"number_exec_menu_items", 0);
}


typedef enum SettingAction
{
	SETTING_READ,
	SETTING_WRITE
}
SettingAction;

static void settings_action(GKeyFile *config, SettingAction action)
{
	guint i;
	StashGroup *group;

	foreach_ptr_array(group, i, keyfile_groups)
	{
		switch (action)
		{
			case SETTING_READ:
				stash_group_load_from_key_file(group, config); break;
			case SETTING_WRITE:
				stash_group_save_to_key_file(group, config); break;
		}
	}
}


static void save_recent_files(GKeyFile *config, GQueue *queue, gchar const *key)
{
	gchar **recent_files = g_new0(gchar*, file_prefs.mru_length + 1);
	guint i;

	for (i = 0; i < file_prefs.mru_length; i++)
	{
		if (! g_queue_is_empty(queue))
		{
			/* copy the values, this is necessary when this function is called from the
			 * preferences dialog or when quitting is canceled to keep the queue intact */
			recent_files[i] = g_strdup(g_queue_peek_nth(queue, i));
		}
		else
		{
			recent_files[i] = NULL;
			break;
		}
	}
	/* There is a bug in GTK 2.6 g_key_file_set_string_list, we must NULL terminate. */
	recent_files[file_prefs.mru_length] = NULL;
	g_key_file_set_string_list(config, "files", key,
				(const gchar**)recent_files, file_prefs.mru_length);
	g_strfreev(recent_files);
}


static gchar *get_session_file_string(GeanyDocument *doc)
{
	gchar *fname;
	gchar *locale_filename;
	gchar *escaped_filename;
	GeanyFiletype *ft = doc->file_type;

	if (ft == NULL) /* can happen when saving a new file when quitting */
		ft = filetypes[GEANY_FILETYPES_NONE];

	locale_filename = utils_get_locale_from_utf8(doc->file_name);
	escaped_filename = g_uri_escape_string(locale_filename, NULL, TRUE);

	fname = g_strdup_printf("%d;%s;%d;E%s;%d;%d;%d;%s;%d;%d",
		sci_get_current_position(doc->editor->sci),
		ft->name,
		doc->readonly,
		doc->encoding,
		doc->editor->indent_type,
		doc->editor->auto_indent,
		doc->editor->line_wrapping,
		escaped_filename,
		doc->editor->line_breaking,
		doc->editor->indent_width);
	g_free(escaped_filename);
	g_free(locale_filename);
	return fname;
}

void configuration_save_open_projects(GKeyFile *config)
{
	gint npage;
	gchar *tmp;
	gchar entry[30];
	guint i = 0, j = 0, max;
	
	for ( i = 0; i < projects_array->len; i++ )
	{
		if ( !projects[i]->is_valid ) 
			continue;

		gchar *fname;

		g_snprintf(entry, sizeof(entry), "PROJECT_FILE_NAME_%d", j);

		gchar* path = g_build_filename( projects[i]->base_path, projects[i]->name, NULL );
		SETPTR( path, g_strconcat( path, ".agk", NULL ) );
		
		gchar* utf8_filename = utils_get_utf8_from_locale(path);
		gchar* escaped_filename = g_uri_escape_string(utf8_filename, NULL, TRUE);

		gint current = 0;
		if ( projects[i] == app->project ) current = 1;

		fname = g_strdup_printf( "%d;%s", current, escaped_filename );

		g_key_file_set_string(config, "projects", entry, fname);
		g_free(fname);
		g_free(path);
		g_free(escaped_filename);
		g_free(utf8_filename);
		j++;
	}
	/* if open filenames less than saved session files, delete existing entries in the list */
	i = j;
	while (TRUE)
	{
		g_snprintf(entry, sizeof(entry), "PROJECT_FILE_NAME_%d", i);
		tmp = g_key_file_get_string(config, "projects", entry, NULL);
		if (G_UNLIKELY(tmp == NULL))
		{
			break;
		}
		else
		{
			g_key_file_remove_key(config, "projects", entry, NULL);
			g_free(tmp);
			i++;
		}
	}
}

void configuration_save_session_files(GKeyFile *config, GeanyProject *project)
{
	gint npage;
	gchar *tmp;
	gchar entry[16];
	guint i = 0, j = 0, max;
	GeanyDocument *doc, *curr_doc = document_get_current();

	//npage = gtk_notebook_get_current_page(GTK_NOTEBOOK(main_widgets.notebook));
	//g_key_file_set_integer(config, "files", "current_page", npage);

	/* store the filenames in the notebook tab order to reopen them the next time */
	/*
	max = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_widgets.notebook));
	for (i = 0; i < max; i++)
	{
		doc = document_get_from_page(i);
	*/
	for ( i = 0; i < project->project_files->len; i++ )
	{
		if ( !project_files_index(project,i)->is_valid ) 
			continue;

		GeanyDocument *doc = document_find_by_real_path( project_files_index(project,i)->file_name );
		if (doc != NULL && doc->is_valid)
		{
			gchar *fname;

			g_snprintf(entry, sizeof(entry), "FILE_NAME_%d", j);

			gchar* relative_path = utils_get_relative_path( project->base_path, doc->real_path );
			utils_str_replace_char(  relative_path, '\\', '/' );
			
			gchar* utf8_filename = utils_get_utf8_from_locale(relative_path);
			gchar* escaped_filename = g_uri_escape_string(utf8_filename, NULL, TRUE);

			gint current = 0;
			if ( doc == curr_doc ) current = 1;

			fname = g_strdup_printf( "%d;%s;%d;%d", current, escaped_filename, sci_get_current_position(doc->editor->sci), document_get_notebook_page(doc) );

			g_key_file_set_string(config, "files", entry, fname);
			g_free(fname);
			g_free(escaped_filename);
			g_free(utf8_filename);
			g_free(relative_path);
			j++;
		}
	}
	/* if open filenames less than saved session files, delete existing entries in the list */
	i = j;
	while (TRUE)
	{
		g_snprintf(entry, sizeof(entry), "FILE_NAME_%d", i);
		tmp = g_key_file_get_string(config, "files", entry, NULL);
		if (G_UNLIKELY(tmp == NULL))
		{
			break;
		}
		else
		{
			g_key_file_remove_key(config, "files", entry, NULL);
			g_free(tmp);
			i++;
		}
	}

#ifdef HAVE_VTE
	if (vte_info.have_vte)
	{
		vte_get_working_directory();	/* refresh vte_info.dir */
		g_key_file_set_string(config, "VTE", "last_dir", vte_info.dir);
	}
#endif
}

void configuration_save_project_files(GKeyFile *config, GeanyProject *project)
{
	gint npage;
	gchar *tmp;
	gchar entry[16];
	guint i = 0, j = 0;

	gint *groupIndex = g_new0(gint, project->project_groups->len);
	
	// save project groups
	for (i = 0; i < project->project_groups->len; i++)
	{
		GeanyProjectGroup *group = project->project_groups->pdata[i];
		if (group != NULL && group->is_valid)
		{
			gchar *fname;

			groupIndex[ i ] = j;
			gint parent = -1;
			if ( group->pParent ) parent = groupIndex[ group->pParent->index ];

			g_snprintf(entry, sizeof(entry), "GROUP_%d", j);
			fname = g_strdup_printf( "%s;%d", group->group_name, parent);
			g_key_file_set_string(config, "projectgroups", entry, fname);
			g_free(fname);
			j++;
		}
	}

	j = 0;
	for (i = 0; i < project->project_files->len; i++)
	{
		GeanyProjectFile *file = project->project_files->pdata[i];
		if (file != NULL && file->is_valid)
		{
			gchar *fname;

			g_snprintf(entry, sizeof(entry), "FILE_%d", j);

			gchar* relative_path = utils_get_relative_path( project->base_path, file->file_name );
			utils_str_replace_char(  relative_path, '\\', '/' );
			
			gchar* utf8_filename = utils_get_utf8_from_locale(relative_path);
			gchar* escaped_filename = g_uri_escape_string(utf8_filename, NULL, TRUE);

			gint parent = -1;
			if ( file->pParent ) parent = groupIndex[ file->pParent->index ];

			fname = g_strdup_printf( "%s;%d", escaped_filename, parent);
			g_key_file_set_string(config, "projectfiles", entry, fname);
			g_free(fname);
			g_free(escaped_filename);
			g_free(utf8_filename);
			g_free(relative_path);
			j++;
		}
	}

	g_free(groupIndex);

	/* if open filenames less than saved session files, delete existing entries in the list */
	i = j;
	while (TRUE)
	{
		g_snprintf(entry, sizeof(entry), "FILE_%d", i);
		tmp = g_key_file_get_string(config, "projectfiles", entry, NULL);
		if (G_UNLIKELY(tmp == NULL))
		{
			break;
		}
		else
		{
			g_key_file_remove_key(config, "projectfiles", entry, NULL);
			g_free(tmp);
			i++;
		}
	}

#ifdef HAVE_VTE
	if (vte_info.have_vte)
	{
		vte_get_working_directory();	/* refresh vte_info.dir */
		g_key_file_set_string(config, "VTE", "last_dir", vte_info.dir);
	}
#endif
}

static void save_install_prefs(GKeyFile *config)
{
	g_key_file_set_string(config, "AGKInstall", "projects_folder", FALLBACK(install_prefs.projects_folder,""));
	g_key_file_set_integer(config, "AGKInstall", "projects_update", install_prefs.update_projects_mode);
	g_key_file_set_string(config, "AGKInstall", "libraries_folder", FALLBACK(install_prefs.tier2_folder,""));
	g_key_file_set_integer(config, "AGKInstall", "libraries_update", install_prefs.update_tier2_mode);
}

static void save_dialog_prefs(GKeyFile *config)
{
	/* new settings should be added in init_pref_groups() */
	settings_action(config, SETTING_WRITE);

	/* Some of the key names are not consistent, but this is for backwards compatibility */

	/* general */
	//g_key_file_set_boolean(config, PACKAGE, "pref_main_load_session", prefs.load_session);
	//g_key_file_set_boolean(config, PACKAGE, "pref_main_project_session", project_prefs.project_session);
	//g_key_file_set_boolean(config, PACKAGE, "pref_main_project_file_in_basedir", project_prefs.project_file_in_basedir);
	g_key_file_set_boolean(config, PACKAGE, "pref_main_save_winpos", prefs.save_winpos);
	g_key_file_set_boolean(config, PACKAGE, "pref_main_confirm_exit", prefs.confirm_exit);
	g_key_file_set_boolean(config, PACKAGE, "pref_remember_projects", prefs.remember_projects);
	g_key_file_set_boolean(config, PACKAGE, "pref_main_suppress_status_messages", prefs.suppress_status_messages);
	g_key_file_set_boolean(config, PACKAGE, "switch_msgwin_pages", prefs.switch_to_status);
	g_key_file_set_boolean(config, PACKAGE, "beep_on_errors", prefs.beep_on_errors);
	g_key_file_set_boolean(config, PACKAGE, "auto_focus", prefs.auto_focus);

	g_key_file_set_integer(config, PACKAGE, "IDE_version_int", AGK_VERSION_INT);

	/* interface */
	g_key_file_set_boolean(config, PACKAGE, "sidebar_symbol_visible", interface_prefs.sidebar_symbol_visible);
	g_key_file_set_boolean(config, PACKAGE, "sidebar_openfiles_visible", interface_prefs.sidebar_openfiles_visible);
	g_key_file_set_string(config, PACKAGE, "editor_font", interface_prefs.editor_font);
	g_key_file_set_string(config, PACKAGE, "tagbar_font", interface_prefs.tagbar_font);
	g_key_file_set_string(config, PACKAGE, "msgwin_font", interface_prefs.msgwin_font);
	g_key_file_set_boolean(config, PACKAGE, "show_notebook_tabs", interface_prefs.show_notebook_tabs);
	g_key_file_set_boolean(config, PACKAGE, "show_tab_cross", file_prefs.show_tab_cross);
	g_key_file_set_boolean(config, PACKAGE, "tab_order_ltr", file_prefs.tab_order_ltr);
	g_key_file_set_boolean(config, PACKAGE, "tab_order_beside", file_prefs.tab_order_beside);
	g_key_file_set_integer(config, PACKAGE, "tab_pos_editor", interface_prefs.tab_pos_editor);
	g_key_file_set_integer(config, PACKAGE, "tab_pos_msgwin", interface_prefs.tab_pos_msgwin);
	g_key_file_set_boolean(config, PACKAGE, "use_native_windows_dialogs", interface_prefs.use_native_windows_dialogs);

	/* display */
	g_key_file_set_boolean(config, PACKAGE, "show_indent_guide", editor_prefs.show_indent_guide);
	g_key_file_set_boolean(config, PACKAGE, "show_white_space", editor_prefs.show_white_space);
	g_key_file_set_boolean(config, PACKAGE, "show_line_endings", editor_prefs.show_line_endings);
	g_key_file_set_boolean(config, PACKAGE, "show_markers_margin", editor_prefs.show_markers_margin);
	g_key_file_set_boolean(config, PACKAGE, "show_linenumber_margin", editor_prefs.show_linenumber_margin);
	g_key_file_set_boolean(config, PACKAGE, "long_line_enabled", editor_prefs.long_line_enabled);
	g_key_file_set_integer(config, PACKAGE, "long_line_type", editor_prefs.long_line_type);
	g_key_file_set_integer(config, PACKAGE, "long_line_column", editor_prefs.long_line_column);
	g_key_file_set_string(config, PACKAGE, "long_line_color", editor_prefs.long_line_color);

	/* editor */
	g_key_file_set_integer(config, PACKAGE, "symbolcompletion_max_height", editor_prefs.symbolcompletion_max_height);
	g_key_file_set_integer(config, PACKAGE, "symbolcompletion_min_chars", editor_prefs.symbolcompletion_min_chars);
	g_key_file_set_boolean(config, PACKAGE, "use_folding", editor_prefs.folding);
	g_key_file_set_boolean(config, PACKAGE, "unfold_all_children", editor_prefs.unfold_all_children);
	g_key_file_set_boolean(config, PACKAGE, "use_indicators", editor_prefs.use_indicators);
	g_key_file_set_boolean(config, PACKAGE, "line_wrapping", editor_prefs.line_wrapping);
	g_key_file_set_boolean(config, PACKAGE, "auto_close_xml_tags", editor_prefs.auto_close_xml_tags);
	g_key_file_set_boolean(config, PACKAGE, "complete_snippets", editor_prefs.complete_snippets);
	g_key_file_set_boolean(config, PACKAGE, "auto_complete_symbols", editor_prefs.auto_complete_symbols);
	g_key_file_set_boolean(config, PACKAGE, "pref_editor_disable_dnd", editor_prefs.disable_dnd);
	g_key_file_set_boolean(config, PACKAGE, "pref_editor_smart_home_key", editor_prefs.smart_home_key);
	g_key_file_set_boolean(config, PACKAGE, "pref_editor_newline_strip", editor_prefs.newline_strip);
	g_key_file_set_boolean(config, PACKAGE, "pref_editor_highlight_selection", editor_prefs.highlight_selection);
	g_key_file_set_integer(config, PACKAGE, "line_break_column", editor_prefs.line_break_column);
	g_key_file_set_boolean(config, PACKAGE, "auto_continue_multiline", editor_prefs.auto_continue_multiline);
	g_key_file_set_string(config, PACKAGE, "comment_toggle_mark", editor_prefs.comment_toggle_mark);
	g_key_file_set_boolean(config, PACKAGE, "scroll_stop_at_last_line", editor_prefs.scroll_stop_at_last_line);
	g_key_file_set_integer(config, PACKAGE, "autoclose_chars", editor_prefs.autoclose_chars);

	/* files */
	g_key_file_set_string(config, PACKAGE, "pref_editor_default_new_encoding", encodings[file_prefs.default_new_encoding].charset);
	if (file_prefs.default_open_encoding == -1)
		g_key_file_set_string(config, PACKAGE, "pref_editor_default_open_encoding", "none");
	else
		g_key_file_set_string(config, PACKAGE, "pref_editor_default_open_encoding", encodings[file_prefs.default_open_encoding].charset);
	g_key_file_set_integer(config, PACKAGE, "default_eol_character", file_prefs.default_eol_character);
	g_key_file_set_boolean(config, PACKAGE, "pref_editor_new_line", file_prefs.final_new_line);
	g_key_file_set_boolean(config, PACKAGE, "pref_editor_ensure_convert_line_endings", file_prefs.ensure_convert_new_lines);
	g_key_file_set_boolean(config, PACKAGE, "pref_editor_replace_tabs", file_prefs.replace_tabs);
	g_key_file_set_boolean(config, PACKAGE, "pref_editor_trail_space", file_prefs.strip_trailing_spaces);

	/* toolbar */
	g_key_file_set_boolean(config, PACKAGE, "pref_toolbar_show", toolbar_prefs.visible);
	g_key_file_set_boolean(config, PACKAGE, "pref_toolbar_append_to_menu", toolbar_prefs.append_to_menu);
	g_key_file_set_boolean(config, PACKAGE, "pref_toolbar_use_gtk_default_style", toolbar_prefs.use_gtk_default_style);
	g_key_file_set_boolean(config, PACKAGE, "pref_toolbar_use_gtk_default_icon", toolbar_prefs.use_gtk_default_icon);
	g_key_file_set_integer(config, PACKAGE, "pref_toolbar_icon_style", toolbar_prefs.icon_style);
	g_key_file_set_integer(config, PACKAGE, "pref_toolbar_icon_size", toolbar_prefs.icon_size);

	/* templates */
	g_key_file_set_string(config, PACKAGE, "pref_template_developer", template_prefs.developer);
	g_key_file_set_string(config, PACKAGE, "pref_template_company", template_prefs.company);
	g_key_file_set_string(config, PACKAGE, "pref_template_mail", template_prefs.mail);
	g_key_file_set_string(config, PACKAGE, "pref_template_initial", template_prefs.initials);
	g_key_file_set_string(config, PACKAGE, "pref_template_version", template_prefs.version);
	g_key_file_set_string(config, PACKAGE, "pref_template_year", template_prefs.year_format);
	g_key_file_set_string(config, PACKAGE, "pref_template_date", template_prefs.date_format);
	g_key_file_set_string(config, PACKAGE, "pref_template_datetime", template_prefs.datetime_format);

	/* tools settings */
	g_key_file_set_string(config, "tools", "terminal_cmd", tool_prefs.term_cmd ? tool_prefs.term_cmd : "");
	g_key_file_set_string(config, "tools", "browser_cmd", tool_prefs.browser_cmd ? tool_prefs.browser_cmd : "");
	g_key_file_set_string(config, "tools", "grep_cmd", tool_prefs.grep_cmd ? tool_prefs.grep_cmd : "");
	g_key_file_set_string(config, PACKAGE, "context_action_cmd", tool_prefs.context_action_cmd);

	/* build menu */
	build_save_menu(config, NULL, GEANY_BCS_PREF);

	/* printing */
	g_key_file_set_string(config, "printing", "print_cmd", printing_prefs.external_print_cmd ? printing_prefs.external_print_cmd : "");
	g_key_file_set_boolean(config, "printing", "use_gtk_printing", printing_prefs.use_gtk_printing);
	g_key_file_set_boolean(config, "printing", "print_line_numbers", printing_prefs.print_line_numbers);
	g_key_file_set_boolean(config, "printing", "print_page_numbers", printing_prefs.print_page_numbers);
	g_key_file_set_boolean(config, "printing", "print_page_header", printing_prefs.print_page_header);
	g_key_file_set_boolean(config, "printing", "page_header_basename", printing_prefs.page_header_basename);
	g_key_file_set_string(config, "printing", "page_header_datefmt", printing_prefs.page_header_datefmt);

	/* VTE */
#ifdef HAVE_VTE
	g_key_file_set_boolean(config, "VTE", "load_vte", vte_info.load_vte);
	if (vte_info.have_vte)
	{
		gchar *tmp_string;

		if (!g_key_file_has_key(config, "VTE", "emulation", NULL))	/* hidden */
			g_key_file_set_string(config, "VTE", "emulation", vc->emulation);
		if (!g_key_file_has_key(config, "VTE", "send_selection_unsafe", NULL))	/* hidden */
			g_key_file_set_boolean(config, "VTE", "send_selection_unsafe",
				vc->send_selection_unsafe);
		if (!g_key_file_has_key(config, "VTE", "send_cmd_prefix", NULL))	/* hidden */
			g_key_file_set_string(config, "VTE", "send_cmd_prefix", vc->send_cmd_prefix);
		g_key_file_set_string(config, "VTE", "font", vc->font);
		g_key_file_set_boolean(config, "VTE", "scroll_on_key", vc->scroll_on_key);
		g_key_file_set_boolean(config, "VTE", "scroll_on_out", vc->scroll_on_out);
		g_key_file_set_boolean(config, "VTE", "enable_bash_keys", vc->enable_bash_keys);
		g_key_file_set_boolean(config, "VTE", "ignore_menu_bar_accel", vc->ignore_menu_bar_accel);
		g_key_file_set_boolean(config, "VTE", "follow_path", vc->follow_path);
		g_key_file_set_boolean(config, "VTE", "run_in_vte", vc->run_in_vte);
		g_key_file_set_boolean(config, "VTE", "skip_run_script", vc->skip_run_script);
		g_key_file_set_boolean(config, "VTE", "cursor_blinks", vc->cursor_blinks);
		g_key_file_set_integer(config, "VTE", "scrollback_lines", vc->scrollback_lines);
		g_key_file_set_string(config, "VTE", "font", vc->font);
		g_key_file_set_string(config, "VTE", "image", vc->image);
		g_key_file_set_string(config, "VTE", "shell", vc->shell);
		tmp_string = utils_get_hex_from_color(&vc->colour_fore);
		g_key_file_set_string(config, "VTE", "colour_fore", tmp_string);
		g_free(tmp_string);
		tmp_string = utils_get_hex_from_color(&vc->colour_back);
		g_key_file_set_string(config, "VTE", "colour_back", tmp_string);
		g_free(tmp_string);
	}
#endif
}


static void save_ui_prefs(GKeyFile *config)
{
	g_key_file_set_boolean(config, PACKAGE, "sidebar_visible", ui_prefs.sidebar_visible);
	g_key_file_set_boolean(config, PACKAGE, "statusbar_visible", interface_prefs.statusbar_visible);
	g_key_file_set_boolean(config, PACKAGE, "msgwindow_visible", ui_prefs.msgwindow_visible);
	g_key_file_set_boolean(config, PACKAGE, "fullscreen", ui_prefs.fullscreen);
	g_key_file_set_boolean(config, PACKAGE, "auto_hide_message_bar", interface_prefs.auto_hide_message_bar);

	/* get the text from the scribble textview */
	{
		GtkTextBuffer *buffer;
		GtkTextIter start, end, iter;
		GtkTextMark *mark;

		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(msgwindow.scribble));
		gtk_text_buffer_get_bounds(buffer, &start, &end);
		scribble_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
		g_key_file_set_string(config, PACKAGE, "scribble_text", scribble_text);
		g_free(scribble_text);

		mark = gtk_text_buffer_get_insert(buffer);
		gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
		scribble_pos = gtk_text_iter_get_offset(&iter);
		g_key_file_set_integer(config, PACKAGE, "scribble_pos", scribble_pos);
	}

	if (prefs.save_winpos)
	{
		GdkWindowState wstate;

		g_key_file_set_integer(config, PACKAGE, "treeview_position",
				gtk_paned_get_position(GTK_PANED(ui_lookup_widget(main_widgets.window, "hpaned1"))));
		//g_key_file_set_integer(config, PACKAGE, "msgwindow_position",
		//		gtk_paned_get_position(GTK_PANED(ui_lookup_widget(main_widgets.window, "vpaned2"))));
		g_key_file_set_integer(config, PACKAGE, "msgwindow_position", get_full_message_height() );

		gtk_window_get_position(GTK_WINDOW(main_widgets.window), &ui_prefs.geometry[0], &ui_prefs.geometry[1]);
		gtk_window_get_size(GTK_WINDOW(main_widgets.window), &ui_prefs.geometry[2], &ui_prefs.geometry[3]);
		wstate = gdk_window_get_state(gtk_widget_get_window(main_widgets.window));
		ui_prefs.geometry[4] = (wstate & GDK_WINDOW_STATE_MAXIMIZED) ? 1 : 0;
		g_key_file_set_integer_list(config, PACKAGE, "geometry", ui_prefs.geometry, 5);
	}

	g_key_file_set_string(config, PACKAGE, "custom_date_format", ui_prefs.custom_date_format);
	if (ui_prefs.custom_commands != NULL)
	{
		g_key_file_set_string_list(config, PACKAGE, "custom_commands",
				(const gchar**) ui_prefs.custom_commands, g_strv_length(ui_prefs.custom_commands));
		g_key_file_set_string_list(config, PACKAGE, "custom_commands_labels",
				(const gchar**) ui_prefs.custom_commands_labels, g_strv_length(ui_prefs.custom_commands_labels));
	}
}


void configuration_save(void)
{
	GKeyFile *config = g_key_file_new();
	gchar *configfile = g_build_filename(app->configdir, "geany.conf", NULL);
	gchar *data;

	g_key_file_load_from_file(config, configfile, G_KEY_FILE_NONE, NULL);

	/* this signal can be used e.g. to prepare any settings before Stash code reads them below */
	g_signal_emit_by_name(geany_object, "save-settings", config);

	save_dialog_prefs(config);
	save_ui_prefs(config);
	save_install_prefs(config);
	build_save_prefs(config);
	project_save_prefs(config);	/* save project filename, etc. */
	save_recent_files(config, ui_prefs.recent_queue, "recent_files");
	save_recent_files(config, ui_prefs.recent_projects_queue, "recent_projects");

	if ( prefs.remember_projects ) configuration_save_open_projects(config);

	/*
	if (cl_options.load_session)
		configuration_save_session_files(config);
#ifdef HAVE_VTE
	else if (vte_info.have_vte)
	{
		vte_get_working_directory();	// refresh vte_info.dir
		g_key_file_set_string(config, "VTE", "last_dir", vte_info.dir);
	}
#endif
	*/

	/* write the file */
	data = g_key_file_to_data(config, NULL, NULL);
	utils_write_file(configfile, data);
	g_free(data);

	g_key_file_free(config);
	g_free(configfile);
}


static void load_recent_files(GKeyFile *config, GQueue *queue, const gchar *key)
{
	gchar **recent_files;
	gsize i, len = 0;

	recent_files = g_key_file_get_string_list(config, "files", key, &len, NULL);
	if (recent_files != NULL)
	{
		for (i = 0; (i < len) && (i < file_prefs.mru_length); i++)
		{
			gchar *filename = g_strdup(recent_files[i]);
			g_queue_push_tail(queue, filename);
		}
		g_strfreev(recent_files);
	}
}

void configuration_load_recent_files(GKeyFile *config)
{
	load_recent_files(config, ui_prefs.recent_queue, "recent_files");
	load_recent_files(config, ui_prefs.recent_projects_queue, "recent_projects");
}

void configuration_load_open_projects(GKeyFile *config)
{
	guint i;
	gboolean have_session_files;
	gchar entry[30];
	gchar **tmp_array;
	GError *error = NULL;

	have_session_files = TRUE;
	i = 0;
	while (have_session_files)
	{
		g_snprintf(entry, sizeof(entry), "PROJECT_FILE_NAME_%d", i);
		tmp_array = g_key_file_get_string_list(config, "projects", entry, NULL, &error);
		if (! tmp_array || error)
		{
			g_error_free(error);
			error = NULL;
			have_session_files = FALSE;
		}
		else
		{
			gchar* unescaped_filename = g_uri_unescape_string(tmp_array[1], NULL);
			gchar* locale_filename = utils_get_locale_from_utf8(unescaped_filename);

			GeanyProject *curr_project = app->project;
			project_load_file(locale_filename);
			if ( atoi(tmp_array[0]) == 0 ) 
				app->project = curr_project;

			g_free(locale_filename);
			g_free(unescaped_filename);
			g_strfreev(tmp_array);
		}
		i++;
	}

	if ( app->project == NULL ) app->project = project_find_first_valid();
	project_update_list();
}

/*
 * Load session list from the given keyfile, and store it in the global
 * session_files variable for later file loading
 * */
void configuration_load_session_files(GKeyFile *config, GeanyProject *project)
{
	guint i;
	gboolean have_session_files;
	gchar entry[16];
	gchar **tmp_array;
	GError *error = NULL;

	//session_notebook_page = utils_get_setting_integer(config, "files", "current_page", -1);

	/* the project may load another list than the main setting */
	if (session_files != NULL)
		g_ptr_array_free(session_files, TRUE);

	session_files = g_ptr_array_new();
	have_session_files = TRUE;
	i = 0;
	while (have_session_files)
	{
		g_snprintf(entry, sizeof(entry), "FILE_NAME_%d", i);
		tmp_array = g_key_file_get_string_list(config, "files", entry, NULL, &error);
		if (! tmp_array || error)
		{
			g_error_free(error);
			error = NULL;
			have_session_files = FALSE;
		}
		g_ptr_array_add(session_files, tmp_array);
		i++;
	}

#ifdef HAVE_VTE
	/* BUG: after loading project at startup, closing project doesn't restore old VTE path */
	if (vte_info.have_vte)
	{
		gchar *tmp_string = utils_get_setting_string(config, "VTE", "last_dir", NULL);
		vte_cwd(tmp_string,TRUE);
		g_free(tmp_string);
	}
#endif

	// open the files straight away, we're not going to overwrite the list
	configuration_open_files( project );
}

void configuration_load_project_files(GKeyFile *config, GeanyProject *project)
{
	guint i, j;
	gchar entry[16];
	gchar **tmp_array;
	GError *error = NULL;

	
	/* the project may load another list than the main setting */
	if (project->project_groups != NULL)
		g_ptr_array_free(project->project_groups, TRUE);

	if (project->project_files != NULL)
		g_ptr_array_free(project->project_files, TRUE);

	project->project_groups = g_ptr_array_new();
	project->project_files = g_ptr_array_new();
	
	i = 0;
	while (1)
	{
		g_snprintf(entry, sizeof(entry), "GROUP_%d", i);
		tmp_array = g_key_file_get_string_list(config, "projectgroups", entry, NULL, &error);
		if (! tmp_array || error)
		{
			g_error_free(error);
			error = NULL;
			break;
		}
		
		gint new_idx = project_get_new_group_idx( project );
		if (new_idx == -1)	/* expand the array, no free places */
		{
			GeanyProjectGroup *group = g_new0(GeanyProjectGroup, 1);

			new_idx = project->project_groups->len;
			g_ptr_array_add(project->project_groups, group);
		}

		GeanyProjectGroup *group = project->project_groups->pdata[new_idx];
		group->is_valid = TRUE;
		group->group_name = g_strdup( tmp_array[0] );
		group->index = new_idx;
		group->pProject = project;
		group->pParent = 0;
		gint index = atoi( tmp_array[1] );
		if ( index > 0 && index < project->project_groups->len ) 
			group->pParent = project->project_groups->pdata[index];

		g_strfreev(tmp_array);
		i++;
	}

	i = 0;
	while (1)
	{
		g_snprintf(entry, sizeof(entry), "FILE_%d", i);
		tmp_array = g_key_file_get_string_list(config, "projectfiles", entry, NULL, &error);
		if (! tmp_array || error)
		{
			g_error_free(error);
			error = NULL;
			break;
		}

		gint new_idx = project_get_new_file_idx( project );
		if (new_idx == -1)	/* expand the array, no free places */
		{
			GeanyProjectFile *file = g_new0(GeanyProjectFile, 1);

			new_idx = project->project_files->len;
			g_ptr_array_add(project->project_files, file);
		}

		gchar* unescaped_filename = g_uri_unescape_string(tmp_array[0], NULL);
		gchar* locale_filename = utils_get_locale_from_utf8(unescaped_filename);

		GeanyProjectFile *file = project->project_files->pdata[new_idx];
		
		if ( !g_path_is_absolute(locale_filename) )
		{
			file->file_name = g_build_filename( project->base_path, locale_filename, NULL );
			//file->file_name = utils_get_real_path( full_path );
			utils_tidy_path( file->file_name );
			//g_free(full_path);
		}
		else
			file->file_name = g_strdup( locale_filename );

		gboolean bFound = FALSE;
		for (j = 0; j < project->project_files->len; j++)
		{
			GeanyProjectFile *file2 = project->project_files->pdata[j];
			if (file2 != NULL && file2->is_valid)
			{
				if ( strcmp( file2->file_name, file->file_name ) == 0 ) 
				{
					g_free(file->file_name);
					file->file_name = 0;
					bFound = TRUE;
					break;
				}
			}
		}

		if ( bFound ) 
		{
			i++;
			g_free(locale_filename);
			g_free(unescaped_filename);
			g_strfreev(tmp_array);
			continue;
		}

		file->is_valid = TRUE;

		gint index = atoi( tmp_array[1] );
		if ( index > 0 && index < project->project_groups->len ) 
			file->pParent = project->project_groups->pdata[index];

		g_free(locale_filename);
		g_free(unescaped_filename);
		g_strfreev(tmp_array);
		i++;
	}
}

#ifdef HAVE_VTE
static void get_setting_color(GKeyFile *config, const gchar *section, const gchar *key,
		GdkColor *color, const gchar *default_color)
{
	gchar *str = utils_get_setting_string(config, section, key, NULL);
	if (str == NULL || ! utils_parse_color(str, color))
		utils_parse_color(default_color, color);
	g_free(str);
}
#endif

static void load_install_prefs(GKeyFile *config)
{
	install_prefs.projects_folder = utils_get_setting_string(config, "AGKInstall", "projects_folder", NULL);
	install_prefs.update_projects_mode = utils_get_setting_integer(config, "AGKInstall", "projects_update", -1);
	install_prefs.tier2_folder = utils_get_setting_string(config, "AGKInstall", "libraries_folder", NULL);
	install_prefs.update_tier2_mode = utils_get_setting_integer(config, "AGKInstall", "libraries_update", -1);
}

/* note: new settings should be added in init_pref_groups() */
static void load_dialog_prefs(GKeyFile *config)
{
	gchar *tmp_string, *tmp_string2;
	const gchar *default_charset = NULL;
	gchar *cmd;

	/* compatibility with Geany 0.20 */
	if (!g_key_file_has_key(config, PACKAGE, atomic_file_saving_key, NULL))
	{
		g_key_file_set_boolean(config, PACKAGE, atomic_file_saving_key,
			utils_get_setting_boolean(config, PACKAGE, "use_safe_file_saving", FALSE));
	}

	/* compatibility with Geany 0.21 */
	{
		gboolean suppress_search_dialogs = utils_get_setting_boolean(config, PACKAGE, "pref_main_suppress_search_dialogs", FALSE);

		if (!g_key_file_has_key(config, "search", "pref_search_always_wrap", NULL))
			g_key_file_set_boolean(config, "search", "pref_search_always_wrap", suppress_search_dialogs);

		if (!g_key_file_has_key(config, "search", "pref_search_hide_find_dialog", NULL))
			g_key_file_set_boolean(config, "search", "pref_search_hide_find_dialog", suppress_search_dialogs);
	}

	/* read stash prefs */
	settings_action(config, SETTING_READ);

	/* general */
	prefs.confirm_exit = utils_get_setting_boolean(config, PACKAGE, "pref_main_confirm_exit", FALSE);
	prefs.remember_projects = utils_get_setting_boolean(config, PACKAGE, "pref_remember_projects", TRUE);
	prefs.suppress_status_messages = utils_get_setting_boolean(config, PACKAGE, "pref_main_suppress_status_messages", FALSE);
	//prefs.load_session = utils_get_setting_boolean(config, PACKAGE, "pref_main_load_session", TRUE);
	prefs.load_session = FALSE;
	//project_prefs.project_session = utils_get_setting_boolean(config, PACKAGE, "pref_main_project_session", TRUE);
	//project_prefs.project_file_in_basedir = utils_get_setting_boolean(config, PACKAGE, "pref_main_project_file_in_basedir", FALSE);
	project_prefs.project_session = TRUE;
	project_prefs.project_file_in_basedir = TRUE;
	prefs.save_winpos = utils_get_setting_boolean(config, PACKAGE, "pref_main_save_winpos", TRUE);
	prefs.beep_on_errors = utils_get_setting_boolean(config, PACKAGE, "beep_on_errors", TRUE);
	prefs.switch_to_status = utils_get_setting_boolean(config, PACKAGE, "switch_msgwin_pages", FALSE);
	prefs.auto_focus = utils_get_setting_boolean(config, PACKAGE, "auto_focus", FALSE);

	/* interface */
	interface_prefs.tab_pos_editor = utils_get_setting_integer(config, PACKAGE, "tab_pos_editor", GTK_POS_TOP);
	interface_prefs.tab_pos_msgwin = utils_get_setting_integer(config, PACKAGE, "tab_pos_msgwin",GTK_POS_LEFT);
	interface_prefs.sidebar_symbol_visible = utils_get_setting_boolean(config, PACKAGE, "sidebar_symbol_visible", TRUE);
	interface_prefs.sidebar_openfiles_visible = utils_get_setting_boolean(config, PACKAGE, "sidebar_openfiles_visible", TRUE);
	interface_prefs.statusbar_visible = utils_get_setting_boolean(config, PACKAGE, "statusbar_visible", TRUE);
	interface_prefs.auto_hide_message_bar = utils_get_setting_boolean(config, PACKAGE, "auto_hide_message_bar", TRUE);
	file_prefs.tab_order_ltr = utils_get_setting_boolean(config, PACKAGE, "tab_order_ltr", TRUE);
	file_prefs.tab_order_beside = utils_get_setting_boolean(config, PACKAGE, "tab_order_beside", FALSE);
	interface_prefs.show_notebook_tabs = utils_get_setting_boolean(config, PACKAGE, "show_notebook_tabs", TRUE);
	file_prefs.show_tab_cross = utils_get_setting_boolean(config, PACKAGE, "show_tab_cross", TRUE);
	interface_prefs.editor_font = utils_get_setting_string(config, PACKAGE, "editor_font", GEANY_DEFAULT_FONT_EDITOR);
	interface_prefs.tagbar_font = utils_get_setting_string(config, PACKAGE, "tagbar_font", GEANY_DEFAULT_FONT_SYMBOL_LIST);
	interface_prefs.msgwin_font = utils_get_setting_string(config, PACKAGE, "msgwin_font", GEANY_DEFAULT_FONT_MSG_WINDOW);
	interface_prefs.use_native_windows_dialogs = utils_get_setting_boolean(config, PACKAGE, "use_native_windows_dialogs", TRUE);

	if ( interface_prefs.auto_hide_message_bar )
		interface_prefs.tab_pos_msgwin = GTK_POS_TOP;

	/* display, editor */
	editor_prefs.long_line_enabled = utils_get_setting_boolean(config, PACKAGE, "long_line_enabled", FALSE);
	editor_prefs.long_line_type = utils_get_setting_integer(config, PACKAGE, "long_line_type", 0);
	if (editor_prefs.long_line_type == 2) /* backward compatibility */
	{
		editor_prefs.long_line_type = 0;
		editor_prefs.long_line_enabled = FALSE;
	}
	editor_prefs.long_line_color = utils_get_setting_string(config, PACKAGE, "long_line_color", "#C2EBC2");
	editor_prefs.long_line_column = utils_get_setting_integer(config, PACKAGE, "long_line_column", 72);
	editor_prefs.symbolcompletion_min_chars = utils_get_setting_integer(config, PACKAGE, "symbolcompletion_min_chars", GEANY_MIN_SYMBOLLIST_CHARS);
	editor_prefs.symbolcompletion_max_height = utils_get_setting_integer(config, PACKAGE, "symbolcompletion_max_height", GEANY_MAX_SYMBOLLIST_HEIGHT);
	editor_prefs.line_wrapping = utils_get_setting_boolean(config, PACKAGE, "line_wrapping", FALSE); /* default is off for better performance */
	editor_prefs.use_indicators = utils_get_setting_boolean(config, PACKAGE, "use_indicators", TRUE);
	editor_prefs.show_indent_guide = utils_get_setting_boolean(config, PACKAGE, "show_indent_guide", FALSE);
	editor_prefs.show_white_space = utils_get_setting_boolean(config, PACKAGE, "show_white_space", FALSE);
	editor_prefs.show_line_endings = utils_get_setting_boolean(config, PACKAGE, "show_line_endings", FALSE);
	editor_prefs.scroll_stop_at_last_line = utils_get_setting_boolean(config, PACKAGE, "scroll_stop_at_last_line", FALSE);

	editor_prefs.IDE_version = utils_get_setting_integer(config, PACKAGE, "IDE_version_int", 0);
	if ( editor_prefs.IDE_version < 1 )
		editor_prefs.scroll_stop_at_last_line = FALSE;

	editor_prefs.auto_close_xml_tags = utils_get_setting_boolean(config, PACKAGE, "auto_close_xml_tags", TRUE);
	editor_prefs.complete_snippets = utils_get_setting_boolean(config, PACKAGE, "complete_snippets", TRUE);
	editor_prefs.auto_complete_symbols = utils_get_setting_boolean(config, PACKAGE, "auto_complete_symbols", TRUE);
	editor_prefs.folding = utils_get_setting_boolean(config, PACKAGE, "use_folding", TRUE);
	editor_prefs.unfold_all_children = utils_get_setting_boolean(config, PACKAGE, "unfold_all_children", FALSE);
	editor_prefs.show_markers_margin = utils_get_setting_boolean(config, PACKAGE, "show_markers_margin", TRUE);
	editor_prefs.show_linenumber_margin = utils_get_setting_boolean(config, PACKAGE, "show_linenumber_margin", TRUE);
	editor_prefs.disable_dnd = utils_get_setting_boolean(config, PACKAGE, "pref_editor_disable_dnd", FALSE);
	editor_prefs.smart_home_key = utils_get_setting_boolean(config, PACKAGE, "pref_editor_smart_home_key", TRUE);
	editor_prefs.newline_strip = utils_get_setting_boolean(config, PACKAGE, "pref_editor_newline_strip", FALSE);
	editor_prefs.highlight_selection = utils_get_setting_boolean(config, PACKAGE, "pref_editor_highlight_selection", TRUE);
	editor_prefs.line_break_column = utils_get_setting_integer(config, PACKAGE, "line_break_column", 72);
	editor_prefs.auto_continue_multiline = utils_get_setting_boolean(config, PACKAGE, "auto_continue_multiline", TRUE);
	editor_prefs.comment_toggle_mark = utils_get_setting_string(config, PACKAGE, "comment_toggle_mark", GEANY_TOGGLE_MARK);
	editor_prefs.autoclose_chars = utils_get_setting_integer(config, PACKAGE, "autoclose_chars", 0);

	/* Files
	 * use current locale encoding as default for new files (should be UTF-8 in most cases) */
	g_get_charset(&default_charset);
	//tmp_string = utils_get_setting_string(config, PACKAGE, "pref_editor_default_new_encoding", "UTF-8");
	// from version 21 default is UTF-8
	tmp_string = g_strdup("UTF-8");
	
	if (tmp_string)
	{
		const GeanyEncoding *enc = encodings_get_from_charset(tmp_string);
		if (enc != NULL)
			file_prefs.default_new_encoding = enc->idx;
		else
			file_prefs.default_new_encoding = GEANY_ENCODING_UTF_8;

		g_free(tmp_string);
	}

	//tmp_string = utils_get_setting_string(config, PACKAGE, "pref_editor_default_open_encoding", "UTF-8");
	// from version 21 default is UTF-8
	tmp_string = g_strdup("UTF-8");

	if (tmp_string)
	{
		const GeanyEncoding *enc = encodings_get_from_charset(tmp_string);
		if (enc != NULL)
			file_prefs.default_open_encoding = enc->idx;
		else
			file_prefs.default_open_encoding = -1;

		g_free(tmp_string);
	}
	file_prefs.default_eol_character = utils_get_setting_integer(config, PACKAGE, "default_eol_character", GEANY_DEFAULT_EOL_CHARACTER);
	file_prefs.replace_tabs = utils_get_setting_boolean(config, PACKAGE, "pref_editor_replace_tabs", FALSE);
	file_prefs.ensure_convert_new_lines = utils_get_setting_boolean(config, PACKAGE, "pref_editor_ensure_convert_line_endings", FALSE);
	file_prefs.final_new_line = utils_get_setting_boolean(config, PACKAGE, "pref_editor_new_line", TRUE);
	file_prefs.strip_trailing_spaces = utils_get_setting_boolean(config, PACKAGE, "pref_editor_trail_space", FALSE);

	/* toolbar */
	toolbar_prefs.visible = utils_get_setting_boolean(config, PACKAGE, "pref_toolbar_show", TRUE);
	toolbar_prefs.append_to_menu = utils_get_setting_boolean(config, PACKAGE, "pref_toolbar_append_to_menu", FALSE);
	{
		toolbar_prefs.use_gtk_default_style = utils_get_setting_boolean(config, PACKAGE, "pref_toolbar_use_gtk_default_style", FALSE);
		if (! toolbar_prefs.use_gtk_default_style)
			toolbar_prefs.icon_style = utils_get_setting_integer(config, PACKAGE, "pref_toolbar_icon_style", GTK_TOOLBAR_BOTH);

		toolbar_prefs.use_gtk_default_icon = utils_get_setting_boolean(config, PACKAGE, "pref_toolbar_use_gtk_default_icon", FALSE);
		if (! toolbar_prefs.use_gtk_default_icon)
			toolbar_prefs.icon_size = utils_get_setting_integer(config, PACKAGE, "pref_toolbar_icon_size", GTK_ICON_SIZE_SMALL_TOOLBAR);
	}

	/* VTE */
#ifdef HAVE_VTE
	vte_info.load_vte = utils_get_setting_boolean(config, "VTE", "load_vte", TRUE);
	if (vte_info.load_vte)
	{
		struct passwd *pw = getpwuid(getuid());
		const gchar *shell = (pw != NULL) ? pw->pw_shell : "/bin/sh";

		vc = g_new0(VteConfig, 1);
		vte_info.dir = utils_get_setting_string(config, "VTE", "last_dir", NULL);
		if ((vte_info.dir == NULL || utils_str_equal(vte_info.dir, "")) && pw != NULL)
			/* last dir is not set, fallback to user's home directory */
			SETPTR(vte_info.dir, g_strdup(pw->pw_dir));
		else if (vte_info.dir == NULL && pw == NULL)
			/* fallback to root */
			vte_info.dir = g_strdup("/");

		vc->emulation = utils_get_setting_string(config, "VTE", "emulation", "xterm");
		vc->send_selection_unsafe = utils_get_setting_boolean(config, "VTE",
			"send_selection_unsafe", FALSE);
		vc->image = utils_get_setting_string(config, "VTE", "image", "");
		vc->shell = utils_get_setting_string(config, "VTE", "shell", shell);
		vc->font = utils_get_setting_string(config, "VTE", "font", "Monospace 10");
		vc->scroll_on_key = utils_get_setting_boolean(config, "VTE", "scroll_on_key", TRUE);
		vc->scroll_on_out = utils_get_setting_boolean(config, "VTE", "scroll_on_out", TRUE);
		vc->enable_bash_keys = utils_get_setting_boolean(config, "VTE", "enable_bash_keys", TRUE);
		vc->ignore_menu_bar_accel = utils_get_setting_boolean(config, "VTE", "ignore_menu_bar_accel", FALSE);
		vc->follow_path = utils_get_setting_boolean(config, "VTE", "follow_path", FALSE);
		vc->send_cmd_prefix = utils_get_setting_string(config, "VTE", "send_cmd_prefix", "");
		vc->run_in_vte = utils_get_setting_boolean(config, "VTE", "run_in_vte", FALSE);
		vc->skip_run_script = utils_get_setting_boolean(config, "VTE", "skip_run_script", FALSE);
		vc->cursor_blinks = utils_get_setting_boolean(config, "VTE", "cursor_blinks", FALSE);
		vc->scrollback_lines = utils_get_setting_integer(config, "VTE", "scrollback_lines", 500);
		get_setting_color(config, "VTE", "colour_fore", &vc->colour_fore, "#ffffff");
		get_setting_color(config, "VTE", "colour_back", &vc->colour_back, "#000000");
	}
#endif
	/* templates */
	template_prefs.developer = utils_get_setting_string(config, PACKAGE, "pref_template_developer", g_get_real_name());
	template_prefs.company = utils_get_setting_string(config, PACKAGE, "pref_template_company", "");
	tmp_string = utils_get_initials(template_prefs.developer);
	template_prefs.initials = utils_get_setting_string(config, PACKAGE, "pref_template_initial", tmp_string);
	g_free(tmp_string);

	template_prefs.version = utils_get_setting_string(config, PACKAGE, "pref_template_version", "1.0");

	tmp_string = g_strdup_printf("%s@%s", g_get_user_name(), g_get_host_name());
	template_prefs.mail = utils_get_setting_string(config, PACKAGE, "pref_template_mail", tmp_string);
	g_free(tmp_string);
	template_prefs.year_format = utils_get_setting_string(config, PACKAGE, "pref_template_year", GEANY_TEMPLATES_FORMAT_YEAR);
	template_prefs.date_format = utils_get_setting_string(config, PACKAGE, "pref_template_date", GEANY_TEMPLATES_FORMAT_DATE);
	template_prefs.datetime_format = utils_get_setting_string(config, PACKAGE, "pref_template_datetime", GEANY_TEMPLATES_FORMAT_DATETIME);

	/* tools */
	cmd = utils_get_setting_string(config, "tools", "terminal_cmd", "");
	if (EMPTY(cmd))
	{
		SETPTR(cmd, utils_get_setting_string(config, "tools", "term_cmd", ""));
		if (!EMPTY(cmd))
		{
			tmp_string = cmd;
#ifdef G_OS_WIN32
			if (strstr(cmd, "cmd.exe"))
				cmd = g_strconcat(cmd, " /Q /C %c", NULL);
			else
				cmd = g_strconcat(cmd, " %c", NULL);
#else
			cmd = g_strconcat(cmd, " -e \"/bin/sh %c\"", NULL);
#endif
			g_free(tmp_string);
		}
		else
			SETPTR(cmd, g_strdup(GEANY_DEFAULT_TOOLS_TERMINAL));
	}
	tool_prefs.term_cmd = cmd;
	tool_prefs.browser_cmd = utils_get_setting_string(config, "tools", "browser_cmd", GEANY_DEFAULT_TOOLS_BROWSER);
	tool_prefs.grep_cmd = utils_get_setting_string(config, "tools", "grep_cmd", GEANY_DEFAULT_TOOLS_GREP);

	tool_prefs.context_action_cmd = utils_get_setting_string(config, PACKAGE, "context_action_cmd", "");

	/* build menu */
	build_set_group_count(GEANY_GBG_FT, build_menu_prefs.number_ft_menu_items);
	build_set_group_count(GEANY_GBG_NON_FT, build_menu_prefs.number_non_ft_menu_items);
	build_set_group_count(GEANY_GBG_EXEC, build_menu_prefs.number_exec_menu_items);
	build_load_menu(config, GEANY_BCS_PREF, NULL);

	/* printing */
	tmp_string2 = g_find_program_in_path(GEANY_DEFAULT_TOOLS_PRINTCMD);
#ifdef G_OS_WIN32
	if (!EMPTY(tmp_string2))
	{
		/* single quote paths on Win32 for g_spawn_command_line_async */
		tmp_string = g_strconcat("'", tmp_string2, "' '%f'", NULL);
	}
	else
	{
		tmp_string = g_strdup("");
	}
#else
	tmp_string = g_strconcat(tmp_string2, " %f", NULL);
#endif
	printing_prefs.external_print_cmd = utils_get_setting_string(config, "printing", "print_cmd", tmp_string);
	g_free(tmp_string);
	g_free(tmp_string2);

	printing_prefs.use_gtk_printing = utils_get_setting_boolean(config, "printing", "use_gtk_printing", TRUE);
	printing_prefs.print_line_numbers = utils_get_setting_boolean(config, "printing", "print_line_numbers", TRUE);
	printing_prefs.print_page_numbers = utils_get_setting_boolean(config, "printing", "print_page_numbers", TRUE);
	printing_prefs.print_page_header = utils_get_setting_boolean(config, "printing", "print_page_header", TRUE);
	printing_prefs.page_header_basename = utils_get_setting_boolean(config, "printing", "page_header_basename", FALSE);
	printing_prefs.page_header_datefmt = utils_get_setting_string(config, "printing", "page_header_datefmt", "%c");
}


static void load_ui_prefs(GKeyFile *config)
{
	gint *geo;
	gsize geo_len;

	ui_prefs.sidebar_visible = utils_get_setting_boolean(config, PACKAGE, "sidebar_visible", TRUE);
	ui_prefs.msgwindow_visible = utils_get_setting_boolean(config, PACKAGE, "msgwindow_visible", TRUE);
	ui_prefs.fullscreen = utils_get_setting_boolean(config, PACKAGE, "fullscreen", FALSE);
	ui_prefs.custom_date_format = utils_get_setting_string(config, PACKAGE, "custom_date_format", "");
	ui_prefs.custom_commands = g_key_file_get_string_list(config, PACKAGE, "custom_commands", NULL, NULL);
	ui_prefs.custom_commands_labels = g_key_file_get_string_list(config, PACKAGE, "custom_commands_labels", NULL, NULL);

	/* sanitize custom commands labels */
	if (ui_prefs.custom_commands || ui_prefs.custom_commands_labels)
	{
		guint i;
		guint cc_len = ui_prefs.custom_commands ? g_strv_length(ui_prefs.custom_commands) : 0;
		guint cc_labels_len = ui_prefs.custom_commands_labels ? g_strv_length(ui_prefs.custom_commands_labels) : 0;

		/* not enough items, resize and fill */
		if (cc_labels_len < cc_len)
		{
			ui_prefs.custom_commands_labels = g_realloc(ui_prefs.custom_commands_labels,
					(cc_len + 1) * sizeof *ui_prefs.custom_commands_labels);
			for (i = cc_labels_len; i < cc_len; i++)
				ui_prefs.custom_commands_labels[i] = g_strdup("");
			ui_prefs.custom_commands_labels[cc_len] = NULL;
		}
		/* too many items, cut off */
		else if (cc_labels_len > cc_len)
		{
			for (i = cc_len; i < cc_labels_len; i++)
			{
				g_free(ui_prefs.custom_commands_labels[i]);
				ui_prefs.custom_commands_labels[i] = NULL;
			}
		}
	}

	scribble_text = utils_get_setting_string(config, PACKAGE, "scribble_text",
				_("Type here what you want, use it as a notice/scratch board"));
	scribble_pos = utils_get_setting_integer(config, PACKAGE, "scribble_pos", -1);

	geo = g_key_file_get_integer_list(config, PACKAGE, "geometry", &geo_len, NULL);
	if (! geo || geo_len < 5)
	{
		ui_prefs.geometry[0] = -1;
		ui_prefs.geometry[1] = -1;
		ui_prefs.geometry[2] = -1;
		ui_prefs.geometry[3] = -1;
		ui_prefs.geometry[4] = 0;
	}
	else
	{
		/* don't use insane values but when main windows was maximized last time, pos might be
		 * negative (due to differences in root window and window decorations) */
		/* quitting when minimized can make pos -32000, -32000 on Windows! */
		ui_prefs.geometry[0] = MAX(-1, geo[0]);
		ui_prefs.geometry[1] = MAX(-1, geo[1]);
		ui_prefs.geometry[2] = MAX(-1, geo[2]);
		ui_prefs.geometry[3] = MAX(-1, geo[3]);
		ui_prefs.geometry[4] = geo[4] != 0;
	}
	hpan_position = utils_get_setting_integer(config, PACKAGE, "treeview_position", 156);
	vpan_position = utils_get_setting_integer(config, PACKAGE, "msgwindow_position", 200);

	g_free(geo);
}


/*
 * Save current session in default configuration file
 */
void configuration_save_default_session(void)
{
	// only project files should load other files
	return;

	/*
	gchar *configfile = g_build_filename(app->configdir, "geany.conf", NULL);
	gchar *data;
	GKeyFile *config = g_key_file_new();

	g_key_file_load_from_file(config, configfile, G_KEY_FILE_NONE, NULL);

	
	if (cl_options.load_session)
		configuration_save_session_files(config);

	// write the file
	data = g_key_file_to_data(config, NULL, NULL);
	utils_write_file(configfile, data);
	g_free(data);

	g_key_file_free(config);
	g_free(configfile);
	*/
}


/*
 * Only reload the session part of the default configuration
 */
void configuration_reload_default_session(void)
{
	/*
	gchar *configfile = g_build_filename(app->configdir, "geany.conf", NULL);
	GKeyFile *config = g_key_file_new();

	g_key_file_load_from_file(config, configfile, G_KEY_FILE_NONE, NULL);
	g_free(configfile);

	configuration_load_session_files(config, FALSE);

	g_key_file_free(config);
	*/
}


gboolean configuration_load(void)
{
	gchar *configfile = g_build_filename(app->configdir, "geany.conf", NULL);
	GKeyFile *config = g_key_file_new();

	if (! g_file_test(configfile, G_FILE_TEST_IS_REGULAR))
	{	/* config file does not (yet) exist, so try to load a global config file which may be */
		/* created by distributors */
		geany_debug("No user config file found, trying to use global configuration.");
		SETPTR(configfile, g_build_filename(app->datadir, "geany.conf", NULL));
	}
	g_key_file_load_from_file(config, configfile, G_KEY_FILE_NONE, NULL);
	g_free(configfile);

	load_dialog_prefs(config);
	load_ui_prefs(config);
	load_install_prefs(config);
	project_load_prefs(config);
	build_load_prefs(config);
	configuration_load_recent_files(config);

	/* this signal can be used e.g. to delay building UI elements until settings have been read */
	g_signal_emit_by_name(geany_object, "load-settings", config);

	g_key_file_free(config);
	return TRUE;
}

gboolean configuration_load_projects(void)
{
	if ( !prefs.remember_projects ) return TRUE;

	gchar *configfile = g_build_filename(app->configdir, "geany.conf", NULL);
	GKeyFile *config = g_key_file_new();

	if (! g_file_test(configfile, G_FILE_TEST_IS_REGULAR))
	{	/* config file does not (yet) exist, so try to load a global config file which may be */
		/* created by distributors */
		geany_debug("No user config file found, trying to use global configuration.");
		SETPTR(configfile, g_build_filename(app->datadir, "geany.conf", NULL));
	}
	g_key_file_load_from_file(config, configfile, G_KEY_FILE_NONE, NULL);
	g_free(configfile);

	configuration_load_open_projects(config);

	g_key_file_free(config);
	return TRUE;
}

static gboolean open_session_file(gchar **tmp, guint len)
{
	guint pos;
	const gchar *ft_name;
	gchar *locale_filename;
	gchar *unescaped_filename;
	const gchar *encoding;
	gint  indent_type;
	gboolean ro, auto_indent, line_wrapping;
	/** TODO when we have a global pref for line breaking, use its value */
	gboolean line_breaking = FALSE;
	gboolean ret = FALSE;

	pos = atoi(tmp[0]);
	ft_name = tmp[1];
	ro = atoi(tmp[2]);
	if (isdigit(tmp[3][0]))
	{
		encoding = encodings_get_charset_from_index(atoi(tmp[3]));
	}
	else
	{
		encoding = &(tmp[3][1]);
	}
	/* try to get the locale equivalent for the filename */
	unescaped_filename = g_uri_unescape_string(tmp[7], NULL);
	locale_filename = utils_get_locale_from_utf8(unescaped_filename);
	if ( !g_path_is_absolute(locale_filename) )
	{
		// todo build absolute path from relative path
	}

	if (g_file_test(locale_filename, G_FILE_TEST_IS_REGULAR))
	{
		GeanyFiletype *ft = filetypes_lookup_by_name(ft_name);
		GeanyDocument *doc = document_open_file_full( NULL, locale_filename, pos, ro, ft, encoding );
	}
	else
	{
		geany_debug("Could not find file '%s'.", tmp[7]);
	}

	g_free(locale_filename);
	g_free(unescaped_filename);
	return ret;
}

static gint session_file_compare(gconstpointer a, gconstpointer b)
{
	gchar **tmpA = *((gchar***) a);
	gchar **tmpB = *((gchar***) b);

	int orderA = 0;
	int orderB = 0;

	if ( tmpA && g_strv_length(tmpA) > 3 ) orderA = atoi(tmpA[3]);
	if ( tmpB && g_strv_length(tmpB) > 3 ) orderB = atoi(tmpB[3]);

	if ( orderA < orderB ) return -1;
	else if ( orderA > orderB ) return 1;
	else return 0;
}

/* Open session files
 * Note: notebook page switch handler and adding to recent files list is always disabled
 * for all files opened within this function */
void configuration_open_files(GeanyProject *project)
{
	gint i;
	gboolean failure = FALSE;

	/* necessary to set it to TRUE for project session support */
	main_status.opening_session_files = TRUE;

	GeanyDocument *set_curr_doc = NULL;

	g_ptr_array_sort( session_files, session_file_compare );

	i = file_prefs.tab_order_ltr ? 0 : (session_files->len - 1);
	while (TRUE)
	{
		gchar **tmp = g_ptr_array_index(session_files, i);
		guint len;

		if (tmp != NULL && g_strv_length(tmp) >= 2)
		{
			gchar* unescaped_filename = g_uri_unescape_string(tmp[1], NULL);
			gchar* locale_filename = utils_get_locale_from_utf8(unescaped_filename);
			if ( !g_path_is_absolute(locale_filename) )
			{
				SETPTR(locale_filename, g_build_filename( project->base_path, locale_filename, NULL ));
				utils_tidy_path( locale_filename );
			}

			if (g_file_test(locale_filename, G_FILE_TEST_IS_REGULAR))
			{
				GeanyDocument *doc = document_open_file_full( NULL, locale_filename, 0, FALSE, NULL, NULL );
				if ( !doc ) failure = TRUE;
				else
				{
					if ( g_strv_length(tmp) > 2 ) editor_goto_pos(doc->editor, atoi(tmp[2]), FALSE);
					if ( atoi(tmp[0]) == 1 ) set_curr_doc = doc;
				}
			}
			else
				failure = TRUE;

			g_free(locale_filename);
			g_free(unescaped_filename);
		}
		g_strfreev(tmp);

		if (file_prefs.tab_order_ltr)
		{
			i++;
			if (i >= (gint)session_files->len)
				break;
		}
		else
		{
			i--;
			if (i < 0)
				break;
		}
	}

	g_ptr_array_free(session_files, TRUE);
	session_files = NULL;

	if (failure)
		ui_set_statusbar(TRUE, _("Failed to load one or more session files."));
	else if (session_notebook_page >= 0)
	{
		/* explicitly allow notebook switch page callback to be called for window title,
		 * encoding settings and so other things */
		main_status.opening_session_files = FALSE;
		/** TODO if session_notebook_page is equal to the current notebook tab(the last opened)
		 ** the notebook switch page callback isn't triggered and e.g. menu items are not updated */
		//gtk_notebook_set_current_page(GTK_NOTEBOOK(main_widgets.notebook), session_notebook_page);
	}
	main_status.opening_session_files = FALSE;

	if ( set_curr_doc ) 
	{
		gint page = document_get_notebook_page(set_curr_doc);
		gtk_notebook_set_current_page( GTK_NOTEBOOK(main_widgets.notebook), page );
		//document_grab_focus(set_curr_doc);
	}

	sidebar_update_tag_list(document_get_current(), FALSE);
}


/* set some settings which are already read from the config file, but need other things, like the
 * realisation of the main window */
void configuration_apply_settings(void)
{
	if (scribble_text)
	{	/* update the scribble widget, because now it's realized */
		GtkTextIter iter;
		GtkTextBuffer *buffer =
			gtk_text_view_get_buffer(GTK_TEXT_VIEW(msgwindow.scribble));

		gtk_text_buffer_set_text(buffer, scribble_text, -1);
		gtk_text_buffer_get_iter_at_offset(buffer, &iter, scribble_pos);
		gtk_text_buffer_place_cursor(buffer, &iter);
	}
	g_free(scribble_text);

	/* set the position of the hpaned and vpaned */
	if (prefs.save_winpos)
	{
		gtk_paned_set_position(GTK_PANED(ui_lookup_widget(main_widgets.window, "hpaned1")), hpan_position);
		//gtk_paned_set_position(GTK_PANED(ui_lookup_widget(main_widgets.window, "vpaned2")), vpan_position);
        set_full_message_height(vpan_position);
        restore_message_height();
	}

	/* set fullscreen after initial draw so that returning to normal view is the right size.
	 * fullscreen mode is disabled by default, so act only if it is true */
	if (ui_prefs.fullscreen)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ui_lookup_widget(main_widgets.window, "menu_fullscreen1")), TRUE);
		ui_prefs.fullscreen = TRUE;
		ui_set_fullscreen();
	}

	msgwin_show_hide_tabs();
}


void configuration_init(void)
{
	keyfile_groups = g_ptr_array_new();
	pref_groups = g_ptr_array_new();
	init_pref_groups();
}


void configuration_finalize(void)
{
	guint i;
	StashGroup *group;

	foreach_ptr_array(group, i, keyfile_groups)
		stash_group_free(group);

	g_ptr_array_free(keyfile_groups, TRUE);
	g_ptr_array_free(pref_groups, TRUE);
}

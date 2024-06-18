/*
 *      sidebar.c - this file is part of Geany, a fast and lightweight IDE
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
 * Sidebar related code for the Symbol list and Open files GtkTreeViews.
 */

#include <string.h>

#include "geany.h"
#include "support.h"
#include "callbacks.h"
#include "sidebar.h"
#include "document.h"
#include "editor.h"
#include "documentprivate.h"
#include "filetypes.h"
#include "utils.h"
#include "ui_utils.h"
#include "symbols.h"
#include "navqueue.h"
#include "project.h"
#include "stash.h"
#include "keyfile.h"
#include "sciwrappers.h"
#include "search.h"
#include "dialogs.h"

#include <gdk/gdkkeysyms.h>


SidebarTreeviews tv = {NULL, NULL, NULL};
/* while typeahead searching, editor should not get focus */
static gboolean may_steal_focus = FALSE;

static struct
{
	GtkWidget *remove_item;
	GtkWidget *add_item;
	GtkWidget *open_external_item;
	GtkWidget *expand_all;
	GtkWidget *collapse_all;
}
doc_items = {NULL, NULL, NULL, NULL};

enum
{
	OPENFILES_ACTION_OPEN = 0,
	OPENFILES_ACTION_REMOVE,
	OPENFILES_ACTION_ADD,
	OPENFILES_ACTION_OPEN_EXTERNAL
};

/* documents tree model columns */
enum
{
	DOCUMENTS_ICON,
	DOCUMENTS_SHORTNAME,	/* dirname for parents, basename for children */
	DOCUMENTS_TYPE,
	DOCUMENTS_COLOR,
	DOCUMENTS_FILENAME,		/* full filename */
	DOCUMENTS_PROJECT		
};

static GtkTreeStore	*store_openfiles;
static GtkWidget *openfiles_popup_menu;
static gboolean documents_show_paths;
static GtkWidget *tag_window;	/* scrolled window that holds the symbol list GtkTreeView */

/* callback prototypes */
static void on_openfiles_document_action(GtkMenuItem *menuitem, gpointer user_data);
static gboolean sidebar_button_press_cb(GtkWidget *widget, GdkEventButton *event,
		gpointer user_data);
static gboolean sidebar_key_press_cb(GtkWidget *widget, GdkEventKey *event,
		gpointer user_data);
static gboolean debug_callstack_button_press_cb(GtkWidget *widget, GdkEventButton *event,
		gpointer user_data);
static gboolean debug_callstack_key_press_cb(GtkWidget *widget, GdkEventKey *event,
		gpointer user_data);
static void on_list_document_activate(GtkCheckMenuItem *item, gpointer user_data);
static void on_list_symbol_activate(GtkCheckMenuItem *item, gpointer user_data);
static void documents_menu_update(GtkTreeSelection *selection);
static void sidebar_tabs_show_hide(GtkNotebook *notebook, GtkWidget *child,
								   guint page_num, gpointer data);


/* the prepare_* functions are document-related, but I think they fit better here than in document.c */
static void prepare_taglist(GtkWidget *tree, GtkTreeStore *store)
{
	GtkCellRenderer *text_renderer, *icon_renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	text_renderer = gtk_cell_renderer_text_new();
	icon_renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new();

	gtk_tree_view_column_pack_start(column, icon_renderer, FALSE);
  	gtk_tree_view_column_set_attributes(column, icon_renderer, "pixbuf", SYMBOLS_COLUMN_ICON, NULL);
  	g_object_set(icon_renderer, "xalign", 0.0, NULL);

  	gtk_tree_view_column_pack_start(column, text_renderer, TRUE);
  	gtk_tree_view_column_set_attributes(column, text_renderer, "text", SYMBOLS_COLUMN_NAME, NULL);
  	g_object_set(text_renderer, "yalign", 0.5, NULL);
  	gtk_tree_view_column_set_title(column, _("Symbols"));

	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

	ui_widget_modify_font_from_string(tree, interface_prefs.tagbar_font);

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	g_object_unref(store);

	g_signal_connect(tree, "button-press-event",
		G_CALLBACK(sidebar_button_press_cb), NULL);
	g_signal_connect(tree, "key-press-event",
		G_CALLBACK(sidebar_key_press_cb), NULL);

	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(tree), interface_prefs.show_symbol_list_expanders);
	if (! interface_prefs.show_symbol_list_expanders)
		gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(tree), 10);
	/* Tooltips */
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(tree), SYMBOLS_COLUMN_TOOLTIP);

	/* selection handling */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	/* callback for changed selection not necessary, will be handled by button-press-event */
}


static gboolean
on_default_tag_tree_button_press_event(GtkWidget *widget, GdkEventButton *event,
		gpointer user_data)
{
	if (event->button == 3)
	{
		gtk_menu_popup(GTK_MENU(tv.popup_taglist), NULL, NULL, NULL, NULL,
			event->button, event->time);
		return TRUE;
	}
	return FALSE;
}


static void create_default_tag_tree(void)
{
	GtkScrolledWindow *scrolled_window = GTK_SCROLLED_WINDOW(tag_window);
	GtkWidget *label;

	/* default_tag_tree is a GtkViewPort with a GtkLabel inside it */
	tv.default_tag_tree = gtk_viewport_new(
		gtk_scrolled_window_get_hadjustment(scrolled_window),
		gtk_scrolled_window_get_vadjustment(scrolled_window));
	label = gtk_label_new(_("No tags found"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.1f, 0.01f);
	gtk_container_add(GTK_CONTAINER(tv.default_tag_tree), label);
	gtk_widget_show_all(tv.default_tag_tree);
	g_signal_connect(tv.default_tag_tree, "button-press-event",
		G_CALLBACK(on_default_tag_tree_button_press_event), NULL);
	g_object_ref((gpointer)tv.default_tag_tree);	/* to hold it after removing */
}


/* update = rescan the tags for doc->filename */
void sidebar_update_tag_list(GeanyDocument *doc, gboolean update)
{
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(tag_window));

	g_return_if_fail(doc == NULL || doc->is_valid);

	/* changes the tree view to the given one, trying not to do useless changes */
	#define CHANGE_TREE(new_child) \
		G_STMT_START { \
			/* only change the tag tree if it's actually not the same (to avoid flickering) and if
			 * it's the one of the current document (to avoid problems when e.g. reloading
			 * configuration files */ \
			if (child != new_child && doc == document_get_current()) \
			{ \
				if (child) \
					gtk_container_remove(GTK_CONTAINER(tag_window), child); \
				gtk_container_add(GTK_CONTAINER(tag_window), new_child); \
			} \
		} G_STMT_END

	if (tv.default_tag_tree == NULL)
		create_default_tag_tree();

	/* show default empty tag tree if there are no tags */
	if (doc == NULL || doc->file_type == NULL || ! filetype_has_tags(doc->file_type))
	{
		CHANGE_TREE(tv.default_tag_tree);
		return;
	}

	if (update)
	{	/* updating the tag list in the left tag window */
		if (doc->priv->tag_tree == NULL)
		{
			doc->priv->tag_store = gtk_tree_store_new(
				SYMBOLS_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, TM_TYPE_TAG, G_TYPE_STRING);
			doc->priv->tag_tree = gtk_tree_view_new();
			prepare_taglist(doc->priv->tag_tree, doc->priv->tag_store);
			gtk_widget_show(doc->priv->tag_tree);
			g_object_ref((gpointer)doc->priv->tag_tree);	/* to hold it after removing */
		}

		doc->has_tags = symbols_recreate_tag_list(doc, SYMBOLS_SORT_USE_PREVIOUS);
	}

	if (doc->has_tags)
	{
		CHANGE_TREE(doc->priv->tag_tree);
	}
	else
	{
		CHANGE_TREE(tv.default_tag_tree);
	}

	#undef CHANGE_TREE
}


/* cleverly sorts documents by their short name */
static gint documents_sort_func(GtkTreeModel *model, GtkTreeIter *iter_a,
								GtkTreeIter *iter_b, gpointer data)
{
	gchar *key_a, *key_b;
	gchar *name_a, *name_b;
	gint cmp;
	gint type_a, type_b;

	gtk_tree_model_get(model, iter_a, DOCUMENTS_TYPE, &type_a, -1);
	gtk_tree_model_get(model, iter_b, DOCUMENTS_TYPE, &type_b, -1);

	if ( type_a < type_b ) return -1;
	else if ( type_a > type_b ) return 1;

	gtk_tree_model_get(model, iter_a, DOCUMENTS_SHORTNAME, &name_a, -1);
	key_a = g_utf8_collate_key_for_filename(name_a, -1);
	g_free(name_a);
	gtk_tree_model_get(model, iter_b, DOCUMENTS_SHORTNAME, &name_b, -1);
	key_b = g_utf8_collate_key_for_filename(name_b, -1);
	g_free(name_b);
	cmp = strcmp(key_a, key_b);
	g_free(key_b);
	g_free(key_a);

	return cmp;
}

static gint callstack_sort_func(GtkTreeModel *model, GtkTreeIter *iter_a, GtkTreeIter *iter_b, gpointer data)
{
	gint id_a, id_b;

	gtk_tree_model_get(model, iter_a, 0, &id_a, -1);
	gtk_tree_model_get(model, iter_b, 0, &id_b, -1);

	if ( id_a < id_b ) return -1;
	else if ( id_a > id_b ) return 1;
	return 0;
}

void debug_variable_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer user_data)
{
	GtkTreeIter iter;
	if ( !gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store_debug_variables), &iter, path_string) )
		return;

	// colons will mess up the message passing, so remove them
	gchar *ptr = new_text;
	while ( *ptr )
	{
		if ( *ptr == ':' ) *ptr = '-';
		ptr++;
	}
	
	gchar *varname;
	gtk_tree_model_get(store_debug_variables, &iter, 0, &varname, -1);

	// if the variable hasn't changed, do nothing
	if ( g_strcasecmp(new_text, varname) == 0 )
	{
		g_free(varname);
		return;
	}

	// remove the old variable from the debugger
	if ( debug_pid && *varname )
	{
		gchar szFinal[1024];
		sprintf( szFinal, "delete watch %s\n", varname );
		write(gdb_in.fd, szFinal, strlen(szFinal) );
	}

	// if the new variable name is empty delete the row
	if ( !new_text || !*new_text )
	{
		gtk_tree_store_remove( store_debug_variables, &iter );
	}
	else
	{
		// change the data store value to match
		gtk_tree_store_set(store_debug_variables, &iter, 0, new_text, 1, "", -1);

		// tell the debugger about the new variable
		if ( debug_pid )
		{
			gchar szFinal[1024];
			sprintf( szFinal, "watch %s\n", new_text );
			write(gdb_in.fd, szFinal, strlen(szFinal) );
		}

		// if row was blank then add a new blank row
		if ( !*varname )
		{
			gtk_tree_store_append(store_debug_variables, &iter, NULL);
			gtk_tree_store_set(store_debug_variables, &iter, 0, "", 1, "", -1);
		}
	}

	g_free(varname);
}

static void prepare_debug_tab(void)
{
	GtkCellRenderer *text_renderer;
	GtkCellRenderer *text_renderer2;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkTreeSortable *sortable;

	tv.debug_callstack = ui_lookup_widget(main_widgets.window, "debug_callstack");
	
	store_debug_callstack = gtk_tree_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv.debug_callstack), GTK_TREE_MODEL(store_debug_callstack));
	gtk_tree_view_set_show_expanders( GTK_TREE_VIEW(tv.debug_callstack), FALSE );

	/* set policy settings for the scolledwindow around the treeview again, because glade
	 * doesn't keep the settings */
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(ui_lookup_widget(main_widgets.window, "scrolledwindow12")), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	text_renderer = gtk_cell_renderer_text_new();
	g_object_set(text_renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, text_renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, text_renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv.debug_callstack), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv.debug_callstack), FALSE);

	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tv.debug_callstack), 1);

	/* sort by frame ID */
	sortable = GTK_TREE_SORTABLE(GTK_TREE_MODEL(store_debug_callstack));
	gtk_tree_sortable_set_sort_func(sortable, 0, callstack_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(sortable, 0, GTK_SORT_ASCENDING);

	ui_widget_modify_font_from_string(tv.debug_callstack, interface_prefs.tagbar_font);

	/* tooltips */
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(tv.debug_callstack), 1);

	/* selection handling */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv.debug_callstack));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_object_unref(store_debug_callstack);

	g_signal_connect(GTK_TREE_VIEW(tv.debug_callstack), "button-press-event", G_CALLBACK(debug_callstack_button_press_cb), NULL);
	g_signal_connect(GTK_TREE_VIEW(tv.debug_callstack), "key-press-event", G_CALLBACK(debug_callstack_key_press_cb), NULL);


	// variable watch window
	tv.debug_variables = ui_lookup_widget(main_widgets.window, "debug_variable_watch");

	store_debug_variables = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv.debug_variables), GTK_TREE_MODEL(store_debug_variables));
	gtk_tree_view_set_show_expanders( GTK_TREE_VIEW(tv.debug_variables), FALSE );

	/* set policy settings for the scolledwindow around the treeview again, because glade
	 * doesn't keep the settings */
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(ui_lookup_widget(main_widgets.window, "scrolledwindow11")), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	// column 1
	text_renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_set_padding( text_renderer, 5, 0 );
	g_object_set(text_renderer, "editable", TRUE, NULL);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, text_renderer, FALSE);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_attributes(column, text_renderer, "text", 0, NULL);
	gtk_tree_view_column_set_title(column, _("Variable"));
	gtk_tree_view_column_set_alignment(column, 0.5);
	gtk_tree_view_column_set_min_width(column, 75);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv.debug_variables), column);

	g_signal_connect(text_renderer, "edited", G_CALLBACK(debug_variable_edited), NULL);

	// column 2
	text_renderer2 = gtk_cell_renderer_text_new();
	gtk_cell_renderer_set_padding( text_renderer2, 5, 0 );
	g_object_set(text_renderer2, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, text_renderer2, TRUE);
	gtk_tree_view_column_set_attributes(column, text_renderer2, "text", 1, NULL);
	gtk_tree_view_column_set_title(column, _("Value"));
	gtk_tree_view_column_set_alignment(column, 0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv.debug_variables), column);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv.debug_variables), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tv.debug_variables), 0);

	ui_widget_modify_font_from_string(tv.debug_variables, interface_prefs.tagbar_font);

	/* tooltips */
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(tv.debug_variables), 1);

	/* selection handling */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv.debug_variables));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_object_unref(store_debug_variables);

	// disable selection color
	GtkStyle *style = gtk_widget_get_style(tv.debug_variables);
	gtk_widget_modify_base( tv.debug_variables, GTK_STATE_SELECTED, &(style->base[GTK_STATE_INSENSITIVE]) );
	//gtk_widget_modify_base( tv.debug_variables, GTK_STATE_INSENSITIVE, &(style->base[GTK_STATE_INSENSITIVE]) );
	//gtk_widget_modify_base( tv.debug_variables, GTK_STATE_ACTIVE, &(style->base[GTK_STATE_NORMAL]) );

	gtk_widget_modify_text( tv.debug_variables, GTK_STATE_SELECTED, &(style->text[GTK_STATE_NORMAL]) );
	gtk_widget_modify_text( tv.debug_variables, GTK_STATE_INSENSITIVE, &(style->text[GTK_STATE_NORMAL]) );
	//gtk_widget_modify_text( tv.debug_variables, GTK_STATE_ACTIVE, &(style->text[GTK_STATE_NORMAL]) );

	static GtkTreeIter file;
	gtk_tree_store_append(store_debug_variables, &file, NULL);

	gtk_tree_store_set(store_debug_variables, &file,
		0, "",
		1, "", 
		-1);
}

/* does some preparing things to the open files list widget */
static void prepare_openfiles(void)
{
	GtkCellRenderer *icon_renderer;
	GtkCellRenderer *text_renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkTreeSortable *sortable;

	tv.tree_openfiles = ui_lookup_widget(main_widgets.window, "treeview6");

	/* store the icon and the short filename to show, and the index as reference,
	 * the colour (black/red/green) and the full name for the tooltip */
	store_openfiles = gtk_tree_store_new(6, GDK_TYPE_PIXBUF, G_TYPE_STRING,
		G_TYPE_INT, GDK_TYPE_COLOR, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv.tree_openfiles), GTK_TREE_MODEL(store_openfiles));

	/* set policy settings for the scolledwindow around the treeview again, because glade
	 * doesn't keep the settings */
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(ui_lookup_widget(main_widgets.window, "scrolledwindow7")),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	icon_renderer = gtk_cell_renderer_pixbuf_new();
	text_renderer = gtk_cell_renderer_text_new();
	g_object_set(text_renderer, "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, icon_renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, icon_renderer, "pixbuf", DOCUMENTS_ICON, NULL);
	gtk_tree_view_column_pack_start(column, text_renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, text_renderer, "text", DOCUMENTS_SHORTNAME,
		"foreground-gdk", DOCUMENTS_COLOR, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv.tree_openfiles), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv.tree_openfiles), FALSE);

	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tv.tree_openfiles),
		DOCUMENTS_SHORTNAME);

	/* sort opened filenames in the store_openfiles treeview */
	sortable = GTK_TREE_SORTABLE(GTK_TREE_MODEL(store_openfiles));
	gtk_tree_sortable_set_sort_func(sortable, DOCUMENTS_SHORTNAME, documents_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(sortable, DOCUMENTS_SHORTNAME, GTK_SORT_ASCENDING);

	ui_widget_modify_font_from_string(tv.tree_openfiles, interface_prefs.tagbar_font);

	/* tooltips */
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(tv.tree_openfiles), DOCUMENTS_FILENAME);

	/* selection handling */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv.tree_openfiles));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_object_unref(store_openfiles);

	g_signal_connect(GTK_TREE_VIEW(tv.tree_openfiles), "button-press-event",
		G_CALLBACK(sidebar_button_press_cb), NULL);
	g_signal_connect(GTK_TREE_VIEW(tv.tree_openfiles), "key-press-event",
		G_CALLBACK(sidebar_key_press_cb), NULL);
}


static gboolean find_tree_iter_doc(GtkTreeIter *iter, const gchar *path)
{
	gint type;
	gchar *name;
	gboolean result;

	gtk_tree_model_get(GTK_TREE_MODEL(store_openfiles), iter, DOCUMENTS_TYPE, &type, -1);
	if ( type != 0 ) return FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(store_openfiles), iter, DOCUMENTS_FILENAME, &name, -1);

	result = utils_filenamecmp(name, path) == 0;
	g_free(name);

	return result;
}

static gboolean find_tree_iter_project(GtkTreeIter *iter, const gchar *path)
{
	gint type;
	gchar *name;
	gboolean result;

	gtk_tree_model_get(GTK_TREE_MODEL(store_openfiles), iter, DOCUMENTS_TYPE, &type, -1);
	if ( type != 1 ) return FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(store_openfiles), iter, DOCUMENTS_FILENAME, &name, -1);

	result = utils_filenamecmp(name, path) == 0;
	g_free(name);

	return result;
}

static gboolean find_tree_iter_group(GtkTreeIter *iter, const gchar *path)
{
	gint type;
	gchar *name;
	gboolean result;

	gtk_tree_model_get(GTK_TREE_MODEL(store_openfiles), iter, DOCUMENTS_TYPE, &type, -1);
	if ( type != 2 ) return FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(store_openfiles), iter, DOCUMENTS_FILENAME, &name, -1);

	result = utils_filenamecmp(name, path) == 0;
	g_free(name);

	return result;
}


static gboolean utils_filename_has_prefix(const gchar *str, const gchar *prefix)
{
	gchar *head = g_strndup(str, strlen(prefix));
	gboolean ret = utils_filenamecmp(head, prefix) == 0;

	g_free(head);
	return ret;
}

/*
static gchar *get_doc_folder(const gchar *path)
{
	gchar *tmp_dirname = g_strdup(path);
	gchar *project_base_path;
	gchar *dirname = NULL;
	const gchar *home_dir = g_get_home_dir();
	const gchar *rest;

	// replace the project base path with the project name
	project_base_path = project_get_base_path();

	if (project_base_path != NULL)
	{
		gsize len = strlen(project_base_path);

		// remove trailing separator so we can match base path exactly 
		if (project_base_path[len-1] == G_DIR_SEPARATOR)
			project_base_path[--len] = '\0';

		// check whether the dir name matches or uses the project base path 
		if (utils_filename_has_prefix(tmp_dirname, project_base_path))
		{
			rest = tmp_dirname + len;
			if (*rest == G_DIR_SEPARATOR || *rest == '\0')
			{
				dirname = g_strdup_printf("%s%s", app->project->name, rest);
			}
		}
		g_free(project_base_path);
	}
	if (dirname == NULL)
	{
		dirname = tmp_dirname;

		// If matches home dir, replace with tilde 
		if (!EMPTY(home_dir) && utils_filename_has_prefix(dirname, home_dir))
		{
			rest = dirname + strlen(home_dir);
			if (*rest == G_DIR_SEPARATOR || *rest == '\0')
			{
				dirname = g_strdup_printf("~%s", rest);
				g_free(tmp_dirname);
			}
		}
	}
	else
		g_free(tmp_dirname);

	return dirname;
}
*/

static GtkTreeIter *get_file_iter(GtkTreeIter* parent, const gchar *filename)
{
	static GtkTreeIter output_iter;
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(store_openfiles);

	if (gtk_tree_model_iter_children(model, &iter, parent))
	{
		do
		{
			if (find_tree_iter_doc(&iter, filename))
			{
				output_iter = iter;
				return &output_iter;
			}

			// search sub items
			GtkTreeIter *child = get_file_iter( &iter, filename );
			if ( child )
				return child;
		}
		while (gtk_tree_model_iter_next(model, &iter));
	}
	return NULL;
}

static GtkTreeIter *get_project_iter(GeanyProject *project)
{
	gchar *path;
	static GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(store_openfiles);

	path = project->file_name;

	if (gtk_tree_model_get_iter_first(model, &iter))
	{
		do
		{
			if (find_tree_iter_project(&iter, path))
			{
				return &iter;
			}
		}
		while (gtk_tree_model_iter_next(model, &iter));
	}
	return NULL;
}

static GtkTreeIter *get_group_iter(GeanyProjectGroup *group)
{
	gchar *path;
	static GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(store_openfiles);

	path = group->full_name;

	if (gtk_tree_model_get_iter_first(model, &iter))
	{
		do
		{
			if (find_tree_iter_group(&iter, path))
			{
				g_free(path);
				return &iter;
			}
		}
		while (gtk_tree_model_iter_next(model, &iter));
	}
	return NULL;
}

gboolean sidebar_openfiles_exists(gchar* filename)
{
	if ( get_file_iter( NULL, filename ) ) return TRUE;
	else return FALSE;
}

/* Also sets doc->priv->iter.
 * This is called recursively in sidebar_openfiles_update_all(). */
void sidebar_openfiles_add(GeanyDocument *doc)
{
	GtkTreeIter *iter;
	GtkTreeIter *parent = NULL;
	GtkTreeIter *docIter = &doc->priv->iter;
	gchar *basename;
	const GdkColor *color = document_get_status_color(doc);
	static GdkPixbuf *file_icon = NULL;

	iter = get_file_iter( NULL, DOC_FILENAME(doc) );
	if ( iter )
	{
		return;
	}

	doc->has_sidebar_entry = TRUE;

	gtk_tree_store_append(store_openfiles, docIter, parent);

	if (!file_icon)
		file_icon = ui_get_mime_icon("text/plain", GTK_ICON_SIZE_MENU);

	basename = g_path_get_basename(DOC_FILENAME(doc));
	gtk_tree_store_set(store_openfiles, docIter,
		DOCUMENTS_ICON, (doc->file_type && doc->file_type->icon) ? doc->file_type->icon : file_icon,
		DOCUMENTS_SHORTNAME, basename, DOCUMENTS_TYPE, 0, DOCUMENTS_COLOR, color,
		DOCUMENTS_FILENAME, DOC_FILENAME(doc), DOCUMENTS_PROJECT, NULL, -1);
	g_free(basename);
}

void sidebar_openfiles_add_file(GeanyProject *project, const gchar* filename)
{
	static GtkTreeIter file;
	GtkTreeIter *iter;
	gchar *basename;
	static GdkPixbuf *file_icon = NULL;
	gboolean add = TRUE;

	iter = get_file_iter( NULL, filename );
	if ( iter )
	{
		// file already exists in tree
		GeanyProject *existing_project = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store_openfiles), iter, DOCUMENTS_PROJECT, &existing_project, -1);
		if ( existing_project == project ) 
			add = FALSE;

		if ( !existing_project )
		{
			GeanyDocument *doc = document_find_by_filename( filename );
			if ( doc ) 
				doc->has_sidebar_entry = FALSE;
			gtk_tree_store_remove( store_openfiles, iter );
		}
	}

	if ( add )
	{
		gtk_tree_store_append(store_openfiles, &file, project ? &(project->iter) : NULL);

		if (!file_icon)
			file_icon = ui_get_mime_icon("text/plain", GTK_ICON_SIZE_MENU);

		basename = g_path_get_basename(filename);
		gtk_tree_store_set(store_openfiles, &file,
			DOCUMENTS_ICON, file_icon,
			DOCUMENTS_SHORTNAME, basename, DOCUMENTS_TYPE, 0, DOCUMENTS_COLOR, NULL,
			DOCUMENTS_FILENAME, filename, DOCUMENTS_PROJECT, project, -1);
		g_free(basename);
	}

	// expand parent if not already
	if ( project )
	{
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store_openfiles), &(project->iter));

		if (!gtk_tree_view_row_expanded(GTK_TREE_VIEW(tv.tree_openfiles), path))
			gtk_tree_view_expand_row(GTK_TREE_VIEW(tv.tree_openfiles), path, FALSE);
	}
}

void sidebar_openfiles_remove_file(GeanyProject *project, const gchar* filename)
{
	static GtkTreeIter file;
	GtkTreeIter *iter = NULL, *project_iter = NULL;
	gchar *basename;
	static GdkPixbuf *file_icon = NULL;
	gboolean add = TRUE;

	if ( project )
		project_iter = get_project_iter( project );

	iter = get_file_iter( project_iter, filename );
	if ( iter )
	{
		GeanyDocument *doc = document_find_by_filename( filename );
		if ( doc )
			doc->has_sidebar_entry = FALSE;

		gtk_tree_store_remove(store_openfiles, iter);
	}
}

void sidebar_openfiles_add_project(GeanyProject *project)
{
	GtkTreeIter *iter;
	static GdkPixbuf *file_icon = NULL;
	static GdkPixbuf *dir_icon = NULL;
	int i;

	iter = get_project_iter( project );
	if ( iter )
	{
		return;
	}

	if (!file_icon)
		file_icon = ui_get_mime_icon("text/plain", GTK_ICON_SIZE_MENU);

	if (!dir_icon)
		dir_icon = ui_get_mime_icon("inode/directory", GTK_ICON_SIZE_MENU);

	gtk_tree_store_append(store_openfiles, &(project->iter), NULL);
	gtk_tree_store_set(store_openfiles, &(project->iter),
		DOCUMENTS_ICON, dir_icon,
		DOCUMENTS_SHORTNAME, project->name, 
		DOCUMENTS_TYPE, 1, 
		DOCUMENTS_FILENAME, project->file_name, 
		DOCUMENTS_PROJECT, project, -1);

	// add project groups
	for( i = 0; i < project->project_groups->len; i++ )
	{
		GeanyProjectGroup *group = g_ptr_array_index(project->project_groups, i);
		if ( !group->is_valid ) 
			continue;

		GtkTreeIter *parent = &(project->iter);
		if ( group->pParent ) parent = &(group->pParent->iter);

		gtk_tree_store_append(store_openfiles, &(group->iter), parent );
		gtk_tree_store_set(store_openfiles, &(group->iter),
			DOCUMENTS_ICON, dir_icon,
			DOCUMENTS_SHORTNAME, group->group_name, 
			DOCUMENTS_TYPE, 2, 
			DOCUMENTS_FILENAME, group->full_name,
			DOCUMENTS_PROJECT, project, -1);
	}

	// add project files
	for( i = 0; i < project->project_files->len; i++ )
	{
		GeanyProjectFile *file = g_ptr_array_index(project->project_files, i);
		if ( !file->is_valid ) 
			continue;

		GtkTreeIter *parent = &(project->iter);
		if ( file->pParent ) parent = &(file->pParent->iter);

		// look for existing files first
		gboolean add = TRUE;
		iter = get_file_iter( &(project->iter), file->file_name );
		if ( iter )
		{
			// file already exists in tree
			GeanyProject *existing_project = NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(store_openfiles), iter, DOCUMENTS_PROJECT, &existing_project, -1);
			if ( existing_project == project ) continue;

			if ( !existing_project )
			{
				add = FALSE;
				gtk_tree_store_set( store_openfiles, iter, DOCUMENTS_PROJECT, project, -1 );
			}
		}

		if ( add )
		{
			gchar *basename = g_path_get_basename(file->file_name);
			gtk_tree_store_append(store_openfiles, &(file->iter), parent );
			gtk_tree_store_set(store_openfiles, &(file->iter),
				DOCUMENTS_ICON, file_icon,
				DOCUMENTS_SHORTNAME, basename, 
				DOCUMENTS_TYPE, 0, 
				DOCUMENTS_FILENAME, file->file_name,
				DOCUMENTS_PROJECT, project, -1);
			g_free(basename);
		}
	}

	GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store_openfiles), &(project->iter));

	if (!gtk_tree_view_row_expanded(GTK_TREE_VIEW(tv.tree_openfiles), path))
		gtk_tree_view_expand_row(GTK_TREE_VIEW(tv.tree_openfiles), path, FALSE);
}

static void openfiles_remove(GeanyDocument *doc)
{
	if ( !doc->has_sidebar_entry ) return;

	GtkTreeIter *iter = &doc->priv->iter;
	GtkTreeIter parent;

	// only remove if it isn't part of a project
	if ( !gtk_tree_model_iter_parent(GTK_TREE_MODEL(store_openfiles), &parent, iter) )
	{
		doc->has_sidebar_entry = FALSE;
		gtk_tree_store_remove(store_openfiles, iter);
	}
}

/*
static void openfiles_force_remove(gchar* filename)
{
	GtkTreeIter *iter = get_file_iter( NULL, filename );
	
	gtk_tree_store_remove(store_openfiles, iter);
}
*/

static void openfiles_close_child_doc(GtkTreeIter *parent)
{
	GtkTreeIter child;
	gchar *filename;
	int type;
	GeanyDocument *doc;

	if ( !gtk_tree_model_iter_children (GTK_TREE_MODEL(store_openfiles), &child, parent) ) 
		return;

	do
	{
		gtk_tree_model_get(GTK_TREE_MODEL(store_openfiles), &child, DOCUMENTS_TYPE, &type, -1);
		
		if ( type == 0 )
		{
			gtk_tree_model_get(GTK_TREE_MODEL(store_openfiles), &child, DOCUMENTS_FILENAME, &filename, -1);
			doc = document_find_by_filename( filename );
			if ( doc && doc->is_valid )
				document_close(doc);
			g_free(filename);
		}
		else if ( type == 2 )
			openfiles_close_child_doc( &child );

	} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store_openfiles), &child));
}

void sidebar_remove_project(GeanyProject *project)
{
	GtkTreeIter *iter = get_project_iter( project );
	
	if ( !iter )
		return;

	// close project documents
	openfiles_close_child_doc( iter );

	// remove tree items
	gtk_tree_store_remove(store_openfiles, iter);
}


void sidebar_openfiles_update(GeanyDocument *doc)
{
	if ( !doc->has_sidebar_entry ) 
		return;

	GtkTreeIter *iter = &doc->priv->iter;
	gchar *fname;

	gtk_tree_model_get(GTK_TREE_MODEL(store_openfiles), iter, DOCUMENTS_FILENAME, &fname, -1);

	if ( !utils_str_equal(fname, DOC_FILENAME(doc)) )
	{
		// path has changed, so update
		gchar *basename = g_path_get_basename(DOC_FILENAME(doc));
		gtk_tree_store_set( store_openfiles, iter, DOCUMENTS_FILENAME, DOC_FILENAME(doc), DOCUMENTS_SHORTNAME, basename, -1 );
		g_free(basename);
	}

	g_free(fname);
}


void sidebar_openfiles_update_all(void)
{
	return;
	/*
	guint i;

	gtk_tree_store_clear(store_openfiles);
	foreach_document (i)
	{
		sidebar_openfiles_add(documents[i]);
	}
	*/
}


void sidebar_remove_document(GeanyDocument *doc)
{
	openfiles_remove(doc);
	//sidebar_openfiles_remove_file( NULL, DOC_FILENAME(doc) );

	if (GTK_IS_WIDGET(doc->priv->tag_tree))
	{
		gtk_widget_destroy(doc->priv->tag_tree); /* make GTK release its references, if any */
		/* Because it was ref'd in sidebar_update_tag_list, it needs unref'ing */
		g_object_unref(doc->priv->tag_tree);
		doc->priv->tag_tree = NULL;
	}
}


static void on_hide_sidebar(void)
{
	ui_prefs.sidebar_visible = FALSE;
	ui_sidebar_show_hide();
}


static gboolean on_sidebar_display_symbol_list_show(GtkWidget *item)
{
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
		interface_prefs.sidebar_symbol_visible);
	return FALSE;
}


static gboolean on_sidebar_display_open_files_show(GtkWidget *item)
{
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
		interface_prefs.sidebar_openfiles_visible);
	return FALSE;
}


void sidebar_add_common_menu_items(GtkMenu *menu)
{
	/*
	GtkWidget *item;

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);

	item = gtk_check_menu_item_new_with_mnemonic(_("Show S_ymbol List"));
	gtk_container_add(GTK_CONTAINER(menu), item);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_signal_connect(item, "draw", G_CALLBACK(on_sidebar_display_symbol_list_show), NULL);
#else
	g_signal_connect(item, "expose-event",
			G_CALLBACK(on_sidebar_display_symbol_list_show), NULL);
#endif
	gtk_widget_show(item);
	g_signal_connect(item, "activate",
			G_CALLBACK(on_list_symbol_activate), NULL);

	item = gtk_check_menu_item_new_with_mnemonic(_("Show _Document List"));
	gtk_container_add(GTK_CONTAINER(menu), item);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_signal_connect(item, "draw", G_CALLBACK(on_sidebar_display_open_files_show), NULL);
#else
	g_signal_connect(item, "expose-event",
			G_CALLBACK(on_sidebar_display_open_files_show), NULL);
#endif
	gtk_widget_show(item);
	g_signal_connect(item, "activate",
			G_CALLBACK(on_list_document_activate), NULL);

	item = gtk_image_menu_item_new_with_mnemonic(_("H_ide Sidebar"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_hide_sidebar), NULL);
	*/
}


static void on_openfiles_show_paths_activate(GtkCheckMenuItem *item, gpointer user_data)
{
	documents_show_paths = gtk_check_menu_item_get_active(item);
	sidebar_openfiles_update_all();
}


static void on_list_document_activate(GtkCheckMenuItem *item, gpointer user_data)
{
	interface_prefs.sidebar_openfiles_visible = gtk_check_menu_item_get_active(item);
	ui_sidebar_show_hide();
	sidebar_tabs_show_hide(GTK_NOTEBOOK(main_widgets.sidebar_notebook), NULL, 0, NULL);
}


static void on_list_symbol_activate(GtkCheckMenuItem *item, gpointer user_data)
{
	interface_prefs.sidebar_symbol_visible = gtk_check_menu_item_get_active(item);
	ui_sidebar_show_hide();
	sidebar_tabs_show_hide(GTK_NOTEBOOK(main_widgets.sidebar_notebook), NULL, 0, NULL);
}


static void on_openfiles_expand_collapse(GtkMenuItem *menuitem, gpointer user_data)
{
	gboolean expand = GPOINTER_TO_INT(user_data);

	if (expand)
		gtk_tree_view_expand_all(GTK_TREE_VIEW(tv.tree_openfiles));
	else
		gtk_tree_view_collapse_all(GTK_TREE_VIEW(tv.tree_openfiles));
}


static void create_openfiles_popup_menu(void)
{
	GtkWidget *item;

	openfiles_popup_menu = gtk_menu_new();

	// open external
	item = gtk_image_menu_item_new_with_label(_("Open Containing Folder"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(openfiles_popup_menu), item);
	g_signal_connect(item, "activate",
			G_CALLBACK(on_openfiles_document_action), GINT_TO_POINTER(OPENFILES_ACTION_OPEN_EXTERNAL));
	doc_items.open_external_item = item;

	// separator
	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(openfiles_popup_menu), item);

	// remove
	item = gtk_image_menu_item_new_with_label(_("Remove From Project"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(openfiles_popup_menu), item);
	g_signal_connect(item, "activate",
			G_CALLBACK(on_openfiles_document_action), GINT_TO_POINTER(OPENFILES_ACTION_REMOVE));
	doc_items.remove_item = item;

	// add
	item = gtk_menu_item_new_with_label(_("Add To Current Project"));
	gtk_widget_hide(item);
	gtk_container_add(GTK_CONTAINER(openfiles_popup_menu), item);
	g_signal_connect(item, "activate",
			G_CALLBACK(on_openfiles_document_action), GINT_TO_POINTER(OPENFILES_ACTION_ADD));
	doc_items.add_item = item;

	// separator
	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(openfiles_popup_menu), item);
	
	// expand
	doc_items.expand_all = ui_image_menu_item_new(GTK_STOCK_ADD, _("_Expand All"));
	gtk_widget_show(doc_items.expand_all);
	gtk_container_add(GTK_CONTAINER(openfiles_popup_menu), doc_items.expand_all);
	g_signal_connect(doc_items.expand_all, "activate",
					 G_CALLBACK(on_openfiles_expand_collapse), GINT_TO_POINTER(TRUE));

	// collapse
	doc_items.collapse_all = ui_image_menu_item_new(GTK_STOCK_REMOVE, _("_Collapse All"));
	gtk_widget_show(doc_items.collapse_all);
	gtk_container_add(GTK_CONTAINER(openfiles_popup_menu), doc_items.collapse_all);
	g_signal_connect(doc_items.collapse_all, "activate",
					 G_CALLBACK(on_openfiles_expand_collapse), GINT_TO_POINTER(FALSE));

	sidebar_add_common_menu_items(GTK_MENU(openfiles_popup_menu));
}


static void unfold_parent(GtkTreeIter *iter)
{
	GtkTreeIter parent;
	GtkTreePath *path;

	if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(store_openfiles), &parent, iter))
	{
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(store_openfiles), &parent);
		gtk_tree_view_expand_row(GTK_TREE_VIEW(tv.tree_openfiles), path, TRUE);
		gtk_tree_path_free(path);
	}
}


/* callbacks */

static void on_openfiles_document_action(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv.tree_openfiles));
	GtkTreeModel *model;
	int type;
	gint action = GPOINTER_TO_INT(user_data);
	GeanyProject *project = NULL;
	gchar *filename;

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, DOCUMENTS_TYPE, &type, -1);
		gtk_tree_model_get(model, &iter, DOCUMENTS_PROJECT, &project, -1);
		
		if ( action == OPENFILES_ACTION_REMOVE )
		{
			if ( type == 0 ) // document
			{
				gtk_tree_model_get(model, &iter, DOCUMENTS_FILENAME, &filename, -1);
				
				if ( project )
					project_remove_file( project, filename, TRUE );
				else
				{
					GeanyDocument *doc = document_find_by_filename( filename );
					if ( doc )
						document_close(doc);
					else
						sidebar_openfiles_remove_file( NULL, filename );
				}
				
				g_free(filename);
			}
			else if ( type == 1 ) // project
			{
				if ( project )
					project_close( project, FALSE );
			}		
		}
		else if ( action == OPENFILES_ACTION_ADD )
		{
			if ( type == 0 && !project )
			{
				gtk_tree_model_get(model, &iter, DOCUMENTS_FILENAME, &filename, -1);
				if ( !g_path_is_absolute( filename ) )
					dialogs_show_msgbox(GTK_MESSAGE_ERROR,"File must be saved before it can be added to a project");
				else
				{
					if ( app->project ) sidebar_openfiles_remove_file( NULL, filename );
					project_add_file( app->project, filename, TRUE );
				}
			}
		}
		else if ( action == OPENFILES_ACTION_OPEN_EXTERNAL )
		{
			if ( type == 0 )
			{
				gtk_tree_model_get(model, &iter, DOCUMENTS_FILENAME, &filename, -1);
				if ( !g_path_is_absolute( filename ) )
					dialogs_show_msgbox(GTK_MESSAGE_ERROR,"File does not have a folder as it has not been saved");
				else
				{
#ifdef G_OS_WIN32
					gchar *filepath = g_strdup( filename );
					utils_str_replace_char( filepath, '\\', '/' );
					char* slash = strrchr( filepath, '/' );
					if ( slash ) *slash = 0;
					utils_str_replace_char( filepath, '/', '\\' );
					gchar *cmdline = g_strconcat("explorer.exe", " \"", filepath, "\"", NULL);
					g_spawn_command_line_async(cmdline, NULL);
					g_free(cmdline);
					g_free(filepath);
#elif __APPLE__ 
                    gchar *filepath = g_strdup( filename );
					utils_str_replace_char( filepath, '\\', '/' );
					char* slash = strrchr( filepath, '/' );
					if ( slash ) *slash = 0;
					gchar *cmdline = g_strconcat("open", " \"", filepath, "\"", NULL);
					g_spawn_command_line_async(cmdline, NULL);
					g_free(cmdline);
                    g_free(filepath);
#else
					gchar *filepath = g_strdup( filename );
					utils_str_replace_char( filepath, '\\', '/' );
					char* slash = strrchr( filepath, '/' );
					if ( slash ) *slash = 0;
					gchar *cmdline = g_strconcat("xdg-open", " \"", filepath, "\"", NULL);
					g_spawn_command_line_async(cmdline, NULL);
					g_free(cmdline);
                    g_free(filepath);
#endif
				}
			}
			else if ( type == 1 && project )
			{
#ifdef G_OS_WIN32
				gchar *filepath = g_strdup( project->base_path );
				utils_str_replace_char( filepath, '/', '\\' );
				if ( filepath[ strlen(filepath) - 1 ] == '\\' ) filepath[ strlen(filepath) - 1 ] = 0;
				gchar *cmdline = g_strconcat("explorer.exe", " \"", filepath, "\"", NULL);
				g_spawn_command_line_async(cmdline, NULL);
				g_free(cmdline);
				g_free(filepath);
#elif __APPLE__
				gchar *cmdline = g_strconcat("open", " \"", project->base_path, "\"", NULL);
				g_spawn_command_line_async(cmdline, NULL);
				g_free(cmdline);
#else
				gchar *cmdline = g_strconcat("xdg-open", " \"", project->base_path, "\"", NULL);
				g_spawn_command_line_async(cmdline, NULL);
				g_free(cmdline);
#endif
			}
		}
	}
}


static void change_focus_to_editor(GeanyDocument *doc, GtkWidget *source_widget)
{
	if (may_steal_focus)
		document_try_focus(doc, source_widget);
	may_steal_focus = FALSE;
}


static gboolean openfiles_go_to_selection(GtkTreeSelection *selection, guint keyval)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GeanyDocument *doc = NULL;
	gchar *filename = NULL;

	/* use switch_notebook_page to ignore changing the notebook page because it is already done */
	if (gtk_tree_selection_get_selected(selection, &model, &iter) && ! ignore_callback)
	{
		gtk_tree_model_get(model, &iter, DOCUMENTS_FILENAME, &filename, -1);
		doc = document_find_by_filename( filename );
		g_free(filename);

		if (! doc)
			return FALSE;	/* parent */

		/* switch to the doc and grab the focus */
		document_show_tab(doc);
		//if (keyval != GDK_space)
		//	change_focus_to_editor(doc, tv.tree_openfiles);
	}
	return FALSE;
}


static gboolean taglist_go_to_selection(GtkTreeSelection *selection, guint keyval, guint state)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint line = 0;
	gboolean handled = TRUE;

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		TMTag *tag;

		gtk_tree_model_get(model, &iter, SYMBOLS_COLUMN_TAG, &tag, -1);
		if (! tag)
			return FALSE;

		line = tag->atts.entry.line;
		if (line > 0)
		{
			GeanyDocument *doc = document_get_current();

			if (doc != NULL)
			{
				navqueue_goto_line(doc, doc, line);
				if (keyval != GDK_space && ! (state & GDK_CONTROL_MASK))
					change_focus_to_editor(doc, NULL);
				else
					handled = FALSE;
			}
		}
		tm_tag_unref(tag);
	}
	return handled;
}


static gboolean sidebar_key_press_cb(GtkWidget *widget, GdkEventKey *event,
											 gpointer user_data)
{
	may_steal_focus = FALSE;
	if (ui_is_keyval_enter_or_return(event->keyval) || event->keyval == GDK_space)
	{
		GtkWidgetClass *widget_class = GTK_WIDGET_GET_CLASS(widget);
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
		may_steal_focus = TRUE;

		/* force the TreeView handler to run before us for it to do its job (selection & stuff).
		 * doing so will prevent further handlers to be run in most cases, but the only one is our
		 * own, so guess it's fine. */
		if (widget_class->key_press_event)
			widget_class->key_press_event(widget, event);

		if (widget == tv.tree_openfiles) /* tag and doc list have separate handlers */
		{
			GtkTreeIter iter;
			GtkTreeModel *model;
			GeanyProject *project = NULL;

			if (gtk_tree_selection_get_selected(selection, &model, &iter))
			{
				gchar *filename;
				int type;

				gtk_tree_model_get(model, &iter, DOCUMENTS_TYPE, &type, -1);
				gtk_tree_model_get(model, &iter, DOCUMENTS_FILENAME, &filename, -1);
				gtk_tree_model_get(model, &iter, DOCUMENTS_PROJECT, &project, -1);

				if ( type == 0 )
					document_open_file( filename, FALSE, NULL, NULL );

				g_free(filename);
			}

			//openfiles_go_to_selection(selection, event->keyval);
		}
		else
			taglist_go_to_selection(selection, event->keyval, event->state);

		return TRUE;
	}
	return FALSE;
}


static gboolean sidebar_button_press_cb(GtkWidget *widget, GdkEventButton *event,
		G_GNUC_UNUSED gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkWidgetClass *widget_class = GTK_WIDGET_GET_CLASS(widget);
	gboolean handled = FALSE;

	/* force the TreeView handler to run before us for it to do its job (selection & stuff).
	 * doing so will prevent further handlers to be run in most cases, but the only one is our own,
	 * so guess it's fine. */
	if (widget_class->button_press_event)
		handled = widget_class->button_press_event(widget, event);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	may_steal_focus = TRUE;

	if (event->type == GDK_2BUTTON_PRESS)
	{	/* double click on parent node(section) expands/collapses it */
		GtkTreeModel *model;
		GtkTreeIter iter;
		GeanyProject *project = 0;

		if (gtk_tree_selection_get_selected(selection, &model, &iter))
		{
			if (gtk_tree_model_iter_has_child(model, &iter))
			{
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);

				if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(widget), path))
					gtk_tree_view_collapse_row(GTK_TREE_VIEW(widget), path);
				else
					gtk_tree_view_expand_row(GTK_TREE_VIEW(widget), path, FALSE);

				gtk_tree_path_free(path);
				return TRUE;
			}
			else
			{
				if (widget == tv.tree_openfiles)
				{
					// open file
					gchar *filename;
					int type;

					gtk_tree_model_get(model, &iter, DOCUMENTS_TYPE, &type, -1);
					gtk_tree_model_get(model, &iter, DOCUMENTS_FILENAME, &filename, -1);
					gtk_tree_model_get(model, &iter, DOCUMENTS_PROJECT, &project, -1);

					if ( type == 0 )
						document_open_file( filename, FALSE, NULL, NULL );

					g_free(filename);
					return TRUE;
				}
			}
		}
	}
	else if (event->button == 1)
	{	/* allow reclicking of taglist treeview item */
		if (widget == tv.tree_openfiles)
		{
			openfiles_go_to_selection(selection, 0);
			handled = TRUE;
		}
		else
			handled = taglist_go_to_selection(selection, 0, event->state);
	}
	else if (event->button == 3)
	{
		if (widget == tv.tree_openfiles)
		{
			if (!openfiles_popup_menu)
				create_openfiles_popup_menu();

			/* update menu item sensitivity */
			documents_menu_update(selection);
			gtk_menu_popup(GTK_MENU(openfiles_popup_menu), NULL, NULL, NULL, NULL,
					event->button, event->time);
		}
		else
		{
			gtk_menu_popup(GTK_MENU(tv.popup_taglist), NULL, NULL, NULL, NULL,
					event->button, event->time);
		}
		handled = TRUE;
	}
	return handled;
}

static gboolean debug_callstack_key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	may_steal_focus = FALSE;
	if (ui_is_keyval_enter_or_return(event->keyval) || event->keyval == GDK_space)
	{
		GtkWidgetClass *widget_class = GTK_WIDGET_GET_CLASS(widget);
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
		may_steal_focus = TRUE;

		/* force the TreeView handler to run before us for it to do its job (selection & stuff).
		 * doing so will prevent further handlers to be run in most cases, but the only one is our
		 * own, so guess it's fine. */
		if (widget_class->key_press_event)
			widget_class->key_press_event(widget, event);

		if (widget == tv.debug_callstack) /* tag and doc list have separate handlers */
		{
			GtkTreeIter iter;
			GtkTreeModel *model;

			if (gtk_tree_selection_get_selected(selection, &model, &iter))
			{
				gchar *filename;
				int line;
				int frame;

				gtk_tree_model_get(model, &iter, 0, &frame, -1);
				gtk_tree_model_get(model, &iter, 2, &filename, -1);
				gtk_tree_model_get(model, &iter, 3, &line, -1);

				GeanyDocument *doc = document_find_by_real_path( filename );
				if ( !DOC_VALID(doc) )
				{
					doc = document_open_file( filename, FALSE, NULL, NULL );
				}

				gint page = document_get_notebook_page(doc);
				gtk_notebook_set_current_page( GTK_NOTEBOOK(main_widgets.notebook), page );
				editor_goto_line( doc->editor, line-1, 0 );

				if ( debug_pid )
				{
					gchar szFrame[ 50 ];
					sprintf( szFrame, "set frame %d\n", frame );
					write(gdb_in.fd, szFrame, strlen(szFrame) );
				}

				g_free(filename);
			}
		}

		return TRUE;
	}
	return FALSE;
}

static gboolean debug_callstack_button_press_cb(GtkWidget *widget, GdkEventButton *event, G_GNUC_UNUSED gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkWidgetClass *widget_class = GTK_WIDGET_GET_CLASS(widget);
	gboolean handled = FALSE;

	/* force the TreeView handler to run before us for it to do its job (selection & stuff).
	 * doing so will prevent further handlers to be run in most cases, but the only one is our own,
	 * so guess it's fine. */
	if (widget_class->button_press_event)
		handled = widget_class->button_press_event(widget, event);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	may_steal_focus = TRUE;

	if (event->type == GDK_2BUTTON_PRESS || event->button == 1)
	{	
		if (widget == tv.debug_callstack) /* tag and doc list have separate handlers */
		{
			GtkTreeIter iter;
			GtkTreeModel *model;

			if (gtk_tree_selection_get_selected(selection, &model, &iter))
			{
				gchar *filename;
				int line;
				int frame;

				gtk_tree_model_get(model, &iter, 0, &frame, -1);
				gtk_tree_model_get(model, &iter, 2, &filename, -1);
				gtk_tree_model_get(model, &iter, 3, &line, -1);

				GeanyDocument *doc = document_find_by_real_path( filename );
				if ( !DOC_VALID(doc) )
				{
					doc = document_open_file( filename, FALSE, NULL, NULL );
				}

				gint page = document_get_notebook_page(doc);
				gtk_notebook_set_current_page( GTK_NOTEBOOK(main_widgets.notebook), page );
				editor_goto_line( doc->editor, line-1, 0 );

				if ( debug_pid )
				{
					gchar szFrame[ 50 ];
					sprintf( szFrame, "set frame %d\n", frame );
					write(gdb_in.fd, szFrame, strlen(szFrame) );
				}

				g_free(filename);

				handled = TRUE;
			}
		}
	}

	return handled;
}

static void documents_menu_update(GtkTreeSelection *selection)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean sel;
	GeanyProject *project = NULL;
	//gchar *shortname = NULL;
	//GeanyDocument *doc = NULL;
	int type;

	/* maybe no selection e.g. if ctrl-click deselected */
	sel = gtk_tree_selection_get_selected(selection, &model, &iter);
	if (sel)
	{
		gtk_tree_model_get(model, &iter, DOCUMENTS_TYPE, &type, -1);
		gtk_tree_model_get(model, &iter, DOCUMENTS_PROJECT, &project, -1);

		if ( project )
		{
			gtk_widget_hide(doc_items.add_item);
			if ( type == 0 || type == 2 ) gtk_menu_item_set_label( GTK_MENU_ITEM(doc_items.remove_item), _("Remove From Project") );
			else if ( type == 1 ) gtk_menu_item_set_label( GTK_MENU_ITEM(doc_items.remove_item), _("Close Project") );
		}
		else 
		{
			gtk_widget_show(doc_items.add_item);
			gtk_menu_item_set_label( GTK_MENU_ITEM(doc_items.remove_item), _("Close Document") );
		}
			
		gtk_widget_set_sensitive(doc_items.expand_all, TRUE);
		gtk_widget_set_sensitive(doc_items.collapse_all, TRUE);
	}
}


static StashGroup *stash_group = NULL;

static void on_load_settings(void)
{
	tag_window = ui_lookup_widget(main_widgets.window, "scrolledwindow2");

	prepare_debug_tab();
	prepare_openfiles();
	/* note: ui_prefs.sidebar_page is reapplied after plugins are loaded */
	stash_group_display(stash_group, NULL);
	sidebar_tabs_show_hide(GTK_NOTEBOOK(main_widgets.sidebar_notebook), NULL, 0, NULL);
}


static void on_save_settings(void)
{
	stash_group_update(stash_group, NULL);
	sidebar_tabs_show_hide(GTK_NOTEBOOK(main_widgets.sidebar_notebook), NULL, 0, NULL);
}


void sidebar_init(void)
{
	StashGroup *group;

	group = stash_group_new(PACKAGE);
	stash_group_add_boolean(group, &documents_show_paths, "documents_show_paths", TRUE);
	stash_group_add_widget_property(group, &ui_prefs.sidebar_page, "sidebar_page", GINT_TO_POINTER(0),
		main_widgets.sidebar_notebook, "page", 0);
	configuration_add_pref_group(group, FALSE);
	stash_group = group;

	/* delay building documents treeview until sidebar font has been read */
	g_signal_connect(geany_object, "load-settings", on_load_settings, NULL);
	g_signal_connect(geany_object, "save-settings", on_save_settings, NULL);

	g_signal_connect(main_widgets.sidebar_notebook, "page-added",
		G_CALLBACK(sidebar_tabs_show_hide), NULL);
	g_signal_connect(main_widgets.sidebar_notebook, "page-removed",
		G_CALLBACK(sidebar_tabs_show_hide), NULL);
	/* tabs may have changed when sidebar is reshown */
	g_signal_connect(main_widgets.sidebar_notebook, "show",
		G_CALLBACK(sidebar_tabs_show_hide), NULL);

	sidebar_tabs_show_hide(GTK_NOTEBOOK(main_widgets.sidebar_notebook), NULL, 0, NULL);
}

#define WIDGET(w) w && GTK_IS_WIDGET(w)

void sidebar_finalize(void)
{
	if (WIDGET(tv.default_tag_tree))
	{
		gtk_widget_destroy(tv.default_tag_tree); /* make GTK release its references, if any... */
		g_object_unref(tv.default_tag_tree); /* ...and release our own */
	}
	if (WIDGET(tv.popup_taglist))
		gtk_widget_destroy(tv.popup_taglist);
	if (WIDGET(openfiles_popup_menu))
		gtk_widget_destroy(openfiles_popup_menu);
}


void sidebar_focus_openfiles_tab(void)
{
	if (ui_prefs.sidebar_visible && interface_prefs.sidebar_openfiles_visible)
	{
		GtkNotebook *notebook = GTK_NOTEBOOK(main_widgets.sidebar_notebook);

		gtk_notebook_set_current_page(notebook, TREEVIEW_OPENFILES);
		gtk_widget_grab_focus(tv.tree_openfiles);
	}
}


void sidebar_focus_symbols_tab(void)
{
	if (ui_prefs.sidebar_visible && interface_prefs.sidebar_symbol_visible)
	{
		GtkNotebook *notebook = GTK_NOTEBOOK(main_widgets.sidebar_notebook);
		GtkWidget *symbol_list_scrollwin = gtk_notebook_get_nth_page(notebook, TREEVIEW_SYMBOL);

		gtk_notebook_set_current_page(notebook, TREEVIEW_SYMBOL);
		gtk_widget_grab_focus(gtk_bin_get_child(GTK_BIN(symbol_list_scrollwin)));
	}
}

static void sidebar_tabs_show_hide(GtkNotebook *notebook, GtkWidget *child,
								   guint page_num, gpointer data)
{
	gint tabs = gtk_notebook_get_n_pages(notebook);

	if (interface_prefs.sidebar_symbol_visible == FALSE)
		tabs--;
	if (interface_prefs.sidebar_openfiles_visible == FALSE)
		tabs--;

	gtk_notebook_set_show_tabs(notebook, (tabs > 1));
}

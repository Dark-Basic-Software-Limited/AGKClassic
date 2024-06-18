/*
 *      project.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2007-2012 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
 *      Copyright 2007-2012 Nick Treleaven <nick(dot)treleaven(at)btinternet(dot)com>
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

/** @file project.h
 * Project Management.
 */

#include "geany.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <glib/gstdio.h>

#include "project.h"

#include "dialogs.h"
#include "support.h"
#include "utils.h"
#include "ui_utils.h"
#include "document.h"
#include "msgwindow.h"
#include "main.h"
#include "keyfile.h"
#include "win32.h"
#include "build.h"
#include "editor.h"
#include "stash.h"
#include "sidebar.h"
#include "filetypes.h"
#include "templates.h"

#include "miniz.h"

#ifdef __APPLE__
    #import <Foundation/Foundation.h>
    NSFileManager* g_pFileManager = 0;
    NSURLConnection* m_connection = 0;
#endif

GPtrArray *projects_array = NULL;

ProjectPrefs project_prefs = { NULL, FALSE, FALSE };

static GSList *stash_groups = NULL;

GlobalProjectPrefs global_project_prefs = { NULL };

static gboolean entries_modified;

/* simple struct to keep references to the elements of the properties dialog */
typedef struct _PropertyDialogElements
{
	GtkWidget *dialog;
	GtkWidget *notebook;
	GtkWidget *name;
	GtkWidget *description;
	GtkWidget *file_name;
	GtkWidget *base_path;
	GtkWidget *patterns;
	BuildTableData build_properties;
	gint build_page_num;
} PropertyDialogElements;


static gboolean update_config(const PropertyDialogElements *e, gboolean new_project);
static void on_file_save_button_clicked(GtkButton *button, PropertyDialogElements *e);
static gboolean load_config(const gchar *filename);
static gboolean write_config(GeanyProject *project, gboolean emit_signal);
static void on_name_entry_changed(GtkEditable *editable, PropertyDialogElements *e);
static void on_entries_changed(GtkEditable *editable, PropertyDialogElements *e);
static void on_radio_long_line_custom_toggled(GtkToggleButton *radio, GtkWidget *spin_long_line);
static void apply_editor_prefs(void);
static void init_stash_prefs(void);


#define SHOW_ERR(args) dialogs_show_msgbox(GTK_MESSAGE_ERROR, args)
#define SHOW_ERR1(args, more) dialogs_show_msgbox(GTK_MESSAGE_ERROR, args, more)
#define MAX_NAME_LEN 50
/* "projects" is part of the default project base path so be careful when translating
 * please avoid special characters and spaces, look at the source for details or ask Frank */
#define PROJECT_DIR _("projects")

static gint ios_exporting_player = 0;

#define AGK_CLEAR_STR(dst) if((dst)) g_free((dst)); (dst)

/* TODO: this should be ported to Glade like the project preferences dialog,
 * then we can get rid of the PropertyDialogElements struct altogether as
 * widgets pointers can be accessed through ui_lookup_widget(). */
void project_new(void)
{
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *image;
	GtkWidget *button;
	GtkWidget *bbox;
	GtkWidget *label;
	PropertyDialogElements *e;

	e = g_new0(PropertyDialogElements, 1);
	e->dialog = gtk_dialog_new_with_buttons(_("New Project"), GTK_WINDOW(main_widgets.window),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	gtk_widget_set_name(e->dialog, "GeanyDialogProject");
	bbox = gtk_hbox_new(FALSE, 0);
	button = gtk_button_new();
	gtk_widget_set_can_default(button, TRUE);
	gtk_window_set_default(GTK_WINDOW(e->dialog), button);
	image = gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_BUTTON);
	label = gtk_label_new_with_mnemonic(_("C_reate"));
	gtk_box_pack_start(GTK_BOX(bbox), image, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(bbox), label, FALSE, FALSE, 3);
	gtk_container_add(GTK_CONTAINER(button), bbox);
	gtk_dialog_add_action_widget(GTK_DIALOG(e->dialog), button, GTK_RESPONSE_OK);
	gtk_window_set_default_size(GTK_WINDOW(e->dialog),500,100);

	vbox = ui_dialog_vbox_new(GTK_DIALOG(e->dialog));

	entries_modified = FALSE;

	table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);

	label = gtk_label_new(_("Name:"));
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0);

	e->name = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(e->name), TRUE);
	ui_entry_add_clear_icon(GTK_ENTRY(e->name));
	gtk_entry_set_max_length(GTK_ENTRY(e->name), MAX_NAME_LEN);

	ui_table_add_row(GTK_TABLE(table), 0, label, e->name, NULL);
	
	label = gtk_label_new(_("Base path:"));
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0);

	e->base_path = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(e->base_path), TRUE);
	ui_entry_add_clear_icon(GTK_ENTRY(e->base_path));
	gtk_widget_set_tooltip_text(e->base_path,
		_("Base directory of all files that make up the project. "
		"This can be a new path, or an existing directory tree. "
		"Must be an absolute path."));
	bbox = ui_path_box_new(_("Choose Project Base Path"),
		GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_ENTRY(e->base_path));

	ui_table_add_row(GTK_TABLE(table), 1, label, bbox, NULL);

	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

	/* signals */
	g_signal_connect(e->name, "changed", G_CALLBACK(on_name_entry_changed), e);
	/* run the callback manually to initialise the base_path and file_name fields */
	on_name_entry_changed(GTK_EDITABLE(e->name), e);

	g_signal_connect(e->base_path, "changed", G_CALLBACK(on_entries_changed), e);

	gtk_widget_show_all(e->dialog);

	while (gtk_dialog_run(GTK_DIALOG(e->dialog)) == GTK_RESPONSE_OK)
	{
		if (update_config(e, TRUE))
		{
			if (!write_config(app->project,TRUE))
				SHOW_ERR(_("Project file could not be written"));
			else
			{
				ui_set_statusbar(TRUE, _("Project \"%s\" created."), app->project->name);

				sidebar_openfiles_add_project( app->project );
				project_update_list();

				ui_add_recent_project_file(app->project->file_name);

				// write some default files
				/*
				gchar *new_filename = g_build_filename( app->project->base_path, "setup.agc", NULL );
				if ( g_file_test( new_filename, G_FILE_TEST_EXISTS ) == 0 )
					copy_template_file("setup.agc", new_filename);
				g_free(new_filename);
				*/

				gchar *new_filename = g_build_filename( app->project->base_path, "main.agc", NULL );
				if ( g_file_test( new_filename, G_FILE_TEST_EXISTS ) == 0 )
					copy_template_file("main.agc", new_filename);
				else
					project_add_file( app->project, new_filename, TRUE );
				g_free(new_filename);

				break;
			}
		}
	}
	gtk_widget_destroy(e->dialog);
	g_free(e);
}


gboolean project_load_file_with_session(const gchar *locale_file_name)
{
	if (project_load_file(locale_file_name))
	{
		if (project_prefs.project_session)
		{
			// todo active this when project sessions work
			//configuration_open_files();
		}
		return TRUE;
	}
	return FALSE;
}


//#ifndef G_OS_WIN32
static void run_open_dialog(GtkDialog *dialog)
{
	while (gtk_dialog_run(dialog) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		
		if ( project_find_by_filename( filename ) )
		{
			gchar *utf8_filename = utils_get_utf8_from_locale(filename);

			SHOW_ERR1(_("Project file \"%s\" is already open"), utf8_filename);
			g_free(utf8_filename);
			g_free(filename);
			continue;
		}

		/* try to load the config */
		if (! project_load_file_with_session(filename))
		{
			gchar *utf8_filename = utils_get_utf8_from_locale(filename);

			SHOW_ERR1(_("Project file \"%s\" could not be loaded."), utf8_filename);
			gtk_widget_grab_focus(GTK_WIDGET(dialog));
			g_free(utf8_filename);
			g_free(filename);
			continue;
		}

		g_free(filename);
		break;
	}
}

static void run_import_dialog(GtkDialog *dialog)
{
	while (gtk_dialog_run(dialog) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		
		gchar *new_file = g_strdup( filename );
		gchar *ext = strrchr( new_file, '.' );
		if ( ext )
		{
			ext[1] = 'a'; ext[2] = 'g'; ext[3] = 'k'; ext[4] = 0;
		}
		if ( project_find_by_filename( new_file ) )
		{
			gchar *utf8_filename = utils_get_utf8_from_locale(filename);

			SHOW_ERR1(_("Project file \"%s\" is already open"), utf8_filename);
			g_free(utf8_filename);
			g_free(new_file);
			g_free(filename);
			continue;
		}

		/* try to load the config */
		if (! project_import_from_file(filename))
		{
			SHOW_ERR1(_("Project file \"%s\" could not be loaded."), filename);
			g_free(new_file);
			g_free(filename);
			continue;
		}

		g_free(new_file);
		g_free(filename);
		break;
	}
}
//#endif


void project_open(void)
{
	const gchar *dir = global_project_prefs.project_file_path;
	gchar *file;
	GtkWidget *dialog;
	GtkFileFilter *filter;
	gchar *locale_path;

	
#ifdef G_OS_WIN32
	if (interface_prefs.use_native_windows_dialogs)
	{
		file = win32_show_project_open_dialog(main_widgets.window, _("Open Project"), dir, FALSE, "AGK Project Files (*.agk)\t*.agk\t");
		if (file != NULL)
		{
			if ( project_find_by_filename( file ) )
			{
				/*
				gchar *utf8_filename = utils_get_utf8_from_locale(file);

				SHOW_ERR1(_("Project file \"%s\" is already open"), utf8_filename);
				g_free(utf8_filename);
				*/
				g_free(file);
				return;
			}

			/* try to load the config */
			if (! project_load_file_with_session(file))
			{
				SHOW_ERR1(_("Project file \"%s\" could not be loaded."), file);
			}
			g_free(file);
		}
	}
	else
#endif
	{
		dialog = gtk_file_chooser_dialog_new(_("Open Project"), GTK_WINDOW(main_widgets.window),
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
		gtk_widget_set_name(dialog, "GeanyDialogProject");

		/* set default Open, so pressing enter can open multiple files */
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
		gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
		gtk_window_set_type_hint(GTK_WINDOW(dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
		gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_widgets.window));
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);

		/* add FileFilters */
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, _("AGK Project files"));
		gtk_file_filter_add_pattern(filter, "*." GEANY_PROJECT_EXT);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

		locale_path = utils_get_locale_from_utf8(dir);
		if (g_file_test(locale_path, G_FILE_TEST_EXISTS) &&
			g_file_test(locale_path, G_FILE_TEST_IS_DIR))
		{
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), locale_path);
		}
		g_free(locale_path);

		gtk_widget_show_all(dialog);
		run_open_dialog(GTK_DIALOG(dialog));
		gtk_widget_destroy(GTK_WIDGET(dialog));
	}
}


void project_import(void)
{
	const gchar *dir = global_project_prefs.project_file_path;
	gchar *file;
	GtkWidget *dialog;
	GtkFileFilter *filter;
	gchar *locale_path;
	
#ifdef G_OS_WIN32
	if (interface_prefs.use_native_windows_dialogs)
	{
		file = win32_show_project_open_dialog(main_widgets.window, _("Import Project"), dir, FALSE, "Old AGK Projects (*.cbp)\t*.cbp\t");
		if (file != NULL)
		{
			gchar *new_file = g_strdup( file );
			gchar *ext = strrchr( new_file, '.' );
			if ( ext )
			{
				ext[1] = 'a'; ext[2] = 'g'; ext[3] = 'k'; ext[4] = 0;
			}
			if ( project_find_by_filename( new_file ) )
			{
				/*
				gchar *utf8_filename = utils_get_utf8_from_locale(file);

				SHOW_ERR1(_("Project file \"%s\" is already open"), utf8_filename);
				g_free(utf8_filename);
				*/
				g_free(new_file);
				g_free(file);
				return;
			}

			/* try to load the config */
			if (! project_import_from_file(file))
			{
				SHOW_ERR1(_("Project file \"%s\" could not be loaded."), file);
			}
			g_free(new_file);
			g_free(file);
		}
	}
	else
#endif
	{
		dialog = gtk_file_chooser_dialog_new(_("Import Project"), GTK_WINDOW(main_widgets.window),
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
		gtk_widget_set_name(dialog, "GeanyDialogProject");

		/* set default Open, so pressing enter can open multiple files */
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
		gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
		gtk_window_set_type_hint(GTK_WINDOW(dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
		gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_widgets.window));
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);

		/* add FileFilters */
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, _("Old AGK Projects"));
		gtk_file_filter_add_pattern(filter, "*.cbp");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

		locale_path = utils_get_locale_from_utf8(dir);
		if (g_file_test(locale_path, G_FILE_TEST_EXISTS) &&
			g_file_test(locale_path, G_FILE_TEST_IS_DIR))
		{
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), locale_path);
		}
		g_free(locale_path);

		gtk_widget_show_all(dialog);
		run_import_dialog(GTK_DIALOG(dialog));
		gtk_widget_destroy(GTK_WIDGET(dialog));
	}
}

static void on_html5_dialog_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	static int running = 0;
	if ( running ) return;

	running = 1;

	// save current values
	if ( app->project )
	{
		GtkWidget *widget;
		widget = ui_lookup_widget(ui_widgets.html5_dialog, "html5_commands_combo");
		app->project->html5_settings.commands_used = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));
		
		widget = ui_lookup_widget(ui_widgets.html5_dialog, "html5_dynamic_memory");
		app->project->html5_settings.dynamic_memory = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );
				
		// output
		widget = ui_lookup_widget(ui_widgets.html5_dialog, "html5_output_file_entry");
		AGK_CLEAR_STR(app->project->html5_settings.output_path) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
	}

	if ( response != 1 )
	{
		gtk_widget_hide(GTK_WIDGET(dialog));
	}
	else
	{
		int i;
		GtkWidget *widget;

		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.html5_dialog, "html5_export1"), FALSE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.html5_dialog, "button12"), FALSE );
		
		while (gtk_events_pending())
			gtk_main_iteration();

		// app details
		widget = ui_lookup_widget(ui_widgets.html5_dialog, "html5_commands_combo");
		int html5_command_int = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));
		int commands_mode = -1;
		if ( html5_command_int == 1 ) commands_mode = 1;
		else if ( html5_command_int == 0 ) commands_mode = 0;

		widget = ui_lookup_widget(ui_widgets.html5_dialog, "html5_dynamic_memory");
		int dynamic_memory = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );
				
		// output
		widget = ui_lookup_widget(ui_widgets.html5_dialog, "html5_output_file_entry");
		gchar *output_file = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		// START CHECKS

		if ( !output_file || !*output_file ) { SHOW_ERR(_("You must choose an output location to save your HTML5 files")); goto html5_dialog_clean_up; }
		if ( commands_mode < 0 ) { SHOW_ERR(_("Unrecognised choice for 'commands used' drop down box")); goto html5_dialog_clean_up; }

		goto html5_dialog_continue;

html5_dialog_clean_up:
		if ( output_file ) g_free(output_file);

		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.html5_dialog, "html5_export1"), TRUE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.html5_dialog, "button12"), TRUE );
		running = 0;
		return;

html5_dialog_continue:

		while (gtk_events_pending())
			gtk_main_iteration();

		// CHECKS COMPLETE, START EXPORT

		// make temporary folder
		gchar* tmp_folder = g_build_filename( app->project->base_path, "build_tmp", NULL );
		utils_str_replace_char( tmp_folder, '\\', '/' );
		
		const gchar *szCommandsFolder = "";
		if ( dynamic_memory ) szCommandsFolder = commands_mode ? "3Ddynamic" : "2Ddynamic";
		else szCommandsFolder = commands_mode ? "3D" : "2D";

		gchar* src_folder = g_build_path( "/", app->datadir, "html5", szCommandsFolder, NULL );
		utils_str_replace_char( src_folder, '\\', '/' );

		// decalrations
		gchar sztemp[30];
		gchar *newcontents = 0;
		gchar *load_package_string = g_new0( gchar, 200000 );
		gchar *additional_folders_string = g_new0( gchar, 200000 );
		gchar* agkplayer_file = NULL;
		gchar* html5data_file = NULL;
		gchar *contents = NULL;
		gchar *contents2 = NULL;
		gchar *contents3 = NULL;
		gsize length = 0;
		GError *error = NULL;
		FILE *pHTML5File = 0;
		gchar *media_folder = 0;

		mz_zip_archive zip_archive;
		memset(&zip_archive, 0, sizeof(zip_archive));
		gchar *str_out = NULL;
				
		if ( !utils_copy_folder( src_folder, tmp_folder, TRUE, NULL ) )
		{
			SHOW_ERR( _("Failed to copy source folder") );
			goto html5_dialog_cleanup2;
		}

		while (gtk_events_pending())
			gtk_main_iteration();

		// create HTML5 data file that we'll add all the media files to
		html5data_file = g_build_path( "/", tmp_folder, "AGKPlayer.data", NULL );
		pHTML5File = fopen( html5data_file, "wb" );
		if ( !pHTML5File )
		{
			SHOW_ERR( _("Failed to open HTML5 data file for writing") );
			goto html5_dialog_cleanup2;
		}

		// start the load package string that will store the list of files, it will be built at the same time as adding the media files
		strcpy( load_package_string, "loadPackage({\"files\":[" );
		strcpy( additional_folders_string, "Module[\"FS_createPath\"](\"/\", \"media\", true, true);" );
		media_folder = g_build_path( "/", app->project->base_path, "media", NULL );
		int currpos = 0;

		if ( g_file_test (media_folder, G_FILE_TEST_EXISTS) )
		{
			// add the media files and construct the load package string, currpos will have the total data size afterwards
			if ( !utils_add_folder_to_html5_data_file( pHTML5File, media_folder, "/media", load_package_string, additional_folders_string, &currpos ) )
			{
				fclose( pHTML5File );
				pHTML5File = 0;

				SHOW_ERR( _("Failed to write HTML5 data file") );
				goto html5_dialog_cleanup2;
			}
		}

		fclose( pHTML5File );
		pHTML5File = 0;

		// remove the final comma that was added
		if ( *load_package_string && load_package_string[strlen(load_package_string)-1] == ',' ) load_package_string[ strlen(load_package_string) - 1 ] = 0;

		// finsh the load package string 
		strcat( load_package_string, "],\"remote_package_size\":" );
		sprintf( sztemp, "%d", currpos );
		strcat( load_package_string, sztemp );
		strcat( load_package_string, ",\"package_uuid\":\"e3c8dd30-b68a-4332-8c93-d0cf8f9d28a0\"})" );

		
		// edit AGKplayer.js to add our load package string
		agkplayer_file = g_build_path( "/", tmp_folder, "AGKPlayer.js", NULL );

		if ( !g_file_get_contents( agkplayer_file, &contents, &length, &error ) )
		{
			SHOW_ERR1( _("Failed to read AGKPlayer.js file: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto html5_dialog_cleanup2;
		}

		newcontents = g_new0( gchar, length + 400000 );

		contents2 = contents;
		contents3 = 0;

		// the order of these relacements is important (if more than one), they must occur in the same order as they occur in the file

		// replace %%ADDITIONALFOLDERS%%
		contents3 = strstr( contents2, "%%ADDITIONALFOLDERS%%" );
		if ( contents3 )
		{
			*contents3 = 0;
			contents3 += strlen("%%ADDITIONALFOLDERS%%");

			strcat( newcontents, contents2 );
			strcat( newcontents, additional_folders_string );
						
			contents2 = contents3;
		}
		else
		{
			SHOW_ERR( _("AGKPlayer.js is corrupt, it is missing the %%ADDITIONALFOLDERS%% variable") );
			goto html5_dialog_cleanup2;
		}

		// replace %%LOADPACKAGE%%
		contents3 = strstr( contents2, "%%LOADPACKAGE%%" );
		if ( contents3 )
		{
			*contents3 = 0;
			contents3 += strlen("%%LOADPACKAGE%%");

			strcat( newcontents, contents2 );
			strcat( newcontents, load_package_string );
						
			contents2 = contents3;
		}
		else
		{
			SHOW_ERR( _("AGKPlayer.js is corrupt, it is missing the %%LOADPACKAGE%% variable") );
			goto html5_dialog_cleanup2;
		}

		// write the rest of the file
		strcat( newcontents, contents2 );
	
		// write new AGKPlayer.js file
		if ( !g_file_set_contents( agkplayer_file, newcontents, strlen(newcontents), &error ) )
		{
			SHOW_ERR1( _("Failed to write AGKPlayer.js file: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto html5_dialog_cleanup2;
		}

		while (gtk_events_pending())
			gtk_main_iteration();

		// reuse variables
		if ( html5data_file ) g_free(html5data_file);
		if ( agkplayer_file ) g_free(agkplayer_file);

		// create zip file
		/*
		if ( !mz_zip_writer_init_file( &zip_archive, output_file, 0 ) )
		{
			SHOW_ERR( "Failed to initialise zip file for writing" );
			goto html5_dialog_cleanup2;
		}

		// copy files to zip
		html5data_file = g_build_path( "/", tmp_folder, "AGKPlayer.asm.js", NULL );
		mz_zip_writer_add_file( &zip_archive, "AGKPlayer.asm.js", html5data_file, NULL, 0, 9 );
		g_free( html5data_file );

		html5data_file = g_build_path( "/", tmp_folder, "AGKPlayer.js", NULL );
		mz_zip_writer_add_file( &zip_archive, "AGKPlayer.js", html5data_file, NULL, 0, 9 );
		g_free( html5data_file );

		html5data_file = g_build_path( "/", tmp_folder, "AGKPlayer.data", NULL );
		mz_zip_writer_add_file( &zip_archive, "AGKPlayer.data", html5data_file, NULL, 0, 9 );
		g_free( html5data_file );

		html5data_file = g_build_path( "/", tmp_folder, "AGKPlayer.html.mem", NULL );
		mz_zip_writer_add_file( &zip_archive, "AGKPlayer.html.mem", html5data_file, NULL, 0, 9 );
		g_free( html5data_file );

		// create main html5 file with project name so it stands out as the file to run
		agkplayer_file = g_new0( gchar, 1024 );
		strcpy( agkplayer_file, app->project->name );
		utils_str_replace_char( agkplayer_file, ' ', '_' );
		strcat( agkplayer_file, ".html" );
		html5data_file = g_build_path( "/", tmp_folder, "AGKPlayer.html", NULL );
		mz_zip_writer_add_file( &zip_archive, agkplayer_file, html5data_file, NULL, 0, 9 );
		*/

		utils_mkdir( output_file, TRUE );

		// copy files to folder
		html5data_file = g_build_path( "/", tmp_folder, "AGKPlayer.asm.js", NULL );
		agkplayer_file = g_build_path( "/", output_file, "AGKPlayer.asm.js", NULL );
		utils_copy_file( html5data_file, agkplayer_file, TRUE, NULL );
		g_free( agkplayer_file );
		g_free( html5data_file );

		html5data_file = g_build_path( "/", tmp_folder, "AGKPlayer.js", NULL );
		agkplayer_file = g_build_path( "/", output_file, "AGKPlayer.js", NULL );
		utils_copy_file( html5data_file, agkplayer_file, TRUE, NULL );
		g_free( agkplayer_file );
		g_free( html5data_file );

		html5data_file = g_build_path( "/", tmp_folder, "AGKPlayer.data", NULL );
		agkplayer_file = g_build_path( "/", output_file, "AGKPlayer.data", NULL );
		utils_copy_file( html5data_file, agkplayer_file, TRUE, NULL );
		g_free( agkplayer_file );
		g_free( html5data_file );

		html5data_file = g_build_path( "/", tmp_folder, "AGKPlayer.html.mem", NULL );
		agkplayer_file = g_build_path( "/", output_file, "AGKPlayer.html.mem", NULL );
		utils_copy_file( html5data_file, agkplayer_file, TRUE, NULL );
		g_free( agkplayer_file );
		g_free( html5data_file );

		html5data_file = g_build_path( "/", tmp_folder, "background.jpg", NULL );
		agkplayer_file = g_build_path( "/", output_file, "background.jpg", NULL );
		utils_copy_file( html5data_file, agkplayer_file, TRUE, NULL );
		g_free( agkplayer_file );
		g_free( html5data_file );

		html5data_file = g_build_path( "/", tmp_folder, "made-with-appgamekit.png", NULL );
		agkplayer_file = g_build_path( "/", output_file, "made-with-appgamekit.png", NULL );
		utils_copy_file( html5data_file, agkplayer_file, TRUE, NULL );
		g_free( agkplayer_file );
		g_free( html5data_file );

		// create main html5 file with project name so it stands out as the file to run
		html5data_file = g_new0( gchar, 1024 );
		strcpy( html5data_file, app->project->name );
		utils_str_replace_char( html5data_file, ' ', '_' );
		strcat( html5data_file, ".html" );
		agkplayer_file = g_build_path( "/", output_file, html5data_file, NULL );
		g_free( html5data_file );
		html5data_file = g_build_path( "/", tmp_folder, "AGKPlayer.html", NULL );
		utils_copy_file( html5data_file, agkplayer_file, TRUE, NULL );

		while (gtk_events_pending())
			gtk_main_iteration();

		/*
		if ( !mz_zip_writer_finalize_archive( &zip_archive ) )
		{
			SHOW_ERR( _("Failed to finalize zip file") );
			goto html5_dialog_cleanup2;
		}
		if ( !mz_zip_writer_end( &zip_archive ) )
		{
			SHOW_ERR( _("Failed to end zip file") );
			goto html5_dialog_cleanup2;
		}
		*/

		while (gtk_events_pending())
			gtk_main_iteration();

		gtk_widget_hide(GTK_WIDGET(dialog));

html5_dialog_cleanup2:
        
        gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.html5_dialog, "html5_export1"), TRUE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.html5_dialog, "button12"), TRUE );

		utils_remove_folder_recursive( tmp_folder );

		if ( newcontents ) g_free(newcontents);
		if ( contents ) g_free(contents);
		if ( load_package_string ) g_free(load_package_string);
		if ( additional_folders_string ) g_free(additional_folders_string);
		if ( agkplayer_file ) g_free(agkplayer_file);
		if ( html5data_file ) g_free(html5data_file);
		if ( media_folder ) g_free(media_folder);
		if ( pHTML5File ) fclose(pHTML5File);

		if ( error ) g_error_free(error);
		
		if ( tmp_folder ) g_free(tmp_folder);
		if ( src_folder ) g_free(src_folder);
		if ( output_file ) g_free(output_file);
	}

	running = 0;
}

void project_export_html5()
{
	static gchar *last_proj_path = 0;

	if ( !app->project ) 
	{
		SHOW_ERR( _("You must have a project open to export it") );
		return;
	}

	// make sure the project is up to date
	build_compile_project(0);

	if (ui_widgets.html5_dialog == NULL)
	{
		ui_widgets.html5_dialog = create_html5_dialog();
		gtk_widget_set_name(ui_widgets.html5_dialog, _("Export HTML5"));
		gtk_window_set_transient_for(GTK_WINDOW(ui_widgets.html5_dialog), GTK_WINDOW(main_widgets.window));

		g_signal_connect(ui_widgets.html5_dialog, "response", G_CALLBACK(on_html5_dialog_response), NULL);
        g_signal_connect(ui_widgets.html5_dialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);

		//ui_setup_open_button_callback_html5(ui_lookup_widget(ui_widgets.html5_dialog, "html5_app_icon_path"), NULL,
		//	GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.html5_dialog, "html5_app_icon_entry")));
		
		ui_setup_open_button_callback_html5(ui_lookup_widget(ui_widgets.html5_dialog, "html5_output_file_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_ENTRY(ui_lookup_widget(ui_widgets.html5_dialog, "html5_output_file_entry")));

		gtk_combo_box_set_active( GTK_COMBO_BOX(ui_lookup_widget(ui_widgets.html5_dialog, "html5_commands_combo")), 0 );
	}

	if ( strcmp( FALLBACK(last_proj_path,""), FALLBACK(app->project->file_name,"") ) != 0 )
	{
		if ( last_proj_path ) g_free(last_proj_path);
		last_proj_path = g_strdup( FALLBACK(app->project->file_name,"") );
	
		GtkWidget *widget;

		// set defaults for this project
		widget = ui_lookup_widget(ui_widgets.html5_dialog, "html5_commands_combo");
		gtk_combo_box_set_active( GTK_COMBO_BOX(widget), app->project->html5_settings.commands_used );

		widget = ui_lookup_widget(ui_widgets.html5_dialog, "html5_dynamic_memory");
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), app->project->html5_settings.dynamic_memory ? 1 : 0 );
		
		if ( !app->project->html5_settings.output_path || !*app->project->html5_settings.output_path )
		{
			gchar* html5_path = g_build_filename( app->project->base_path, "HTML5", NULL );
			widget = ui_lookup_widget(ui_widgets.html5_dialog, "html5_output_file_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), html5_path );
			g_free(html5_path);
		}
		else
		{
			widget = ui_lookup_widget(ui_widgets.html5_dialog, "html5_output_file_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), app->project->html5_settings.output_path );
		}
	}

	gtk_window_present(GTK_WINDOW(ui_widgets.html5_dialog));
}

static void on_android_dialog_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	static int running = 0;
	if ( running ) return;

	running = 1;
    
#ifdef __APPLE__
    gchar* android_path = g_build_path( "/", app->configdir, "AndroidExport", "aapt2", NULL );
    if ( !g_file_test(android_path, G_FILE_TEST_EXISTS) )
    {
        g_free( android_path);
        if ( m_connection ) SHOW_ERR( "Android export files have not finished downloading, please wait and try again" );
        else SHOW_ERR( "Android export files not found. Please close the export dialog and reopen it to try downloading again" );
        return;
    }
    g_free( android_path);
#endif

	// save default settings
	if ( app->project && user_data == 0 )
	{
		GtkWidget *widget;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_app_name_entry");
		AGK_CLEAR_STR(app->project->apk_settings.app_name) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_package_name_entry");
		AGK_CLEAR_STR(app->project->apk_settings.package_name) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_app_icon_entry");
		AGK_CLEAR_STR(app->project->apk_settings.app_icon_path) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_notif_icon_entry");
		AGK_CLEAR_STR(app->project->apk_settings.notif_icon_path) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_ouya_icon_entry");
		AGK_CLEAR_STR(app->project->apk_settings.ouya_icon_path) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_firebase_config_entry");
		AGK_CLEAR_STR(app->project->apk_settings.firebase_config_path) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_orientation_combo");
		app->project->apk_settings.orientation = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_arcore_combo");
		app->project->apk_settings.arcore = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_sdk_combo");
		gchar *app_sdk = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
		app->project->apk_settings.sdk_version = 1; // 4.1
		if ( strncmp(app_sdk,"4.2",3) == 0 ) app->project->apk_settings.sdk_version = 2;
		if ( strncmp(app_sdk,"4.3",3) == 0 ) app->project->apk_settings.sdk_version = 3;
		if ( strncmp(app_sdk,"4.4",3) == 0 ) app->project->apk_settings.sdk_version = 4;
		if ( strncmp(app_sdk,"5.0",3) == 0 ) app->project->apk_settings.sdk_version = 5;
		if ( strncmp(app_sdk,"5.1",3) == 0 ) app->project->apk_settings.sdk_version = 6;
		if ( strncmp(app_sdk,"6.0",3) == 0 ) app->project->apk_settings.sdk_version = 7;
		if ( strncmp(app_sdk,"7.0",3) == 0 ) app->project->apk_settings.sdk_version = 8;
		if ( strncmp(app_sdk,"7.1",3) == 0 ) app->project->apk_settings.sdk_version = 9;
		if ( strncmp(app_sdk,"8.0",3) == 0 ) app->project->apk_settings.sdk_version = 10;
		if ( strncmp(app_sdk,"8.1",3) == 0 ) app->project->apk_settings.sdk_version = 11;
		if ( strncmp(app_sdk,"9.0",3) == 0 ) app->project->apk_settings.sdk_version = 12;
		if ( strncmp(app_sdk,"10.0",3) == 0 ) app->project->apk_settings.sdk_version = 13;
		g_free(app_sdk);
				
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_url_scheme");
		AGK_CLEAR_STR(app->project->apk_settings.url_scheme) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_deep_link");
		AGK_CLEAR_STR(app->project->apk_settings.deep_link) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_google_play_app_id");
		AGK_CLEAR_STR(app->project->apk_settings.play_app_id) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_admob_app_id");
		AGK_CLEAR_STR(app->project->apk_settings.admob_app_id) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_snapchat_client_id");
		AGK_CLEAR_STR(app->project->apk_settings.snapchat_client_id) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		// permissions
		app->project->apk_settings.permission_flags = 0;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_external_storage");
		if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) app->project->apk_settings.permission_flags |= AGK_ANDROID_PERMISSION_WRITE;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_location_fine");
		if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) app->project->apk_settings.permission_flags |= AGK_ANDROID_PERMISSION_GPS;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_location_coarse");
		if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) app->project->apk_settings.permission_flags |= AGK_ANDROID_PERMISSION_LOCATION;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_internet");
		if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) app->project->apk_settings.permission_flags |= AGK_ANDROID_PERMISSION_INTERNET;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_wake");
		if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) app->project->apk_settings.permission_flags |= AGK_ANDROID_PERMISSION_WAKE;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_billing");
		if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) app->project->apk_settings.permission_flags |= AGK_ANDROID_PERMISSION_IAP;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_push_notifications");
		if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) app->project->apk_settings.permission_flags |= AGK_ANDROID_PERMISSION_PUSH;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_camera");
		if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) app->project->apk_settings.permission_flags |= AGK_ANDROID_PERMISSION_CAMERA;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_expansion");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) app->project->apk_settings.permission_flags |= AGK_ANDROID_PERMISSION_EXPANSION;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_vibrate");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) app->project->apk_settings.permission_flags |= AGK_ANDROID_PERMISSION_VIBRATE;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_record_audio");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) app->project->apk_settings.permission_flags |= AGK_ANDROID_PERMISSION_RECORD_AUDIO;

		// signing
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_keystore_file_entry");
		AGK_CLEAR_STR(app->project->apk_settings.keystore_path) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_version_number_entry");
		AGK_CLEAR_STR(app->project->apk_settings.version_name) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_build_number_entry");
		int build_number = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
		app->project->apk_settings.version_number = build_number;

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_alias_entry");
		AGK_CLEAR_STR(app->project->apk_settings.alias) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		// output
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_output_file_entry");
		AGK_CLEAR_STR(app->project->apk_settings.output_path) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_output_type_combo");
		app->project->apk_settings.app_type = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));
	}

	if ( response != 1 )
	{
		if ( dialog ) gtk_widget_hide(GTK_WIDGET(dialog));
	}
	else
	{
		int i;
		GtkWidget *widget;

		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.android_dialog, "android_export1"), FALSE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.android_dialog, "button7"), FALSE );
		
		while (gtk_events_pending())
			gtk_main_iteration();

		// app details
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_app_name_entry");
		gchar *app_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_package_name_entry");
		gchar *package_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_app_icon_entry");
		gchar *app_icon = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_notif_icon_entry");
		gchar *notif_icon = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_ouya_icon_entry");
		gchar *ouya_icon = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_firebase_config_entry");
		gchar *firebase_config = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_orientation_combo");
		int app_orientation_int = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));
		int orientation = 10;
		if ( app_orientation_int == 0 ) orientation = 6;
		else if ( app_orientation_int == 1 ) orientation = 7;
		gchar szOrientation[ 20 ];
		sprintf( szOrientation, "%d", orientation );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_arcore_combo");
		int arcore_mode = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));
				
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_sdk_combo");
		gchar *app_sdk = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
		int sdk = 16;
		if ( strncmp(app_sdk,"4.2",3) == 0 ) sdk = 17;
		if ( strncmp(app_sdk,"4.3",3) == 0 ) sdk = 18;
		if ( strncmp(app_sdk,"4.4",3) == 0 ) sdk = 19;
		if ( strncmp(app_sdk,"5.0",3) == 0 ) sdk = 21;
		if ( strncmp(app_sdk,"5.1",3) == 0 ) sdk = 22;
		if ( strncmp(app_sdk,"6.0",3) == 0 ) sdk = 23;
		if ( strncmp(app_sdk,"7.0",3) == 0 ) sdk = 24;
		if ( strncmp(app_sdk,"7.1",3) == 0 ) sdk = 25;
		if ( strncmp(app_sdk,"8.0",3) == 0 ) sdk = 26;
		if ( strncmp(app_sdk,"8.1",3) == 0 ) sdk = 27;
		if ( strncmp(app_sdk,"9.0",3) == 0 ) sdk = 28;
		if ( strncmp(app_sdk,"10.0",3) == 0 ) sdk = 29;
		g_free(app_sdk);
		gchar szSDK[ 20 ];
		sprintf( szSDK, "%d", sdk );
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_url_scheme");
		gchar *url_scheme = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_deep_link");
		gchar *deep_link = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_google_play_app_id");
		gchar *google_play_app_id = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_admob_app_id");
		gchar *admob_app_id = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_snapchat_client_id");
		gchar *snapchat_client_id = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		// permissions
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_external_storage");
		int permission_external_storage = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_location_fine");
		int permission_location_fine = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_location_coarse");
		int permission_location_coarse = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_internet");
		int permission_internet = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_wake");
		int permission_wake = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_billing");
		int permission_billing = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_push_notifications");
		int permission_push = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_camera");
		int permission_camera = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_expansion");
		int permission_expansion = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_vibrate");
		int permission_vibrate = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_record_audio");
		int permission_record_audio = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		// signing
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_keystore_file_entry");
		gchar *keystore_file = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_keystore_password_entry");
		gchar *keystore_password = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_version_number_entry");
		gchar *version_number = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		if ( !*version_number ) SETPTR( version_number, g_strdup("1.0.0") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_build_number_entry");
		int build_number = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
		if ( build_number == 0 ) build_number = 1;
		gchar szBuildNum[ 20 ];
		sprintf( szBuildNum, "%d", build_number );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_alias_entry");
		gchar *alias_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_alias_password_entry");
		gchar *alias_password = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		// output
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_output_file_entry");
		gchar *output_file = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_output_type_combo");
		gchar *output_type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
		int app_type = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));

		int isGoogle = (app_type == 0 || app_type == 1) ? 1 : 0;
		int isAmazon = (app_type == 2) ? 1 : 0;
		int isOuya = (app_type == 3) ? 1 : 0;
		int isBundle = (app_type == 0) ? 1 : 0;
				
		gchar *percent = 0;
		while ( (percent = strchr(output_file, '%')) != 0 )
		{
			if ( strncmp( percent+1, "[version]", strlen("[version]") ) == 0 )
			{
				*percent = 0;
				percent += strlen("[version]") + 1;
				gchar *new_output = g_strconcat( output_file, szBuildNum, percent, NULL );
				g_free(output_file);
				output_file = new_output;
				continue;
			}

			if ( strncmp( percent+1, "[type]", strlen("[type]") ) == 0 )
			{
				*percent = 0;
				percent += strlen("[type]") + 1;
				gchar *new_output = g_strconcat( output_file, output_type, percent, NULL );
				g_free(output_file);
				output_file = new_output;
				continue;
			}

			break;
		}

		g_free(output_type);

		// START CHECKS

		if ( !output_file || !*output_file ) { SHOW_ERR(_("You must choose an output location to save your APK")); goto android_dialog_clean_up; }
		if ( strchr(output_file, '.') == 0 ) { SHOW_ERR(_("The output location must be a file not a directory")); goto android_dialog_clean_up; }

		const char* desiredExt = ".apk";
		if ( isBundle ) desiredExt = ".aab";

		char* ext = strrchr( output_file, '.' );
		if ( ext && strlen(ext) < 6 ) strcpy( ext, desiredExt );
		else strcat( output_file, desiredExt );

		if ( g_file_test( output_file, G_FILE_TEST_EXISTS ) )
		{
			if ( !dialogs_show_question("Output file already exists, do you want to overwrite it?") )
			{
				goto android_dialog_clean_up;
			}

			g_unlink( output_file );
		}

		// check app name
		if ( !app_name || !*app_name ) { SHOW_ERR(_("You must enter an app name")); goto android_dialog_clean_up; }
		if ( strlen(app_name) > 30 ) { SHOW_ERR(_("App name must be less than 30 characters")); goto android_dialog_clean_up; }
		for( i = 0; i < strlen(app_name); i++ )
		{
			/*
			if ( (app_name[i] < 97 || app_name[i] > 122)
			  && (app_name[i] < 65 || app_name[i] > 90) 
			  && (app_name[i] < 48 || app_name[i] > 57) 
			  && app_name[i] != 32 
			  && app_name[i] != 45
			  && app_name[i] != 95 ) 
			{ 
				SHOW_ERR(_("App name contains invalid characters, must be A-Z, 0-9, dash, spaces, and undersore only")); 
				goto android_dialog_clean_up; 
			}
			*/
			//switch to black list
			if ( app_name[i] == 34 || app_name[i] == 60 || app_name[i] == 62 || app_name[i] == 39 )
			{
				SHOW_ERR(_("App name contains invalid characters, it must not contain quotes or < > characters.")); 
				goto android_dialog_clean_up; 
			}
		}
		
		// check package name
		if ( !package_name || !*package_name ) { SHOW_ERR(_("You must enter a package name")); goto android_dialog_clean_up; }
		if ( strlen(package_name) > 100 ) { SHOW_ERR(_("Package name must be less than 100 characters")); goto android_dialog_clean_up; }
		if ( strchr(package_name,'.') == NULL ) { SHOW_ERR(_("Package name must contain at least one dot character")); goto android_dialog_clean_up; }
		if ( (package_name[0] < 65 || package_name[0] > 90) && (package_name[0] < 97 || package_name[0] > 122) ) { SHOW_ERR(_("Package name must begin with a letter")); goto android_dialog_clean_up; }
		if ( package_name[strlen(package_name)-1] == '.' ) { SHOW_ERR(_("Package name must not end with a dot")); goto android_dialog_clean_up; }

		gchar last = 0;
		for( i = 0; i < strlen(package_name); i++ )
		{
			if ( last == '.' && (package_name[i] < 65 || package_name[i] > 90) && (package_name[i] < 97 || package_name[i] > 122) )
			{
				SHOW_ERR(_("Package name invalid, a dot must be followed by a letter"));
				goto android_dialog_clean_up; 
			}

			if ( (package_name[i] < 97 || package_name[i] > 122) // a-z
			  && (package_name[i] < 65 || package_name[i] > 90) // A-Z
			  && (package_name[i] < 48 || package_name[i] > 57) //0-9
			  && package_name[i] != 46 // .
			  && package_name[i] != 95 ) // _
			{ 
				SHOW_ERR(_("Package name contains invalid characters, must be A-Z 0-9 . and undersore only")); 
				goto android_dialog_clean_up; 
			}

			last = package_name[i];
		}

		if ( url_scheme && *url_scheme )
		{
			if ( strchr(url_scheme, ':') || strchr(url_scheme, '/') )
			{
				SHOW_ERR(_("URL scheme must not contain : or /"));
				goto android_dialog_clean_up; 
			}
		}

		if ( deep_link && *deep_link )
		{
			if ( strncmp( deep_link, "https://", strlen("https://") ) != 0 && strncmp( deep_link, "http://", strlen("http://") ) != 0 )
			{
				SHOW_ERR(_("Deep link must start with http:// or https://"));
				goto android_dialog_clean_up; 
			}

			if ( strcmp( deep_link, "https://" ) == 0 || strcmp( deep_link, "http://" ) == 0 )
			{
				SHOW_ERR(_("Deep link must have a domain after http:// or https://"));
				goto android_dialog_clean_up; 
			}
		}

		// check icon
		//if ( !app_icon || !*app_icon ) { SHOW_ERR(_("You must select an app icon")); goto android_dialog_clean_up; }
		if ( app_icon && *app_icon )
		{
			if ( !strrchr( app_icon, '.' ) || utils_str_casecmp( strrchr( app_icon, '.' ), ".png" ) != 0 ) { SHOW_ERR(_("App icon must be a PNG file")); goto android_dialog_clean_up; }
			if ( !g_file_test( app_icon, G_FILE_TEST_EXISTS ) ) { SHOW_ERR(_("Could not find app icon location")); goto android_dialog_clean_up; }
		}

		if ( notif_icon && *notif_icon )
		{
			if ( !strrchr( notif_icon, '.' ) || utils_str_casecmp( strrchr( notif_icon, '.' ), ".png" ) != 0 ) { SHOW_ERR(_("Notification icon must be a PNG file")); goto android_dialog_clean_up; }
			if ( !g_file_test( notif_icon, G_FILE_TEST_EXISTS ) ) { SHOW_ERR(_("Could not find notification icon location")); goto android_dialog_clean_up; }
		}

		if ( isOuya )
		{
			//if ( !ouya_icon || !*ouya_icon ) { SHOW_ERR(_("You must select an Ouya large icon")); goto android_dialog_clean_up; }
			if ( ouya_icon && *ouya_icon )
			{
				if ( !strrchr( ouya_icon, '.' ) || utils_str_casecmp( strrchr( ouya_icon, '.' ), ".png" ) != 0 ) { SHOW_ERR(_("Ouya large icon must be a PNG file")); goto android_dialog_clean_up; }
				if ( !g_file_test( ouya_icon, G_FILE_TEST_EXISTS ) ) { SHOW_ERR(_("Could not find ouya large icon location")); goto android_dialog_clean_up; }
			}
		}

		// check firebase config file
		if ( firebase_config && *firebase_config )
		{
			if ( !strrchr( firebase_config, '.' ) || utils_str_casecmp( strrchr( firebase_config, '.' ), ".json" ) != 0 ) { SHOW_ERR(_("Google services config file must be a .json file")); goto android_dialog_clean_up; }
			if ( !g_file_test( firebase_config, G_FILE_TEST_EXISTS ) ) { SHOW_ERR(_("Could not find Google services config file")); goto android_dialog_clean_up; }
		}
				
		// check version
		if ( version_number && *version_number )
		{
			for( i = 0; i < strlen(version_number); i++ )
			{
				if ( (version_number[i] < 48 || version_number[i] > 57) && version_number[i] != 46 ) 
				{ 
					SHOW_ERR(_("Version name contains invalid characters, must be 0-9 and . only")); 
					goto android_dialog_clean_up; 
				}
			}
		}

		// check keystore
		if ( keystore_file && *keystore_file )
		{
			if ( !g_file_test( keystore_file, G_FILE_TEST_EXISTS ) ) { SHOW_ERR(_("Could not find keystore file location")); goto android_dialog_clean_up; }
		}

		// check passwords
		if ( keystore_password && strchr(keystore_password,'"') ) { SHOW_ERR(_("Keystore password cannot contain double quotes")); goto android_dialog_clean_up; }
		if ( alias_password && strchr(alias_password,'"') ) { SHOW_ERR(_("Alias password cannot contain double quotes")); goto android_dialog_clean_up; }

		if ( keystore_file && *keystore_file )
		{
			if ( !keystore_password || !*keystore_password ) { SHOW_ERR(_("You must enter your keystore password when using your own keystore")); goto android_dialog_clean_up; }
		}

		if ( alias_name && *alias_name )
		{
			if ( !alias_password || !*alias_password ) { SHOW_ERR(_("You must enter your alias password when using a custom alias")); goto android_dialog_clean_up; }
		}
		
		int includeFirebase = (firebase_config && *firebase_config && (isGoogle || isAmazon)) ? 1 : 0;
		int includePushNotify = (permission_push && isGoogle) ? 1 : 0;
		int includeGooglePlay = (google_play_app_id && *google_play_app_id && isGoogle) ? 1 : 0;
		int includeAdMob = (admob_app_id && *admob_app_id && isGoogle) ? 1 : 0;

		if ( includePushNotify && !includeFirebase )
		{
			SHOW_ERR( _("Push Notifications on Android now use Firebase, so you must include a Firebase config file to use them") );
			goto android_dialog_clean_up;
		}

		goto android_dialog_continue;

android_dialog_clean_up:
		if ( app_name ) g_free(app_name);
		if ( package_name ) g_free(package_name);
		if ( app_icon ) g_free(app_icon);
		if ( ouya_icon ) g_free(ouya_icon);
		if ( notif_icon ) g_free(notif_icon);
		if ( firebase_config ) g_free(firebase_config);
		if ( url_scheme ) g_free(url_scheme);
		if ( deep_link ) g_free(deep_link);
		if ( google_play_app_id ) g_free(google_play_app_id);
		if ( admob_app_id ) g_free(admob_app_id);
		if ( snapchat_client_id ) g_free(snapchat_client_id);

		if ( keystore_file ) g_free(keystore_file);
		if ( keystore_password ) g_free(keystore_password);
		if ( version_number ) g_free(version_number);
		if ( alias_name ) g_free(alias_name);
		if ( alias_password ) g_free(alias_password);

		if ( output_file ) g_free(output_file);

		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.android_dialog, "android_export1"), TRUE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.android_dialog, "button7"), TRUE );
		running = 0;
		return;

android_dialog_continue:

		while (gtk_events_pending())
			gtk_main_iteration();

		// CHECKS COMPLETE, START EXPORT

		const char* androidJar = "android30.jar";

#if defined(G_OS_WIN32)
		gchar* path_to_aapt2 = g_build_path( "\\", app->datadir, "android", "aapt2-bundle.exe", NULL );
		gchar* path_to_android_jar = g_build_path( "\\", app->datadir, "android", androidJar, NULL );
		gchar* path_to_bundletool = g_build_path( "\\", app->datadir, "android", "bundletool.jar", NULL );
		gchar* path_to_apksigner = g_build_path( "\\", app->datadir, "android", "apksigner.jar", NULL );
		gchar* path_to_zipalign = g_build_path( "\\", app->datadir, "android", "zipalign.exe", NULL );

		// convert forward slashes to backward slashes for parameters that will be passed to aapt2
		gchar *pathPtr = path_to_android_jar;
		while( *pathPtr ) { if ( *pathPtr == '/' ) *pathPtr = '\\'; pathPtr++; }

		pathPtr = output_file;
		while( *pathPtr ) { if ( *pathPtr == '/' ) *pathPtr = '\\'; pathPtr++; }
        
        gchar* android_folder = g_build_filename( app->datadir, "android", NULL );
        gchar* src_folder;
        if ( isOuya ) src_folder = g_build_path( "/", app->datadir, "android", "sourceOuya", NULL );
        else if ( isAmazon ) src_folder = g_build_path( "/", app->datadir, "android", "sourceAmazon", NULL );
        else src_folder = g_build_path( "/", app->datadir, "android", "sourceGoogle", NULL );
#elif defined(__APPLE__)
        gchar* path_to_aapt2 = g_build_path( "/", app->configdir, "AndroidExport", "aapt2-bundle", NULL );
        gchar* path_to_android_jar = g_build_path( "/", app->configdir, "AndroidExport", androidJar, NULL );
        gchar* path_to_bundletool = g_build_path( "\\", app->configdir, "android", "bundletool.jar", NULL );
		gchar* path_to_apksigner = g_build_path( "\\", app->configdir, "android", "apksigner.jar", NULL );
        gchar* path_to_zipalign = g_build_path( "/", app->configdir, "AndroidExport", "zipalign", NULL );
        
        gchar* android_folder = g_build_filename( app->configdir, "AndroidExport", NULL );
        gchar* src_folder;
        if ( isOuya ) src_folder = g_build_path( "/", app->configdir, "AndroidExport", "sourceOuya", NULL );
        else if ( isAmazon ) src_folder = g_build_path( "/", app->configdir, "AndroidExport", "sourceAmazon", NULL );
        else src_folder = g_build_path( "/", app->configdir, "AndroidExport", "sourceGoogle", NULL );
#else
		
	gchar* path_to_aapt2 = g_build_path( "/", app->datadir, "android", "aapt2-bundle", NULL );
	gchar* path_to_android_jar = g_build_path( "/", app->datadir, "android", androidJar, NULL );
	// mike - paths are wrong
	gchar* path_to_bundletool = g_build_path( "\\", app->datadir, "android", "bundletool.jar", NULL );
	gchar* path_to_apksigner = g_build_path( "\\", app->datadir, "android", "apksigner.jar", NULL );
	//gchar* path_to_bundletool = g_build_path( "/", app->datadir, "android", "bundletool.jar", NULL );
	//gchar* path_to_apksigner = g_build_path( "/", app->datadir, "android", "apksigner.jar", NULL );
	gchar* path_to_zipalign = g_build_path( "/", app->datadir, "android", "zipalign", NULL );
	
        gchar* android_folder = g_build_filename( app->datadir, "android", NULL );
        gchar* src_folder;
        if ( isOuya ) src_folder = g_build_path( "/", app->datadir, "android", "sourceOuya", NULL );
        else if ( isAmazon ) src_folder = g_build_path( "/", app->datadir, "android", "sourceAmazon", NULL );
        else src_folder = g_build_path( "/", app->datadir, "android", "sourceGoogle", NULL );
     
#endif

		// make temporary folder
		gchar* tmp_folder = g_build_filename( app->project->base_path, "build_tmp", NULL );
		
		utils_str_replace_char( android_folder, '\\', '/' );
		utils_str_replace_char( tmp_folder, '\\', '/' );
		utils_str_replace_char( src_folder, '\\', '/' );

		utils_remove_folder_recursive( tmp_folder );
		
		gchar *output_file_zip = g_strdup( output_file );
		ext = strrchr( output_file_zip, '.' );
		if ( ext && strlen(ext) < 6 ) *ext = 0;
		SETPTR( output_file_zip, g_strconcat( output_file_zip, ".zip", NULL ) );

		if ( g_file_test( output_file_zip, G_FILE_TEST_EXISTS ) )
		{
			g_unlink( output_file_zip );
		}

		if ( !keystore_file || !*keystore_file )
		{
			if ( keystore_file ) g_free(keystore_file);
			if ( keystore_password ) g_free(keystore_password);

			keystore_file = g_build_path( "/", android_folder, "debug.keystore", NULL );
			keystore_password = g_strdup("android");

			if ( alias_name ) g_free(alias_name);
			if ( alias_password ) g_free(alias_password);

			alias_name = g_strdup("androiddebugkey");
			alias_password = g_strdup("android");
		}
		else
		{
			if ( !alias_name || !*alias_name )
			{
				if ( alias_name ) g_free(alias_name);
				if ( alias_password ) g_free(alias_password);

				alias_name = g_strdup("mykeystore");
				alias_password = g_strdup(keystore_password);
			}
		}

#define AGK_NEW_CONTENTS_SIZE 1000000

		// declarations
		gchar *newcontents = g_new0( gchar, AGK_NEW_CONTENTS_SIZE );
		gchar *newcontents2 = g_new0( gchar, AGK_NEW_CONTENTS_SIZE );
		gchar* manifest_file = NULL;
		gchar *contents = NULL;
		gchar *contents2 = NULL;
		gchar *contents3 = NULL;
		gchar *contentsOther = NULL;
		gchar *contentsOther2 = NULL;
		gchar *contentsOther3 = NULL;
		gsize length = 0;
		gchar* resources_file = NULL;
		GError *error = NULL;
		GdkPixbuf *icon_image = NULL;
		gchar *image_filename = NULL;
		GdkPixbuf *icon_scaled_image = NULL;
		gchar **argv = NULL;
		gint status = 0;
		mz_zip_archive zip_archive;
		memset(&zip_archive, 0, sizeof(zip_archive));
		gchar *zip_add_file = 0;
		gchar *str_out = NULL;
		gsize resLength = 0;
		gint package_count = 0;
		gint package_index = 0;

		// check for java
		argv = g_new0( gchar*, 3 );
		argv[0] = g_strdup( "which" );
		argv[1] = g_strdup( "java" );
		argv[2] = NULL;

		#ifdef G_OS_WIN32
			argv [ 0 ] = g_strdup ( "where" );
		#endif

		if ( !utils_spawn_sync( app->project->base_path, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to check for Java: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto android_dialog_cleanup2;
		}

#ifdef G_OS_WIN32
		if ( !str_out || str_out[0] == 0 || str_out[1] != ':' ) 
#else
		if ( !str_out || str_out[0] != '/' ) 
#endif
		{
			SHOW_ERR( _("Could not find Java, make sure you have the Java Development Kit (JDK) 8 or above installed") );
			goto android_dialog_cleanup2;
		}

		if ( str_out ) g_free(str_out);
		str_out = 0;

		argv[1] = g_strdup( "jarsigner" );

		if ( !utils_spawn_sync( app->project->base_path, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to check for Jarsigner: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto android_dialog_cleanup2;
		}

#ifdef G_OS_WIN32
		if ( !str_out || str_out[0] == 0 || str_out[1] != ':' ) 
#else
		if ( !str_out || str_out[0] != '/' ) 
#endif
		{
			SHOW_ERR( _("Could not find Jarsigner, make sure you have the Java Development Kit (JDK) 8 or above installed") );
			goto android_dialog_cleanup2;
		}

		if ( str_out ) g_free(str_out);
		str_out = 0;

		g_strfreev(argv);
		argv = 0;

		while (gtk_events_pending())
			gtk_main_iteration();

		// copy android export files to tmp folder
		if ( !utils_copy_folder( src_folder, tmp_folder, TRUE, NULL ) )
		{
			SHOW_ERR1( _("Failed to copy source folder %s"), src_folder );
			goto android_dialog_cleanup2;
		}

		while (gtk_events_pending())
			gtk_main_iteration();
		
		// edit AndroidManifest.xml
		manifest_file = g_build_path( "/", tmp_folder, "AndroidManifest.xml", NULL );

		if ( !g_file_get_contents( manifest_file, &contents, &length, NULL ) )
		{
			SHOW_ERR( _("Failed to read AndroidManifest.xml file") );
			goto android_dialog_cleanup2;
		}

		strcpy( newcontents, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"\n\
      android:versionCode=\"" );
		strcat( newcontents, szBuildNum );
		strcat( newcontents, "\"\n      android:versionName=\"" );
		strcat( newcontents, version_number );
		strcat( newcontents, "\" package=\"" );
		strcat( newcontents, package_name );
		strcat( newcontents, "\"" );
		strcat( newcontents, " android:installLocation=\"auto\">\n\
    <uses-feature android:glEsVersion=\"0x00020000\"></uses-feature>\n\
    <uses-sdk android:minSdkVersion=\"" );
		
		strcat( newcontents, szSDK );
		
		// mike - 021221 - ensure we target SDK version 30
		strcat( newcontents, "\" android:targetSdkVersion=\"" );
		if ( isGoogle ) strcat( newcontents, "30" );
		else if ( isAmazon ) strcat( newcontents, "22" );
		else strcat( newcontents, "16" );
		strcat( newcontents, "\" />\n\n" );

		if ( permission_external_storage ) strcat( newcontents, "    <uses-permission android:name=\"android.permission.WRITE_EXTERNAL_STORAGE\"></uses-permission>\n" );
		if ( permission_internet ) 
		{
			strcat( newcontents, "    <uses-permission android:name=\"android.permission.INTERNET\"></uses-permission>\n" );
			strcat( newcontents, "    <uses-permission android:name=\"android.permission.ACCESS_NETWORK_STATE\"></uses-permission>\n" );
			strcat( newcontents, "    <uses-permission android:name=\"android.permission.ACCESS_WIFI_STATE\"></uses-permission>\n" );
		}
		if ( permission_wake ) strcat( newcontents, "    <uses-permission android:name=\"android.permission.WAKE_LOCK\"></uses-permission>\n" );
		if ( permission_location_coarse && isGoogle ) strcat( newcontents, "    <uses-permission android:name=\"android.permission.ACCESS_COARSE_LOCATION\"></uses-permission>\n" );
		if ( permission_location_fine && isGoogle ) strcat( newcontents, "    <uses-permission android:name=\"android.permission.ACCESS_FINE_LOCATION\"></uses-permission>\n" );
		if ( permission_billing && isGoogle ) strcat( newcontents, "    <uses-permission android:name=\"com.android.vending.BILLING\"></uses-permission>\n" );
		if ( permission_camera ) strcat( newcontents, "    <uses-permission android:name=\"android.permission.CAMERA\"></uses-permission>\n" );
		if ( ((google_play_app_id && *google_play_app_id) || permission_push) && isGoogle ) strcat( newcontents, "    <uses-permission android:name=\"com.google.android.c2dm.permission.RECEIVE\" />\n" );
		if ( permission_push && isGoogle ) 
		{
			strcat( newcontents, "    <permission android:name=\"" );
			strcat( newcontents, package_name );
			strcat( newcontents, ".permission.C2D_MESSAGE\" android:protectionLevel=\"signature\" />\n" );
			strcat( newcontents, "    <uses-permission android:name=\"" );
			strcat( newcontents, package_name );
			strcat( newcontents, ".permission.C2D_MESSAGE\" />\n" );
		}
		if ( permission_expansion && isGoogle ) 
		{
			//strcat( newcontents, "    <uses-permission android:name=\"android.permission.GET_ACCOUNTS\"></uses-permission>\n" );
			strcat( newcontents, "    <uses-permission android:name=\"com.android.vending.CHECK_LICENSE\"></uses-permission>\n" );
			strcat( newcontents, "    <uses-permission android:name=\"android.permission.FOREGROUND_SERVICE\"></uses-permission>\n" );
		}
		if ( permission_vibrate ) strcat( newcontents, "    <uses-permission android:name=\"android.permission.VIBRATE\"></uses-permission>\n" );
		if ( permission_record_audio ) strcat( newcontents, "    <uses-permission android:name=\"android.permission.RECORD_AUDIO\"></uses-permission>\n" );
		
		// supports FireTV
		if ( 0 )
		{
			strcat( newcontents, "    <uses-feature android:name=\"android.hardware.touchscreen\" android:required=\"false\" />\n" );
		}

		// if ARCore required
		if ( arcore_mode == 2 )
		{
			strcat( newcontents, "    <uses-feature android:name=\"android.hardware.camera.ar\" android:required=\"true\" />" );
		}

		contents2 = contents;
		contents3 = 0;

		// the order of these relacements is important, they must occur in the same order as they occur in the file

		// replace orientation
		contents3 = strstr( contents2, "screenOrientation=\"fullSensor\"" );
		if ( contents3 )
		{
			*contents3 = 0;
			contents3 += strlen("screenOrientation=\"fullSensor");

			strcat( newcontents, contents2 );

			switch( orientation )
			{
				case 6: strcat( newcontents, "screenOrientation=\"sensorLandscape" ); break;
				case 7: 
				{
					// all now use API 23 with correct spelling
					strcat( newcontents, "screenOrientation=\"sensorPortrait" ); 
					break;
				}
				default: strcat( newcontents, "screenOrientation=\"fullSensor" ); break;
			}

			contents2 = contents3;
		}

		// add intent filters
		contents3 = strstr( contents2, "<!--ADDITIONAL_INTENT_FILTERS-->" );
		if ( contents3 )
		{
			*contents3 = 0;
			contents3 += strlen("<!--ADDITIONAL_INTENT_FILTERS-->");

			strcat( newcontents, contents2 );

			if ( url_scheme && *url_scheme )
			{
				strcat( newcontents, "<intent-filter>\n\
			<action android:name=\"android.intent.action.VIEW\" />\n\
			<category android:name=\"android.intent.category.DEFAULT\" />\n\
			<category android:name=\"android.intent.category.BROWSABLE\" />\n\
			<data android:scheme=\"" );
			
				strcat( newcontents, url_scheme );
				strcat( newcontents, "\" />\n    </intent-filter>\n" );
			}

			if ( deep_link && *deep_link )
			{
				gchar *szScheme = 0;
				gchar *szHost = 0;
				gchar *szPath = 0;
				gchar *szTemp = strstr( deep_link, "://" );
				if ( szTemp )
				{
					*szTemp = 0;
					szScheme = g_strdup( deep_link );
					*szTemp = ':';

					szTemp += 3;
					gchar *szTemp2 = strstr( szTemp, "/" );
					if ( szTemp2 )
					{
						szPath = g_strdup( szTemp2 );
						*szTemp2 = 0;
						szHost = g_strdup( szTemp );
						*szTemp2 = '/';
					}
					else szHost = g_strdup( szTemp );
				}

				if ( szScheme && *szScheme )
				{
					strcat( newcontents, "<intent-filter>\n\
			<action android:name=\"android.intent.action.VIEW\" />\n\
			<category android:name=\"android.intent.category.DEFAULT\" />\n\
			<category android:name=\"android.intent.category.BROWSABLE\" />\n\
			<data android:scheme=\"" );
			
					strcat( newcontents, szScheme );
					if ( szHost && *szHost )
					{
						strcat( newcontents, "\" android:host=\"" );
						strcat( newcontents, szHost );

						if ( szPath && *szPath )
						{
							strcat( newcontents, "\" android:pathPrefix=\"" );
							strcat( newcontents, szPath );
						}
					}
			
					strcat( newcontents, "\" />\n    </intent-filter>\n" );
				}
				

				if ( szScheme ) g_free( szScheme );
				if ( szHost ) g_free( szHost );
				if ( szPath ) g_free( szPath );
			}

			contents2 = contents3;
		}

		// replace package name
		contents3 = strstr( contents2, "YOUR_PACKAGE_NAME_HERE" );
		if ( contents3 )
		{
			*contents3 = 0;
			contents3 += strlen("YOUR_PACKAGE_NAME_HERE");

			strcat( newcontents, contents2 );
			strcat( newcontents, package_name );
			contents2 = contents3;
		}

		// replace application ID
		contents3 = strstr( contents2, "${applicationId}" );
		while ( contents3 )
		{
			*contents3 = 0;
			contents3 += strlen("${applicationId}");

			strcat( newcontents, contents2 );
			strcat( newcontents, package_name );
			contents2 = contents3;
			contents3 = strstr( contents2, "${applicationId}" );
		}

		// write the rest of the manifest file
		strcat( newcontents, contents2 );

		if ( permission_expansion && isGoogle ) 
		{
			strcat( newcontents, "\n\
		<service android:name=\"com.google.android.vending.expansion.downloader.impl.DownloaderService\"\n\
            android:enabled=\"true\"/>\n\
        <receiver android:name=\"com.google.android.vending.expansion.downloader.impl.DownloaderService$AlarmReceiver\"\n\
            android:enabled=\"true\"/>" );
		}

		// Google sign in
		if ( isGoogle )
		{
			strcat( newcontents, "\n\
		<activity android:name=\"com.google.android.gms.auth.api.signin.internal.SignInHubActivity\"\n\
            android:excludeFromRecents=\"true\"\n\
            android:exported=\"false\"\n\
            android:theme=\"@android:style/Theme.Translucent.NoTitleBar\" />\n\
        <service android:name=\"com.google.android.gms.auth.api.signin.RevocationBoundService\"\n\
            android:exported=\"true\"\n\
            android:permission=\"com.google.android.gms.auth.api.signin.permission.REVOCATION_NOTIFICATION\" />\n" );
		}

		// IAP Purchase Activity
		if ( permission_billing && isGoogle )
		{
			strcat(newcontents, "\n\
			<meta-data\n\
				android:name=\"com.google.android.play.billingclient.version\"\n\
				android:value=\"3.0.3\" />\n\
			<activity\n\
				android:name=\"com.android.billingclient.api.ProxyBillingActivity\"\n\
				android:configChanges=\"keyboard|keyboardHidden|screenLayout|screenSize|orientation\"\n\
				android:exported=\"false\"\n\
				android:theme=\"@android:style/Theme.Translucent.NoTitleBar\" />\n");
		}

		// Google API Activity - for Game Services
		if ( includeGooglePlay )
		{
			strcat( newcontents, "\n\
        <activity android:name=\"com.google.android.gms.common.api.GoogleApiActivity\" \n\
                  android:exported=\"false\" \n\
                  android:theme=\"@android:style/Theme.Translucent.NoTitleBar\" />" );
		}

		// Firebase Init Provider - for Game Services and Firebase
		if ( includeGooglePlay || includeFirebase || includePushNotify )
		{
			strcat( newcontents, "\n        <provider android:authorities=\"" );
			strcat( newcontents, package_name );
			strcat( newcontents, ".firebaseinitprovider\"\n\
                  android:directBootAware=\"true\"\n\
                  android:name=\"com.google.firebase.provider.FirebaseInitProvider\"\n\
                  android:exported=\"false\"\n\
                  android:initOrder=\"100\" />\n" );
		}

		// Firebase activities
		if ( includeFirebase )
		{
			strcat( newcontents, "\n\
        <receiver\n\
            android:name=\"com.google.android.gms.measurement.AppMeasurementReceiver\"\n\
            android:enabled=\"true\"\n\
            android:exported=\"false\" >\n\
        </receiver>\n\
\n\
        <service android:name=\"com.google.android.gms.measurement.AppMeasurementService\"\n\
                 android:enabled=\"true\"\n\
                 android:exported=\"false\"/>\n\
        <service\n\
            android:name=\"com.google.android.gms.measurement.AppMeasurementJobService\"\n\
            android:enabled=\"true\"\n\
            android:exported=\"false\"\n\
            android:permission=\"android.permission.BIND_JOB_SERVICE\" />\n\
		<service\n\
            android:name=\"com.google.firebase.components.ComponentDiscoveryService\"\n\
			android:directBootAware=\"true\"\n\
            android:exported=\"false\" >\n\
			<meta-data\n\
                android:name=\"com.google.firebase.components:com.google.firebase.messaging.FirebaseMessagingRegistrar\"\n\
                android:value=\"com.google.firebase.components.ComponentRegistrar\" />\n\
            <meta-data\n\
                android:name=\"com.google.firebase.components:com.google.firebase.analytics.connector.internal.AnalyticsConnectorRegistrar\"\n\
                android:value=\"com.google.firebase.components.ComponentRegistrar\" />\n\
            <meta-data\n\
                android:name=\"com.google.firebase.components:com.google.firebase.datatransport.TransportRegistrar\"\n\
                android:value=\"com.google.firebase.components.ComponentRegistrar\" />\n\
            <meta-data\n\
                android:name=\"com.google.firebase.components:com.google.firebase.installations.FirebaseInstallationsRegistrar\"\n\
                android:value=\"com.google.firebase.components.ComponentRegistrar\" />\n\
        </service>\n\
		<service\
            android:name=\"com.google.android.datatransport.runtime.backends.TransportBackendDiscovery\"\n\
            android:exported=\"false\" >\n\
            <meta-data\n\
                android:name=\"backend:com.google.android.datatransport.cct.CctBackendFactory\"\n\
                android:value=\"cct\" />\n\
        </service>\n\
        <service\n\
            android:name=\"com.google.android.datatransport.runtime.scheduling.jobscheduling.JobInfoSchedulerService\"\n\
            android:exported=\"false\"\n\
            android:permission=\"android.permission.BIND_JOB_SERVICE\" >\n\
        </service>\n\
        <receiver\n\
            android:name=\"com.google.android.datatransport.runtime.scheduling.jobscheduling.AlarmManagerSchedulerBroadcastReceiver\"\n\
            android:exported=\"false\" />" );
		}

		if ( includeFirebase || includePushNotify )
		{
			strcat( newcontents, "\n\
        <receiver android:name=\"com.google.firebase.iid.FirebaseInstanceIdReceiver\" \n\
                  android:exported=\"true\" \n\
                  android:permission=\"com.google.android.c2dm.permission.SEND\" > \n\
            <intent-filter> \n\
                <action android:name=\"com.google.android.c2dm.intent.RECEIVE\" /> \n\
            </intent-filter> \n\
        </receiver>" );
		}

		if ( includePushNotify )
		{
			strcat( newcontents, "\n\
		<meta-data android:name=\"com.google.firebase.messaging.default_notification_icon\"\n\
            android:resource=\"@drawable/icon_white\" />\n\
		<service android:name=\"com.google.firebase.messaging.FirebaseMessagingService\" \n\
            android:exported=\"false\" > \n\
            <intent-filter android:priority=\"-500\" > \n\
                <action android:name=\"com.google.firebase.MESSAGING_EVENT\" /> \n\
            </intent-filter> \n\
        </service>" );
		}

		if ( includeAdMob )
		{
			strcat( newcontents, "\n\
        <provider\n\
            android:name=\"com.google.android.gms.ads.MobileAdsInitProvider\"\n\
            android:authorities=\"" );
			strcat( newcontents, package_name );
			strcat( newcontents, ".mobileadsinitprovider\"\n\
            android:exported=\"false\"\n\
            android:initOrder=\"100\" />" );
		}

		// arcore activity
		if ( arcore_mode > 0 )
		{
			strcat( newcontents, "\n\
		<meta-data android:name=\"com.google.ar.core\" android:value=\"");
			if ( arcore_mode == 1 ) strcat( newcontents, "optional" );
			else strcat( newcontents, "required" );
			strcat( newcontents, "\" />\n\
		<meta-data android:name=\"com.google.ar.core.min_apk_version\" android:value=\"190519000\" />\n\
        <activity\n\
            android:name=\"com.google.ar.core.InstallActivity\"\n\
            android:configChanges=\"keyboardHidden|orientation|screenSize\"\n\
            android:excludeFromRecents=\"true\"\n\
            android:exported=\"false\"\n\
            android:launchMode=\"singleTop\"\n\
            android:theme=\"@android:style/Theme.Material.Light.Dialog.Alert\" />" );
		}


		strcat( newcontents, "\n    </application>\n</manifest>\n" );
	
		// write new Android Manifest.xml file
		if ( !g_file_set_contents( manifest_file, newcontents, strlen(newcontents), &error ) )
		{
			SHOW_ERR1( _("Failed to write AndroidManifest.xml file: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto android_dialog_cleanup2;
		}

		if ( contents ) g_free(contents);
		contents = 0;

		// read resources file
		resources_file = g_build_path( "/", tmp_folder, "resOrig", "values", "values.xml", NULL );
		if ( !g_file_get_contents( resources_file, &contents, &resLength, &error ) )
		{
			SHOW_ERR1( _("Failed to read resource values.xml file: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto android_dialog_cleanup2;
		}

		contents2 = strstr( contents, "<string name=\"app_name\">" );
		if ( !contents2 )
		{
			SHOW_ERR( _("Could not find app name entry in values.xml file") );
			goto android_dialog_cleanup2;
		}

		contents2 += strlen("<string name=\"app_name\"");
		*contents2 = 0;
		contents3 = contents2;
		contents3++;
		contents3 = strstr( contents3, "</string>" );
		if ( !contents3 )
		{
			SHOW_ERR( _("Could not find end of app name entry in values.xml file") );
			goto android_dialog_cleanup2;
		}

		// write resources file
		strcpy( newcontents, contents );
		strcat( newcontents, ">" );
		strcat( newcontents, app_name );
		strcat( newcontents, contents3 );

		// repair original file
		*contents2 = '>';

		if ( isGoogle && google_play_app_id && *google_play_app_id )
		{
			memcpy( newcontents2, newcontents, AGK_NEW_CONTENTS_SIZE );
			contents2 = strstr( newcontents2, "<string name=\"games_app_id\">" );
			if ( !contents2 )
			{
				SHOW_ERR( _("Could not find games_app_id entry in values.xml file") );
				goto android_dialog_cleanup2;
			}

			contents2 += strlen("<string name=\"games_app_id\"");
			*contents2 = 0;
			contents3 = contents2;
			contents3++;
			contents3 = strstr( contents3, "</string>" );
			if ( !contents3 )
			{
				SHOW_ERR( _("Could not find end of games_app_id entry in values.xml file") );
				goto android_dialog_cleanup2;
			}

			// write resources file
			strcpy( newcontents, newcontents2 );
			strcat( newcontents, ">" );
			strcat( newcontents, google_play_app_id );
			strcat( newcontents, contents3 );

			// repair original file
			*contents2 = '>';
		}

		// admob app id
		if ( isGoogle && admob_app_id && *admob_app_id )
		{
			memcpy( newcontents2, newcontents, AGK_NEW_CONTENTS_SIZE );
			contents2 = strstr( newcontents2, "<string name=\"admob_app_id\">" );
			if ( !contents2 )
			{
				SHOW_ERR( _("Could not find admob_app_id entry in values.xml file") );
				goto android_dialog_cleanup2;
			}

			contents2 += strlen("<string name=\"admob_app_id\"");
			*contents2 = 0;
			contents3 = contents2;
			contents3++;
			contents3 = strstr( contents3, "</string>" );
			if ( !contents3 )
			{
				SHOW_ERR( _("Could not find end of admob_app_id entry in values.xml file") );
				goto android_dialog_cleanup2;
			}

			// write resources file
			strcpy( newcontents, newcontents2 );
			strcat( newcontents, ">" );
			strcat( newcontents, admob_app_id );
			strcat( newcontents, contents3 );

			// repair original file
			*contents2 = '>';
		}

		// snapchat client id
		if ( isGoogle && snapchat_client_id && *snapchat_client_id )
		{
			memcpy( newcontents2, newcontents, AGK_NEW_CONTENTS_SIZE );
			contents2 = strstr( newcontents2, "<string name=\"snap_chat_id\">" );
			if ( !contents2 )
			{
				SHOW_ERR( _("Could not find snap_chat_id entry in values.xml file") );
				goto android_dialog_cleanup2;
			}

			contents2 += strlen("<string name=\"snap_chat_id\"");
			*contents2 = 0;
			contents3 = contents2;
			contents3++;
			contents3 = strstr( contents3, "</string>" );
			if ( !contents3 )
			{
				SHOW_ERR( _("Could not find end of snap_chat_id entry in values.xml file") );
				goto android_dialog_cleanup2;
			}

			// write resources file
			strcpy( newcontents, newcontents2 );
			strcat( newcontents, ">" );
			strcat( newcontents, snapchat_client_id );
			strcat( newcontents, contents3 );

			// repair original file
			*contents2 = '>';
		}

		// firebase
		if ( firebase_config && *firebase_config && (isGoogle || isAmazon) ) // Google and Amazon only
		{
			// read json values
			if ( !g_file_get_contents( firebase_config, &contentsOther, &resLength, &error ) )
			{
				SHOW_ERR1( _("Failed to read firebase config file: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}

			memcpy( newcontents2, newcontents, AGK_NEW_CONTENTS_SIZE );

			// find project_number value
			{
				contentsOther2 = strstr( contentsOther, "\"project_number\": \"" );
				if ( !contentsOther2 )
				{
					SHOW_ERR( _("Could not find project_number entry in Firebase config file") );
					goto android_dialog_cleanup2;
				}

				contentsOther2 += strlen("\"project_number\": \"");
				contentsOther3 = strstr( contentsOther2, "\"" );
				if ( !contentsOther3 )
				{
					SHOW_ERR( _("Could not find end of project_number entry in Firebase config file") );
					goto android_dialog_cleanup2;
				}
				*contentsOther3 = 0;

				// find entry in newcontents2
				contents2 = strstr( newcontents2, "<string name=\"gcm_defaultSenderId\" translatable=\"false\"" );
				if ( !contents2 )
				{
					SHOW_ERR( _("Could not find gcm_defaultSenderId entry in values.xml file") );
					goto android_dialog_cleanup2;
				}

				contents2 += strlen("<string name=\"gcm_defaultSenderId\" translatable=\"false\"");
				*contents2 = 0;
				contents3 = contents2;
				contents3++;
				contents3 = strstr( contents3, "</string>" );
				if ( !contents3 )
				{
					SHOW_ERR( _("Could not find end of gcm_defaultSenderId entry in values.xml file") );
					goto android_dialog_cleanup2;
				}

				// write resources file
				strcpy( newcontents, newcontents2 );
				strcat( newcontents, ">" );
				strcat( newcontents, contentsOther2 );
				strcat( newcontents, contents3 );

				*contents2 = '>'; // repair file
				*contentsOther3 = '"'; // repair file
				memcpy( newcontents2, newcontents, AGK_NEW_CONTENTS_SIZE );
			}
			
			// find firebase_url value
			{
				contentsOther2 = strstr( contentsOther, "\"firebase_url\": \"" );
				if ( !contentsOther2 )
				{
					SHOW_ERR( _("Could not find firebase_url entry in Firebase config file") );
					goto android_dialog_cleanup2;
				}

				contentsOther2 += strlen("\"firebase_url\": \"");
				contentsOther3 = strstr( contentsOther2, "\"" );
				if ( !contentsOther3 )
				{
					SHOW_ERR( _("Could not find end of firebase_url entry in Firebase config file") );
					goto android_dialog_cleanup2;
				}
				*contentsOther3 = 0;

				// find entry in newcontents2
				contents2 = strstr( newcontents2, "<string name=\"firebase_database_url\" translatable=\"false\"" );
				if ( !contents2 )
				{
					SHOW_ERR( _("Could not find firebase_database_url entry in values.xml file") );
					goto android_dialog_cleanup2;
				}

				contents2 += strlen("<string name=\"firebase_database_url\" translatable=\"false\"");
				*contents2 = 0;
				contents3 = contents2;
				contents3++;
				contents3 = strstr( contents3, "</string>" );
				if ( !contents3 )
				{
					SHOW_ERR( _("Could not find end of firebase_database_url entry in values.xml file") );
					goto android_dialog_cleanup2;
				}

				// write resources file
				strcpy( newcontents, newcontents2 );
				strcat( newcontents, ">" );
				strcat( newcontents, contentsOther2 );
				strcat( newcontents, contents3 );

				*contents2 = '>'; // repair file
				*contentsOther3 = '"'; // repair file
				memcpy( newcontents2, newcontents, AGK_NEW_CONTENTS_SIZE );
			}

			// find mobilesdk_app_id value
			// if the config file contains multiple Android apps then there will be multiple mobilesdk_app_id's, and only the corect one will work
			// look for the corresponding package_name that matches this export
			{
				package_count = 0;
				contentsOther2 = contentsOther;
				while( *contentsOther2 && (contentsOther2 = strstr( contentsOther2, "\"mobilesdk_app_id\": \"" )) )
				{
					package_count++;
					contentsOther2 += strlen("\"mobilesdk_app_id\": \"");
					contentsOther3 = strstr( contentsOther2, "\"" );
					if ( !contentsOther3 )
					{
						SHOW_ERR( _("Could not find end of mobilesdk_app_id entry in Firebase config file") );
						goto android_dialog_cleanup2;
					}
					*contentsOther3 = 0;

					// look for the package_name for this mobilesdk_app_id
					gchar* contentsOther4 = strstr( contentsOther3+1, "\"package_name\": \"" );
					if ( !contentsOther4 )
					{
						SHOW_ERR( _("Could not find package_name for mobilesdk_app_id entry in Firebase config file") );
						goto android_dialog_cleanup2;
					}
					contentsOther4 += strlen("\"package_name\": \"");
					if ( strncmp( contentsOther4, package_name, strlen(package_name) ) == 0 )
					{
						contentsOther4 += strlen(package_name);
						if ( *contentsOther4 == '\"' ) 
						{
							break;
						}
					}

					*contentsOther3 = '"'; // repair file
				}
				
				if ( !contentsOther2 || !*contentsOther2 )
				{
					SHOW_ERR1( _("Could not find mobilesdk_app_id for android package_name \"%s\" in the Firebase config file"), package_name );
					goto android_dialog_cleanup2;
				}

				// find entry in newcontents2
				contents2 = strstr( newcontents2, "<string name=\"google_app_id\" translatable=\"false\"" );
				if ( !contents2 )
				{
					SHOW_ERR( _("Could not find google_app_id entry in values.xml file") );
					goto android_dialog_cleanup2;
				}

				contents2 += strlen("<string name=\"google_app_id\" translatable=\"false\"");
				*contents2 = 0;
				contents3 = contents2;
				contents3++;
				contents3 = strstr( contents3, "</string>" );
				if ( !contents3 )
				{
					SHOW_ERR( _("Could not find end of google_app_id entry in values.xml file") );
					goto android_dialog_cleanup2;
				}

				// write resources file
				strcpy( newcontents, newcontents2 );
				strcat( newcontents, ">" );
				strcat( newcontents, contentsOther2 );
				strcat( newcontents, contents3 );

				*contents2 = '>'; // repair file
				*contentsOther3 = '"'; // repair file
				memcpy( newcontents2, newcontents, AGK_NEW_CONTENTS_SIZE );
			}

			// find current_key value
			{
				contentsOther2 = strstr( contentsOther, "\"current_key\": \"" );
				if ( !contentsOther2 )
				{
					SHOW_ERR( _("Could not find current_key entry in Firebase config file") );
					goto android_dialog_cleanup2;
				}

				contentsOther2 += strlen("\"current_key\": \"");
				contentsOther3 = strstr( contentsOther2, "\"" );
				if ( !contentsOther3 )
				{
					SHOW_ERR( _("Could not find end of current_key entry in Firebase config file") );
					goto android_dialog_cleanup2;
				}
				*contentsOther3 = 0;

				// find entry in newcontents2
				contents2 = strstr( newcontents2, "<string name=\"google_api_key\" translatable=\"false\"" );
				if ( !contents2 )
				{
					SHOW_ERR( _("Could not find google_api_key entry in values.xml file") );
					goto android_dialog_cleanup2;
				}

				contents2 += strlen("<string name=\"google_api_key\" translatable=\"false\"");
				*contents2 = 0;
				contents3 = contents2;
				contents3++;
				contents3 = strstr( contents3, "</string>" );
				if ( !contents3 )
				{
					SHOW_ERR( _("Could not find end of google_api_key entry in values.xml file") );
					goto android_dialog_cleanup2;
				}

				// write resources file
				strcpy( newcontents, newcontents2 );
				strcat( newcontents, ">" );
				strcat( newcontents, contentsOther2 );
				strcat( newcontents, contents3 );

				*contents2 = '>'; // repair file
				memcpy( newcontents2, newcontents, AGK_NEW_CONTENTS_SIZE );

				// also copy it to google_crash_reporting_api_key
				contents2 = strstr( newcontents2, "<string name=\"google_crash_reporting_api_key\" translatable=\"false\"" );
				if ( !contents2 )
				{
					SHOW_ERR( _("Could not find google_crash_reporting_api_key entry in values.xml file") );
					goto android_dialog_cleanup2;
				}

				contents2 += strlen("<string name=\"google_crash_reporting_api_key\" translatable=\"false\"");
				*contents2 = 0;
				contents3 = contents2;
				contents3++;
				contents3 = strstr( contents3, "</string>" );
				if ( !contents3 )
				{
					SHOW_ERR( _("Could not find end of google_crash_reporting_api_key entry in values.xml file") );
					goto android_dialog_cleanup2;
				}

				// write resources file
				strcpy( newcontents, newcontents2 );
				strcat( newcontents, ">" );
				strcat( newcontents, contentsOther2 );
				strcat( newcontents, contents3 );

				*contents2 = '>'; // repair file
				*contentsOther3 = '"'; // repair file
				memcpy( newcontents2, newcontents, AGK_NEW_CONTENTS_SIZE );
			}

			if ( contentsOther ) g_free(contentsOther);
			contentsOther = 0;
		}

		if ( !g_file_set_contents( resources_file, newcontents, strlen(newcontents), &error ) )
		{
			SHOW_ERR1( _("Failed to write resource values.xml file: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto android_dialog_cleanup2;
		}

		if ( contents ) g_free(contents);
		contents = 0;

		// start packaging app
		gchar *aaptcommand = g_new0( gchar*, 1000000 );
		if ( !g_file_test( path_to_aapt2, G_FILE_TEST_EXISTS ) )
		{
			SHOW_ERR( _("Failed to export project, AAPT2 program not found") );
			goto android_dialog_cleanup2;
		}

		argv = g_new0(gchar *, 3);
		argv[0] = g_strdup(path_to_aapt2);
		argv[1] = g_strdup("m"); // open for stdin commands
		argv[2] = NULL;

		GPid aapt2_pid = 0;
		GPollFD aapt2_in = { -1, G_IO_OUT | G_IO_ERR, 0 };
		if (! g_spawn_async_with_pipes(tmp_folder, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_STDOUT_TO_DEV_NULL, NULL, NULL, &aapt2_pid, 
									   &aapt2_in.fd, NULL, NULL, &error))
		{
			SHOW_ERR1("g_spawn_async() failed: %s", error->message);
			g_error_free(error);
			error = NULL;
			goto android_dialog_cleanup2;
		}

		if ( aapt2_pid == 0 )
		{
			SHOW_ERR( _("Failed to start packaging tool") );
			goto android_dialog_cleanup2;
		}

		g_strfreev(argv);
		argv = 0;

		// compile values.xml file
	#ifdef G_OS_WIN32
		strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\values\\values.xml\n\n" );
	#else
		strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/values/values.xml\n\n" );
	#endif
		write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

		if ( error )
		{
			g_error_free(error);
			error = NULL;
		}
		
		// load icon file
		if ( app_icon && *app_icon )
		{
			if ( icon_image ) gdk_pixbuf_unref(icon_image);
			icon_image = gdk_pixbuf_new_from_file( app_icon, &error );
			if ( !icon_image || error )
			{
				SHOW_ERR1( _("Failed to load image icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}

			// scale it and save it
			if ( isGoogle || isAmazon )
			{
				// 192x192
				image_filename = g_build_path( "/", tmp_folder, "resOrig", "drawable-xxxhdpi", "icon.png", NULL );
				icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 192, 192, GDK_INTERP_HYPER );
				if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
				{
					SHOW_ERR1( _("Failed to save xxxhdpi icon: %s"), error->message );
					g_error_free(error);
					error = NULL;
					goto android_dialog_cleanup2;
				}
				gdk_pixbuf_unref( icon_scaled_image );
				g_free( image_filename );

			#ifdef G_OS_WIN32
				strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\drawable-xxxhdpi\\icon.png\n\n" );
			#else
				strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/drawable-xxxhdpi/icon.png\n\n" );
			#endif
				write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

				// 144x144
				image_filename = g_build_path( "/", tmp_folder, "resOrig", "drawable-xxhdpi", "icon.png", NULL );
				icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 144, 144, GDK_INTERP_HYPER );
				if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
				{
					SHOW_ERR1( _("Failed to save xxhdpi icon: %s"), error->message );
					g_error_free(error);
					error = NULL;
					goto android_dialog_cleanup2;
				}
				gdk_pixbuf_unref( icon_scaled_image );
				g_free( image_filename );

			#ifdef G_OS_WIN32
				strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\drawable-xxhdpi\\icon.png\n\n" );
			#else
				strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/drawable-xxhdpi/icon.png\n\n" );
			#endif
				write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );
			}

			const gchar* szDrawable_xhdpi = (isOuya) ? "drawable-xhdpi-v4" : "drawable-xhdpi";
			const gchar* szDrawable_hdpi = (isOuya) ? "drawable-hdpi-v4" : "drawable-hdpi";
			const gchar* szDrawable_mdpi = (isOuya) ? "drawable-mdpi-v4" : "drawable-mdpi";
			const gchar* szDrawable_ldpi = (isOuya) ? "drawable-ldpi-v4" : "drawable-ldpi";

			const gchar* szMainIcon = (isOuya) ? "app_icon.png" : "icon.png";
			
			// 96x96
			image_filename = g_build_path( "/", tmp_folder, "resOrig", szDrawable_xhdpi, szMainIcon, NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 96, 96, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save xhdpi icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			g_free( image_filename );

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\" ); strcat( aaptcommand, szDrawable_xhdpi ); strcat( aaptcommand, "\\" );
			strcat( aaptcommand, szMainIcon ); strcat( aaptcommand, "\n\n" );
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/" ); strcat( aaptcommand, szDrawable_xhdpi ); strcat( aaptcommand, "/" );
			strcat( aaptcommand, szMainIcon ); strcat( aaptcommand, "\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			// 72x72
			image_filename = g_build_path( "/", tmp_folder, "resOrig", szDrawable_hdpi, szMainIcon, NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 72, 72, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save hdpi icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			g_free( image_filename );

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\" ); strcat( aaptcommand, szDrawable_hdpi ); strcat( aaptcommand, "\\" );
			strcat( aaptcommand, szMainIcon ); strcat( aaptcommand, "\n\n" );
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/" ); strcat( aaptcommand, szDrawable_hdpi ); strcat( aaptcommand, "/" );
			strcat( aaptcommand, szMainIcon ); strcat( aaptcommand, "\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			// 48x48
			image_filename = g_build_path( "/", tmp_folder, "resOrig", szDrawable_mdpi, szMainIcon, NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 48, 48, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save mdpi icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			g_free( image_filename );

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\" ); strcat( aaptcommand, szDrawable_mdpi ); strcat( aaptcommand, "\\" );
			strcat( aaptcommand, szMainIcon ); strcat( aaptcommand, "\n\n" );
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/" ); strcat( aaptcommand, szDrawable_mdpi ); strcat( aaptcommand, "/" );
			strcat( aaptcommand, szMainIcon ); strcat( aaptcommand, "\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			// 36x36
			image_filename = g_build_path( "/", tmp_folder, "resOrig", szDrawable_ldpi, szMainIcon, NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 36, 36, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save ldpi icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
					
			gdk_pixbuf_unref( icon_scaled_image );
			icon_scaled_image = NULL;

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\" ); strcat( aaptcommand, szDrawable_ldpi ); strcat( aaptcommand, "\\" );
			strcat( aaptcommand, szMainIcon ); strcat( aaptcommand, "\n\n" );
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/" ); strcat( aaptcommand, szDrawable_ldpi ); strcat( aaptcommand, "/" );
			strcat( aaptcommand, szMainIcon ); strcat( aaptcommand, "\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			g_free( image_filename );
			image_filename = NULL;
		}

		// load notification icon file
		if ( notif_icon && *notif_icon && (isGoogle || isAmazon) )
		{
			if ( icon_image ) gdk_pixbuf_unref(icon_image);
			icon_image = gdk_pixbuf_new_from_file( notif_icon, &error );
			if ( !icon_image || error )
			{
				SHOW_ERR1( _("Failed to load notification icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}

			// scale it and save it
			// 96x96
			image_filename = g_build_path( "/", tmp_folder, "resOrig", "drawable-xxxhdpi", "icon_white.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 96, 96, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save xxxhdpi icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			g_free( image_filename );

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\drawable-xxxhdpi\\icon_white.png\n\n" );
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/drawable-xxxhdpi/icon_white.png\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			// 72x72
			image_filename = g_build_path( "/", tmp_folder, "resOrig", "drawable-xxhdpi", "icon_white.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 72, 72, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save xxhdpi icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			g_free( image_filename );

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\drawable-xxhdpi\\icon_white.png\n\n" );
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/drawable-xxhdpi/icon_white.png\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			const gchar* szDrawable_xhdpi = (isOuya) ? "drawable-xhdpi-v4" : "drawable-xhdpi";
			const gchar* szDrawable_hdpi = (isOuya) ? "drawable-hdpi-v4" : "drawable-hdpi";
			const gchar* szDrawable_mdpi = (isOuya) ? "drawable-mdpi-v4" : "drawable-mdpi";
			const gchar* szDrawable_ldpi = (isOuya) ? "drawable-ldpi-v4" : "drawable-ldpi";

			// 48x48
			image_filename = g_build_path( "/", tmp_folder, "resOrig", szDrawable_xhdpi, "icon_white.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 48, 48, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save xhdpi icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			g_free( image_filename );

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\" ); strcat( aaptcommand, szDrawable_xhdpi ); 
			strcat( aaptcommand, "\\icon_white.png\n\n" );
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/" ); strcat( aaptcommand, szDrawable_xhdpi );
			strcat( aaptcommand, "/icon_white.png\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			// 36x36
			image_filename = g_build_path( "/", tmp_folder, "resOrig", szDrawable_hdpi, "icon_white.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 36, 36, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save hdpi icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			g_free( image_filename );

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\" ); strcat( aaptcommand, szDrawable_hdpi ); 
			strcat( aaptcommand, "\\icon_white.png\n\n" );
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/" ); strcat( aaptcommand, szDrawable_hdpi );
			strcat( aaptcommand, "/icon_white.png\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			// 24x24
			image_filename = g_build_path( "/", tmp_folder, "resOrig", szDrawable_mdpi, "icon_white.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 24, 24, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save mdpi icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			g_free( image_filename );

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\" ); strcat( aaptcommand, szDrawable_mdpi ); 
			strcat( aaptcommand, "\\icon_white.png\n\n" );
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/" ); strcat( aaptcommand, szDrawable_mdpi );
			strcat( aaptcommand, "/icon_white.png\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			// 24x24
			image_filename = g_build_path( "/", tmp_folder, "resOrig", szDrawable_ldpi, "icon_white.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 24, 24, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save ldpi icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
					
			gdk_pixbuf_unref( icon_scaled_image );
			icon_scaled_image = NULL;

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\" ); strcat( aaptcommand, szDrawable_ldpi ); 
			strcat( aaptcommand, "\\icon_white.png\n\n" );
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/" ); strcat( aaptcommand, szDrawable_ldpi );
			strcat( aaptcommand, "/icon_white.png\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			g_free( image_filename );
			image_filename = NULL;
		}

		// load ouya icon and check size
		if ( isOuya && ouya_icon && *ouya_icon )
		{
			if ( icon_image ) gdk_pixbuf_unref(icon_image);
			icon_image = gdk_pixbuf_new_from_file( ouya_icon, &error );
			if ( !icon_image || error )
			{
				SHOW_ERR1( _("Failed to load Ouya large icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}

			if ( gdk_pixbuf_get_width( icon_image ) != 732 || gdk_pixbuf_get_height( icon_image ) != 412 )
			{
				SHOW_ERR( _("Ouya large icon must be 732x412 pixels") );
				goto android_dialog_cleanup2;
			}

			// copy it to the res folder
			image_filename = g_build_path( "/", tmp_folder, "resOrig", "drawable-xhdpi-v4", "ouya_icon.png", NULL );
			utils_copy_file( ouya_icon, image_filename, TRUE, NULL );
			g_free( image_filename );

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\drawable-xhdpi-v4\\ouya_icon.png\n\n" ); 
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/drawable-xhdpi-v4/ouya_icon.png\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			// 320x180
			image_filename = g_build_path( "/", tmp_folder, "resOrig", "drawable", "icon.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 320, 180, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save lean back icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
					
			gdk_pixbuf_unref( icon_scaled_image );
			icon_scaled_image = NULL;

		#ifdef G_OS_WIN32
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig\\drawable\\icon.png\n\n" ); 
		#else
			strcpy( aaptcommand, "compile\n-o\nresMerged\nresOrig/drawable/icon.png\n\n" );
		#endif
			write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

			g_free( image_filename );
			image_filename = NULL;
		}

		while (gtk_events_pending())
			gtk_main_iteration();
		
		strcpy( aaptcommand, "l\n-I\n" );
		strcat( aaptcommand, path_to_android_jar );
		strcat( aaptcommand, "\n--manifest\n" );
		strcat( aaptcommand, tmp_folder );
		strcat( aaptcommand, "/AndroidManifest.xml\n-o\n" );
		strcat( aaptcommand, output_file );
		strcat( aaptcommand, "\n--auto-add-overlay\n--no-version-vectors\n" );

		if ( isBundle )
		{
			strcat( aaptcommand, "--proto-format\n" );
		}

		gchar* resMergedPath = g_build_filename( tmp_folder, "resMerged", NULL );
		GDir *dir = g_dir_open(resMergedPath, 0, NULL);
		
		const gchar *filename;
		foreach_dir(filename, dir)
		{
			gchar* fullsrcpath = g_build_filename( tmp_folder, "resMerged", filename, NULL );
			
			#ifdef MIKE_DEBUG
				//printf ( fullsrcpath );
				//printf ( "\n" );
			#endif

			if ( g_file_test( fullsrcpath, G_FILE_TEST_IS_REGULAR ) )
			{
				strcat( aaptcommand, "-R\n" );
				strcat( aaptcommand, fullsrcpath );
				strcat( aaptcommand, "\n" );
			}

			g_free(fullsrcpath);
		}

		g_dir_close(dir);
		g_free( resMergedPath );

		/*
		gchar* fullsrcpath = g_build_filename( tmp_folder, "resMerged\\values_values.arsc.flat", NULL );
		strcat( aaptcommand, "-R\n" );
		strcat( aaptcommand, fullsrcpath );
		strcat( aaptcommand, "\n" );
		g_free(fullsrcpath);
		*/

		strcat( aaptcommand, "\nquit\n\n" );

	#ifdef G_OS_WIN32
		gchar *ptr = aaptcommand;
		while( *ptr )
		{
			if ( *ptr == '/' ) *ptr = '\\';
			ptr++;
		}
	#endif

		//
		//gchar* logpath = g_build_filename( tmp_folder, "log.txt", NULL );
		//FILE *pFile = fopen( logpath, "wb" );
		//fputs( aaptcommand, pFile );
		//fclose( pFile );
		//
		
		// mike	
		//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, "mike - check log.txt" );

		write(aapt2_in.fd, aaptcommand, strlen(aaptcommand) );

	#ifdef G_OS_WIN32
		WaitForProcess( aapt2_pid );
	#else
		waitpid( aapt2_pid, &status, 0 );
	#endif
		aapt2_pid = 0;


		// mike - zip appears to be wrong at this point
		//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, "CHECK TEST.ZIP" );

		// if we have previously called g_spawn_async then g_spawn_sync will never return the correct exit status due to ECHILD being returned from waitpid()
		
		// check the file was created instead
		if ( !g_file_test( output_file, G_FILE_TEST_EXISTS ) )
		{
			SHOW_ERR( _("Failed to write output files, check that your project directory is not in a write protected location") );
			goto android_dialog_cleanup2;
		}
		
		while (gtk_events_pending())
			gtk_main_iteration();

		g_rename( output_file, output_file_zip );

		if ( isBundle )
		{
		
			// mike - where is the bundle folder?
			//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, "BUILD_TMP IS FINE AT THIS POINT" );
		
			// need to extract and recreate zip file to move AndroidManifest.xml file
			mz_zip_archive zip_archive;
			memset(&zip_archive, 0, sizeof(zip_archive));
			if ( !mz_zip_reader_init_file( &zip_archive, output_file_zip, 0 ) )
			{
				mz_zip_reader_end( &zip_archive );
				SHOW_ERR( "Failed to open output zip file" );
				goto android_dialog_cleanup2;
			}

			gchar* bundle_folder = g_build_filename( app->project->base_path, "build_bundle", NULL );
			utils_str_replace_char( bundle_folder, '\\', '/' );
			utils_remove_folder_recursive( bundle_folder );

			int numFiles = mz_zip_reader_get_num_files( &zip_archive );
			char filename[ 1024 ];
			int i;
			
			//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, "build_bundle" );
			
			// mike
			//SHOW_ERR1 ( _ ( "numFiles: %i" ), numFiles );
			
			for( i = 0; i < numFiles; i++ )
			{
				mz_zip_reader_get_filename( &zip_archive, i, filename, 1024 );
				gchar* final_path = g_build_path( "/", bundle_folder, filename, NULL );
				
				if ( mz_zip_reader_is_file_a_directory( &zip_archive, i ) ) 
				{
					if ( !g_file_test( final_path, G_FILE_TEST_EXISTS ) ) 
					{
						if ( g_mkdir_with_parents( final_path, 0755 ) < 0 )
						{
							SHOW_ERR( "Failed to create folder in the projects directory" );
							mz_zip_reader_end( &zip_archive );
							g_free(bundle_folder);
							g_free(final_path);
							goto android_dialog_cleanup2;
						}
					}
					g_free(final_path);
					continue;
				}
            
				char* withoutFile = (char*)malloc( strlen(final_path)+1 );
				strcpy( withoutFile, final_path );
				char *szSlash = strrchr( withoutFile, '/' );
				if ( szSlash ) *szSlash = 0;
            
				// create directory
				if ( !g_file_test( withoutFile, G_FILE_TEST_EXISTS ) ) 
				{
					if ( g_mkdir_with_parents( withoutFile, 0755 ) < 0 )
					{
						SHOW_ERR( "Failed to create folder in the projects directory" );
						mz_zip_reader_end( &zip_archive );
						g_free(bundle_folder);
						g_free(final_path);
						g_free(withoutFile);
						goto android_dialog_cleanup2;
					}
				}
            
				free(withoutFile);
				
				// mike
				//SHOW_ERR1 ( _ ( "extract: %i" ), i );
				//SHOW_ERR1 ( _ ( "extract: %s" ), final_path );
            
				mz_zip_reader_extract_to_file( &zip_archive, i, final_path, 0 );
            
				g_free(final_path);
			}
        
			mz_zip_reader_end( &zip_archive );
						
			// move manifest
			gchar* old_manifest_path = g_build_path( "/", bundle_folder, "AndroidManifest.xml", NULL );
			gchar* new_manifest_path = g_build_path( "/", bundle_folder, "manifest", "AndroidManifest.xml", NULL );
			
			gchar *contents = 0;
			gsize length = 0;

			if ( !g_file_get_contents( old_manifest_path, &contents, &length, NULL ) )
			{
				SHOW_ERR1( "Failed to read manifest file at %s", old_manifest_path );
				g_free(bundle_folder);
				g_free(new_manifest_path);
				g_free(old_manifest_path);
				goto android_dialog_cleanup2;
			}

			gchar* new_manifest_folder = g_build_path( "/", bundle_folder, "manifest", NULL );
			g_mkdir_with_parents( new_manifest_folder, 0755 );
			g_free( new_manifest_folder );

			if ( !g_file_set_contents( new_manifest_path, contents, length, NULL ) )
			{
				SHOW_ERR1( "Failed to write manifest file at %s", new_manifest_path );
				g_free(contents);
				g_free(bundle_folder);
				g_free(new_manifest_path);
				g_free(old_manifest_path);
				goto android_dialog_cleanup2;
			}
    
			g_free(contents);

			g_unlink( old_manifest_path );

			g_free(new_manifest_path);
			g_free(old_manifest_path);

			g_unlink( output_file_zip );

			// zip everything back up
			memset(&zip_archive, 0, sizeof(zip_archive));
			if ( !mz_zip_writer_init_file( &zip_archive, output_file_zip, 0 ) )
			{
				SHOW_ERR( _("Failed to initialise zip file for writing") );
				g_free(bundle_folder);
				goto android_dialog_cleanup2;
			}
			
			
			
			// mike - where is the bundle folder?
			//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, "about to make zip" );
			//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, output_file_zip );
			//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, bundle_folder );
		
			if ( !utils_add_folder_to_zip( &zip_archive, bundle_folder, "", TRUE, TRUE ) )
			{
				SHOW_ERR( _("Failed to add files to zip file") );
				g_free(bundle_folder);
				goto android_dialog_cleanup2;
			}

			if ( !mz_zip_writer_finalize_archive( &zip_archive ) )
			{
				SHOW_ERR( _("Failed to finalize zip file") );
				g_free(bundle_folder);
				goto android_dialog_cleanup2;
			}
			if ( !mz_zip_writer_end( &zip_archive ) )
			{
				SHOW_ERR( _("Failed to end zip file") );
				g_free(bundle_folder);
				goto android_dialog_cleanup2;
			}

			// remove temp folder
			utils_remove_folder_recursive( bundle_folder );
			g_free(bundle_folder);
		}

		// open APK as a zip file
		if ( !mz_zip_reader_init_file( &zip_archive, output_file_zip, 0 ) )
		{
			SHOW_ERR( _("Failed to initialise zip file for reading") );
			goto android_dialog_cleanup2;
		}
		if ( !mz_zip_writer_init_from_reader( &zip_archive, output_file_zip ) )
		{
			SHOW_ERR( _("Failed to open zip file for writing") );
			goto android_dialog_cleanup2;
		}

		// copy in extra files
		zip_add_file = g_build_path( "/", src_folder, "classes.dex", NULL );
		mz_zip_writer_add_file( &zip_archive, (isBundle) ? "dex/classes.dex" : "classes.dex", zip_add_file, NULL, 0, 9 );
		
		g_free( zip_add_file );
		zip_add_file = g_build_path( "/", android_folder, "lib", "arm64-v8a", "libandroid_player.so", NULL );
		mz_zip_writer_add_file( &zip_archive, "lib/arm64-v8a/libandroid_player.so", zip_add_file, NULL, 0, 9 );
		
		g_free( zip_add_file );
		zip_add_file = g_build_path( "/", android_folder, "lib", "armeabi-v7a", "libandroid_player.so", NULL );
		mz_zip_writer_add_file( &zip_archive, "lib/armeabi-v7a/libandroid_player.so", zip_add_file, NULL, 0, 9 );
		
		if ( arcore_mode > 0 )
		{
			// use real ARCore lib
			g_free( zip_add_file );
			zip_add_file = g_build_path( "/", android_folder, "lib", "arm64-v8a", "libarcore_sdk.so", NULL );
			mz_zip_writer_add_file( &zip_archive, "lib/arm64-v8a/libarcore_sdk.so", zip_add_file, NULL, 0, 9 );

			g_free( zip_add_file );
			zip_add_file = g_build_path( "/", android_folder, "lib", "armeabi-v7a", "libarcore_sdk.so", NULL );
			mz_zip_writer_add_file( &zip_archive, "lib/armeabi-v7a/libarcore_sdk.so", zip_add_file, NULL, 0, 9 );
		}

		if ( snapchat_client_id && *snapchat_client_id )
		{
			g_free( zip_add_file );
			zip_add_file = g_build_path( "/", android_folder, "lib", "arm64-v8a", "libpruneau.so", NULL );
			mz_zip_writer_add_file( &zip_archive, "lib/arm64-v8a/libpruneau.so", zip_add_file, NULL, 0, 9 );

			g_free( zip_add_file );
			zip_add_file = g_build_path( "/", android_folder, "lib", "armeabi-v7a", "libpruneau.so", NULL );
			mz_zip_writer_add_file( &zip_archive, "lib/armeabi-v7a/libpruneau.so", zip_add_file, NULL, 0, 9 );
		}
		
		while (gtk_events_pending())
			gtk_main_iteration();

		if ( !isOuya )
		{
			// copy assets for Google and Amazon
			g_free( zip_add_file );
			zip_add_file = g_build_path( "/", android_folder, "assets", NULL );
			if ( !utils_add_folder_to_zip( &zip_archive, zip_add_file, "assets", TRUE, TRUE ) )
			{
				SHOW_ERR( _("Failed to add media files to APK") );
				goto android_dialog_cleanup2;
			}
		}

		// copy in media files
		g_free( zip_add_file );
		zip_add_file = g_build_path( "/", app->project->base_path, "media", NULL );
		if ( !utils_add_folder_to_zip( &zip_archive, zip_add_file, "assets/media", TRUE, TRUE ) )
		{
			SHOW_ERR( _("Failed to add media files to APK") );
			goto android_dialog_cleanup2;
		}

		if ( !mz_zip_writer_finalize_archive( &zip_archive ) )
		{
			SHOW_ERR( _("Failed to add finalize zip file") );
			goto android_dialog_cleanup2;
		}
		if ( !mz_zip_writer_end( &zip_archive ) )
		{
			SHOW_ERR( _("Failed to end zip file") );
			goto android_dialog_cleanup2;
		}

		while (gtk_events_pending())
			gtk_main_iteration();

		if ( isBundle )
		{
			// run bundletool
			argv = g_new0( gchar*, 11 );
			argv[0] = g_strdup( "java" );
			argv[1] = g_strdup( "-jar" );
			argv[2] = g_strdup( path_to_bundletool );
			argv[3] = g_strdup( "build-bundle" );
			argv[4] = g_strdup( "--modules" );
			argv[5] = g_strdup( output_file_zip );

			gchar* mapping_file_path = g_build_path( "/", src_folder, "mapping", "mapping.txt", NULL );
			if ( g_file_test( mapping_file_path, G_FILE_TEST_EXISTS ) )
			{
				argv[6] = g_strdup( "--metadata-file" );
				argv[7] = g_strconcat( "com.android.tools.build.obfuscation/proguard.map:", mapping_file_path, NULL );

				/*
				#ifdef G_OS_WIN32
					// mike - 031221 - use bundle config file - DOES NOT WORK FOR SOME REASON
					gchar* bundleConfig = g_build_path ( "/", src_folder, "", "BundleConfig.json", NULL );

					argv [ 8 ] = g_strdup ( "--config" );
					argv [ 9 ] = g_strdup ( bundleConfig );
					argv [ 10 ] = g_strdup ( "--output" );
					argv [ 11 ] = g_strdup ( output_file );
					argv [ 12 ] = NULL;
				#else
					argv[8] = g_strdup( "--output" );
					argv[9] = g_strdup( output_file );
					argv[10] = NULL;
				#endif
				*/

				argv [ 8 ] = g_strdup ( "--output" );
				argv [ 9 ] = g_strdup ( output_file );
				argv [ 10 ] = NULL;
			}
			else
			{
				/*
				#ifdef G_OS_WIN32
				// mike - 031221 - use bundle config file - DOES NOT WORK FOR SOME REASON
					gchar* bundleConfig = g_build_path ( "/", src_folder, "", "BundleConfig.json", NULL );

					argv [ 8 ] = g_strdup ( "--config" );
					argv [ 9 ] = g_strdup ( bundleConfig );
					argv [ 10 ] = g_strdup ( "--output" );
					argv [ 11 ] = g_strdup ( output_file );
					argv [ 12 ] = NULL;
				#else
					argv [ 6 ] = g_strdup ( "--output" );
					argv [ 7 ] = g_strdup ( output_file );
					argv [ 8 ] = NULL;
				#endif
				*/

				
				argv [ 6 ] = g_strdup ( "--output" );
				argv [ 7 ] = g_strdup ( output_file );
				argv [ 8 ] = NULL;
			}

			g_free(mapping_file_path);
			
			if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
			{
				SHOW_ERR1( _("Failed to run bundletool: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
		
			if ( status != 0 && str_out && *str_out )
			{
				SHOW_ERR1( _("Failed to run bundletool, (output: %s)"), str_out );
				goto android_dialog_cleanup2;
			}

			if ( str_out ) g_free(str_out);
			str_out = 0;

			g_strfreev(argv);
			argv = 0;

			while (gtk_events_pending())
				gtk_main_iteration();
		}

		// sign apk
		int argIndex = 0;
		argv = g_new0( gchar*, 14 );
		argv[argIndex++] = g_strdup( "jarsigner" );
		if ( !isBundle )
		{
			argv[argIndex++] = g_strdup("-sigalg");
			argv[argIndex++] = g_strdup("MD5withRSA");
			argv[argIndex++] = g_strdup("-digestalg");
			argv[argIndex++] = g_strdup("SHA1");
		}
		argv[argIndex++] = g_strdup("-storepass");
#ifdef G_OS_WIN32
		argv[argIndex++] = g_strconcat( "\"", keystore_password, "\"", NULL );
#else
		argv[argIndex++] = g_strdup( keystore_password );
#endif
		argv[argIndex++] = g_strdup("-keystore");
		argv[argIndex++] = g_strdup(keystore_file);
		if ( isBundle ) argv[argIndex++] = g_strdup(output_file);
		else argv[argIndex++] = g_strdup(output_file_zip);
		argv[argIndex++] = g_strdup(alias_name);
		argv[argIndex++] = g_strdup("-keypass");
#ifdef G_OS_WIN32
		argv[argIndex++] = g_strconcat( "\"", alias_password, "\"", NULL );
#else
		argv[argIndex++] = g_strdup( alias_password );
#endif
		argv[argIndex++] = NULL;
		
		// MIKE - need to look at all params for this AND TEST IT IN THE TERMINAL
		
		// mike			
		//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, tmp_folder );
		
		// mike
		//for ( unsigned int i = 0; i < argIndex; i++ )
			//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, argv [ i ] );

		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run signing tool: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto android_dialog_cleanup2;
		}
		
		if ( status != 0 && str_out && *str_out && strstr(str_out,"jar signed") == 0 )
		{
			SHOW_ERR1( _("Failed to sign APK, is your keystore password and alias correct? (error: %s)"), str_out );
			goto android_dialog_cleanup2;
		}

		if ( str_out ) g_free(str_out);
		str_out = 0;

		g_strfreev(argv);
		argv = 0;

		while (gtk_events_pending())
			gtk_main_iteration();

		// align apk
		argv = g_new0( gchar*, 5 );
		argv[0] = g_strdup( path_to_zipalign );
		argv[1] = g_strdup("4");
		argv[2] = g_strdup(output_file_zip);
		argv[3] = g_strdup(output_file);
		argv[4] = NULL;


		// mike			
		//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, "zipalign" );
		
		//for ( unsigned int i = 0; i < 4; i++ )
			//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, argv [ i ] );

		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run zipalign tool: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto android_dialog_cleanup2;
		}
		
		if ( status != 0 && str_out && *str_out )
		{
			SHOW_ERR1( _("Zip align tool returned error: %s"), str_out );
			goto android_dialog_cleanup2;
		}
		
		if ( str_out ) g_free(str_out);
		str_out = 0;

		g_strfreev(argv);
		argv = 0;

		while (gtk_events_pending())
			gtk_main_iteration();

		// mike			
		//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, "alignment complete" );

		if ( !isBundle )
		{
			// sign with V2 signature
			argIndex = 0;
			argv = g_new0( gchar*, 15 );
			argv[argIndex++] = g_strdup( "java" );
			argv[argIndex++] = g_strdup( "-jar" );
			argv[argIndex++] = g_strdup( path_to_apksigner );
			argv[argIndex++] = g_strdup( "sign" );
			argv[argIndex++] = g_strdup( "--ks" );
			argv[argIndex++] = g_strdup( keystore_file );
			argv[argIndex++] = g_strdup( "--ks-pass" );
	#ifdef G_OS_WIN32
			argv[argIndex++] = g_strconcat( "pass:\"", keystore_password, "\"", NULL );
	#else
			argv[argIndex++] = g_strconcat( "pass:", keystore_password, NULL );
	#endif
			argv[argIndex++] = g_strdup( "--ks-key-alias" );
			argv[argIndex++] = g_strdup( alias_name );
			argv[argIndex++] = g_strdup( "--key-pass" );
	#ifdef G_OS_WIN32
			argv[argIndex++] = g_strconcat( "pass:\"", alias_password, "\"", NULL );
	#else
			argv[argIndex++] = g_strconcat( "pass:", alias_password, NULL );
	#endif
			argv[argIndex++] = g_strdup( output_file );
			argv[argIndex++] = NULL;

			// mike			
			//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, tmp_folder );
			
			// mike
			//for ( unsigned int i = 0; i < argIndex; i++ )
				//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, argv [ i ] );

			if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
			{
				SHOW_ERR1( _("Failed to run apksigner tool: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto android_dialog_cleanup2;
			}
		
			if ( status != 0 && str_out && *str_out )
			{
				SHOW_ERR1( _("Failed to sign APK with apksigner, is your keystore password and alias correct? (error: %s)"), str_out );
				goto android_dialog_cleanup2;
			}

			if ( str_out ) g_free(str_out);
			str_out = 0;

			g_strfreev(argv);
			argv = 0;

			while (gtk_events_pending())
				gtk_main_iteration();
		}

		if ( dialog ) gtk_widget_hide(GTK_WIDGET(dialog));

android_dialog_cleanup2:
        
        gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.android_dialog, "android_export1"), TRUE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.android_dialog, "button7"), TRUE );

		if ( aapt2_pid ) 
		{
		#ifdef G_OS_WIN32
			TerminateProcess(aapt2_pid, 0);
		#else
			kill(aapt2_pid, SIGTERM);
		#endif
		}

		g_unlink( output_file_zip );
		utils_remove_folder_recursive( tmp_folder );

		if ( path_to_aapt2 ) g_free(path_to_aapt2);
		if ( path_to_android_jar ) g_free(path_to_android_jar);
		if ( path_to_bundletool ) g_free(path_to_bundletool);
		if ( path_to_apksigner ) g_free(path_to_apksigner);
		if ( path_to_zipalign ) g_free(path_to_zipalign);

		if ( zip_add_file ) g_free(zip_add_file);
		if ( manifest_file ) g_free(manifest_file);
		if ( newcontents ) g_free(newcontents);
		if ( newcontents2 ) g_free(newcontents2);
		if ( contents ) g_free(contents);
		if ( contentsOther ) g_free(contentsOther);
		if ( resources_file ) g_free(resources_file);
		if ( error ) g_error_free(error);
		if ( icon_image ) gdk_pixbuf_unref(icon_image);
		if ( image_filename ) g_free(image_filename);
		if ( icon_scaled_image ) gdk_pixbuf_unref(icon_scaled_image);
		if ( argv ) g_strfreev(argv);
		if ( aaptcommand ) g_free(aaptcommand);
		
		if ( output_file_zip ) g_free(output_file_zip);
		if ( tmp_folder ) g_free(tmp_folder);
		if ( android_folder ) g_free(android_folder);
		if ( src_folder ) g_free(src_folder);
		if ( str_out ) g_free(str_out);

		if ( output_file ) g_free(output_file);
		if ( app_name ) g_free(app_name);
		if ( package_name ) g_free(package_name);
		if ( app_icon ) g_free(app_icon);
		if ( ouya_icon ) g_free(ouya_icon);
		if ( firebase_config ) g_free(firebase_config);
		if ( url_scheme ) g_free(url_scheme);
		if ( deep_link ) g_free(deep_link);
		if ( google_play_app_id ) g_free(google_play_app_id);
		if ( admob_app_id ) g_free(admob_app_id);
		if ( snapchat_client_id ) g_free(snapchat_client_id);

		if ( keystore_file ) g_free(keystore_file);
		if ( keystore_password ) g_free(keystore_password);
		if ( version_number ) g_free(version_number);
		if ( alias_name ) g_free(alias_name);
		if ( alias_password ) g_free(alias_password);
	}

	running = 0;
}

static gchar *last_proj_path_android = 0;

#ifdef __APPLE__
@interface HTTPListener : NSObject
{
@public
    int m_length;
    int m_received;
    char m_filename[1024];
    FILE *m_file;
}
@end

@implementation HTTPListener
- (id) init
{
    m_file = 0;
    m_filename[0] = 0;
    
    self = [super init];
    
    return self;
}

- (void) reset
{
    m_file = 0;
    m_filename[0] = 0;
}

- (void) dealloc
{
    [super dealloc];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    if ( m_connection )
    {
        [m_connection release];
        m_connection = nil;
    }
    
    if ( m_file )
    {
        fclose( m_file );
        m_file = 0;

        // extract zip file
        mz_zip_archive zip_archive;
        memset(&zip_archive, 0, sizeof(zip_archive));
        if ( !mz_zip_reader_init_file( &zip_archive, m_filename, 0 ) )
        {
            mz_zip_reader_end( &zip_archive );
            SHOW_ERR( "Failed to open downloaded AndroidExport.zip" );
            return;
        }
        int numFiles = mz_zip_reader_get_num_files( &zip_archive );
        char filename[ 1024 ];
        for( int i = 0; i < numFiles; i++ )
        {
			mz_zip_reader_get_filename( &zip_archive, i, filename, 1024 );

            if ( mz_zip_reader_is_file_a_directory( &zip_archive, i ) ) 
			{
                gchar* final_path = g_build_path( "/", app->configdir, filename, NULL );
				NSString* pWriteDir = [ [ NSString alloc ] initWithUTF8String:final_path ];
				[ g_pFileManager createDirectoryAtPath:pWriteDir withIntermediateDirectories:YES attributes:nil error:nil ];
				[ pWriteDir release ];
                g_free(final_path);
				continue;
			}
            
            gchar* final_path = g_build_path( "/", app->configdir, filename, NULL );
                        
            char* withoutFile = (char*)malloc( strlen(final_path)+1 );
            strcpy( withoutFile, final_path );
            char *szSlash = strrchr( withoutFile, '/' );
            if ( szSlash ) *szSlash = '\0';
            
            // create directory
            NSString* pWriteDir = [ [ NSString alloc ] initWithUTF8String:withoutFile ];
            [ g_pFileManager createDirectoryAtPath:pWriteDir withIntermediateDirectories:YES attributes:nil error:nil ];
            [ pWriteDir release ];
            
            free(withoutFile);
            
            mz_zip_reader_extract_to_file( &zip_archive, i, final_path, 0 );
            
            g_free(final_path);
        }
        
        mz_zip_reader_end( &zip_archive );
        
        gchar* file_path = g_build_path( "/", app->configdir, "AndroidExport", "aapt2", NULL );
        chmod( file_path, 0755 );
        g_free( file_path );
        
        file_path = g_build_path( "/", app->configdir, "AndroidExport", "zipalign", NULL );
        chmod( file_path, 0755 );
        g_free( file_path );
        
        dialogs_show_msgbox( GTK_MESSAGE_INFO, "Android export download completed, you can now export Android APKs" );
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    if ( m_file ) fclose( m_file );
    m_file = fopen( m_filename, "wb" );
        
    //NSHTTPURLResponse *responseHTTP = (NSHTTPURLResponse*) response;
    //m_pHTTP->m_iStatusCode = (int)[responseHTTP statusCode];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    if ( !m_file ) m_file = fopen( m_filename, "wb" );
    if ( m_file && data ) fwrite( [data bytes], 1, (int)[data length], m_file );
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    if ( m_file )
    {
        fclose( m_file );
        m_file = 0;
    }
    
    SHOW_ERR1( "Download Error: %s", [[error localizedDescription] UTF8String] );
    
    if ( m_connection )
    {
        [m_connection release];
        m_connection = nil;
    }
}

- (void)connection:(NSURLConnection *)connection didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite
{
    
}

- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace
{
    return NO;
}

- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    [challenge.sender useCredential:[NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust] forAuthenticationChallenge:challenge];
}
@end

HTTPListener* m_listener = 0;
#endif



#define MIKE_DEBUG

void project_export_apk()
{
	if ( !app->project ) 
	{
		SHOW_ERR( _("You must have a project open to export it") );
		return;
	}
    
#ifdef __APPLE__
	if ( !g_pFileManager ) g_pFileManager = [[NSFileManager alloc] init];

    if ( !g_file_test("/usr/bin/jarsigner", G_FILE_TEST_EXISTS) )
    {
        if ( dialogs_show_question("Due to changes by Apple you must install JDK 14 or above to export for Android. JDK 14 can be found on the Oracle website. Would you like to open their downloads page?", "Download JDK") )
        {
            utils_open_browser( "https://www.oracle.com/java/technologies/javase-jdk14-downloads.html" );
        }
        return;
    }
    
    gchar* android_path = g_build_path( "/", app->configdir, "AndroidExport", "aapt2", NULL );
    if ( !g_file_test(android_path, G_FILE_TEST_EXISTS) )
    {
        if ( !m_connection )
        {
            if ( dialogs_show_question("The Android export files must be downloaded separately. Would you like to download them now?") )
            {
                NSString* sURL = [ [ NSString alloc ] initWithUTF8String:"https://www.appgamekit.com/agkclassicfiles/AndroidExport.zip" ];
                NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:sURL] cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:10.0 ];
                if ( !m_listener ) m_listener = [[HTTPListener alloc] init];
                else [ m_listener reset ];
                strcpy( m_listener->m_filename, app->configdir );
                strcat( m_listener->m_filename, "/AndroidExport.zip" );
                m_connection = [ [NSURLConnection alloc] initWithRequest:request delegate:m_listener ];
                
                dialogs_show_msgbox( GTK_MESSAGE_INFO, "The files will be downloaded in the background" );
            }
            else
            {
                return;
            }
        }
    }
    g_free(android_path);
#endif
    
	// make sure the project is up to date
	build_compile_project(0);

	if (ui_widgets.android_dialog == NULL)
	{
		ui_widgets.android_dialog = create_android_dialog();
		gtk_widget_set_name(ui_widgets.android_dialog, "Export APK");
		gtk_window_set_transient_for(GTK_WINDOW(ui_widgets.android_dialog), GTK_WINDOW(main_widgets.window));

		g_signal_connect(ui_widgets.android_dialog, "response", G_CALLBACK(on_android_dialog_response), NULL);
        g_signal_connect(ui_widgets.android_dialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);

		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_app_icon_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_app_icon_entry")));
		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_notif_icon_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_notif_icon_entry")));
		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_ouya_icon_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_ouya_icon_entry")));
		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_keystore_file_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_keystore_file_entry")));
		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_firebase_config_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_firebase_config_entry")));

		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_output_file_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_output_file_entry")));

		gtk_combo_box_set_active( GTK_COMBO_BOX(ui_lookup_widget(ui_widgets.android_dialog, "android_output_type_combo")), 0 );
		gtk_combo_box_set_active( GTK_COMBO_BOX(ui_lookup_widget(ui_widgets.android_dialog, "android_orientation_combo")), 0 );
		gtk_combo_box_set_active( GTK_COMBO_BOX(ui_lookup_widget(ui_widgets.android_dialog, "android_sdk_combo")), 0 ); 
		gtk_combo_box_set_active( GTK_COMBO_BOX(ui_lookup_widget(ui_widgets.android_dialog, "android_arcore_combo")), 0 );
	}

	// pointers could be the same even if the project is different, so check project path instead
	if ( strcmp( FALLBACK(last_proj_path_android,""), FALLBACK(app->project->file_name,"") ) != 0 )
	{
		if ( last_proj_path_android ) g_free(last_proj_path_android);
		last_proj_path_android = g_strdup( FALLBACK(app->project->file_name,"") );
		
		GtkWidget *widget;

		// set defaults for this project
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_app_name_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.app_name, "") );
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_package_name_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.package_name, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_app_icon_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.app_icon_path, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_notif_icon_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.notif_icon_path, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_ouya_icon_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.ouya_icon_path, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_firebase_config_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.firebase_config_path, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_orientation_combo");
		gtk_combo_box_set_active( GTK_COMBO_BOX(widget), app->project->apk_settings.orientation );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_arcore_combo");
		gtk_combo_box_set_active( GTK_COMBO_BOX(widget), app->project->apk_settings.arcore );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_sdk_combo");
		int version = app->project->apk_settings.sdk_version - 1;
		if ( version < 0 ) version = 0;
		gtk_combo_box_set_active( GTK_COMBO_BOX(widget), version );
								
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_url_scheme");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.url_scheme, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_deep_link");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.deep_link, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_google_play_app_id");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.play_app_id, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_admob_app_id");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.admob_app_id, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_snapchat_client_id");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.snapchat_client_id, "") );

		// permissions
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_external_storage");
		int mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_WRITE) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_location_fine");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_GPS) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_location_coarse");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_LOCATION) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_internet");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_INTERNET) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_wake");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_WAKE) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_billing");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_IAP) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_push_notifications");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_PUSH) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_camera");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_CAMERA) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_expansion");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_EXPANSION) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_vibrate");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_VIBRATE) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_record_audio");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_RECORD_AUDIO) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		// signing
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_keystore_file_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.keystore_path, "") );

		// keep old password and assume it is the same for all projects to save time
		//widget = ui_lookup_widget(ui_widgets.android_dialog, "android_keystore_password_entry");
		//gtk_entry_set_text( GTK_ENTRY(widget), "" );
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_version_number_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.version_name, "") );
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_build_number_entry");
		if ( app->project->apk_settings.version_number == 0 )
		{
			gtk_entry_set_text( GTK_ENTRY(widget), "" );
		}
		else
		{
			char szBuildNum[ 20 ];
			sprintf( szBuildNum, "%d", app->project->apk_settings.version_number );
			gtk_entry_set_text( GTK_ENTRY(widget), szBuildNum );
		}
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_alias_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.alias, "") );

		// keep old password and assume it is the same for all projects to save time
		//widget = ui_lookup_widget(ui_widgets.android_dialog, "android_alias_password_entry");
		//gtk_entry_set_text( GTK_ENTRY(widget), "" );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_output_type_combo");
		gtk_combo_box_set_active( GTK_COMBO_BOX(widget), app->project->apk_settings.app_type );
		
		if ( !app->project->apk_settings.output_path || !*app->project->apk_settings.output_path )
		{
			gchar *filename = g_strconcat( app->project->name, ".apk", NULL );
			gchar* apk_path = g_build_filename( app->project->base_path, filename, NULL );
			gtk_entry_set_text( GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_output_file_entry")), apk_path );
			g_free(apk_path);
			g_free(filename);
		}
		else
		{
			widget = ui_lookup_widget(ui_widgets.android_dialog, "android_output_file_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), app->project->apk_settings.output_path );
		}
	}

	gtk_window_present(GTK_WINDOW(ui_widgets.android_dialog));
}

void on_android_all_dialog_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	if ( response != 1 )
	{
		if ( dialog ) gtk_widget_hide(GTK_WIDGET(dialog));
		return;
	}

	GeanyProject *orig_project = app->project;

	// export all output folder
	GtkWidget *widget = ui_lookup_widget(ui_widgets.android_all_dialog, "export_all_android_output_file_entry");
	gchar *output_file = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

	if ( !*output_file ) 
	{
		g_free(output_file);
		SHOW_ERR(_("You must choose an output folder to save your APKs"));
		return;
	}

	// get export all options
	widget = ui_lookup_widget(ui_widgets.android_all_dialog, "export_all_android_keystore_password_entry");
	gchar *keystore_password = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

	widget = ui_lookup_widget(ui_widgets.android_all_dialog, "export_all_android_version_number_entry");
	gchar *version_number = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
	if ( !*version_number ) SETPTR( version_number, g_strdup("1.0.0") );

	widget = ui_lookup_widget(ui_widgets.android_all_dialog, "export_all_android_build_number_entry");
	gchar* build_number = gtk_entry_get_text(GTK_ENTRY(widget));
	if ( !*build_number ) SETPTR( build_number, g_strdup("1") );
	
	int i;
	for ( i = 0; i < projects_array->len; i++ )
	{
		if ( !projects[i]->is_valid ) continue;
		
		GtkWidget *export_all_progress = ui_lookup_widget(ui_widgets.android_all_dialog, "export_all_android_progress");
		gchar *text = g_strconcat( "Exporting: ", projects[i]->name, " - Google", NULL );
		gtk_label_set_text( GTK_LABEL(export_all_progress), text );
		g_free(text);

		while (gtk_events_pending()) gtk_main_iteration();

		// change current project
		app->project = projects[i];

		// set up the android export dialog
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_app_name_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.app_name, "") );
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_package_name_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.package_name, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_app_icon_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.app_icon_path, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_notif_icon_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.notif_icon_path, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_ouya_icon_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.ouya_icon_path, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_firebase_config_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.firebase_config_path, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_orientation_combo");
		gtk_combo_box_set_active( GTK_COMBO_BOX(widget), app->project->apk_settings.orientation );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_arcore_combo");
		gtk_combo_box_set_active( GTK_COMBO_BOX(widget), app->project->apk_settings.arcore );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_sdk_combo");
		int version = app->project->apk_settings.sdk_version - 1;
		if ( version < 0 ) version = 0;
		gtk_combo_box_set_active( GTK_COMBO_BOX(widget), version );
								
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_url_scheme");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.url_scheme, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_deep_link");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.deep_link, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_google_play_app_id");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.play_app_id, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_admob_app_id");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.admob_app_id, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_snapchat_client_id");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.snapchat_client_id, "") );

		// permissions
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_external_storage");
		int mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_WRITE) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_location_fine");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_GPS) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_location_coarse");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_LOCATION) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_internet");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_INTERNET) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_wake");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_WAKE) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_billing");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_IAP) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_push_notifications");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_PUSH) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_camera");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_CAMERA) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_expansion");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_EXPANSION) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_vibrate");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_VIBRATE) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_permission_record_audio");
		mode = (app->project->apk_settings.permission_flags & AGK_ANDROID_PERMISSION_RECORD_AUDIO) ? 1 : 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), mode );

		// signing
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_keystore_file_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.keystore_path, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_keystore_password_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), keystore_password );
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_version_number_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), version_number );
		
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_build_number_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), build_number );
				
		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_alias_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->apk_settings.alias, "") );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_alias_password_entry");
		gtk_entry_set_text( GTK_ENTRY(widget), keystore_password );

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_output_type_combo");
		gtk_combo_box_set_active( GTK_COMBO_BOX(widget), 0 ); // Google
		
		gchar *filename = g_strconcat( app->project->name, "-Google-", version_number, ".apk", NULL );
		gchar* apk_path = g_build_filename( output_file, filename, NULL );
		gtk_entry_set_text( GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_output_file_entry")), apk_path );
		g_free(apk_path);
		g_free(filename);

		on_android_dialog_response( 0, 1, 1 ); // no dialog, export response, don't save settings

		widget = ui_lookup_widget(ui_widgets.android_dialog, "android_output_type_combo");
		gtk_combo_box_set_active( GTK_COMBO_BOX(widget), 2 ); // Amazon

		filename = g_strconcat( app->project->name, "-Amazon-", version_number, ".apk", NULL );
		apk_path = g_build_filename( output_file, filename, NULL );
		gtk_entry_set_text( GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_output_file_entry")), apk_path );
		g_free(apk_path);
		g_free(filename);

		text = g_strconcat( "Exporting: ", projects[i]->name, " - Amazon", NULL );
		gtk_label_set_text( GTK_LABEL(export_all_progress), text );
		g_free(text);

		while (gtk_events_pending()) gtk_main_iteration();

		on_android_dialog_response( 0, 1, 1 ); // no dialog, export response, don't save settings
	}

	gtk_widget_hide(GTK_WIDGET(dialog));

	// reset current project and set future exports to reload normal settings
	app->project = orig_project;
	if ( last_proj_path_android ) g_free(last_proj_path_android);
	last_proj_path_android = g_strdup( "" );
}

void project_export_apk_all()
{
	if ( projects_array->len <= 0 ) 
	{
		SHOW_ERR( _("You must have at least one project open to export all") );
		return;
	}

	// make sure original android dialog exists
	if (ui_widgets.android_dialog == NULL)
	{
		ui_widgets.android_dialog = create_android_dialog();
		gtk_widget_set_name(ui_widgets.android_dialog, "Export APK");
		gtk_window_set_transient_for(GTK_WINDOW(ui_widgets.android_dialog), GTK_WINDOW(main_widgets.window));

		g_signal_connect(ui_widgets.android_dialog, "response", G_CALLBACK(on_android_dialog_response), NULL);
        g_signal_connect(ui_widgets.android_dialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);

		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_app_icon_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_app_icon_entry")));
		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_notif_icon_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_notif_icon_entry")));
		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_ouya_icon_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_ouya_icon_entry")));
		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_keystore_file_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_keystore_file_entry")));
		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_firebase_config_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_firebase_config_entry")));

		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_dialog, "android_output_file_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_dialog, "android_output_file_entry")));

		gtk_combo_box_set_active( GTK_COMBO_BOX(ui_lookup_widget(ui_widgets.android_dialog, "android_output_type_combo")), 0 );
		gtk_combo_box_set_active( GTK_COMBO_BOX(ui_lookup_widget(ui_widgets.android_dialog, "android_orientation_combo")), 0 );
		gtk_combo_box_set_active( GTK_COMBO_BOX(ui_lookup_widget(ui_widgets.android_dialog, "android_sdk_combo")), 0 ); 
		gtk_combo_box_set_active( GTK_COMBO_BOX(ui_lookup_widget(ui_widgets.android_dialog, "android_arcore_combo")), 0 ); 
	}

	// make sure export all dialog exists
	if (ui_widgets.android_all_dialog == NULL)
	{
		ui_widgets.android_all_dialog = create_android_all_dialog();
		gtk_widget_set_name(ui_widgets.android_all_dialog, "Export APK (All Projects)");
		gtk_window_set_transient_for(GTK_WINDOW(ui_widgets.android_all_dialog), GTK_WINDOW(main_widgets.window));

		g_signal_connect(ui_widgets.android_all_dialog, "response", G_CALLBACK(on_android_all_dialog_response), NULL);
        g_signal_connect(ui_widgets.android_all_dialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);

		ui_setup_open_button_callback_android(ui_lookup_widget(ui_widgets.android_all_dialog, "export_all_android_output_file_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_ENTRY(ui_lookup_widget(ui_widgets.android_all_dialog, "export_all_android_output_file_entry")));
	}

	gtk_window_present(GTK_WINDOW(ui_widgets.android_all_dialog));
}

static void on_keystore_dialog_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	static int running = 0;
	if ( running ) return;

	running = 1;

	if ( response != 1 )
	{
		gtk_widget_hide(GTK_WIDGET(dialog));
	}
	else
	{
		int i;
		GtkWidget *widget;

		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.keystore_dialog, "button9"), FALSE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.keystore_dialog, "button8"), FALSE );
		
		// keystore details
		widget = ui_lookup_widget(ui_widgets.keystore_dialog, "keystore_full_name_entry");
		gchar *full_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		
		widget = ui_lookup_widget(ui_widgets.keystore_dialog, "keystore_company_name_entry");
		gchar *company_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.keystore_dialog, "keystore_city_entry");
		gchar *city = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.keystore_dialog, "keystore_country_entry");
		gchar *country = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.keystore_dialog, "keystore_password1_entry");
		gchar *password1 = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.keystore_dialog, "keystore_password2_entry");
		gchar *password2 = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		// output
		widget = ui_lookup_widget(ui_widgets.keystore_dialog, "keystore_output_file_entry");
		gchar *output_file = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		// START CHECKS

		if ( !output_file || !*output_file ) { SHOW_ERR(_("You must choose an output location to save your keystore file")); goto keystore_dialog_clean_up; }

		if ( g_file_test( output_file, G_FILE_TEST_EXISTS ) )
		{
			if ( !dialogs_show_question(_("\"%s\" already exists. Do you want to overwrite it?"), output_file) )
			{
				goto keystore_dialog_clean_up;
			}
		}

		// check full name
		if ( strlen(full_name) > 30 ) { SHOW_ERR(_("Full name must be less than 30 characters")); goto keystore_dialog_clean_up; }
		for( i = 0; i < strlen(full_name); i++ )
		{
			if ( (full_name[i] < 97 || full_name[i] > 122)
			  && (full_name[i] < 65 || full_name[i] > 90) 
			  && full_name[i] != 32 ) 
			{ 
				SHOW_ERR(_("Full name contains invalid characters, must be A-Z and spaces only")); 
				goto keystore_dialog_clean_up; 
			}
		}
		if ( !*full_name )
		{
			g_free(full_name);
			full_name = g_strdup("Unknown");
		}

		// check company name
		if ( strlen(company_name) > 30 ) { SHOW_ERR(_("Company name must be less than 30 characters")); goto keystore_dialog_clean_up; }
		for( i = 0; i < strlen(company_name); i++ )
		{
			if ( (company_name[i] < 97 || company_name[i] > 122)
			  && (company_name[i] < 65 || company_name[i] > 90) 
			  && company_name[i] != 32 ) 
			{ 
				SHOW_ERR(_("Company name contains invalid characters, must be A-Z and spaces only")); 
				goto keystore_dialog_clean_up; 
			}
		}
		if ( !*company_name )
		{
			g_free(company_name);
			company_name = g_strdup("Unknown");
		}

		// city
		if ( strlen(city) > 30 ) { SHOW_ERR(_("City must be less than 30 characters")); goto keystore_dialog_clean_up; }
		for( i = 0; i < strlen(city); i++ )
		{
			if ( (city[i] < 97 || city[i] > 122)
			  && (city[i] < 65 || city[i] > 90) 
			  && city[i] != 32 ) 
			{ 
				SHOW_ERR(_("City contains invalid characters, must be A-Z and spaces only")); 
				goto keystore_dialog_clean_up; 
			}
		}
		if ( !*city )
		{
			g_free(city);
			city = g_strdup("Unknown");
		}

		// country
		if ( strlen(country) > 0 && strlen(country) != 2 ) { SHOW_ERR(_("Country code must be 2 characters")); goto keystore_dialog_clean_up; }
		for( i = 0; i < strlen(country); i++ )
		{
			if ( (country[i] < 97 || country[i] > 122)
			  && (country[i] < 65 || country[i] > 90) ) 
			{ 
				SHOW_ERR(_("Country code contains invalid characters, must be A-Z only")); 
				goto keystore_dialog_clean_up; 
			}
		}
		if ( !*country )
		{
			g_free(country);
			country = g_strdup("Unknown");
		}
		
		// check passwords
		if ( !password1 || !*password1 ) { SHOW_ERR(_("Password cannot be blank")); goto keystore_dialog_clean_up; }
		if ( strlen(password1) < 6 ) { SHOW_ERR(_("Password must be at least 6 characters long")); goto keystore_dialog_clean_up; }
		if ( strchr(password1,'"') ) { SHOW_ERR(_("Password cannot contain double quotes")); goto keystore_dialog_clean_up; }
		if ( strcmp(password1,password2) != 0 ) { SHOW_ERR(_("Passwords do not match")); goto keystore_dialog_clean_up; }

		goto keystore_dialog_continue;

keystore_dialog_clean_up:
		if ( full_name ) g_free(full_name);
		if ( company_name ) g_free(company_name);
		if ( city ) g_free(city);
		if ( country ) g_free(country);
		if ( password1 ) g_free(password1);
		if ( password2 ) g_free(password2);

		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.keystore_dialog, "button8"), TRUE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.keystore_dialog, "button9"), TRUE );
		running = 0;
		return;

keystore_dialog_continue:

		;

		FILE* fp = NULL;
		gchar* buffer = NULL;
		long lSize = 0;

		gchar* keytoolFile = g_build_path ( "/", app->datadir, "keytool.txt", NULL );

		fp = fopen ( keytoolFile, "rt" );

		if ( fp )
		{
			fseek ( fp, 0, SEEK_END );
			lSize = ftell ( fp );
			rewind ( fp );

			// allocate memory to contain the whole file:
			buffer = ( gchar* ) malloc ( sizeof ( gchar ) * lSize );

			fread ( buffer, 1, lSize, fp );

			fclose ( fp );
		}

		// CHECKS COMPLETE, START KEY GENERATION

#if defined(G_OS_WIN32)
		gchar* path_to_keytool = g_build_path( "/", app->datadir, "android", "jre", "bin", "keytool.exe", NULL );
#elif defined(__APPLE__)
        gchar* path_to_keytool = g_strdup( "/usr/bin/keytool" );
#else
        gchar* path_to_keytool = g_build_path( "/", app->datadir, "android", "jre", "bin", "keytool", NULL );
#endif

		if ( buffer )
		{
			path_to_keytool = g_build_path ( "/", buffer, NULL );

			free ( buffer );
			buffer = NULL;
		}

		// decalrations
		gchar **argv = NULL;
		gchar *dname = NULL;
		int status = 0;
		GError *error = 0;
		gchar *keystore_name = NULL;
		gchar* str_out = 0;

		utils_str_replace_char( output_file, '\\', '/' );
		gchar* slash = strrchr( output_file, '/' );
		if ( slash )
		{
			keystore_name = g_strdup( slash+1 );
			*slash = 0;
		}
		else
		{
			keystore_name = g_strdup( output_file );
			g_free(output_file);
			output_file = global_project_prefs.project_file_path;
		}


		if ( !g_file_test( path_to_keytool, G_FILE_TEST_EXISTS ) )
		{
			SHOW_ERR1( _("Could not find keytool program, the path \"%s\" is incorrect"), path_to_keytool );
			goto keystore_dialog_cleanup2;
		}

		dname = g_strdup_printf( "CN=%s, O=%s, L=%s, C=%s", full_name, company_name, city, country );
		
		// package manifest and resources
		argv = g_new0( gchar*, 19 );
		argv[0] = g_strdup( path_to_keytool );
		argv[1] = g_strdup("-genkey");
		argv[2] = g_strdup("-keystore");
		argv[3] = g_strdup(keystore_name);
		argv[4] = g_strdup("-alias");
		argv[5] = g_strdup("mykeystore");
		argv[6] = g_strdup("-keyalg");
		argv[7] = g_strdup("RSA");
		argv[8] = g_strdup("-keysize");
		argv[9] = g_strdup("2048");
		argv[10] = g_strdup("-validity");
		argv[11] = g_strdup("20000");
		argv[12] = g_strdup("-storepass");
		argv[13] = g_strdup(password1);
		argv[14] = g_strdup("-keypass");
		argv[15] = g_strdup(password1);
		argv[16] = g_strdup("-dname");
		argv[17] = g_strdup(dname);
		argv[18] = NULL;

		if ( !utils_spawn_sync( output_file, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run keytool program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto keystore_dialog_cleanup2;
		}
		
        if ( status != 0 && str_out && *str_out )
		{
			SHOW_ERR1( _("keytool program returned error: %s"), str_out );
			goto keystore_dialog_cleanup2;
		}

		gtk_widget_hide(GTK_WIDGET(dialog));

keystore_dialog_cleanup2:
        
        gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.keystore_dialog, "button8"), TRUE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.keystore_dialog, "button9"), TRUE );

		if ( error ) g_error_free(error);

		if ( path_to_keytool ) g_free(path_to_keytool);
		if ( argv ) g_strfreev(argv);
		if ( dname ) g_free(dname);
		if ( keystore_name ) g_free(keystore_name);
		if ( str_out ) g_free(str_out);
		
		if ( full_name ) g_free(full_name);
		if ( company_name ) g_free(company_name);
		if ( city ) g_free(city);
		if ( country ) g_free(country);
		if ( password1 ) g_free(password1);
		if ( password2 ) g_free(password2);
	}

	running = 0;
}

void project_generate_keystore()
{


	if (ui_widgets.keystore_dialog == NULL)
	{
		ui_widgets.keystore_dialog = create_keystore_dialog();
		gtk_widget_set_name(ui_widgets.keystore_dialog, _("Generate Keystore"));
		gtk_window_set_transient_for(GTK_WINDOW(ui_widgets.keystore_dialog), GTK_WINDOW(main_widgets.window));

		g_signal_connect(ui_widgets.keystore_dialog, "response", G_CALLBACK(on_keystore_dialog_response), NULL);
        g_signal_connect(ui_widgets.keystore_dialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);

		ui_setup_open_button_callback_keystore(ui_lookup_widget(ui_widgets.keystore_dialog, "keystore_output_file_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_ENTRY(ui_lookup_widget(ui_widgets.keystore_dialog, "keystore_output_file_entry"))); 
	}

	GtkWidget *widget = ui_lookup_widget(ui_widgets.keystore_dialog, "keystore_output_file_entry");
	const gchar *output_file = gtk_entry_get_text(GTK_ENTRY(widget));
	if ( !output_file || !*output_file )
	{
		gchar* out_path = g_build_filename( global_project_prefs.project_file_path, "release.keystore", NULL );
		gtk_entry_set_text( GTK_ENTRY(ui_lookup_widget(ui_widgets.keystore_dialog, "keystore_output_file_entry")), out_path );
		g_free(out_path);
	}

	gtk_window_present(GTK_WINDOW(ui_widgets.keystore_dialog));
}

static int ios_convert_to_cgbi_png( const char* oldfilename, const char* newfilename )
{
	gint status = 0;
	GError *error = NULL;

	gchar** argv = g_new0( gchar*, 8 );
	argv[0] = g_strdup( "/usr/bin/xcrun" );
	argv[1] = g_strdup("-sdk");
	argv[2] = g_strdup("iphoneos");
	argv[3] = g_strdup("pngcrush");
	argv[4] = g_strdup("-iphone");
	argv[5] = g_strdup(oldfilename);
	argv[6] = g_strdup(newfilename);
	argv[7] = NULL;

	if ( !utils_spawn_sync( "/", argv, NULL, 0, NULL, NULL, NULL, NULL, &status, &error) )
	{
		SHOW_ERR1( _("Failed to run \"pngcrush\" program: %s"), error->message );
		g_error_free(error);
		error = NULL;
		g_strfreev(argv);
		return 0;
	}
		
	if ( !g_file_test (newfilename, G_FILE_TEST_EXISTS) )
	{
		SHOW_ERR1( _("Failed to convert png file \"%s\" to Apple's CgBI format"), newfilename );
		g_strfreev(argv);
		return 0;
	}

	g_strfreev(argv);
	return 1;
}

static int hextoint( unsigned char letter )
{
	if ( letter >= 48 && letter <= 57 ) return letter - 48;
	if ( letter >= 65 && letter <= 70 ) return letter - 55;
	if ( letter >= 97 && letter <= 102 ) return letter - 87;
	return 0;
}

static void on_ios_dialog_response(GtkDialog *dialog, gint response, gpointer user_data)
{
	static int running = 0;
	if ( running ) return;

	running = 1;

	if ( app->project && !ios_exporting_player )
	{
		GtkWidget *widget; 

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_name_entry");
		AGK_CLEAR_STR(app->project->ipa_settings.app_name) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		
		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_provisioning_entry");
		AGK_CLEAR_STR(app->project->ipa_settings.prov_profile_path) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_icon_entry");
		AGK_CLEAR_STR(app->project->ipa_settings.app_icon_path) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_firebase_config_entry");
		AGK_CLEAR_STR(app->project->ipa_settings.firebase_config_path) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		// splash screens
		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_splash_entry");
		AGK_CLEAR_STR(app->project->ipa_settings.splash_logo) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_splash_entry2");
		AGK_CLEAR_STR(app->project->ipa_settings.splash_color) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		//widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_facebook_id_entry");
		//AGK_CLEAR_STR(app->project->ipa_settings.facebook_id) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_url_scheme_entry");
		AGK_CLEAR_STR(app->project->ipa_settings.url_scheme) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_deep_link_entry");
		AGK_CLEAR_STR(app->project->ipa_settings.deep_link) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_admob_app_id_entry");
		AGK_CLEAR_STR(app->project->ipa_settings.admob_app_id) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_snapchat_client_id_entry");
		AGK_CLEAR_STR(app->project->ipa_settings.snapchat_client_id) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_orientation_combo");
		app->project->ipa_settings.orientation = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));
				
		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_version_number_entry");
		AGK_CLEAR_STR(app->project->ipa_settings.version_number) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		
		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_build_number_entry");
		AGK_CLEAR_STR(app->project->ipa_settings.build_number) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_device_combo");
		app->project->ipa_settings.device_type = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));
		
		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_uses_ads");
		app->project->ipa_settings.uses_ads = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		// output
		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_output_file_entry");
		app->project->ipa_settings.output_path = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
	}

	if ( response != 1 )
	{
		gtk_widget_hide(GTK_WIDGET(dialog));
	}
	else
	{
		int i;
		GtkWidget *widget;

		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.ios_dialog, "ios_export1"), FALSE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.ios_dialog, "button6"), FALSE );
		
		while (gtk_events_pending())
			gtk_main_iteration();

		// app details
		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_name_entry");
		gchar *app_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		
		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_provisioning_entry");
		gchar *profile = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_icon_entry");
		gchar *app_icon = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_firebase_config_entry");
		gchar *firebase_config = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		// splash screens
		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_splash_entry");
		gchar *app_splash_logo = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_splash_entry2");
		gchar *app_splash_color = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		//widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_facebook_id_entry");
		//gchar *facebook_id = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_url_scheme_entry");
		gchar *url_scheme = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_deep_link_entry");
		gchar *deep_link = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_admob_app_id_entry");
		gchar *admob_app_id = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_snapchat_client_id_entry");
		gchar *snapchat_client_id = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_orientation_combo");
		int orientation = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));
				
		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_version_number_entry");
		gchar *version_number = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		if ( !*version_number ) SETPTR( version_number, g_strdup("1.0.0") );

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_build_number_entry");
		gchar *build_number = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		if ( !*build_number ) SETPTR( build_number, g_strdup("1.0") );

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_device_combo");
		int device_type = gtk_combo_box_get_active(GTK_COMBO_BOX_TEXT(widget));;

		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_uses_ads");
		int uses_ads = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

		// output
		widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_output_file_entry");
		gchar *output_file = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

		gchar *percent = 0;
		while ( (percent = strchr(output_file, '%')) != 0 )
		{
			if ( strncmp( percent+1, "[version]", strlen("[version]") ) == 0 )
			{
				*percent = 0;
				percent += strlen("[version]") + 1;
				gchar *new_output = g_strconcat( output_file, build_number, percent, NULL );
				g_free(output_file);
				output_file = new_output;
				continue;
			}

			break;
		}


		// START CHECKS

		if ( !output_file || !*output_file ) { SHOW_ERR(_("You must choose an output location to save your IPA")); goto ios_dialog_clean_up; }
		if ( strchr(output_file, '.') == 0 ) { SHOW_ERR(_("The output location must be a file not a directory")); goto ios_dialog_clean_up; }

		// check app name
		if ( !app_name || !*app_name ) { SHOW_ERR(_("You must enter an app name")); goto ios_dialog_clean_up; }
		if ( strlen(app_name) > 30 ) { SHOW_ERR(_("App name must be less than 30 characters")); goto ios_dialog_clean_up; }
		for( i = 0; i < strlen(app_name); i++ )
		{
			/*
			if ( (app_name[i] < 97 || app_name[i] > 122)
			  && (app_name[i] < 65 || app_name[i] > 90) 
			  && (app_name[i] < 48 || app_name[i] > 57) 
			  && app_name[i] != 32 
			  && app_name[i] != 95 ) 
			{ 
				SHOW_ERR(_("App name contains invalid characters, must be A-Z 0-9 spaces and undersore only")); 
				goto ios_dialog_clean_up; 
			}
			*/
			//switch to black list
			if ( app_name[i] == 34 || app_name[i] == 60 
			  || app_name[i] == 62 || app_name[i] == 39
			  || app_name[i] == 42 || app_name[i] == 46
			  || app_name[i] == 47 || app_name[i] == 92
			  || app_name[i] == 58 || app_name[i] == 59
			  || app_name[i] == 124 || app_name[i] == 61
			  || app_name[i] == 44 || app_name[i] == 38 )
			{
				SHOW_ERR(_("App name contains invalid characters, it must not contain quotes or any of the following < > * . / \\ : ; | = , &"));
				goto ios_dialog_clean_up; 
			}
		}
		
		// check icon
		//if ( !app_icon || !*app_icon ) { SHOW_ERR(_("You must select an app icon")); goto ios_dialog_clean_up; }
		if ( app_icon && *app_icon )
		{
			if ( !strrchr( app_icon, '.' ) || utils_str_casecmp( strrchr( app_icon, '.' ), ".png" ) != 0 ) { SHOW_ERR(_("App icon must be a PNG file")); goto ios_dialog_clean_up; }
			if ( !g_file_test( app_icon, G_FILE_TEST_EXISTS ) ) { SHOW_ERR(_("Could not find app icon location")); goto ios_dialog_clean_up; }
		}

		if ( firebase_config && *firebase_config )
		{
			if ( !strrchr( firebase_config, '.' ) || utils_str_casecmp( strrchr( firebase_config, '.' ), ".plist" ) != 0 ) { SHOW_ERR(_("Firebase config file must be a .plist file")); goto ios_dialog_clean_up; }
			if ( !g_file_test( firebase_config, G_FILE_TEST_EXISTS ) ) { SHOW_ERR(_("Could not find Firebase config file")); goto ios_dialog_clean_up; }
		}

		// check splash screen logo
		if ( app_splash_logo && *app_splash_logo )
		{
			if ( !strrchr( app_splash_logo, '.' ) || utils_str_casecmp( strrchr( app_splash_logo, '.' ), ".png" ) != 0 ) { SHOW_ERR(_("Splash Screen Logo must be a PNG file")); goto ios_dialog_clean_up; }
			if ( !g_file_test( app_splash_logo, G_FILE_TEST_EXISTS ) ) { SHOW_ERR(_("Could not find Splash Screen Logo location")); goto ios_dialog_clean_up; }
		}

		// check splash screen color
		if ( app_splash_color && *app_splash_color )
		{
			if ( strlen( app_splash_color ) != 6 ) { SHOW_ERR(_("Splash Screen Color must be a 6 digit hex value, e.g. E86C2B")); goto ios_dialog_clean_up; }
			int i;
			for( i = 0; i < strlen(app_splash_color); i++ )
			{
				if ( app_splash_color[i] >= 97 && app_splash_color[i] <= 102 ) app_splash_color[i] -= 32; // change to upper case
				if ( (app_splash_color[i] < 48 || app_splash_color[i] > 57) && (app_splash_color[i] < 65 || app_splash_color[i] > 70) ) 
				{ 
					SHOW_ERR(_("Splash Screen Color must be a 6 digit hex value, e.g. E86C2B")); 
					goto ios_dialog_clean_up; 
				}
			}
		}

		// check profile
		if ( !profile || !*profile ) { SHOW_ERR(_("You must select a provisioning profile")); goto ios_dialog_clean_up; }
		if ( !strrchr( profile, '.' ) || utils_str_casecmp( strrchr( profile, '.' ), ".mobileprovision" ) != 0 ) { SHOW_ERR(_("Provisioning profile must have .mobileprovision extension")); goto ios_dialog_clean_up; }
		if ( !g_file_test( profile, G_FILE_TEST_EXISTS ) ) { SHOW_ERR(_("Could not find provisioning profile location")); goto ios_dialog_clean_up; }

		// check version
		if ( !version_number || !*version_number ) { SHOW_ERR(_("You must enter a version number, e.g. 1.0.0")); goto ios_dialog_clean_up; }
		for( i = 0; i < strlen(version_number); i++ )
		{
			if ( (version_number[i] < 48 || version_number[i] > 57) && version_number[i] != 46 ) 
			{ 
				SHOW_ERR(_("Version number contains invalid characters, must be 0-9 and . only")); 
				goto ios_dialog_clean_up; 
			}
		}

		// check facebook id
		/*
		if ( facebook_id && *facebook_id )
		{
			for( i = 0; i < strlen(facebook_id); i++ )
			{
				if ( (facebook_id[i] < 48 || facebook_id[i] > 57) ) 
				{ 
					SHOW_ERR(_("Facebook App ID must be numbers only")); 
					goto ios_dialog_clean_up; 
				}
			}
		}
		*/

		if ( url_scheme && *url_scheme )
		{
			if ( strchr(url_scheme, ':') || strchr(url_scheme, '/') )
			{
				SHOW_ERR(_("URL scheme must not contain : or /"));
				goto ios_dialog_clean_up; 
			}
		}

		if ( deep_link && *deep_link )
		{
			if ( strchr( deep_link, '.' ) == 0 )
			{
				SHOW_ERR(_("Universal link must be a domain, e.g. www.appgamekit.com"));
				goto ios_dialog_clean_up; 
			}
		}

		if ( admob_app_id && *admob_app_id )
		{
			if ( strchr( admob_app_id, '~' ) == 0 )
			{
				SHOW_ERR(_("AdMob App ID must include a ~ character, for example \"ca-app-pub-3940256099942544~1458002511\""));
				goto ios_dialog_clean_up; 
			}
		}

		if ( !g_file_test( "/Applications/XCode.app/Contents/Developer/usr/bin/actool", G_FILE_TEST_EXISTS ) )
		{
			SHOW_ERR(_("You must install XCode to export iOS apps from the AGK IDE. XCode can be downloaded from the Mac AppStore")); 
			goto ios_dialog_clean_up;
		}
	
		goto ios_dialog_continue;

ios_dialog_clean_up:
		if ( app_name ) g_free(app_name);
		if ( profile ) g_free(profile);
		if ( app_icon ) g_free(app_icon);
		if ( firebase_config ) g_free(firebase_config);
		if ( app_splash_logo ) g_free(app_splash_logo);
		if ( app_splash_color ) g_free(app_splash_color);
		//if ( facebook_id ) g_free(facebook_id);
		if ( url_scheme ) g_free(url_scheme);
		if ( deep_link ) g_free(deep_link);
		if ( admob_app_id ) g_free(admob_app_id);
		if ( snapchat_client_id ) g_free(snapchat_client_id);
		if ( version_number ) g_free(version_number);
		if ( output_file ) g_free(output_file);

		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.ios_dialog, "ios_export1"), TRUE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.ios_dialog, "button6"), TRUE );
		running = 0;
		return;

ios_dialog_continue:

		while (gtk_events_pending())
			gtk_main_iteration();

		// CHECKS COMPLETE, START EXPORT

		const gchar* path_to_codesign = "/usr/bin/codesign";
		const gchar* path_to_security = "/usr/bin/security";
		const gchar* path_to_actool = "/Applications/XCode.app/Contents/Developer/usr/bin/actool";
		const gchar* path_to_ibtool = "/Applications/XCode.app/Contents/Developer/usr/bin/ibtool";

		// make temporary folder
		gchar* ios_folder = g_build_filename( app->datadir, "ios", NULL );
		gchar* tmp_folder;
        if ( !ios_exporting_player && app->project )
            tmp_folder = g_build_filename( app->project->base_path, "build_tmp", NULL );
        else
            tmp_folder = g_build_filename( global_project_prefs.project_file_path, "build_tmp", NULL );
		
        gchar* app_folder = g_build_filename( tmp_folder, app_name, NULL );
		SETPTR(app_folder, g_strconcat( app_folder, ".app", NULL ));

		gchar* app_folder_name = g_strdup( app_name );
		SETPTR(app_folder_name, g_strconcat( app_folder_name, ".app", NULL ));

		utils_str_replace_char( ios_folder, '\\', '/' );
		utils_str_replace_char( tmp_folder, '\\', '/' );
		
		gchar* src_folder = g_build_path( "/", app->datadir, "ios", "source", "AppGameKit Player.app", NULL );
		utils_str_replace_char( src_folder, '\\', '/' );

		gchar* no_ads_binary = g_build_path( "/", app->datadir, "ios", "source", "AppGameKit Player No Ads", NULL );
		utils_str_replace_char( no_ads_binary, '\\', '/' );

		gchar* icons_src_folder = g_build_path( "/", app->datadir, "ios", "source", "Icons.xcassets", NULL );
		utils_str_replace_char( icons_src_folder, '\\', '/' );

		gchar* icons_dst_folder = g_build_path( "/", tmp_folder, "Icons.xcassets", NULL );
		utils_str_replace_char( icons_dst_folder, '\\', '/' );

		gchar* icons_sub_folder = g_build_path( "/", tmp_folder, "Icons.xcassets", "AppIcon.appiconset", NULL );
		utils_str_replace_char( icons_sub_folder, '\\', '/' );

		gchar *output_file_zip = g_strdup( output_file );
		gchar *ext = strrchr( output_file_zip, '.' );
		if ( ext ) *ext = 0;
		SETPTR( output_file_zip, g_strconcat( output_file_zip, ".zip", NULL ) );

		// decalrations
		gchar newcontents[ 32000 ];
		gchar *contents = NULL;
		gsize length = 0;
		gchar *certificate_data = NULL;
		gchar *bundle_id = NULL;
		gchar *app_group_data = 0;
		gchar *team_id = NULL;
		gchar *cert_hash = NULL;
		gchar *cert_temp = NULL;
		gchar **argv = NULL;
		gchar *str_out = NULL;
		gint status = 0;
		GError *error = NULL;
		gchar *entitlements_file = NULL;
		gchar *expanded_entitlements_file = NULL;
		gchar *temp_filename1 = NULL;
		gchar *temp_filename2 = NULL;
		gchar *version_string = NULL;
		gchar *build_string = NULL;
		gchar *bundle_id2 = NULL; // don't free, pointer to sub string
		gchar *image_filename = NULL;
		gchar *temp_image_filename = NULL;
		GdkPixbuf *icon_scaled_image = NULL;
		GdkPixbuf *icon_image = NULL;
		GdkPixbuf *splash_image = NULL;
		gchar *user_name = NULL;
		gchar *group_name = NULL;
		mz_zip_archive zip_archive;
		memset(&zip_archive, 0, sizeof(zip_archive));
		
		if ( !utils_copy_folder( src_folder, app_folder, TRUE, NULL ) )
		{
			SHOW_ERR( _("Failed to copy source folder") );
			goto ios_dialog_cleanup2;
		}

		if ( !uses_ads )
		{
			gchar *binary_path = g_build_filename( app_folder, "AppGameKit Player", NULL );
			utils_copy_file( no_ads_binary, binary_path, TRUE, NULL );
			g_free( binary_path );
		}
		
		// rename executable
		g_chdir( app_folder );
		g_rename( "AppGameKit Player", app_name );
		
		while (gtk_events_pending())
			gtk_main_iteration();

		// open provisioning profile and extract certificate
		if ( !g_file_get_contents( profile, &contents, &length, NULL ) )
		{
			SHOW_ERR( _("Failed to read provisioning profile") );
			goto ios_dialog_cleanup2;
		}

        // provisioning profile starts as binary, so skip 100 bytes to get to text
		gchar* certificate = strstr( contents+100, "<key>DeveloperCertificates</key>" );
		if ( !certificate )
		{
			SHOW_ERR( _("Failed to read certificate from provisioning profile") );
			goto ios_dialog_cleanup2;
		}

		certificate = strstr( certificate, "<data>" );
		if ( !certificate )
		{
			SHOW_ERR( _("Failed to read certificate data from provisioning profile") );
			goto ios_dialog_cleanup2;
		}

		certificate += strlen("<data>");
		gchar* certificate_end = strstr( certificate, "</data>" );
		if ( !certificate_end )
		{
			SHOW_ERR( _("Failed to read certificate end data from provisioning profile") );
			goto ios_dialog_cleanup2;
		}

        // copy certificate into local storage
		gint cert_length = (gint) (certificate_end - certificate);
		certificate_data = g_new0( gchar, cert_length+1 );
		strncpy( certificate_data, certificate, cert_length );
		certificate_data[ cert_length ] = 0;

		utils_str_remove_chars( certificate_data, "\n\r" );
        
		// extract bundle ID, reuse variables
		certificate = strstr( contents+100, "<key>application-identifier</key>" );
		if ( !certificate )
		{
			SHOW_ERR( _("Failed to read bundle ID from provisioning profile") );
			goto ios_dialog_cleanup2;
		}

		certificate = strstr( certificate, "<string>" );
		if ( !certificate )
		{
			SHOW_ERR( _("Failed to read bundle ID data from provisioning profile") );
			goto ios_dialog_cleanup2;
		}

		certificate += strlen("<string>");
		certificate_end = strstr( certificate, "</string>" );
		if ( !certificate_end )
		{
			SHOW_ERR( _("Failed to read bundle ID end data from provisioning profile") );
			goto ios_dialog_cleanup2;
		}
		
        // copy bundle ID to local storage
		cert_length = (gint) (certificate_end - certificate);
		bundle_id = g_new0( gchar, cert_length+1 );
		strncpy( bundle_id, certificate, cert_length );
		bundle_id[ cert_length ] = 0;
        
        // look for beta entitlement
        int betaReports = 0;
        if ( strstr(contents+100, "<key>beta-reports-active</key>") != 0 )
        {
            betaReports = 1;
        }
        
        // look for push notification entitlement
        int pushNotifications = 0;
        gchar* pushStr = strstr(contents+100, "<key>aps-environment</key>");
        if ( pushStr != 0 )
        {
            gchar* pushType = strstr( pushStr, "<string>" );
            if ( strncmp( pushType, "<string>development</string>", strlen("<string>development</string>") ) == 0 )
                pushNotifications = 1;
            else 
                pushNotifications = 2;
        }

		// look for app groups
		certificate = strstr( contents+100, "<key>com.apple.security.application-groups</key>" );
		if ( certificate )
		{
			certificate = strstr( certificate, "<array>" );
			if ( !certificate )
			{
				SHOW_ERR( _("Failed to read App Group data from provisioning profile") );
				goto ios_dialog_cleanup2;
			}

			app_group_data = strstr( certificate, "</array>" );
			if ( !app_group_data )
			{
				SHOW_ERR( _("Failed to read App Group end data from provisioning profile") );
				goto ios_dialog_cleanup2;
			}

			// quick hack to prevent next search going beyond the array list
			*app_group_data = 0;

			// check there is at least one string
			certificate_end = strstr( certificate, "<string>" );

			// repair the string
			*app_group_data = '<'; 
			app_group_data = 0;

			if ( certificate_end )
			{
				// find the end of the list
				certificate_end = strstr( certificate, "</array>" );
				if ( !certificate_end )
				{
					SHOW_ERR( _("Failed to read App Group end data from provisioning profile") );
					goto ios_dialog_cleanup2;
				}

				certificate_end += strlen( "</array>" );
				
				// copy App Group strings to local storage
				cert_length = (gint) (certificate_end - certificate);
				app_group_data = g_new0( gchar, cert_length+1 );
				strncpy( app_group_data, certificate, cert_length );
				app_group_data[ cert_length ] = 0;
			}
		}

		// look for cloud kit
		int cloudKit = 0;
        if ( strstr(contents+100, "<key>com.apple.developer.ubiquity-kvstore-identifier</key>") != 0 )
        {
            cloudKit = 1;
        }
		
		// extract team ID, reuse variables
		certificate = strstr( contents+100, "<key>com.apple.developer.team-identifier</key>" );
		if ( !certificate )
		{
			certificate = strstr( contents+100, "<key>TeamIdentifier</key>" );
			if ( !certificate )
			{
				SHOW_ERR( _("Failed to read team ID from provisioning profile") );
				goto ios_dialog_cleanup2;
			}
		}

		certificate = strstr( certificate, "<string>" );
		if ( !certificate )
		{
			SHOW_ERR( _("Failed to read team ID data from provisioning profile") );
			goto ios_dialog_cleanup2;
		}

		certificate += strlen("<string>");
		certificate_end = strstr( certificate, "</string>" );
		if ( !certificate_end )
		{
			SHOW_ERR( _("Failed to read team ID end data from provisioning profile") );
			goto ios_dialog_cleanup2;
		}
		
        // copy team ID to local storage
		cert_length = (gint) (certificate_end - certificate);
		team_id = g_new0( gchar, cert_length+1 );
		strncpy( team_id, certificate, cert_length );
		team_id[ cert_length ] = 0;
		
		if ( strncmp( team_id, bundle_id, strlen(team_id) ) == 0 )
		{
			// remove team ID
			bundle_id2 = strchr( bundle_id, '.' );
			if ( bundle_id2 ) bundle_id2++;
			else bundle_id2 = bundle_id;
		}
		else
		{
			bundle_id2 = bundle_id;
		}

		// find all certificates, the identity is just the hash of the certificate
		argv = g_new0( gchar*, 8 );
		argv[0] = g_strdup( path_to_security );
		argv[1] = g_strdup("find-certificate");
		argv[2] = g_strdup("-a");
		// certificate not guaranteed to have "iPhone" in the name
		argv[3] = g_strdup("-p"); // use PEM format, same as provisioning profile
		argv[4] = g_strdup("-Z"); // display hash
		argv[5] = NULL;

		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run \"security\" program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}

		g_strfreev(argv);
		argv = 0;
		
		if ( status != 0 && strstr(str_out,"SHA-1") == 0 )
		{
			if ( str_out && *str_out ) dialogs_show_msgbox(GTK_MESSAGE_ERROR, _("Failed to get code signing identities (error %d: %s)"), status, str_out );
			else SHOW_ERR1( _("Failed to get code signing identities (error: %d)"), status );
			goto ios_dialog_cleanup2;
		}

        // cycle through each certificate looking for one that matches provisioning profile
		gchar *sha = strstr( str_out, "SHA-1 hash: " );
		while ( sha )
		{
			sha += strlen( "SHA-1 hash: " );
			gchar *sha_end = strchr( sha, '\n' );
			if ( !sha_end )
			{
				SHOW_ERR( _("Failed to read code signing identity from certificate list") );
				goto ios_dialog_cleanup2;
			}

			gint length = (gint) (sha_end - sha);

            // save hash for later, if this is the correct certificate then this will be the codesigning identity
			if ( cert_hash ) g_free(cert_hash);
			cert_hash = g_new0( gchar, length+1 );
			strncpy( cert_hash, sha, length );
			cert_hash[ length ] = 0;

			sha = sha_end + 1;
			sha = strstr( sha, "-----BEGIN CERTIFICATE-----" );
			if ( !sha )
			{
				SHOW_ERR( _("Failed to read certificate data from certificate list") );
				goto ios_dialog_cleanup2;
			}

			sha += strlen( "-----BEGIN CERTIFICATE-----" ) + 1;
			sha_end = strstr( sha, "-----END CERTIFICATE-----" );
			if ( !sha_end )
			{
				SHOW_ERR( _("Failed to read certificate end data from certificate list") );
				goto ios_dialog_cleanup2;
			}

			length = (gint) (sha_end - sha);

            // copy certificate to temp variable and check it
			if ( cert_temp ) g_free(cert_temp);
			cert_temp = g_new0( gchar, length+1 );
			strncpy( cert_temp, sha, length );
			cert_temp[ length ] = 0;

            // remove new line characters
			utils_str_remove_chars( cert_temp, "\n\r" );
            
			if ( strcmp( cert_temp, certificate_data ) == 0 ) break; // we found the certificate

			if ( cert_hash ) g_free(cert_hash);
			cert_hash = 0;

            // look for next certificate
			sha = sha_end+1;
			sha = strstr( sha, "SHA-1 hash: " );
		}

		if ( !cert_hash )
		{
			SHOW_ERR( _("Could not find the certificate used to create the provisioning profile, have you added the certificate to your keychain?") );
			goto ios_dialog_cleanup2;
		}

		g_free(str_out);
		str_out = 0;

		// find all valid identities
		argv = g_new0( gchar*, 6 );
		argv[0] = g_strdup( path_to_security );
		argv[1] = g_strdup("find-identity");
		argv[2] = g_strdup("-p");
		argv[3] = g_strdup("codesigning");
		argv[4] = g_strdup("-v");
		argv[5] = NULL;

		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run \"security\" program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}

		g_strfreev(argv);
		argv = 0;
		
		if ( status != 0 && strncmp(str_out,"  1) ",strlen("  1) ") != 0) )
		{
			if ( str_out && *str_out ) dialogs_show_msgbox(GTK_MESSAGE_ERROR, _("Failed to get code signing identities (error %d: %s)"), status, str_out );
			else SHOW_ERR1( _("Failed to get code signing identities (error: %d)"), status );
			goto ios_dialog_cleanup2;
		}

		// parse identities, look for the identity we found earlier
		if ( strstr( str_out, cert_hash ) == 0 )
		{
			SHOW_ERR( _("Signing certificate is not valid, either the private key is missing from your keychain, or the certificate has expired") );
			goto ios_dialog_cleanup2;
		}
		
		if ( str_out ) g_free(str_out);
		str_out = 0;

		while (gtk_events_pending())
			gtk_main_iteration();

		// write entitlements file
		strcpy( newcontents, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\
<plist version=\"1.0\">\n<dict>\n	<key>application-identifier</key>\n	<string>" );
		strcat( newcontents, bundle_id );
		strcat( newcontents, "</string>\n	<key>com.apple.developer.team-identifier</key>\n	<string>" );
		strcat( newcontents, team_id );
		strcat( newcontents, "</string>\n" );
		
		if ( betaReports )
			strcat( newcontents, "	<key>beta-reports-active</key>\n	<true/>\n" );
			

		if ( pushNotifications == 1 )
			strcat( newcontents, "	<key>aps-environment</key>\n	<string>development</string>\n" );
		else if ( pushNotifications == 2 )
			strcat( newcontents, "	<key>aps-environment</key>\n	<string>production</string>\n" );

		strcat( newcontents, "	<key>get-task-allow</key>\n	<false/>\n" );

		if ( app_group_data ) 
		{
			strcat( newcontents, "	<key>com.apple.security.application-groups</key>\n" );
			strcat( newcontents, app_group_data );
			strcat( newcontents, "\n" );
		}

		if ( cloudKit )
		{
			strcat( newcontents, "  <key>com.apple.developer.icloud-container-identifiers</key>\n	<array/>" );
			strcat( newcontents, "  <key>com.apple.developer.ubiquity-kvstore-identifier</key>\n	<string>" );
			strcat( newcontents, bundle_id );
			strcat( newcontents, "</string>\n" );
		}

		if ( deep_link && *deep_link )
		{
			gchar *szDomain = strstr( deep_link, "://" );
			if ( szDomain ) szDomain += 3;
			else szDomain = deep_link;

			gchar *szSlash = strchr( szDomain, '/' );
			if ( szSlash ) *szSlash = 0;
			strcat( newcontents, "  <key>com.apple.developer.associated-domains</key>\n <array>\n  <string>applinks:" );
			strcat( newcontents, szDomain );
			strcat( newcontents, "</string>\n</array>\n" );
		}

		strcat( newcontents, "</dict>\n</plist>" );

		entitlements_file = g_build_filename( tmp_folder, "entitlements.xcent", NULL );

		if ( !g_file_set_contents( entitlements_file, newcontents, strlen(newcontents), &error ) )
		{
			SHOW_ERR1( _("Failed to write entitlements file: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}

		// write archived expanded entitlements file
		strcpy( newcontents, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\
<plist version=\"1.0\">\n<dict>\n	<key>application-identifier</key>\n	<string>" );
		strcat( newcontents, bundle_id );
		strcat( newcontents, "</string>\n" );

		if ( app_group_data )
		{
			strcat( newcontents, "	<key>com.apple.security.application-groups</key>\n" );
			strcat( newcontents, app_group_data );
			strcat( newcontents, "\n" );
		}

		strcat( newcontents, "</dict>\n</plist>" );

		expanded_entitlements_file = g_build_filename( app_folder, "archived-expanded-entitlements.xcent", NULL );

		if ( !g_file_set_contents( expanded_entitlements_file, newcontents, strlen(newcontents), &error ) )
		{
			SHOW_ERR1( _("Failed to write expanded entitlements file: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}

		// copy Firebase config file
		if ( firebase_config && *firebase_config )
		{
			temp_filename1 = g_build_filename( app_folder, "GoogleService-Info.plist", NULL );
			utils_copy_file( firebase_config, temp_filename1, TRUE, NULL );
			g_free(temp_filename1);
		}

		// copy provisioning profile
		temp_filename1 = g_build_filename( app_folder, "embedded.mobileprovision", NULL );
		utils_copy_file( profile, temp_filename1, TRUE, NULL );
		// edit Info.plist
		g_free(temp_filename1);
		temp_filename1 = g_build_filename( app_folder, "Info.plist", NULL );

		if ( contents ) g_free(contents);
		contents = 0;
		if ( !g_file_get_contents( temp_filename1, &contents, &length, NULL ) )
		{
			SHOW_ERR( _("Failed to read Info.plist file") );
			goto ios_dialog_cleanup2;
		}

		utils_str_replace_all( &contents, "${PRODUCT_NAME}", app_name );
		utils_str_replace_all( &contents, "${EXECUTABLE_NAME}", app_name );
		if ( admob_app_id && *admob_app_id ) utils_str_replace_all( &contents, "${ADMOB_APP_ID}", admob_app_id );
		else utils_str_replace_all( &contents, "${ADMOB_APP_ID}", "ca-app-pub-3940256099942544~1458002511" ); // needs something, use TGC Admob value
		if ( snapchat_client_id && *snapchat_client_id ) utils_str_replace_all( &contents, "${SNAPCHAT_ID}", snapchat_client_id );
		utils_str_replace_all( &contents, "com.thegamecreators.agk2player", bundle_id2 );
		//if ( facebook_id && *facebook_id ) utils_str_replace_all( &contents, "358083327620324", facebook_id );
		const char* urlschemereplacement = "<string>${URLSCHEMES}</string>\n";
		if ( strstr( contents, urlschemereplacement ) == 0 ) urlschemereplacement = "<string>${URLSCHEMES}</string>\r\n";
		if ( url_scheme && *url_scheme ) 
		{
			gchar* newUrlSchemes = g_new0( gchar*, strlen(url_scheme) + 30 );
			strcpy( newUrlSchemes, "<string>" );
			strcat( newUrlSchemes, url_scheme );
			strcat( newUrlSchemes, "</string>\n" );
			utils_str_replace_all( &contents, urlschemereplacement, newUrlSchemes );
			g_free( newUrlSchemes );
		}
		else
		{
			utils_str_replace_all( &contents, urlschemereplacement, "" );
		}

		switch( orientation )
		{
			case 0:
            {
                utils_str_replace_all( &contents, "<string>UIInterfaceOrientationPortrait</string>", "" );
                utils_str_replace_all( &contents, "<string>UIInterfaceOrientationPortraitUpsideDown</string>", "" );
                utils_str_replace_all( &contents, "${InitialInterfaceOrientation}", "UIInterfaceOrientationLandscapeLeft" );
                break;
            }
			case 1:
            {
                utils_str_replace_all( &contents, "<string>UIInterfaceOrientationLandscapeLeft</string>", "" );
                utils_str_replace_all( &contents, "<string>UIInterfaceOrientationLandscapeRight</string>", "" );
                utils_str_replace_all( &contents, "${InitialInterfaceOrientation}", "UIInterfaceOrientationPortrait" );
                break;
            }
			case 2:
            {
				utils_str_replace_all( &contents, "${InitialInterfaceOrientation}", "UIInterfaceOrientationPortrait" );
                break;
            }
		}
		
		utils_str_replace_all( &contents, "${VERSION}", version_number );
		utils_str_replace_all( &contents, "${BUILD}", build_number );

		if ( device_type == 1 ) utils_str_replace_all( &contents, "\t\t<integer>2</integer>\n", "" );
		else if ( device_type == 2 ) utils_str_replace_all( &contents, "\t\t<integer>1</integer>\n", "" );

		//
		if ( uses_ads )
		{
			//char advert [ 1024 ] = "";

			
			gchar* advert = g_new0 ( gchar*, 30000 );

			
			strcpy ( advert, "<key>NSUserTrackingUsageDescription</key>\n" );
			strcat ( advert, "<string>This identifier will be used to deliver personalized ads to you.</string>\n" );
			strcat ( advert, "<key>SKAdNetworkItems</key>\n" );
			strcat ( advert, "<array>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>2u9pt9hc89.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>44jx6755aq.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>4fzdc2evr5.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>4pfyvq9l8r.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>737z793b9f.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>7ug5zh24hu.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>8s468mfl3y.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>9rd848q2bz.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>9t245vhmpl.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>av6w8kgt66.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>c6k4g5qg8m.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>cj5566h2ga.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>F38H382JLK.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>glqzh8vgby.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>hs6bdukanm.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>mlmmfzh3r3.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>prcb7njmu6.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>t38b2kh725.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>tl55sbb4fm.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>w9q455wk68.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>wg4vff78zm.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>yclnxrl5pm.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>ydx93a7ass.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>s39g8k73mm.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>m8dbw4sv7c.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>4468km3ulz.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>8s468mfl3y.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>lr83yxwka7.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>9nlqeag3gk.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>mls7yz5dvl.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>p78axxw29g.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>v72qych5uu.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>6xzpu9s2p8.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>g28c52eehv.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>523jb4fst2.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>ggvn48r87g.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>3sh42y64q3.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>24t9a8vw3c.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>ejvt5qm6ak.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>cstr6suwn9.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>wzmmz9fp6w.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>5lm9lj6jb7.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>zmvfpc5aq8.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>7rz58n8ntl.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>feyaarzu9v.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>n9x2a789qt.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>5a6flpkh64.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>424m5254lk.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>5l3tpt7t6e.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>uw77j35x4d.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>gta9lk7p23.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>kbd757ywx3.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>cg4yq2srnc.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>ludvb6z3bs.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>22mmun2rn5.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>su67r6k2v3.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>4w7y6s5ca2.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>klf5c3l5u5.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>dzg6xy7pwj.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>y45688jllp.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>hdw39hrw9y.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>ppxm28t8ap.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>4dzt52r2t5.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>mtkv5xtk9e.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>5tjdwbrq8w.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>3rd42ekr43.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>klf5c3l5u5.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>x44k69ngh6.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>pwa73g5rt2.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>mp6xlyr22a.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>qqp299437r.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>xy9t38ct57.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>54nzkqm89y.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>9b89h5y424.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>32z4fx6l9h.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>79pbpufp6p.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>275upjj5gd.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>294l99pt4k.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>2fnua5tdw4.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>32z4fx6l9h.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>3qcr597p9d.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>44n7hlldy6.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>54nzkqm89y.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>578prtvx9j.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>6g9af3uyq4.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>6p4ks3rnbw.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>79pbpufp6p.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>97r2b46745.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>9b89h5y424.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>e5fvkxwrpn.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>f73kdq92p3.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>g2y4y55b64.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>k674qkevps.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>mp6xlyr22a.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>n6fk4nfna4.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>qqp299437r.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>rx5hdcabgc.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>s69wq72ugq.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>u679fj5vs4.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>xy9t38ct57.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>kbmxgpxpgc.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "<dict>\n" );
			strcat ( advert, "<key>SKAdNetworkIdentifier</key>\n" );
			strcat ( advert, "<string>rvh3l7un93.skadnetwork</string>\n" );
			strcat ( advert, "</dict>\n" );
			strcat ( advert, "</array>\n" );

			utils_str_replace_all ( &contents, "<key>${ADVERT_ITEMS}</key><string></string>", advert );
			

			g_free ( advert );
			
		}
		
		//

		if ( !g_file_set_contents( temp_filename1, contents, strlen(contents), NULL ) )
		{
			SHOW_ERR( _("Failed to write Info.plist file") );
			goto ios_dialog_cleanup2;
		}

		g_free(str_out);
		str_out = 0;

		// convert plist to binary
		argv = g_new0( gchar*, 6 );
		argv[0] = g_strdup( "/usr/bin/plutil" );
		argv[1] = g_strdup("-convert");
		argv[2] = g_strdup("binary1");
		argv[3] = g_strdup(temp_filename1);
		argv[4] = NULL;

		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run userid program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}

		g_strfreev(argv);
		argv = 0;
		
        /*
		if ( status != 0 )
		{
			SHOW_ERR1( _("Failed to get user name (error: %d)"), status );
			goto ios_dialog_cleanup2;
		}
         */

		// load icon file
		if ( app_icon && *app_icon )
		{
			// write Icons.xcassets file
			if ( !utils_copy_folder( icons_src_folder, icons_dst_folder, TRUE, NULL ) )
			{
				SHOW_ERR( _("Failed to create icon asset catalog") );
				goto ios_dialog_cleanup2;
			}

			icon_image = gdk_pixbuf_new_from_file( app_icon, &error );
			if ( !icon_image || error )
			{
				SHOW_ERR1( _("Failed to load image icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto ios_dialog_cleanup2;
			}

			temp_image_filename = g_build_path( "/", icons_sub_folder, "temp.png", NULL );

			// scale it and save it
			// 152x152
			image_filename = g_build_path( "/", icons_sub_folder, "icon-152.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 152, 152, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, temp_image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save 152x152 icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto ios_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			if ( !ios_convert_to_cgbi_png( temp_image_filename, image_filename ) ) goto ios_dialog_cleanup2;
			g_unlink( temp_image_filename );
			g_free( image_filename );

			// 180x180
			image_filename = g_build_path( "/", icons_sub_folder, "icon-180.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 180, 180, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, temp_image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save 180x180 icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto ios_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			if ( !ios_convert_to_cgbi_png( temp_image_filename, image_filename ) ) goto ios_dialog_cleanup2;
			g_unlink( temp_image_filename );
			g_free( image_filename );

			// 167x167
			image_filename = g_build_path( "/", icons_sub_folder, "icon-167.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 167, 167, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, temp_image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save 167x167 icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto ios_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			if ( !ios_convert_to_cgbi_png( temp_image_filename, image_filename ) ) goto ios_dialog_cleanup2;
			g_unlink( temp_image_filename );
			g_free( image_filename );

			// 120x120
			image_filename = g_build_path( "/", icons_sub_folder, "icon-120.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 120, 120, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, temp_image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save 120x120 icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto ios_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			if ( !ios_convert_to_cgbi_png( temp_image_filename, image_filename ) ) goto ios_dialog_cleanup2;
			g_unlink( temp_image_filename );
			g_free( image_filename );

			// 76x76
			image_filename = g_build_path( "/", icons_sub_folder, "icon-76.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 76, 76, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, temp_image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save 76x76 icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto ios_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			if ( !ios_convert_to_cgbi_png( temp_image_filename, image_filename ) ) goto ios_dialog_cleanup2;
			g_unlink( temp_image_filename );
			g_free( image_filename );

			/*
			// 60x60
			image_filename = g_build_path( "/", icons_sub_folder, "icon-60.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 60, 60, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, temp_image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save 60x60 icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto ios_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			if ( !ios_convert_to_cgbi_png( temp_image_filename, image_filename ) ) goto ios_dialog_cleanup2;
			g_unlink( temp_image_filename );
			g_free( image_filename );
			*/

			// 1024x1024
			image_filename = g_build_path( "/", icons_sub_folder, "icon-1024.png", NULL );
			icon_scaled_image = gdk_pixbuf_scale_simple( icon_image, 1024, 1024, GDK_INTERP_HYPER );
			if ( !gdk_pixbuf_save( icon_scaled_image, temp_image_filename, "png", &error, "compression", "9", NULL ) )
			{
				SHOW_ERR1( _("Failed to save 1024x1024 icon: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto ios_dialog_cleanup2;
			}
			gdk_pixbuf_unref( icon_scaled_image );
			if ( !ios_convert_to_cgbi_png( temp_image_filename, image_filename ) ) goto ios_dialog_cleanup2;
			g_unlink( temp_image_filename );
			g_free( image_filename );
			g_free( temp_image_filename );

			icon_scaled_image = NULL;
			image_filename = NULL;
			temp_image_filename = NULL;

			// run actool to compile asset catalog, it will copy the app icons to the app_folder and create the Assets.car file
			argv = g_new0( gchar*, 20 );
			argv[0] = g_strdup( path_to_actool );
			argv[1] = g_strdup("--output-partial-info-plist");
			argv[2] = g_strdup("temp.plist");
			argv[3] = g_strdup("--app-icon");
			argv[4] = g_strdup("AppIcon");
			argv[5] = g_strdup("--target-device");
			argv[6] = g_strdup("iphone");
			argv[7] = g_strdup("--target-device");
			argv[8] = g_strdup("ipad");
			argv[9] = g_strdup("--minimum-deployment-target");
			argv[10] = g_strdup("9.0");
			argv[11] = g_strdup("--platform");
			argv[12] = g_strdup("iphoneos");
			argv[13] = g_strdup("--product-type");
			argv[14] = g_strdup("com.apple.product-type.application");
			argv[15] = g_strdup("--compress-pngs");
			argv[16] = g_strdup("--compile");
			argv[17] = g_strdup(app_folder_name);
			argv[18] = g_strdup("Icons.xcassets");
			argv[19] = NULL;

			if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
			{
				SHOW_ERR1( _("Failed to run \"actool\" program: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto ios_dialog_cleanup2;
			}
		
			if ( !str_out || strstr(str_out,"actool.errors") != 0 || strstr(str_out,"actool.warnings") != 0 || strstr(str_out,"actool.notices") != 0 )
			{
				if ( str_out && *str_out ) dialogs_show_msgbox(GTK_MESSAGE_ERROR, _("Failed to compile asset catalog (error %d: %s)"), status, str_out );
				else SHOW_ERR1( _("Failed to get compile asset catalog (error: %d)"), status );
				goto ios_dialog_cleanup2;
			}

			g_free(str_out);
			str_out = 0;
			g_strfreev(argv);
			argv = 0;
		}

		while (gtk_events_pending())
			gtk_main_iteration();

		// copy splash logo
		if ( app_splash_logo && *app_splash_logo )
		{
			if ( temp_filename1 ) g_free(temp_filename1);
			temp_filename1 = g_build_filename( app_folder, "SplashLogo.png", NULL );
			utils_copy_file( app_splash_logo, temp_filename1, TRUE, NULL );
		}
		
		if ( app_splash_color && *app_splash_color )
		{
			int iRed = hextoint( app_splash_color[0] ) * 16 + hextoint( app_splash_color[1] );
			int iGreen = hextoint( app_splash_color[2] ) * 16 + hextoint( app_splash_color[3] );
			int iBlue = hextoint( app_splash_color[4] ) * 16 + hextoint( app_splash_color[5] );

			float red = iRed / 255.0f;
			float green = iGreen / 255.0f;
			float blue = iBlue / 255.0f;

			// edit UntitledViewController
			if ( temp_filename1 ) g_free(temp_filename1);
			temp_filename1 = g_build_filename( app_folder, "UntitledViewController.xib", NULL );

			if ( contents ) g_free(contents);
			contents = 0;
			if ( !g_file_get_contents( temp_filename1, &contents, &length, NULL ) )
			{
				SHOW_ERR( _("Failed to read UntitledViewController.xib file") );
				goto ios_dialog_cleanup2;
			}

			*newcontents = 0;
			char* start = strstr( contents, "backgroundColor\"" );
			if ( !start )
			{
				SHOW_ERR( _("Failed to modify UntitledViewController.xib, backgroundColor not found") );
				goto ios_dialog_cleanup2;
			}

			start += strlen("backgroundColor\"");
			*start = 0;
			strcpy( newcontents, contents );
			*start = ' ';

			char newcolor[ 256 ];
			*newcolor = 0;
			sprintf( newcolor, " red=\"%f\" green=\"%f\" blue=\"%f\" ", red, green, blue );
			strcat( newcontents, newcolor );

			start = strstr( start, "alpha=" );
			if ( !start )
			{
				SHOW_ERR( _("Failed to modify UntitledViewController.xib, alpha not found") );
				goto ios_dialog_cleanup2;
			}

			strcat( newcontents, start );

			if ( !g_file_set_contents( temp_filename1, newcontents, strlen(newcontents), &error ) )
			{
				SHOW_ERR1( _("Failed to write UntitledViewController.xib file: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto ios_dialog_cleanup2;
			}
			
			// edit storyboard
			if ( temp_filename1 ) g_free(temp_filename1);
			temp_filename1 = g_build_filename( app_folder, "LaunchScreen.storyboard", NULL );

			if ( contents ) g_free(contents);
			contents = 0;
			if ( !g_file_get_contents( temp_filename1, &contents, &length, NULL ) )
			{
				SHOW_ERR( _("Failed to read LaunchScreen.storyboard file") );
				goto ios_dialog_cleanup2;
			}	

			*newcontents = 0;
			start = strstr( contents, "backgroundColor\"" );
			if ( !start )
			{
				SHOW_ERR( _("Failed to modify LaunchScreen.storyboard, backgroundColor not found") );
				goto ios_dialog_cleanup2;
			}

			start += strlen("backgroundColor\"");
			*start = 0;
			strcpy( newcontents, contents );
			*start = ' ';

			*newcolor = 0;
			sprintf( newcolor, " red=\"%f\" green=\"%f\" blue=\"%f\" ", red, green, blue );
			strcat( newcontents, newcolor );

			start = strstr( start, "alpha=" );
			if ( !start )
			{
				SHOW_ERR( _("Failed to modify LaunchScreen.storyboard, alpha not found") );
				goto ios_dialog_cleanup2;
			}

			strcat( newcontents, start );

			if ( !g_file_set_contents( temp_filename1, newcontents, strlen(newcontents), &error ) )
			{
				SHOW_ERR1( _("Failed to write LaunchScreen.storyboard file: %s"), error->message );
				g_error_free(error);
				error = NULL;
				goto ios_dialog_cleanup2;
			}
		}

		// compile UntitledViewController
		argv = g_new0( gchar*, 19 );
		argv[0] = g_strdup( path_to_ibtool );
		argv[1] = g_strdup("--errors");
		argv[2] = g_strdup("--warnings");
		argv[3] = g_strdup("--notices");
		argv[4] = g_strdup("--module");
		argv[5] = g_strdup("AppGameKit_Player");
		argv[6] = g_strdup("--auto-activate-custom-fonts");
		argv[7] = g_strdup("--target-device");
		argv[8] = g_strdup("iphone");
		argv[9] = g_strdup("--target-device");
		argv[10] = g_strdup("ipad");
		argv[11] = g_strdup("--minimum-deployment-target");
		argv[12] = g_strdup("9.0");
		argv[13] = g_strdup("--output-format");
		argv[14] = g_strdup("human-readable-text");
		argv[15] = g_strdup("--compile");
		argv[16] = g_strdup("UntitledViewController.nib");
		argv[17] = g_strdup("UntitledViewController.xib");
		argv[18] = NULL;
			
		if ( !utils_spawn_sync( app_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run \"ibtool\" program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}
		
		if ( !str_out || strstr(str_out,"ibtool.errors") != 0 || strstr(str_out,"ibtool.warnings") != 0 || strstr(str_out,"ibtool.notices") != 0 )
		{
			if ( str_out && *str_out ) dialogs_show_msgbox(GTK_MESSAGE_ERROR, _("Failed to compile UntitledViewController.nib (error %d: %s)"), status, str_out );
			else SHOW_ERR1( _("Failed to get compile UntitledViewController.nib (error: %d)"), status );
			goto ios_dialog_cleanup2;
		}

		g_free(str_out);
		str_out = 0;
		g_strfreev(argv);
		argv = 0;

		// delete old UntitledViewController.xib
		if ( temp_filename1 ) g_free(temp_filename1);
		temp_filename1 = g_build_filename( app_folder, "UntitledViewController.xib", NULL );
		g_unlink( temp_filename1 );


		// compile storyboard
		argv = g_new0( gchar*, 19 );
		argv[0] = g_strdup( path_to_ibtool );
		argv[1] = g_strdup("--errors");
		argv[2] = g_strdup("--warnings");
		argv[3] = g_strdup("--notices");
		argv[4] = g_strdup("--module");
		argv[5] = g_strdup("AppGameKit_Player");
		argv[6] = g_strdup("--auto-activate-custom-fonts");
		argv[7] = g_strdup("--target-device");
		argv[8] = g_strdup("iphone");
		argv[9] = g_strdup("--target-device");
		argv[10] = g_strdup("ipad");
		argv[11] = g_strdup("--minimum-deployment-target");
		argv[12] = g_strdup("9.0");
		argv[13] = g_strdup("--output-format");
		argv[14] = g_strdup("human-readable-text");
		argv[15] = g_strdup("--compilation-directory");
		argv[16] = g_strdup("./");
		argv[17] = g_strdup("LaunchScreen.storyboard");
		argv[18] = NULL;
			
		if ( !utils_spawn_sync( app_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run \"ibtool\" program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}
		
		if ( !str_out || strstr(str_out,"ibtool.errors") != 0 || strstr(str_out,"ibtool.warnings") != 0 || strstr(str_out,"ibtool.notices") != 0 )
		{
			if ( str_out && *str_out ) dialogs_show_msgbox(GTK_MESSAGE_ERROR, _("Failed to compile LaunchScreen.storyboard (error %d: %s)"), status, str_out );
			else SHOW_ERR1( _("Failed to get compile LaunchScreen.storyboard (error: %d)"), status );
			goto ios_dialog_cleanup2;
		}

		g_free(str_out);
		str_out = 0;
		g_strfreev(argv);
		argv = 0;

		// delete old storyboard
		if ( temp_filename1 ) g_free(temp_filename1);
		temp_filename1 = g_build_filename( app_folder, "LaunchScreen.storyboard", NULL );
		g_unlink( temp_filename1 );
		
		while (gtk_events_pending())
			gtk_main_iteration();

		// copy media folder
        if ( !ios_exporting_player && app->project )
        {
            if ( temp_filename1 ) g_free(temp_filename1);
            temp_filename1 = g_build_filename( app->project->base_path, "media", NULL );
            temp_filename2 = g_build_filename( app_folder, "media", NULL );
            utils_copy_folder( temp_filename1, temp_filename2, TRUE, NULL );
        }

		g_free(str_out);
		str_out = 0;

		// find user name
		argv = g_new0( gchar*, 6 );
		argv[0] = g_strdup( "/usr/bin/id" );
		argv[1] = g_strdup("-u");
		argv[2] = g_strdup("-n");
		argv[3] = NULL;

		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run userid program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}
		
		if ( !str_out || !*str_out )
		{
			SHOW_ERR1( _("Failed to get user name (error: %d)"), status );
			goto ios_dialog_cleanup2;
		}

		user_name = g_strdup( str_out );
        user_name[ strlen(user_name)-1 ] = 0;

		g_free(str_out);
		str_out = 0;
		g_strfreev(argv);
		argv = 0;

		// find group name
		argv = g_new0( gchar*, 6 );
		argv[0] = g_strdup( "/usr/bin/id" );
		argv[1] = g_strdup("-g");
		argv[2] = g_strdup("-n");
		argv[3] = NULL;

		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run groupid program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}
		
		if ( !str_out || !*str_out )
		{
			SHOW_ERR1( _("Failed to get group name (error: %d)"), status );
			goto ios_dialog_cleanup2;
		}

		group_name = g_strdup( str_out );
        group_name[ strlen(group_name)-1 ] = 0;

		g_free(str_out);
		str_out = 0;
		g_strfreev(argv);
		argv = 0;

		// prepare bundle
		argv = g_new0( gchar*, 6 );
		argv[0] = g_strdup( "/usr/sbin/chown" );
		argv[1] = g_strdup("-RH");
		argv[2] = g_strconcat( user_name, ":", group_name, NULL );
		argv[3] = g_strdup(app_folder);
		argv[4] = NULL;
        
		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run chown program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}
		
        /*
		if ( status != 0 && status < 256 )
		{
			if ( str_out && *str_out ) SHOW_ERR1( _("Failed to set file ownership (error: %s)"), str_out );
			else SHOW_ERR1( _("Failed to set file ownership (error: %d)"), status );
			goto ios_dialog_cleanup2;
		}
         */

		g_free(str_out);
		str_out = 0;
		g_strfreev(argv);
		argv = 0;

		// prepare bundle 2
		argv = g_new0( gchar*, 6 );
		argv[0] = g_strdup( "/bin/chmod" );
		argv[1] = g_strdup("-RH");
		argv[2] = g_strdup("u+w,go-w,a+rX");
		argv[3] = g_strdup(app_folder);
		argv[4] = NULL;

		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run chmod program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}
		
        /*
		if ( status != 0 && status < 256 )
		{
			if ( str_out && *str_out ) SHOW_ERR1( _("Failed to set file permissions (error: %s)"), str_out );
			else SHOW_ERR1( _("Failed to set file permissions (error: %d)"), status );
			goto ios_dialog_cleanup2;
		}
         */

		g_free(str_out);
		str_out = 0;
		g_strfreev(argv);
		argv = 0;

		// sign SnapChat
		if ( temp_filename2 ) g_free(temp_filename2);
		temp_filename2 = g_build_filename( app_folder, "Frameworks/SCSDKCoreKit.framework", NULL );
		argv = g_new0( gchar*, 10 );
		argv[0] = g_strdup( path_to_codesign );
		argv[1] = g_strdup("--force");
		argv[2] = g_strdup("--sign");
		argv[3] = g_strdup(cert_hash);
		//argv[4] = g_strdup("--entitlements");
		//argv[5] = g_strdup(entitlements_file);
		argv[4] = g_strdup(temp_filename2);
		argv[5] = NULL;
        
		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run codesign program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}

		g_strfreev(argv);
		argv = 0;

		// sign SnapChat2
		if ( temp_filename2 ) g_free(temp_filename2);
		temp_filename2 = g_build_filename( app_folder, "Frameworks/SCSDKCreativeKit.framework", NULL );
		argv = g_new0( gchar*, 10 );
		argv[0] = g_strdup( path_to_codesign );
		argv[1] = g_strdup("--force");
		argv[2] = g_strdup("--sign");
		argv[3] = g_strdup(cert_hash);
		//argv[4] = g_strdup("--entitlements");
		//argv[5] = g_strdup(entitlements_file);
		argv[4] = g_strdup(temp_filename2);
		argv[5] = NULL;

		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run codesign program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}

		g_strfreev(argv);
		argv = 0;

		// sign bundle
		argv = g_new0( gchar*, 10 );
		argv[0] = g_strdup( path_to_codesign );
		argv[1] = g_strdup("--force");
		argv[2] = g_strdup("--sign");
		argv[3] = g_strdup(cert_hash);
		//argv[4] = g_strdup("--resource-rules"); // Apple stopped using resource rules?
		//argv[5] = g_strconcat( app_folder, "/ResourceRules.plist", NULL );
		argv[4] = g_strdup("--entitlements");
		argv[5] = g_strdup(entitlements_file);
		argv[6] = g_strdup(app_folder);
		argv[7] = NULL;
        
		if ( !utils_spawn_sync( tmp_folder, argv, NULL, 0, NULL, NULL, &str_out, NULL, &status, &error) )
		{
			SHOW_ERR1( _("Failed to run codesign program: %s"), error->message );
			g_error_free(error);
			error = NULL;
			goto ios_dialog_cleanup2;
		}

		g_strfreev(argv);
		argv = 0;
		
        /*
		if ( status != 0 && status < 256 )
		{
			if ( str_out && *str_out ) SHOW_ERR1( _("Failed to sign app (error: %s)"), str_out );
			else SHOW_ERR1( _("Failed to sign app (error: %d)"), status );
			goto ios_dialog_cleanup2;
		}
         */
				

		// create IPA zip file
		if ( !mz_zip_writer_init_file( &zip_archive, output_file_zip, 0 ) )
		{
			SHOW_ERR( _("Failed to initialise zip file for writing") );
			goto ios_dialog_cleanup2;
		}
		
		if ( temp_filename1 ) g_free(temp_filename1);
		temp_filename1 = g_strconcat( "Payload/", app_name, ".app", NULL );
		if ( !utils_add_folder_to_zip( &zip_archive, app_folder, temp_filename1, TRUE, FALSE ) )
		{
			SHOW_ERR( _("Failed to add files to IPA") );
			goto ios_dialog_cleanup2;
		}

		if ( !mz_zip_writer_finalize_archive( &zip_archive ) )
		{
			SHOW_ERR( _("Failed to finalize IPA file") );
			goto ios_dialog_cleanup2;
		}
		if ( !mz_zip_writer_end( &zip_archive ) )
		{
			SHOW_ERR( _("Failed to end IPA file") );
			goto ios_dialog_cleanup2;
		}

		g_rename( output_file_zip, output_file );

		while (gtk_events_pending())
			gtk_main_iteration();

		gtk_widget_hide(GTK_WIDGET(dialog));

ios_dialog_cleanup2:
        
        gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.ios_dialog, "ios_export1"), TRUE );
		gtk_widget_set_sensitive( ui_lookup_widget(ui_widgets.ios_dialog, "button6"), TRUE );

		utils_remove_folder_recursive( tmp_folder );

		if ( output_file_zip ) g_free(output_file_zip);
		if ( ios_folder ) g_free(ios_folder);
		if ( tmp_folder ) g_free(tmp_folder);
		if ( src_folder ) g_free(src_folder);
		if ( app_folder ) g_free(app_folder);
		if ( app_folder_name ) g_free(app_folder_name);
		if ( no_ads_binary ) g_free(no_ads_binary);
		if ( icons_src_folder ) g_free(icons_src_folder);
		if ( icons_dst_folder ) g_free(icons_dst_folder);
		if ( icons_sub_folder ) g_free(icons_sub_folder);

		if ( error ) g_error_free(error);
		if ( str_out ) g_free(str_out);
		if ( argv ) g_strfreev(argv);
		if ( contents ) g_free(contents);
		if ( certificate_data ) g_free(certificate_data);
		if ( team_id ) g_free(team_id);
		if ( bundle_id ) g_free(bundle_id);
		if ( cert_hash ) g_free(cert_hash);
		if ( cert_temp ) g_free(cert_temp);
		if ( app_group_data ) g_free(app_group_data);

		if ( entitlements_file ) g_free(entitlements_file);
		if ( expanded_entitlements_file ) g_free(expanded_entitlements_file);
		if ( temp_filename1 ) g_free(temp_filename1);
		if ( temp_filename2 ) g_free(temp_filename2);
		if ( version_string ) g_free(version_string);
		if ( build_string ) g_free(build_string);
		if ( image_filename ) g_free(image_filename);
		if ( user_name ) g_free(user_name);
		if ( group_name ) g_free(group_name);
		if ( icon_scaled_image ) gdk_pixbuf_unref(icon_scaled_image);
		if ( icon_image ) gdk_pixbuf_unref(icon_image);
		if ( splash_image ) gdk_pixbuf_unref(splash_image);
		
		if ( app_name ) g_free(app_name);
		if ( profile ) g_free(profile);
		if ( app_icon ) g_free(app_icon);
		if ( firebase_config ) g_free(firebase_config);
		//if ( facebook_id ) g_free(facebook_id);
		if ( url_scheme ) g_free(url_scheme);
		if ( deep_link ) g_free(deep_link);
		if ( admob_app_id ) g_free(admob_app_id);
		if ( snapchat_client_id ) g_free(snapchat_client_id);
		if ( version_number ) g_free(version_number);
		if ( build_number ) g_free(build_number);
		if ( output_file ) g_free(output_file);
	}

	running = 0;
}

void project_export_ipa()
{
	static GeanyProject *last_proj = -1;
	static gchar *last_proj_path_ios = 0;

	if ( app->project )
	{
		// make sure the project is up to date
		build_compile_project(0);
	}

	if (ui_widgets.ios_dialog == NULL)
	{
		ui_widgets.ios_dialog = create_ios_dialog();
		gtk_widget_set_name(ui_widgets.ios_dialog, _("Export IPA"));
		gtk_window_set_transient_for(GTK_WINDOW(ui_widgets.ios_dialog), GTK_WINDOW(main_widgets.window));

		g_signal_connect(ui_widgets.ios_dialog, "response", G_CALLBACK(on_ios_dialog_response), NULL);
        g_signal_connect(ui_widgets.ios_dialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);

		ui_setup_open_button_callback_ios(ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_icon_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_icon_entry")));
		ui_setup_open_button_callback_ios(ui_lookup_widget(ui_widgets.ios_dialog, "ios_provisioning_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.ios_dialog, "ios_provisioning_entry")));
		
		ui_setup_open_button_callback_ios(ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_splash_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_splash_entry")));
		
		ui_setup_open_button_callback_ios(ui_lookup_widget(ui_widgets.ios_dialog, "ios_firebase_config_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_ENTRY(ui_lookup_widget(ui_widgets.ios_dialog, "ios_firebase_config_entry")));
		
		ui_setup_open_button_callback_ios(ui_lookup_widget(ui_widgets.ios_dialog, "ios_output_file_path"), NULL,
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_ENTRY(ui_lookup_widget(ui_widgets.ios_dialog, "ios_output_file_entry")));

		gtk_combo_box_set_active( GTK_COMBO_BOX(ui_lookup_widget(ui_widgets.ios_dialog, "ios_orientation_combo")), 0 );
		gtk_combo_box_set_active( GTK_COMBO_BOX(ui_lookup_widget(ui_widgets.ios_dialog, "ios_device_combo")), 0 );
	}

	if ( app->project == 0 )
	{
		// AGK Player

		if ( last_proj != 0 )
		{
			last_proj = app->project;

			GtkWidget *widget; 

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_name_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );
			
			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_provisioning_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_icon_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_firebase_config_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );

			// splash screens
			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_splash_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_splash_entry2");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );

			//widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_facebook_id_entry");
			//gtk_entry_set_text( GTK_ENTRY(widget), "" );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_url_scheme_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_deep_link_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_admob_app_id_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_snapchat_client_id_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_orientation_combo");
			gtk_combo_box_set_active( GTK_COMBO_BOX(widget), 0 );
			
			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_version_number_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );
			
			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_build_number_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), "" );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_device_combo");
			gtk_combo_box_set_active( GTK_COMBO_BOX(widget), 0 );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_uses_ads");
			gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), 0 );

			gchar* apk_path = g_build_filename( global_project_prefs.project_file_path, "AppGameKit Player.ipa", NULL );
			gtk_entry_set_text( GTK_ENTRY(ui_lookup_widget(ui_widgets.ios_dialog, "ios_output_file_entry")), apk_path );
			g_free(apk_path);
		}
	}
	else
	{	
		last_proj = app->project;

        if ( strcmp( FALLBACK(last_proj_path_ios,""), FALLBACK(app->project->file_name,"") ) != 0 )
        {
			if ( last_proj_path_ios ) g_free(last_proj_path_ios);
			last_proj_path_ios = g_strdup( FALLBACK(app->project->file_name,"") );

			GtkWidget *widget; 

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_name_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.app_name, "") );
			
			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_provisioning_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.prov_profile_path, "") );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_icon_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.app_icon_path, "") );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_firebase_config_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.firebase_config_path, "") );

			// splash screens
			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_splash_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.splash_logo, "") );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_splash_entry2");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.splash_color, "") );

			//widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_facebook_id_entry");
			//gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.facebook_id, "") );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_url_scheme_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.url_scheme, "") );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_deep_link_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.deep_link, "") );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_admob_app_id_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.admob_app_id, "") );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_snapchat_client_id_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.snapchat_client_id, "") );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_orientation_combo");
			gtk_combo_box_set_active( GTK_COMBO_BOX(widget), app->project->ipa_settings.orientation );
			
			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_version_number_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.version_number, "") );
			
			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_build_number_entry");
			gtk_entry_set_text( GTK_ENTRY(widget), FALLBACK(app->project->ipa_settings.build_number, "") );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_device_combo");
			gtk_combo_box_set_active( GTK_COMBO_BOX(widget), app->project->ipa_settings.device_type );

			widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_app_uses_ads");
			gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), app->project->ipa_settings.uses_ads ? 1 : 0 );

			if ( !app->project->ipa_settings.output_path || !*app->project->ipa_settings.output_path )
			{
				gchar *filename = g_strconcat( app->project->name, ".ipa", NULL );
				gchar* apk_path = g_build_filename( app->project->base_path, filename, NULL );
				gtk_entry_set_text( GTK_ENTRY(ui_lookup_widget(ui_widgets.ios_dialog, "ios_output_file_entry")), apk_path );
				g_free(apk_path);
				g_free(filename);
			}
			else
			{
				widget = ui_lookup_widget(ui_widgets.ios_dialog, "ios_output_file_entry");
				gtk_entry_set_text( GTK_ENTRY(widget), app->project->ipa_settings.output_path );
			}
        }
	}

	ios_exporting_player = app->project ? 0 : 1;

	gtk_window_present(GTK_WINDOW(ui_widgets.ios_dialog));
}

/* Called when creating, opening, closing and updating projects. */
static void update_ui(void)
{
	if (main_status.quitting)
		return;

	ui_set_window_title(NULL);
	build_menu_update(NULL);
	sidebar_openfiles_update_all();
}

GeanyProject* find_project_for_document( gchar* filename )
{
	gint i, j;
	for ( i = 0; i < projects_array->len; i++ )
	{
		if ( projects[i]->is_valid )
		{
			for( j = 0; j < projects[i]->project_files->len; j++ )
			{
				if ( project_files_index(projects[i],j)->is_valid )
				{
					if ( strcmp( project_files_index(projects[i],j)->file_name, filename ) == 0 )
					{
						return projects[i];
					}
				}
			}
		}
	}

	return 0;
}

gboolean project_close_all()
{
	gint i;
	for ( i = 0; i < projects_array->len; i++ )
	{
		if ( projects[i]->is_valid )
		{
			if ( !project_close( projects[i], FALSE ) ) return FALSE;
			if ( app->project == projects[i] ) app->project = NULL;
		}
	}

	app->project = NULL;

	return TRUE;
}

void init_android_settings( GeanyProject* project )
{
	project->apk_settings.alias = 0;
	project->apk_settings.app_icon_path = 0;
	project->apk_settings.notif_icon_path = 0;
	project->apk_settings.app_name = 0;
	project->apk_settings.app_type = 0; // Google
	project->apk_settings.url_scheme = 0;
	project->apk_settings.deep_link = 0;
	project->apk_settings.keystore_path = 0;
	project->apk_settings.orientation = 0;
	project->apk_settings.output_path = 0;
	project->apk_settings.ouya_icon_path = 0;
	project->apk_settings.package_name = 0;
	project->apk_settings.permission_flags = AGK_ANDROID_PERMISSION_INTERNET | AGK_ANDROID_PERMISSION_WAKE;
	project->apk_settings.play_app_id = 0;
	project->apk_settings.admob_app_id = 0;
	project->apk_settings.snapchat_client_id = 0;
	project->apk_settings.sdk_version = 1; // 4.1
	project->apk_settings.version_name = 0;
	project->apk_settings.version_number = 0;
	project->apk_settings.firebase_config_path = 0;
	project->apk_settings.arcore = 0;
}

void init_ios_settings( GeanyProject* project )
{
	project->ipa_settings.app_icon_path = 0;
	project->ipa_settings.app_name = 0;
	project->ipa_settings.build_number = 0;
	project->ipa_settings.device_type = 0; // iPhone and iPad
	project->ipa_settings.facebook_id = 0;
	project->ipa_settings.url_scheme = 0;
	project->ipa_settings.deep_link = 0;
	project->ipa_settings.admob_app_id = 0;
	project->ipa_settings.snapchat_client_id = 0;
	project->ipa_settings.orientation = 0; // Landscape
	project->ipa_settings.output_path = 0;
	project->ipa_settings.prov_profile_path = 0;
	project->ipa_settings.splash_logo = 0;
	project->ipa_settings.splash_color = 0;
	project->ipa_settings.uses_ads = 0;
	project->ipa_settings.version_number = 0;
	project->ipa_settings.firebase_config_path = 0;
}

void init_html5_settings( GeanyProject* project )
{
	project->html5_settings.commands_used = 0; // 2D only
	project->html5_settings.dynamic_memory = 0;
	project->html5_settings.output_path = 0;
}

void free_android_settings( GeanyProject* project )
{
	if ( project->apk_settings.alias ) g_free(project->apk_settings.alias);
	if ( project->apk_settings.app_icon_path ) g_free(project->apk_settings.app_icon_path);
	if ( project->apk_settings.notif_icon_path ) g_free(project->apk_settings.notif_icon_path);
	if ( project->apk_settings.app_name ) g_free(project->apk_settings.app_name);
	if ( project->apk_settings.url_scheme ) g_free(project->apk_settings.url_scheme);
	if ( project->apk_settings.deep_link ) g_free(project->apk_settings.deep_link);
	if ( project->apk_settings.keystore_path ) g_free(project->apk_settings.keystore_path);
	if ( project->apk_settings.output_path ) g_free(project->apk_settings.output_path);
	if ( project->apk_settings.ouya_icon_path ) g_free(project->apk_settings.ouya_icon_path);
	if ( project->apk_settings.package_name ) g_free(project->apk_settings.package_name);
	if ( project->apk_settings.play_app_id ) g_free(project->apk_settings.play_app_id);
	if ( project->apk_settings.admob_app_id ) g_free(project->apk_settings.admob_app_id);
	if ( project->apk_settings.snapchat_client_id ) g_free(project->apk_settings.snapchat_client_id);
	if ( project->apk_settings.version_name ) g_free(project->apk_settings.version_name);
	if ( project->apk_settings.firebase_config_path ) g_free(project->apk_settings.firebase_config_path);
}

void free_ios_settings( GeanyProject* project )
{
	if ( project->ipa_settings.app_icon_path ) g_free(project->ipa_settings.app_icon_path);
	if ( project->ipa_settings.app_name ) g_free(project->ipa_settings.app_name);
	if ( project->ipa_settings.build_number ) g_free(project->ipa_settings.build_number);
	if ( project->ipa_settings.facebook_id ) g_free(project->ipa_settings.facebook_id);
	if ( project->ipa_settings.deep_link ) g_free(project->ipa_settings.deep_link);
	if ( project->ipa_settings.admob_app_id ) g_free(project->ipa_settings.admob_app_id);
	if ( project->ipa_settings.snapchat_client_id ) g_free(project->ipa_settings.snapchat_client_id);
	if ( project->ipa_settings.output_path ) g_free(project->ipa_settings.output_path);
	if ( project->ipa_settings.prov_profile_path ) g_free(project->ipa_settings.prov_profile_path);
	if ( project->ipa_settings.splash_logo ) g_free(project->ipa_settings.splash_logo);
	if ( project->ipa_settings.splash_color ) g_free(project->ipa_settings.splash_color);
	if ( project->ipa_settings.version_number ) g_free(project->ipa_settings.version_number);
	if ( project->ipa_settings.firebase_config_path ) g_free(project->ipa_settings.firebase_config_path);
}

void free_html5_settings( GeanyProject* project )
{
	if ( project->html5_settings.output_path ) g_free(project->html5_settings.output_path);
}

void save_android_settings( GKeyFile *config, GeanyProject* project )
{
	g_key_file_set_string( config, "apk_settings", "alias", FALLBACK(project->apk_settings.alias,"") );
	g_key_file_set_string( config, "apk_settings", "app_icon_path", FALLBACK(project->apk_settings.app_icon_path,"") );
	g_key_file_set_string( config, "apk_settings", "notif_icon_path", FALLBACK(project->apk_settings.notif_icon_path,"") );
	g_key_file_set_string( config, "apk_settings", "app_name", FALLBACK(project->apk_settings.app_name,"") );
	g_key_file_set_integer( config, "apk_settings", "app_type", project->apk_settings.app_type ); 
	g_key_file_set_string( config, "apk_settings", "url_scheme", FALLBACK(project->apk_settings.url_scheme,"") );
	g_key_file_set_string( config, "apk_settings", "deep_link", FALLBACK(project->apk_settings.deep_link,"") );
	g_key_file_set_string( config, "apk_settings", "keystore_path", FALLBACK(project->apk_settings.keystore_path,"") );
	g_key_file_set_integer( config, "apk_settings", "orientation", project->apk_settings.orientation );
	g_key_file_set_string( config, "apk_settings", "output_path", FALLBACK(project->apk_settings.output_path,"") );
	g_key_file_set_string( config, "apk_settings", "ouya_icon_path", FALLBACK(project->apk_settings.ouya_icon_path,"") );
	g_key_file_set_string( config, "apk_settings", "package_name", FALLBACK(project->apk_settings.package_name,"") );
	g_key_file_set_integer( config, "apk_settings", "permission_flags", project->apk_settings.permission_flags );
	g_key_file_set_string( config, "apk_settings", "play_app_id", FALLBACK(project->apk_settings.play_app_id,"") );
	g_key_file_set_string( config, "apk_settings", "admob_app_id", FALLBACK(project->apk_settings.admob_app_id,"") );
	g_key_file_set_string( config, "apk_settings", "snapchat_client_id", FALLBACK(project->apk_settings.snapchat_client_id,"") );
	g_key_file_set_integer( config, "apk_settings", "sdk_version", project->apk_settings.sdk_version ); 
	g_key_file_set_integer( config, "apk_settings", "arcore", project->apk_settings.arcore ); 
	g_key_file_set_string( config, "apk_settings", "version_name", FALLBACK(project->apk_settings.version_name,"") );
	g_key_file_set_integer( config, "apk_settings", "version_number", project->apk_settings.version_number );
	g_key_file_set_string( config, "apk_settings", "firebase_config_path", FALLBACK(project->apk_settings.firebase_config_path,"") );
}

void save_ios_settings( GKeyFile *config, GeanyProject* project )
{
	g_key_file_set_string( config, "ipa_settings", "app_icon_path", FALLBACK(project->ipa_settings.app_icon_path,"") );
	g_key_file_set_string( config, "ipa_settings", "app_name", FALLBACK(project->ipa_settings.app_name,"") );
	g_key_file_set_string( config, "ipa_settings", "build_number", FALLBACK(project->ipa_settings.build_number,"") );
	g_key_file_set_integer( config, "ipa_settings", "device_type", project->ipa_settings.device_type );
	g_key_file_set_string( config, "ipa_settings", "facebook_id", FALLBACK(project->ipa_settings.facebook_id,"") );
	g_key_file_set_string( config, "ipa_settings", "url_scheme", FALLBACK(project->ipa_settings.url_scheme,"") );
	g_key_file_set_string( config, "ipa_settings", "deep_link", FALLBACK(project->ipa_settings.deep_link,"") );
	g_key_file_set_string( config, "ipa_settings", "admob_app_id", FALLBACK(project->ipa_settings.admob_app_id,"") );
	g_key_file_set_string( config, "ipa_settings", "snapchat_client_id", FALLBACK(project->ipa_settings.snapchat_client_id,"") );
	g_key_file_set_integer( config, "ipa_settings", "orientation", project->ipa_settings.orientation );
	g_key_file_set_string( config, "ipa_settings", "output_path", FALLBACK(project->ipa_settings.output_path,"") );
	g_key_file_set_string( config, "ipa_settings", "prov_profile_path", FALLBACK(project->ipa_settings.prov_profile_path,"") );
	g_key_file_set_string( config, "ipa_settings", "splash_logo", FALLBACK(project->ipa_settings.splash_logo,"") );
	g_key_file_set_string( config, "ipa_settings", "splash_color", FALLBACK(project->ipa_settings.splash_color,"") );
	g_key_file_set_integer( config, "ipa_settings", "uses_ads", project->ipa_settings.uses_ads );
	g_key_file_set_string( config, "ipa_settings", "version_number", FALLBACK(project->ipa_settings.version_number,"") );
	g_key_file_set_string( config, "ipa_settings", "firebase_config_path", FALLBACK(project->ipa_settings.firebase_config_path,"") );
}

void save_html5_settings( GKeyFile *config, GeanyProject* project )
{
	g_key_file_set_integer( config, "html5_settings", "commands_used", project->html5_settings.commands_used );
	g_key_file_set_integer( config, "html5_settings", "dynamic_memory", project->html5_settings.dynamic_memory );
	g_key_file_set_string( config, "html5_settings", "output_path", FALLBACK(project->html5_settings.output_path,"") );
}

void load_android_settings( GKeyFile *config, GeanyProject* project )
{
	project->apk_settings.alias = g_key_file_get_string( config, "apk_settings", "alias", 0 );
	project->apk_settings.app_icon_path = g_key_file_get_string( config, "apk_settings", "app_icon_path", 0 );
	project->apk_settings.notif_icon_path = g_key_file_get_string( config, "apk_settings", "notif_icon_path", 0 );
	project->apk_settings.app_name = g_key_file_get_string( config, "apk_settings", "app_name", 0 );
	project->apk_settings.app_type = utils_get_setting_integer( config, "apk_settings", "app_type", 0 ); 
	project->apk_settings.url_scheme = g_key_file_get_string( config, "apk_settings", "url_scheme", 0 );
	project->apk_settings.deep_link = g_key_file_get_string( config, "apk_settings", "deep_link", 0 );
	project->apk_settings.keystore_path = g_key_file_get_string( config, "apk_settings", "keystore_path", 0 );
	project->apk_settings.orientation = utils_get_setting_integer( config, "apk_settings", "orientation", 0 );
	project->apk_settings.output_path = g_key_file_get_string( config, "apk_settings", "output_path", 0 );
	project->apk_settings.ouya_icon_path = g_key_file_get_string( config, "apk_settings", "ouya_icon_path", 0 );
	project->apk_settings.package_name = g_key_file_get_string( config, "apk_settings", "package_name", 0 );
	project->apk_settings.permission_flags = utils_get_setting_integer( config, "apk_settings", "permission_flags", AGK_ANDROID_PERMISSION_INTERNET | AGK_ANDROID_PERMISSION_WAKE );
	project->apk_settings.play_app_id = g_key_file_get_string( config, "apk_settings", "play_app_id", 0 );
	project->apk_settings.admob_app_id = g_key_file_get_string( config, "apk_settings", "admob_app_id", 0 );
	project->apk_settings.snapchat_client_id = g_key_file_get_string( config, "apk_settings", "snapchat_client_id", 0 );
	project->apk_settings.sdk_version = utils_get_setting_integer( config, "apk_settings", "sdk_version", 0 );
	project->apk_settings.arcore = utils_get_setting_integer( config, "apk_settings", "arcore", 0 );
	project->apk_settings.version_name = g_key_file_get_string( config, "apk_settings", "version_name", 0 );
	project->apk_settings.version_number = utils_get_setting_integer( config, "apk_settings", "version_number", 0 );
	project->apk_settings.firebase_config_path = g_key_file_get_string( config, "apk_settings", "firebase_config_path", 0 );
}

void load_ios_settings( GKeyFile *config, GeanyProject* project )
{
	project->ipa_settings.app_icon_path = g_key_file_get_string( config, "ipa_settings", "app_icon_path", 0 );
	project->ipa_settings.app_name = g_key_file_get_string( config, "ipa_settings", "app_name", 0 );
	project->ipa_settings.build_number = g_key_file_get_string( config, "ipa_settings", "build_number", 0 );
	project->ipa_settings.device_type = utils_get_setting_integer( config, "ipa_settings", "device_type", 0 );
	project->ipa_settings.facebook_id = g_key_file_get_string( config, "ipa_settings", "facebook_id", 0 );
	project->ipa_settings.url_scheme = g_key_file_get_string( config, "ipa_settings", "url_scheme", 0 );
	project->ipa_settings.deep_link = g_key_file_get_string( config, "ipa_settings", "deep_link", 0 );
	project->ipa_settings.admob_app_id = g_key_file_get_string( config, "ipa_settings", "admob_app_id", 0 );
	project->ipa_settings.snapchat_client_id = g_key_file_get_string( config, "ipa_settings", "snapchat_client_id", 0 );
	project->ipa_settings.orientation = utils_get_setting_integer( config, "ipa_settings", "orientation", 0 );
	project->ipa_settings.output_path = g_key_file_get_string( config, "ipa_settings", "output_path", 0 );
	project->ipa_settings.prov_profile_path = g_key_file_get_string( config, "ipa_settings", "prov_profile_path", 0 );
	project->ipa_settings.splash_logo = g_key_file_get_string( config, "ipa_settings", "splash_logo", 0 );
	project->ipa_settings.splash_color = g_key_file_get_string( config, "ipa_settings", "splash_color", 0 );
	project->ipa_settings.uses_ads = utils_get_setting_integer( config, "ipa_settings", "uses_ads", 0 );
	project->ipa_settings.version_number = g_key_file_get_string( config, "ipa_settings", "version_number", 0 );
	project->ipa_settings.firebase_config_path = g_key_file_get_string( config, "ipa_settings", "firebase_config_path", 0 );
}

void load_html5_settings( GKeyFile *config, GeanyProject* project )
{
	project->html5_settings.commands_used = utils_get_setting_integer( config, "html5_settings", "commands_used", 0 );
	project->html5_settings.dynamic_memory = utils_get_setting_integer( config, "html5_settings", "dynamic_memory", 0 );
	project->html5_settings.output_path = g_key_file_get_string( config, "html5_settings", "output_path", 0 );
}


/* open_default will make function reload default session files on close */
gboolean project_close(GeanyProject *project, gboolean open_default)
{
	GSList *node;
	guint i;

	if ( project == NULL ) return TRUE;
	if ( !project->is_valid ) return TRUE;

	/* save project session files, etc */
	if (!write_config(project,FALSE))
		g_warning(_("Project file \"%s\" could not be written"), project->file_name);

	if (project_prefs.project_session)
	{
		/* close all existing tabs first */
		if (!document_close_all_project(project))
			return FALSE;
	}
	ui_set_statusbar(TRUE, _("Project \"%s\" closed."), project->name);

	sidebar_remove_project( project );
	
	/* remove project non filetype build menu items */
	//build_remove_menu_item(GEANY_BCS_PROJ, GEANY_GBG_NON_FT, -1);
	//build_remove_menu_item(GEANY_BCS_PROJ, GEANY_GBG_EXEC, -1);

	project->is_valid = FALSE;

	free_android_settings(project);
	init_android_settings(project);

	free_ios_settings(project);
	init_ios_settings(project);	

	free_html5_settings(project);
	init_html5_settings(project);

	g_free(project->name);
	g_free(project->description);
	g_free(project->file_name);
	g_free(project->base_path);

	// free file and group arrays
	for (i = 0; i < project->project_files->len; i++)
		g_free(project->project_files->pdata[i]);
	g_ptr_array_free(project->project_files, TRUE);

	for (i = 0; i < project->project_groups->len; i++)
		g_free(project->project_groups->pdata[i]);
	g_ptr_array_free(project->project_groups, TRUE);

	//g_free(project);
	memset(project, 0, sizeof(GeanyProject));
	app->project = project_find_first_valid();
	project_update_list();
	ui_project_buttons_update();

	foreach_slist(node, stash_groups)
		stash_group_free(node->data);

	g_slist_free(stash_groups);
	stash_groups = NULL;

	apply_editor_prefs(); /* ensure that global settings are restored */

	if (project_prefs.project_session)
	{
		/* after closing all tabs let's open the tabs found in the default config */
		if (open_default && cl_options.load_session)
		{
			//configuration_reload_default_session();
			//configuration_open_files();
			/* open a new file if no other file was opened */
			//document_new_file_if_non_open();
			//ui_focus_current_document();
		}
	}
	g_signal_emit_by_name(geany_object, "project-close");

	update_ui();

	return TRUE;
}

gint project_get_new_file_idx(GeanyProject *project)
{
	guint i;

	for (i = 0; i < project->project_files->len; i++)
	{
		if ( !project_files_index(project,i)->is_valid )
		{
			return (gint) i;
		}
	}
	return -1;
}

gint project_get_new_group_idx(GeanyProject *project)
{
	guint i;

	for (i = 0; i < project->project_groups->len; i++)
	{
		if ( !project_groups_index(project,i)->is_valid )
		{
			return (gint) i;
		}
	}
	return -1;
}

gboolean project_add_file(GeanyProject *project, gchar* filename, gboolean update_sidebar)
{
	if ( !project )
	{
		SHOW_ERR( _("Failed to add file to project, no current project selected. Click Project in the menu bar to create a new project or open an existing one.") );
		return FALSE;
	}

	gint new_idx = project_get_new_file_idx( project );
	if (new_idx == -1)	/* expand the array, no free places */
	{
		GeanyProjectFile *file = g_new0(GeanyProjectFile, 1);

		new_idx = project->project_files->len;
		g_ptr_array_add(project->project_files, file);
	}

	GeanyProjectFile *file = project->project_files->pdata[new_idx];
	file->is_valid = TRUE;
	file->file_name = g_strdup( filename );

	if ( update_sidebar )
	{
		if (!write_config(project,TRUE))
			SHOW_ERR(_("Project file could not be saved"));
		else
			ui_set_statusbar(TRUE, _("Project \"%s\" saved."), project->name);

		sidebar_openfiles_add_file( project, filename );
	}

	return TRUE;
}

void project_remove_file(GeanyProject *project, gchar* filename, gboolean update_sidebar)
{
	if ( !project )
	{
		SHOW_ERR( _("Failed to remove file from project, no current project selected") );
		return;
	}

	int i;
	for( i = 0; i < project->project_files->len; i++ )
	{
		if ( project_files_index(project,i)->is_valid )
		{
			if ( strcmp( project_files_index(project,i)->file_name, filename ) == 0 )
			{
				g_free( project_files_index(project,i)->file_name );
				project_files_index(project,i)->is_valid = FALSE;
			}
		}
	}
	
	if ( update_sidebar )
	{
		if (!write_config(project,TRUE))
			SHOW_ERR(_("Project file could not be saved"));
		else
			ui_set_statusbar(TRUE, _("Project \"%s\" saved."), project->name);

		sidebar_openfiles_remove_file( project, filename );
		GeanyDocument *doc = document_find_by_filename( filename );
		if ( doc ) 
			sidebar_openfiles_add( doc );
	}
}

/* Shows the file chooser dialog when base path button is clicked
 * FIXME: this should be connected in Glade but 3.8.1 has a bug
 * where it won't pass any objects as user data (#588824). */
G_MODULE_EXPORT void
on_project_properties_base_path_button_clicked(GtkWidget *button,
	GtkWidget *base_path_entry)
{
	GtkWidget *dialog;

	g_return_if_fail(base_path_entry != NULL);
	g_return_if_fail(GTK_IS_WIDGET(base_path_entry));

	dialog = gtk_file_chooser_dialog_new(_("Choose Project Base Path"),
		NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gtk_entry_set_text(GTK_ENTRY(base_path_entry),
			gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));
	}

	gtk_widget_destroy(dialog);
}


static void insert_build_page(PropertyDialogElements *e)
{
	GtkWidget *build_table, *label;
	GeanyDocument *doc = document_get_current();
	GeanyFiletype *ft = NULL;

	if (doc != NULL)
		ft = doc->file_type;

	//build_table = build_commands_table(doc, GEANY_BCS_PROJ, &(e->build_properties), ft);
	//gtk_container_set_border_width(GTK_CONTAINER(build_table), 6);
	//label = gtk_label_new(_("Build"));
	//e->build_page_num = gtk_notebook_append_page(GTK_NOTEBOOK(e->notebook),
	//	build_table, label);
}


static void create_properties_dialog(PropertyDialogElements *e)
{
	GtkWidget *base_path_button;
	static guint base_path_button_handler_id = 0;
	static guint radio_long_line_handler_id = 0;

	e->dialog = create_project_dialog();
	e->notebook = ui_lookup_widget(e->dialog, "project_notebook");
	e->file_name = ui_lookup_widget(e->dialog, "label_project_dialog_filename");
	e->name = ui_lookup_widget(e->dialog, "entry_project_dialog_name");
	e->description = ui_lookup_widget(e->dialog, "textview_project_dialog_description");
	e->base_path = ui_lookup_widget(e->dialog, "entry_project_dialog_base_path");
	e->patterns = ui_lookup_widget(e->dialog, "entry_project_dialog_file_patterns");

	gtk_entry_set_max_length(GTK_ENTRY(e->name), MAX_NAME_LEN);

	ui_entry_add_clear_icon(GTK_ENTRY(e->name));
	ui_entry_add_clear_icon(GTK_ENTRY(e->base_path));
	ui_entry_add_clear_icon(GTK_ENTRY(e->patterns));

	/* Workaround for bug in Glade 3.8.1, see comment above signal handler */
	if (base_path_button_handler_id == 0)
	{
		base_path_button = ui_lookup_widget(e->dialog, "button_project_dialog_base_path");
		base_path_button_handler_id =
			g_signal_connect(base_path_button, "clicked",
			G_CALLBACK(on_project_properties_base_path_button_clicked),
			e->base_path);
	}

	/* Same as above, should be in Glade but can't due to bug in 3.8.1 */
	if (radio_long_line_handler_id == 0)
	{
		radio_long_line_handler_id =
			g_signal_connect(ui_lookup_widget(e->dialog,
			"radio_long_line_custom_project"), "toggled",
			G_CALLBACK(on_radio_long_line_custom_toggled),
			ui_lookup_widget(e->dialog, "spin_long_line_project"));
	}
}


/* checks whether there is an already open project and asks the user if he wants to close it or
 * abort the current action. Returns FALSE when the current action(the caller) should be cancelled
 * and TRUE if we can go ahead */
gboolean project_ask_close(void)
{
	if (app->project != NULL)
	{
		if (dialogs_show_question_full(NULL, GTK_STOCK_CLOSE, GTK_STOCK_CANCEL,
			_("Do you want to close it before proceeding?"),
			_("The '%s' project is open."), app->project->name))
		{
			project_close(app->project,FALSE);
			return TRUE;
		}
		else
			return FALSE;
	}
	else
		return TRUE;
}

static gint project_get_new_idx(void)
{
	guint i;

	for (i = 0; i < projects_array->len; i++)
	{
		if ( !projects[i]->is_valid )
		{
			return (gint) i;
		}
	}
	return -1;
}

static GeanyProject *create_project(void)
{
	GeanyProject *project;
	gint new_idx;

	new_idx = project_get_new_idx();
	if (new_idx == -1)	/* expand the array, no free places */
	{
		project = g_new0(GeanyProject, 1);

		new_idx = projects_array->len;
		g_ptr_array_add(projects_array, project);
	}

	project = projects[new_idx];
	project->index = new_idx;
	project->project_files = g_ptr_array_new();
	project->project_groups = g_ptr_array_new();

	init_android_settings(project);
	init_ios_settings(project);
	init_html5_settings(project);

	app->project = project;
	return project;
}

gboolean project_import_from_file(const gchar *filename)
{
	GeanyProject *p;
	gchar *file_data;
	GError *err;
	
	g_return_val_if_fail(filename != NULL, FALSE);

	if ( !g_file_get_contents( filename, &file_data, NULL, &err ) )
	{
		ui_set_statusbar(TRUE, "%s", err->message);
		g_error_free(err);
		return FALSE;
	}

	p = create_project();

	//p->name = utils_get_setting_string(config, "project", "name", GEANY_STRING_UNTITLED);
	p->name = g_path_get_basename( filename );
	gchar* dot = strrchr( p->name, '.' );
	if ( dot ) *dot = 0;
	p->description = g_strdup("");
	p->file_name = utils_get_utf8_from_locale(filename);
	dot = strrchr(p->file_name, '.');
	if ( dot ) 
	{
		*(dot+1) = 'a';
		*(dot+2) = 'g';
		*(dot+3) = 'k';
		*(dot+4) = 0;
	}
	//p->base_path = utils_get_setting_string(config, "project", "base_path", "");
	p->base_path = g_strdup( p->file_name );
	gchar* slash = strrchr( p->base_path, '/' );
	gchar* slash2 = strrchr( p->base_path, '\\' );
	if ( slash || slash2 ) 
	{
		if ( slash > slash2 ) 
			*(slash+1) = 0;
		else 
			*(slash2+1) = 0;
	}

	ui_project_buttons_update();
	
	// import project files
	gchar* file_ptr = file_data;
	while( *file_ptr )
	{
		if ( strncmp( file_ptr, "<Unit filename=\"", strlen("<Unit filename=\"") ) == 0 )
		{
			gchar *start = file_ptr + strlen("<Unit filename=\"");
			gchar *end = start;
			while ( *end != '"' && *end ) end++;

			if ( !*end )
			{
				SHOW_ERR(_("Failed to import project source file, project file may be corrupt"));
			}
			else
			{
				gint len = (gint)(end-start);
				if ( len < 1000 )
				{
					gchar source_file[1024];
					strncpy( source_file, start, len );
					source_file[ len ] = 0;
					if ( !g_path_is_absolute(source_file) )
					{
						if ( strlen(p->base_path) + strlen(source_file) < 1000 )
						{
							gchar* tmp = g_strdup(source_file);
							strcpy(source_file, p->base_path);
							strcat(source_file, tmp);
							g_free(tmp);
							utils_tidy_path( source_file );
							//gchar* full_path = utils_get_real_path(source_file);
							//strcpy(source_file, full_path);
							//g_free(full_path);

							project_add_file( p, source_file, FALSE );
						}
					}
					else
						project_add_file( p, source_file, FALSE );
				}
			}

			file_ptr = end;
		}
		else
			file_ptr++;
	}

	p->is_valid = TRUE;

	// save new project file
	if ( !write_config(p,FALSE) )
		SHOW_ERR(_("Project file could not be written"));
	else
		ui_set_statusbar(TRUE, _("Project \"%s\" imported."), p->name);
	
	sidebar_openfiles_add_project( p );
	project_update_list();

	update_ui();

	ui_add_recent_project_file(p->file_name);
	return TRUE;
}

/* Verifies data for New & Properties dialogs.
 * Returns: FALSE if the user needs to change any data. */
static gboolean update_config(const PropertyDialogElements *e, gboolean new_project)
{
	const gchar *name, *file_name;
	gchar *base_path;
	gchar *locale_filename;
	gsize name_len;
	gint err_code = 0;
	GeanyProject *p;

	g_return_val_if_fail(e != NULL, TRUE);

	name = gtk_entry_get_text(GTK_ENTRY(e->name));
	name_len = strlen(name);
	if (name_len == 0)
	{
		SHOW_ERR(_("The specified project name is too short."));
		gtk_widget_grab_focus(e->name);
		return FALSE;
	}
	else if (name_len > MAX_NAME_LEN)
	{
		SHOW_ERR1(_("The specified project name is too long (max. %d characters)."), MAX_NAME_LEN);
		gtk_widget_grab_focus(e->name);
		return FALSE;
	}

	base_path = g_strdup(gtk_entry_get_text(GTK_ENTRY(e->base_path)));
	if (EMPTY(base_path))
	{
		SHOW_ERR(_("The project must have a base path"));
		gtk_widget_grab_focus(e->base_path);
		g_free(base_path);
		return FALSE;
	}
	else
	{	/* check whether the given directory actually exists */
		gchar *locale_path = utils_get_locale_from_utf8(base_path);

		if (! g_path_is_absolute(locale_path))
		{	
			SHOW_ERR(_("The project path must be an absolute path"));
			gtk_widget_grab_focus(e->base_path);
			g_free(base_path);
			return FALSE;
		}

		if (! g_file_test(locale_path, G_FILE_TEST_IS_DIR))
		{
			gboolean create_dir;

			err_code = utils_mkdir(locale_path, TRUE);

			if (err_code != 0)
			{
				SHOW_ERR1(_("Project base directory could not be created (%s)."), g_strerror(err_code));
				gtk_widget_grab_focus(e->base_path);
				utils_free_pointers(1, locale_path, NULL);
				g_free(base_path);
				return FALSE;
			}
		}
		g_free(locale_path);
	}

	if (new_project)
	{
		// make sure base path ends in a slash
		if ( base_path[ strlen(base_path)-1 ] != '/' && base_path[ strlen(base_path)-1 ] != '\\' )
			SETPTR(base_path, g_strconcat(base_path, G_DIR_SEPARATOR_S, NULL) );

		// generate project filename from project path and name
		file_name = g_strconcat(base_path, name, "." GEANY_PROJECT_EXT, NULL);
	}
	else
		file_name = gtk_label_get_text(GTK_LABEL(e->file_name));

	if (G_UNLIKELY(EMPTY(file_name)))
	{
		SHOW_ERR(_("You have specified an invalid project filename."));
		gtk_widget_grab_focus(e->file_name);
		g_free(base_path);
		return FALSE;
	}

	locale_filename = utils_get_locale_from_utf8(file_name);
		
	/* finally test whether the given project file can be written */
	if ((err_code = utils_is_file_writable(locale_filename)) != 0 ||
		(err_code = g_file_test(locale_filename, G_FILE_TEST_IS_DIR) ? EISDIR : 0) != 0)
	{
		SHOW_ERR1(_("Project file could not be written (%s)."), g_strerror(err_code));
		gtk_widget_grab_focus(e->file_name);
		g_free(locale_filename);
		g_free(base_path);
		return FALSE;
	}
	g_free(locale_filename);

	create_project();
	new_project = TRUE;
		
	p = app->project;
	p->is_valid = TRUE;

	SETPTR(p->name, g_strdup(name));
	SETPTR(p->file_name, g_strdup(file_name));
	/* use "." if base_path is empty */
	SETPTR(p->base_path, g_strdup(!EMPTY(base_path) ? base_path : "./"));

	ui_project_buttons_update();	
	update_ui();

	g_free(base_path);

	return TRUE;
}


#ifndef G_OS_WIN32
static void run_dialog(GtkWidget *dialog, GtkWidget *entry)
{
	/* set filename in the file chooser dialog */
	const gchar *utf8_filename = gtk_entry_get_text(GTK_ENTRY(entry));
	gchar *locale_filename = utils_get_locale_from_utf8(utf8_filename);

	if (g_path_is_absolute(locale_filename))
	{
		if (g_file_test(locale_filename, G_FILE_TEST_EXISTS))
		{
			/* if the current filename is a directory, we must use
			 * gtk_file_chooser_set_current_folder(which expects a locale filename) otherwise
			 * we end up in the parent directory */
			if (g_file_test(locale_filename, G_FILE_TEST_IS_DIR))
				gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), locale_filename);
			else
				gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), utf8_filename);
		}
		else /* if the file doesn't yet exist, use at least the current directory */
		{
			gchar *locale_dir = g_path_get_dirname(locale_filename);
			gchar *name = g_path_get_basename(utf8_filename);

			if (g_file_test(locale_dir, G_FILE_TEST_EXISTS))
				gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), locale_dir);
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), name);

			g_free(name);
			g_free(locale_dir);
		}
	}
	else if (gtk_file_chooser_get_action(GTK_FILE_CHOOSER(dialog)) != GTK_FILE_CHOOSER_ACTION_OPEN)
	{
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), utf8_filename);
	}
	g_free(locale_filename);

	/* run it */
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gchar *tmp_utf8_filename = utils_get_utf8_from_locale(filename);

		gtk_entry_set_text(GTK_ENTRY(entry), tmp_utf8_filename);

		g_free(tmp_utf8_filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}
#endif


static void on_file_save_button_clicked(GtkButton *button, PropertyDialogElements *e)
{
#ifdef G_OS_WIN32
	gchar *path = win32_show_project_open_dialog(e->dialog, _("Choose Project Filename"),
						gtk_entry_get_text(GTK_ENTRY(e->file_name)), TRUE, "AGK Project Files\t*.agk\t");
	if (path != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(e->file_name), path);
		g_free(path);
	}
#else
	GtkWidget *dialog;

	/* initialise the dialog */
	dialog = gtk_file_chooser_dialog_new(_("Choose Project Filename"), NULL,
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_widget_set_name(dialog, "GeanyDialogProject");
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

	run_dialog(dialog, e->file_name);
#endif
}


/* sets the project base path and the project file name according to the project name */
static void on_name_entry_changed(GtkEditable *editable, PropertyDialogElements *e)
{
	gchar *base_path;
	gchar *name;
	const gchar *project_dir = global_project_prefs.project_file_path;

	if (entries_modified)
		return;

	name = gtk_editable_get_chars(editable, 0, -1);
	if (!EMPTY(name))
	{
		base_path = g_strconcat(project_dir, G_DIR_SEPARATOR_S, name, G_DIR_SEPARATOR_S, NULL);
	}
	else
	{
		base_path = g_strconcat(project_dir, G_DIR_SEPARATOR_S, NULL);
	}
	g_free(name);

	gtk_entry_set_text(GTK_ENTRY(e->base_path), base_path);

	entries_modified = FALSE;

	g_free(base_path);
}


static void on_entries_changed(GtkEditable *editable, PropertyDialogElements *e)
{
	entries_modified = TRUE;
}


static void on_radio_long_line_custom_toggled(GtkToggleButton *radio, GtkWidget *spin_long_line)
{
	gtk_widget_set_sensitive(spin_long_line, gtk_toggle_button_get_active(radio));
}


gboolean project_load_file(const gchar *locale_file_name)
{
	g_return_val_if_fail(locale_file_name != NULL, FALSE);

	if (load_config(locale_file_name))
	{
		gchar *utf8_filename = utils_get_utf8_from_locale(locale_file_name);

		ui_set_statusbar(TRUE, _("Project \"%s\" opened."), app->project->name);

		ui_add_recent_project_file(utf8_filename);
		g_free(utf8_filename);



		return TRUE;
	}
	else
	{
		gchar *utf8_filename = utils_get_utf8_from_locale(locale_file_name);

		ui_set_statusbar(TRUE, _("Project file \"%s\" could not be loaded."), utf8_filename);
		g_free(utf8_filename);
	}
	return FALSE;
}


/* Reads the given filename and creates a new project with the data found in the file.
 * At this point there should not be an already opened project in Geany otherwise it will just
 * return.
 * The filename is expected in the locale encoding. */
static gboolean load_config(const gchar *filename) 
{
	GKeyFile *config;
	GeanyProject *p;
	GSList *node;

	g_return_val_if_fail(filename != NULL, FALSE);

	config = g_key_file_new();
	if (! g_key_file_load_from_file(config, filename, G_KEY_FILE_NONE, NULL))
	{
		g_key_file_free(config);
		return FALSE;
	}

	p = create_project();

	foreach_slist(node, stash_groups)
		stash_group_load_from_key_file(node->data, config);

	//p->name = utils_get_setting_string(config, "project", "name", GEANY_STRING_UNTITLED);
	p->name = g_path_get_basename( filename );
	gchar* dot = strrchr( p->name, '.' );
	if ( dot ) *dot = 0;
	p->description = utils_get_setting_string(config, "project", "description", "");
	p->file_name = utils_get_utf8_from_locale(filename);
	//p->base_path = utils_get_setting_string(config, "project", "base_path", "");
	p->base_path = g_strdup( p->file_name );
	gchar* slash = strrchr( p->base_path, '/' );
	gchar* slash2 = strrchr( p->base_path, '\\' );
	if ( slash || slash2 ) 
	{
		if ( slash > slash2 ) 
			*(slash+1) = 0;
		else 
			*(slash2+1) = 0;
	}

	ui_project_buttons_update();
	
	configuration_load_project_files(config, p);

	p->is_valid = TRUE;

	sidebar_openfiles_add_project( p );
	project_update_list();

	if (project_prefs.project_session)
	{
		/* read session files so they can be opened with configuration_open_files() */
		configuration_load_session_files(config, p);
	}

	load_android_settings( config, p );
	load_ios_settings( config, p );
	load_html5_settings( config, p );

	g_signal_emit_by_name(geany_object, "project-open", config);
	g_key_file_free(config);

	update_ui();
	return TRUE;
}

static void apply_editor_prefs(void)
{
	guint i;

	foreach_document(i)
		editor_apply_update_prefs(documents[i]->editor);
}


/* Write the project settings as well as the project session files into its configuration files.
 * emit_signal defines whether the project-save signal should be emitted. When write_config()
 * is called while closing a project, this is used to skip emitting the signal because
 * project-close will be emitted afterwards.
 * Returns: TRUE if project file was written successfully. */
static gboolean write_config(GeanyProject *project, gboolean emit_signal)
{
	GeanyProject *p;
	GKeyFile *config;
	gchar *filename;
	gchar *data;
	gboolean ret = FALSE;
	GSList *node;

	g_return_val_if_fail(app->project != NULL, FALSE);

	config = g_key_file_new();
	/* try to load an existing config to keep manually added comments */
	filename = utils_get_locale_from_utf8(project->file_name);
	g_key_file_load_from_file(config, filename, G_KEY_FILE_NONE, NULL);

	foreach_slist(node, stash_groups)
		stash_group_save_to_key_file(node->data, config);

	//g_key_file_set_string(config, "project", "name", p->name);
	//g_key_file_set_string(config, "project", "base_path", p->base_path);

	if (project->description)
		g_key_file_set_string(config, "project", "description", project->description);

	configuration_save_project_files(config,project);
	
	/* store the session files into the project too */
	if (project_prefs.project_session)
		configuration_save_session_files(config,project);

	save_android_settings( config, project );
	save_ios_settings( config, project );
	save_html5_settings( config, project );
	
	if (emit_signal)
	{
		g_signal_emit_by_name(geany_object, "project-save", config);
	}
	/* write the file */
	data = g_key_file_to_data(config, NULL, NULL);
	ret = (utils_write_file(filename, data) == 0);

	g_free(data);
	g_free(filename);
	g_key_file_free(config);

	return ret;
}


/* Constructs the project's base path which is used for "Make all" and "Execute".
 * The result is an absolute string in UTF-8 encoding which is either the same as
 * base path if it is absolute or it is built out of project file name's dir and base_path.
 * If there is no project or project's base_path is invalid, NULL will be returned.
 * The returned string should be freed when no longer needed. */
gchar *project_get_base_path(void)
{
	GeanyProject *project = app->project;

	if (project && !EMPTY(project->base_path))
	{
		if (g_path_is_absolute(project->base_path))
			return g_strdup(project->base_path);
		else
		{	/* build base_path out of project file name's dir and base_path */
			gchar *path;
			gchar *dir = g_path_get_dirname(project->file_name);

			if (utils_str_equal(project->base_path, "./"))
				return dir;

			path = g_build_filename(dir, project->base_path, NULL);
			g_free(dir);
			return path;
		}
	}
	return NULL;
}


/* This is to save project-related global settings, NOT project file settings. */
void project_save_prefs(GKeyFile *config)
{
	GeanyProject *project = app->project;

	if (cl_options.load_session)
	{
		const gchar *utf8_filename = (project == NULL) ? "" : project->file_name;

		g_key_file_set_string(config, "project", "session_file", utf8_filename);
	}
	g_key_file_set_string(config, "project", "project_file_path",
		FALLBACK(global_project_prefs.project_file_path, ""));
}


void project_load_prefs(GKeyFile *config)
{
	if (cl_options.load_session)
	{
		g_return_if_fail(project_prefs.session_file == NULL);
		project_prefs.session_file = utils_get_setting_string(config, "project",
			"session_file", "");
	}
	global_project_prefs.project_file_path = utils_get_setting_string(config, "project",
		"project_file_path", NULL);
	
	if (global_project_prefs.project_file_path == NULL)
	{
		//global_project_prefs.project_file_path = g_build_filename(g_get_home_dir(), "AGK Projects", NULL);
		global_project_prefs.project_file_path = g_build_filename(g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS), "AGK Projects", NULL);
	}

	/*
	if (global_project_prefs.project_file_path == NULL)
	{
		//global_project_prefs.project_file_path = g_build_filename(g_get_home_dir(), PROJECT_DIR, NULL);
		gchar *path;
#ifdef G_OS_WIN32
		path = win32_get_installation_dir();
#else
		path = g_strdup(GEANY_DATADIR);
#endif
		global_project_prefs.project_file_path = g_build_filename(path, "../../Projects", NULL);
		utils_tidy_path( global_project_prefs.project_file_path );
	}
	*/
}


/* Initialize project-related preferences in the Preferences dialog. */
void project_setup_prefs(void)
{
	GtkWidget *path_entry = ui_lookup_widget(ui_widgets.prefs_dialog, "project_file_path_entry");
	GtkWidget *path_btn = ui_lookup_widget(ui_widgets.prefs_dialog, "project_file_path_button");
	static gboolean callback_setup = FALSE;

	g_return_if_fail(global_project_prefs.project_file_path != NULL);

	gtk_entry_set_text(GTK_ENTRY(path_entry), global_project_prefs.project_file_path);
	if (! callback_setup)
	{	/* connect the callback only once */
		callback_setup = TRUE;
		ui_setup_open_button_callback(path_btn, NULL,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_ENTRY(path_entry));
	}
}


/* Update project-related preferences after using the Preferences dialog. */
void project_apply_prefs(void)
{
	GtkWidget *path_entry = ui_lookup_widget(ui_widgets.prefs_dialog, "project_file_path_entry");
	const gchar *str;

	str = gtk_entry_get_text(GTK_ENTRY(path_entry));
	SETPTR(global_project_prefs.project_file_path, g_strdup(str));
}


static void add_stash_group(StashGroup *group)
{
	stash_groups = g_slist_prepend(stash_groups, group);
}

const GeanyFilePrefs *project_get_file_prefs(void)
{
	return &file_prefs;
}

static gint combo_sort_func(GtkTreeModel *model, GtkTreeIter *iter_a,
								GtkTreeIter *iter_b, gpointer data)
{
	gchar *name_a, *name_b;
	gint cmp;
	
	gtk_tree_model_get(model, iter_a, 0, &name_a, -1);
	gtk_tree_model_get(model, iter_b, 0, &name_b, -1);
	cmp = strcmp(name_a, name_b);
	
	g_free(name_a);
	g_free(name_b);

	return cmp;
}

void project_init(void)
{
	GtkTreeIter iter;

	projects_array = g_ptr_array_new();

	project_choice = ui_lookup_widget(main_widgets.window, "combobox1");
	project_choice_container = ui_lookup_widget(main_widgets.window, "hbox4");

	gtk_widget_hide( project_choice_container );

	GtkListStore *list = gtk_list_store_new( 2, G_TYPE_STRING, G_TYPE_POINTER );
	
	/*
	// test items
	gtk_list_store_append( list, &iter );
	gtk_list_store_set( list, &iter, 0, "None", 1, 0, -1 );
	gtk_list_store_append( list, &iter );
	gtk_list_store_set( list, &iter, 0, "First", 1, 0, -1 );
	*/

	gtk_combo_box_set_model( GTK_COMBO_BOX(project_choice), GTK_TREE_MODEL(list) );
	g_object_unref( G_OBJECT(list) );

	GtkTreeSortable *sortable = GTK_TREE_SORTABLE(GTK_TREE_MODEL(list));
	gtk_tree_sortable_set_sort_func(sortable, 0, combo_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(sortable, 0, GTK_SORT_ASCENDING);
}

GtkTreeIter* get_combo_iter( GeanyProject *project )
{
	GtkTreeModel *model = gtk_combo_box_get_model( GTK_COMBO_BOX(project_choice) );
	static GtkTreeIter iter;
	GeanyProject *other_project;

	if ( !gtk_tree_model_get_iter_first( model, &iter ) )
		return NULL;

	do
	{
		gtk_tree_model_get( model, &iter, 1, &other_project, -1 );
		if ( project == other_project ) return &iter;
	} while( gtk_tree_model_iter_next( model, &iter ) );

	return NULL;
}

void project_combo_add( GeanyProject *project )
{
	GtkTreeIter iter;
	GtkListStore *list = GTK_LIST_STORE( gtk_combo_box_get_model( GTK_COMBO_BOX(project_choice) ) );

	if ( get_combo_iter( project ) ) 
		return;

	gtk_list_store_append( list, &iter );
	gtk_list_store_set( list, &iter, 0, project->name, 1, project, -1 );
}

void project_update_list(void)
{
	GtkListStore *list = GTK_LIST_STORE( gtk_combo_box_get_model( GTK_COMBO_BOX(project_choice) ) );
	gtk_list_store_clear( list );	

	if ( projects_array->len < 2 )
		gtk_widget_hide( project_choice_container );
	else
	{
		int i;
		int count = 0;
		for( i = 0; i < projects_array->len; i++ )
		{
			if ( projects[i]->is_valid )
			{
				project_combo_add( projects[i] );
				count++;
			}
		}

		if ( count < 2 )
			gtk_widget_hide( project_choice_container );
		else
		{
			GtkTreeIter *iter = get_combo_iter( app->project );
		
			if ( !iter )
				return;

			gtk_combo_box_set_active_iter( GTK_COMBO_BOX(project_choice), iter );

			gtk_widget_show( project_choice_container );
		}
	}	
}

void project_finalize(void)
{
	guint i;

	for (i = 0; i < projects_array->len; i++)
		g_free(projects[i]);
	g_ptr_array_free(projects_array, TRUE);
}

GeanyProject* project_find_by_filename(const gchar *filename)
{
	guint i;

	if ( ! filename)
		return NULL;	/* file doesn't exist on disk */

	for (i = 0; i < projects_array->len; i++)
	{
		GeanyProject *project = projects[i];

		if (! project->is_valid || ! project->file_name)
			continue;

		if (utils_filenamecmp(filename, project->file_name) == 0)
		{
			return project;
		}
	}
	return NULL;
}

GeanyProject* project_find_first_valid()
{
	guint i;

	for (i = 0; i < projects_array->len; i++)
	{
		GeanyProject *project = projects[i];

		if (! project->is_valid || ! project->file_name)
			continue;

		return project;
	}
	return NULL;
}

/*
 *      about.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2005-2012 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
 *      Copyright 2006-2012 Nick Treleaven <nick(dot)treleaven(at)btinternet(dot)com>
 *      Copyright 2006-2012 Frank Lanitz <frank@frank.uvena.de>
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
 * About dialog and credits.
 */

#include "about.h"
#include "geany.h"
#include "utils.h"
#include "ui_utils.h"
#include "support.h"
#include "geanywraplabel.h"
#include "main.h"
#include "templates.h"

#include "gb.c"


#define HEADER "<span size=\"larger\" weight=\"bold\">AppGameKit IDE %s</span>"
#define INFO "<span size=\"larger\" weight=\"bold\">%s</span>"
#define CODENAME _("<span>Built on Geany 1.24.1</span>")
#define BUILDDATE "<span size=\"smaller\">%s</span>"
#define COPYRIGHT _("App Game Kit (c) 2014\nThe Game Creators Ltd. All Rights Reserved.\n\nGeany Copyright (c)  2005-2014\nColomban Wendling\nNick Treleaven\nMatthew Brush\nEnrico Tröger\nFrank Lanitz\nAll rights reserved.")

static const gchar *contributors =
"Adam Ples, "
"Alexander Rodin, Alexey Antipov, Andrew Rowland, Anh Phạm, blackdog, Bo Lorentsen, Bob Doan, "
"Bronisław Białek, Can Koy, Catalin Marinas, "
"Chris Macksey, Christoph Berg, Colomban Wendling, Conrad Steenberg, Daniel Richard G., "
"Daniel Marjamaki, Dave Moore, "
"Dimitar Zhekov, Dirk Weber, Elias Pschernig, Eric Forgeot, "
"Erik de Castro Lopo, Eugene Arshinov, Felipe Pena, François Cami, "
"Giuseppe Torelli, Guillaume de Rorthais, Guillaume Hoffmann, Herbert Voss, Jason Oster, "
"Jean-François Wauthy, Jeff Pohlmeyer, Jesse Mayes, Jiří Techet, "
"John Gabriele, Jon Senior, Jon Strait, Josef Whiter, "
"Jörn Reder, Kelvin Gardiner, Kevin Ellwood, Kristoffer A. Tjernås, Lex Trotman, "
"Manuel Bua, Mário Silva, Marko Peric, Matthew Brush, Matti Mårds, "
"Moritz Barsnick, Nicolas Sierro, Ondrej Donek, Peter Strand, Philipp Gildein, "
"Pierre Joye, Rob van der Linde, "
"Robert McGinley, Roland Baudin, Ross McKay, S Jagannathan, Saleem Abdulrasool, "
"Sebastian Kraft, Shiv, Slava Semushin, Stefan Oltmanns, Tamim, Taylor Venable, "
"Thomas Huth, Thomas Martitz, Tomás Vírseda, "
"Tyler Mulligan, Walery Studennikov, Yura Siamashka";

static void header_eventbox_style_set(GtkWidget *widget);
static void header_label_style_set(GtkWidget *widget);
static void homepage_clicked(GtkButton *button, gpointer data);


#define ROW(text, row, col, x_align, y_padding, col_span) \
	label = gtk_label_new((text)); \
	gtk_table_attach(GTK_TABLE(table), label, (col), (col) + (col_span) + 1, (row), (row) + 1, \
			(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, (y_padding)); \
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE); \
	gtk_misc_set_alignment(GTK_MISC(label), (x_align), 0);


static GtkWidget *create_dialog(void)
{
	GtkWidget *dialog;
	GtkWidget *header_image;
	GtkWidget *header_label;
	GtkWidget *label_info;
	GtkWidget *codename_label;
	GtkWidget *builddate_label;
	GtkWidget *url_button;
	GtkWidget *cop_label;
	GtkWidget *label;
	GtkWidget *license_textview;
	GtkWidget *notebook;
	GtkWidget *box;
	GtkWidget *credits_scrollwin;
	GtkWidget *table;
	GtkWidget *license_scrollwin;
	GtkWidget *info_box;
	GtkWidget *header_hbox;
	GtkWidget *header_eventbox;
	GtkTextBuffer* tb;
	gchar *license_text = NULL;
	gchar buffer[512];
	gchar buffer2[128];
	guint i, row = 0;
	gchar *build_date;

	GtkWidget *eula_textview;
	GtkWidget *eula_scrollwin;
	gchar *eula_text = NULL;

	dialog = gtk_dialog_new();

	/* configure dialog */
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_widgets.window));
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(dialog), _("About AppGameKit"));
	gtk_window_set_icon_name(GTK_WINDOW(dialog), "agk");
	gtk_widget_set_name(dialog, "GeanyDialog");
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(dialog),600,480);
	g_signal_connect(dialog, "key-press-event", G_CALLBACK(gb_on_key_pressed), NULL);

	/* create header */
	header_eventbox = gtk_event_box_new();
	gtk_widget_show(header_eventbox);
	header_hbox = gtk_hbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(header_hbox), 4);
	gtk_widget_show(header_hbox);
	gtk_container_add(GTK_CONTAINER(header_eventbox), header_hbox);
	header_image = gtk_image_new_from_icon_name("agk", GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(header_hbox), header_image, FALSE, FALSE, 0);
	header_label = gtk_label_new(NULL);
	gtk_label_set_use_markup(GTK_LABEL(header_label), TRUE);
	/* print the subversion revision generated by ./configure if it is available */
	g_snprintf(buffer, sizeof(buffer), HEADER, AGK_VERSION);
	gtk_label_set_markup(GTK_LABEL(header_label), buffer);
	gtk_widget_show(header_label);
	gtk_box_pack_start(GTK_BOX(header_hbox), header_label, FALSE, FALSE, 0);
	header_eventbox_style_set(header_eventbox);
	header_label_style_set(header_label);
	g_signal_connect_after(header_eventbox, "style-set", G_CALLBACK(header_eventbox_style_set), NULL);
	g_signal_connect_after(header_label, "style-set", G_CALLBACK(header_label_style_set), NULL);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), header_eventbox, FALSE, FALSE, 0);

	/* create notebook */
	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_container_set_border_width(GTK_CONTAINER(notebook), 2);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), notebook, TRUE, TRUE, 0);

	/* create "Info" tab */
	info_box = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(info_box), 6);
	gtk_widget_show(info_box);

	label_info = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(label_info), GTK_JUSTIFY_CENTER);
	gtk_label_set_selectable(GTK_LABEL(label_info), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(label_info), TRUE);
	g_snprintf(buffer, sizeof(buffer), INFO, _("A fast and lightweight IDE"));
	gtk_label_set_markup(GTK_LABEL(label_info), buffer);
	gtk_misc_set_padding(GTK_MISC(label_info), 2, 11);
	gtk_widget_show(label_info);
	gtk_box_pack_start(GTK_BOX(info_box), label_info, FALSE, FALSE, 0);

	/* Codename label */
	codename_label = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(codename_label), GTK_JUSTIFY_CENTER);
	gtk_label_set_selectable(GTK_LABEL(codename_label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(codename_label), TRUE);
	gtk_label_set_markup(GTK_LABEL(codename_label), CODENAME);
	gtk_misc_set_padding(GTK_MISC(codename_label), 2, 8);
	gtk_widget_show(codename_label);
	gtk_box_pack_start(GTK_BOX(info_box), codename_label, FALSE, FALSE, 0);

	/* build date label */
	builddate_label = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(builddate_label), GTK_JUSTIFY_CENTER);
	gtk_label_set_selectable(GTK_LABEL(builddate_label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(builddate_label), TRUE);
	build_date = utils_parse_and_format_build_date(__DATE__);
	g_snprintf(buffer2, sizeof(buffer2), _("(built on %s)"), build_date);
	g_free(build_date);
	g_snprintf(buffer, sizeof(buffer), BUILDDATE, buffer2);
	gtk_label_set_markup(GTK_LABEL(builddate_label), buffer);
	gtk_misc_set_padding(GTK_MISC(builddate_label), 2, 2);
	gtk_widget_show(builddate_label);
	gtk_box_pack_start(GTK_BOX(info_box), builddate_label, FALSE, FALSE, 0);

	/* copyright label */
	cop_label = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(cop_label), GTK_JUSTIFY_CENTER);
	gtk_label_set_selectable(GTK_LABEL(cop_label), FALSE);
	gtk_label_set_use_markup(GTK_LABEL(cop_label), TRUE);
	gtk_label_set_markup(GTK_LABEL(cop_label), COPYRIGHT);
	gtk_misc_set_padding(GTK_MISC(cop_label), 2, 10);
	gtk_widget_show(cop_label);
	gtk_box_pack_start(GTK_BOX(info_box), cop_label, FALSE, FALSE, 0);
	/*gtk_container_add(GTK_CONTAINER(info_box), cop_label); */

	label = gtk_label_new(_("Info"));
	gtk_widget_show(label);
	gtk_widget_show_all(info_box);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), info_box, label);

	/* create "Credits" tab */
	credits_scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(credits_scrollwin), 6);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(credits_scrollwin),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	table = gtk_table_new(23, 3, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);

	row = 0;
	g_snprintf(buffer, sizeof(buffer),
		"<span size=\"larger\" weight=\"bold\">%s</span>", _("AppGameKit Credits"));
	label = gtk_label_new(buffer);
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, row, row + 1, GTK_FILL, 0, 0, 5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	row++;

	g_snprintf(buffer, sizeof(buffer), "Richard Vanner - %s", _("Producer"));
	ROW(buffer, row, 0, 0, 0, 1);
	row++;

	g_snprintf(buffer, sizeof(buffer), "Paul Johnston - %s", _("Developer"));
	ROW(buffer, row, 0, 0, 0, 1);
	row++;

	g_snprintf(buffer, sizeof(buffer), "Lee Bamber - %s", _("Developer"));
	ROW(buffer, row, 0, 0, 0, 1);
	row++;

	g_snprintf(buffer, sizeof(buffer), "Mike Johnson - %s", _("Examples, Demos, and Help"));
	ROW(buffer, row, 0, 0, 0, 1);
	row++;

	g_snprintf(buffer, sizeof(buffer), "Peter Jovanovic - %s", _("Graphics"));
	ROW(buffer, row, 0, 0, 0, 1);
	row++;

	g_snprintf(buffer, sizeof(buffer), "Dado Almeida - %s", _("Graphics"));
	ROW(buffer, row, 0, 0, 0, 1);
	row++;
	ROW("", row, 0, 0, 0, 0);
	row++;

	ROW("", row, 0, 0, 0, 0);
	row++;


	g_snprintf(buffer, sizeof(buffer),
		"<span size=\"larger\" weight=\"bold\">%s</span>", _("Geany Credits"));
	label = gtk_label_new(buffer);
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, row, row + 1, GTK_FILL, 0, 0, 5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	row++;

	g_snprintf(buffer, sizeof(buffer), "Colomban Wendling - %s", _("maintainer"));
	ROW(buffer, row, 0, 0, 0, 1);
	row++;
	
	g_snprintf(buffer, sizeof(buffer), "Nick Treleaven - %s", _("developer"));
	ROW(buffer, row, 0, 0, 0, 1);
	row++;
	
	g_snprintf(buffer, sizeof(buffer), "Enrico Tröger - %s", _("developer"));
	ROW(buffer, row, 0, 0, 0, 1);
	row++;
	
	g_snprintf(buffer, sizeof(buffer), "Matthew Brush - %s", _("developer"));
	ROW(buffer, row, 0, 0, 0, 1);
	row++;
	
	g_snprintf(buffer, sizeof(buffer), "Frank Lanitz - %s", _("translation maintainer"));
	ROW(buffer, row, 0, 0, 0, 1);
	row++;
	
	ROW("", row, 0, 0, 0, 0);
	row++;

	g_snprintf(buffer, sizeof(buffer),
		"<span size=\"larger\" weight=\"bold\">%s</span>", _("Geany Contributors"));
	label = gtk_label_new(buffer);
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, row, row + 1,
					(GtkAttachOptions) (GTK_FILL),
					(GtkAttachOptions) (0), 0, 5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	row++;

	label = geany_wrap_label_new(contributors);
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, row, row + 1,
					(GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
					(GtkAttachOptions) (0), 0, 5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	row++;

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(credits_scrollwin), table);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(gtk_widget_get_parent(table)), GTK_SHADOW_NONE);
	gtk_widget_show_all(table);
	label = gtk_label_new(_("Credits"));
	gtk_widget_show(label);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), credits_scrollwin, label);

	/* create "License" tab */
	license_scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(license_scrollwin), 6);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(license_scrollwin),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(license_scrollwin), GTK_SHADOW_IN);
	license_textview = gtk_text_view_new();
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(license_textview), 2);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(license_textview), 2);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(license_textview), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(license_textview), FALSE);
	gtk_widget_show(license_textview);

	gtk_container_add(GTK_CONTAINER(license_scrollwin), license_textview);
	
	GtkWidget *pLicenseLabel = gtk_label_new(NULL);
	//gtk_label_set_justify(GTK_LABEL(pLicenseLabel), GTK_JUSTIFY_CENTER);
	gtk_misc_set_alignment(GTK_MISC(pLicenseLabel),0,0.5);
	gtk_label_set_selectable(GTK_LABEL(pLicenseLabel), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(pLicenseLabel), TRUE);
	gtk_label_set_markup(GTK_LABEL(pLicenseLabel), _("The following license applies to the IDE only\nThe source code can be found at https://github.com/TheGameCreators/AGKIDE"));
	gtk_misc_set_padding(GTK_MISC(pLicenseLabel), 0, 0);
	gtk_widget_show(pLicenseLabel);

	GtkWidget *pLicenseBox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pLicenseBox), pLicenseLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pLicenseBox), license_scrollwin, TRUE, TRUE, 0);

	//gtk_box_pack_start(GTK_BOX(license_scrollwin), pLicenseLabel, FALSE, FALSE, 0);
	//gtk_container_add(GTK_CONTAINER(license_scrollwin), pLicenseLabel);

	g_snprintf(buffer, sizeof(buffer), "%s" G_DIR_SEPARATOR_S "GPL-2", app->datadir);

	g_file_get_contents(buffer, &license_text, NULL, NULL);
	if (license_text == NULL)
	{
		license_text = g_strdup(
			_("License text could not be found, please visit http://www.gnu.org/licenses/gpl-2.0.txt to view it online."));
	}
	tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(license_textview));
	gtk_text_buffer_set_text(tb, license_text, -1);

	g_free(license_text);

	label = gtk_label_new(_("License"));
	gtk_widget_show(label);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pLicenseBox, label);



	// create EULA tab
	eula_scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(eula_scrollwin), 6);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(eula_scrollwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(eula_scrollwin), GTK_SHADOW_IN);
	eula_textview = gtk_text_view_new();
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(eula_textview), 2);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(eula_textview), 2);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(eula_textview), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(eula_textview), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(eula_textview),GTK_WRAP_WORD);
	gtk_widget_show(eula_textview);

	gtk_container_add(GTK_CONTAINER(eula_scrollwin), eula_textview);

	g_snprintf(buffer, sizeof(buffer), "%s" G_DIR_SEPARATOR_S "AGK2EULA.txt", app->datadir);

	g_file_get_contents(buffer, &eula_text, NULL, NULL);
	if (eula_text == NULL)
	{
		eula_text = g_strdup(
			_("EULA text could not be found, please visit http://www.appgamekit.com for more information."));
	}
	tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(eula_textview));
	gtk_text_buffer_set_text(tb, eula_text, -1);

	g_free(eula_text);

	label = gtk_label_new(_("EULA"));
	gtk_widget_show(label);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), eula_scrollwin, label);


	gtk_widget_show_all(dialog);
	return dialog;
}


void about_dialog_show(void)
{
	GtkWidget *dialog;

	dialog = create_dialog();

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


static void header_eventbox_style_set(GtkWidget *widget)
{
	static gint recursive = 0;
	GtkStyle *style;

	if (recursive > 0)
		return;

	++recursive;
	style = gtk_widget_get_style(widget);
	//gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, &style->bg[GTK_STATE_SELECTED]);
	--recursive;
}


static void header_label_style_set(GtkWidget *widget)
{
	static gint recursive = 0;
	GtkStyle *style;

	if (recursive > 0)
		return;

	++recursive;
	style = gtk_widget_get_style(widget);
	//gtk_widget_modify_fg(widget, GTK_STATE_NORMAL, &style->fg[GTK_STATE_SELECTED]);
	--recursive;
}


static void homepage_clicked(GtkButton *button, gpointer data)
{
	utils_open_browser(data);
}

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "stparse.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module:	sfedit.c                                                */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	October 17th, 2002                                      */
/*									*/
/* Description:	Structure File editor using the GTK widget set.  Used	*/
/*		to, for instance, build the centrallix.conf server	*/
/*		configuration file.					*/
/************************************************************************/



/*** icon table.  <grunt> move this to a file or something dynamic!! ***/
#include "x-unknown.unknown.xpm"
#include "system.smtpguard-config.xpm"
#include "smtpguard-config.ipcontrol.xpm"
#include "smtpguard-config.ip.xpm"
#include "smtpguard-config.domaincontrol.xpm"
#include "smtpguard-config.domain.xpm"
static struct
    {
    char**	XPM;
    char*	Type;
    GdkPixmap*	Pixmap;
    GdkBitmap*	Mask;
    }
  icon_table[] =
    {
	{ x_unknown_unknown_xpm , "x-unknown/unknown", NULL, NULL },
	{ system_smtpguard_config_xpm, "system/smtpguard-config", NULL, NULL },
	{ smtpguard_config_ipcontrol_xpm, "smtpguard-config/ipcontrol", NULL, NULL },
	{ smtpguard_config_ip_xpm, "smtpguard-config/ip", NULL, NULL },
	{ smtpguard_config_domaincontrol_xpm, "smtpguard-config/domaincontrol", NULL, NULL },
	{ smtpguard_config_domain_xpm, "smtpguard-config/domain", NULL, NULL },
    };

#define SFE_DEFAULT_ICON	0


/*** application globals ***/
struct
    {
    GtkWidget	*OpenDialog;
    GtkWidget	*Window;
    GtkWidget	*TreeView;
    GtkWidget	*NameEdit;
    GtkWidget	*TypeCombo;
    GtkWidget	*AnnotText;
    GtkWidget	*AttrsCList;
    GtkWidget	*AttrEditWindow;
    pStructInf	Data;
    char	Filename[256];
    int		ArgC;
    char**	ArgV;
    int		Modified:1;
    int		HideColTitles:1;
    }
    SFE_Globals;


/*** sfeSetTitle - set the window title, given a particular filename
 ***/
int
sfeSetTitle(GtkWindow *window, char* filename)
    {
    gchar* newtitle;
	
	/** Set the title **/
	newtitle = (gchar*)g_malloc(strlen(filename) + strlen("Centrallix Structure File Editor - ") + 1);
	sprintf(newtitle, "Centrallix Structure File Editor - %s", filename);
	gtk_window_set_title(window, newtitle);
	g_free(newtitle);

    return 0;
    }


/*** sfeBuildTreeItemWithImage - create a new treeview item which contains
 *** the appropriate image for the given node type as well as a label with
 *** the node's name.
 ***/
GtkWidget*
sfeBuildTreeItemWithImage(pStructInf data_item)
    {
    GtkWidget *hbox, *label, *pixmap, *treeitem, *hiddenlabel;
    int i, n_icons, found;
    GtkStyle *style;
    gchar hiddenlabel_text[32];

	/** Figure out which pixmap file to use. **/
	found = SFE_DEFAULT_ICON;
	n_icons = sizeof(icon_table) / sizeof(icon_table[0]);
	for (i=0;i<n_icons;i++) 
	    if (!strcasecmp(data_item->UsrType, icon_table[i].Type))
		found = i;

	/** build the gdk pixmap structure if needed **/
	if (!icon_table[found].Pixmap)
	    {
	    style = gtk_widget_get_style(SFE_Globals.TreeView);
	    icon_table[found].Pixmap = gdk_pixmap_create_from_xpm_d(SFE_Globals.Window->window, 
		    &(icon_table[found].Mask), &style->bg[GTK_STATE_NORMAL], 
		    (gchar**) icon_table[found].XPM);
	    }

	/** Create the hbox, label, and pixmap **/
	hbox = gtk_hbox_new(FALSE,1);
	pixmap = gtk_pixmap_new(icon_table[found].Pixmap, icon_table[found].Mask);
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 0);
	gtk_widget_show(pixmap);
	label = gtk_label_new(data_item->Name);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	gtk_widget_show(hbox);

	/** We need a hidden label to reference the structure **/
	snprintf(hiddenlabel_text, 32, "%16.16lx", (unsigned long)(data_item));
	hiddenlabel = gtk_label_new(hiddenlabel_text);
	gtk_box_pack_start(GTK_BOX(hbox), hiddenlabel, FALSE, FALSE, 0);
	
	/** Create the tree item and plop the hbox in it **/
	treeitem = gtk_tree_item_new();
	gtk_container_add(GTK_CONTAINER(treeitem), hbox);

    return treeitem;
    }


/*** sfeRebuildUI_r - recursively build the new treeview display of the
 *** parsed structure file representation.
 ***/
int
sfeRebuildUI_r(pStructInf data, GtkTreeItem *subitem)
    {
    int i;
    pStructInf subinf;
    GtkWidget *subtreeitem, *subsubtree;
    GtkWidget *subtree = NULL;

	/** Loop through the subinfs, and add items/subtrees for each. **/
	for(i=0;i<data->nSubInf;i++) if (stStructType(data->SubInf[i]) == ST_T_SUBGROUP)
	    {
	    subinf = data->SubInf[i];

	    /** Only add the subtree once, and only if we need a subtree.  This
	     ** is why we do this in here instead of outside the loop.
	     **/
	    if (!subtree)
		{
		subtree = gtk_tree_new();
		gtk_tree_item_set_subtree(GTK_TREE_ITEM(subitem), subtree);
		gtk_tree_set_view_mode(GTK_TREE(subtree), GTK_TREE_VIEW_ITEM);
		}

	    /** Add the item to the subtree **/
	    subtreeitem = sfeBuildTreeItemWithImage(subinf);
	    gtk_tree_append(GTK_TREE(subtree), subtreeitem);
	    gtk_widget_show(subtreeitem);
	    sfeRebuildUI_r(subinf, GTK_TREE_ITEM(subtreeitem));
	    }
    
    return 0;
    }


/*** sfeRebuildUI - take the structure file data and completely rebuild the
 *** user interface representation, including the treeview navigation.
 ***/
int
sfeRebuildUI(pStructInf new_data, GtkTree *treeview)
    {
    GtkWidget *treeitem, *subtree;
	
	/** Clear any existing treeview content. **/
	gtk_tree_clear_items(treeview, 0, 9999);

	/** Add a root item **/
	treeitem = sfeBuildTreeItemWithImage(new_data);
	gtk_tree_prepend(treeview, treeitem);
	gtk_widget_show(treeitem);

	/** Build the tree, recursively. **/
	sfeRebuildUI_r(new_data, GTK_TREE_ITEM(treeitem));

	/** Expand the root node **/
	gtk_tree_item_expand(GTK_TREE_ITEM(treeitem));

    return 0;
    }


/*** sfeLoadFile - loads a new file into the program.
 ***/
int
sfeLoadFile(char* filename)
    {
    pFile fd;
    pStructInf new_filedata;

	/** open the file. **/
	if (strlen(filename) > 255) return -1;
	fd = fdOpen(filename, O_RDONLY, 0600);
	if (!fd) return -1;

	/** Try to parse it **/
	new_filedata = stParseMsg(fd, 0);
	fdClose(fd,0);
	if (!new_filedata) return -1;
	if (SFE_Globals.Data) stFreeInf(SFE_Globals.Data);
	SFE_Globals.Data = new_filedata;
	SFE_Globals.Modified = 0;

	/** Set the filename and title **/
	strcpy(SFE_Globals.Filename, filename);
	sfeSetTitle(GTK_WINDOW(SFE_Globals.Window), SFE_Globals.Filename);

	/** Rebuild the ui **/
	sfeRebuildUI(SFE_Globals.Data, GTK_TREE(SFE_Globals.TreeView));

    return 0;
    }


void
sfe_ui_FileOpen(GtkWidget *w)
    {

	gtk_widget_show(SFE_Globals.OpenDialog);

    return;
    }


void
sfe_ui_FileNew(GtkWidget *w)
    {

	strcpy(SFE_Globals.Filename, "untitled.struct");
	SFE_Globals.Data = stCreateStruct("new","x-unknown/x-unknown");
	SFE_Globals.Modified = 0;
	sfeSetTitle(GTK_WINDOW(SFE_Globals.Window), SFE_Globals.Filename);

	/** Rebuild the ui **/
	sfeRebuildUI(SFE_Globals.Data, GTK_TREE(SFE_Globals.TreeView));

    return;
    }


void
sfe_ui_FileExit(GtkWidget *w)
    {
    gtk_main_quit();
    return;
    }

void
sfe_ui_FileOpenCancel(GtkWidget *w, GtkWidget *dialogbox)
    {
    gtk_widget_hide(dialogbox);
    return;
    }

void
sfe_ui_FileOpenOk(GtkWidget *w, GtkWidget *dialogbox)
    {
    gchar* filename;
    
	filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(dialogbox));
	if (access(filename, R_OK) < 0 || sfeLoadFile(filename) < 0)
	    {
	    printf("\07");
	    fflush(stdout);
	    }
	else
	    {
	    gtk_widget_hide(dialogbox);
	    }

    return;
    }

void
sfe_ui_TitleToggle(GtkWidget *w, GtkWidget *menuitem)
    {
    SFE_Globals.HideColTitles = !SFE_Globals.HideColTitles;
    if (SFE_Globals.HideColTitles)
	{
	gtk_clist_column_titles_hide(GTK_CLIST(SFE_Globals.AttrsCList));
	}
    else
	{
	gtk_clist_column_titles_show(GTK_CLIST(SFE_Globals.AttrsCList));
	}
    return;
    }


/*** menu definitions ***/
static GtkItemFactoryEntry menu_items[] =
    {
	{ "/_File",		NULL,		NULL,			0, "<Branch>" },
	{ "/File/_New",		"<control>N",	sfe_ui_FileNew,		0, NULL },
	{ "/File/_Open File",	"<control>O",	sfe_ui_FileOpen,	0, NULL },
	{ "/File/_Save",	"<control>S",	NULL,			0, NULL },
	{ "/File/Save _As File",NULL,		NULL,			0, NULL },
	{ "/File/sep1",		NULL,		NULL,			0, "<Separator>" },
	{ "/File/Open Via RCP",	NULL,		NULL,			0, NULL },
	{ "/File/Save Via RCP",	NULL,		NULL,			0, NULL },
	{ "/File/sep2",		NULL,		NULL,			0, "<Separator>" },
	{ "/File/E_xit",	"<control>Q",	sfe_ui_FileExit,	0, NULL },
	{ "/_Edit",		NULL,		NULL,			0, "<Branch>" },
	{ "/Edit/_Delete",	NULL,		NULL,			0, NULL },
	{ "/Edit/Cu_t",		"<control>X",	NULL,			0, NULL },
	{ "/Edit/_Copy",	"<control>C",	NULL,			0, NULL },
	{ "/Edit/_Paste",	"<control>V",	NULL,			0, NULL },
	{ "/_View",		NULL,		NULL,			0, "<Branch>" },
	{ "/View/Show _Source",	"<control>U",	NULL,			0, NULL },
	{ "/View/sep3",		NULL,		NULL,			0, "<Separator>" },
	{ "/View/Hide Column _Titles", NULL,	sfe_ui_TitleToggle,	0, "<CheckItem>" },
	{ "/_Help",		NULL,		NULL,			0, "<LastBranch>" },
	{ "/Help/_About sfedit", NULL,		NULL,			0, NULL },
    };

static gchar* props_clist_titles[] =
    {
    "Attribute",
    "Value",
    };


/* Note that this is never called */
static void 
cb_unselect_child( GtkWidget *root_tree, GtkWidget *child, GtkWidget *subtree )
    {
    return;
    }


/* Note that this is called every time the user clicks on an item,
   whether it is already selected or not. */
static void 
cb_select_child (GtkWidget *root_tree, GtkWidget *child, GtkWidget *subtree)
    {
    return;
    }


static void
sfe_ui_CListAttrSelected(GtkWidget* clist, gint row, gint col, GdkEventButton *event, void* d)
    {
	
	if (event->type == GDK_2BUTTON_PRESS)
	    {
	    gtk_widget_show(SFE_Globals.AttrEditWindow);
	    gtk_window_set_title(GTK_WINDOW(SFE_Globals.AttrEditWindow), "Edit attribute...");
	    }

    return;
    }


static void 
sfe_ui_TreeSelectionChanged( GtkWidget *tree )
    {
    GList *itemlist;
    gchar *name;
    GtkBox *hbox;
    GtkLabel *label;
    GtkWidget *item;
    pStructInf infptr, subinf;
    gchar* annotation;
    int i;
    gchar* rowtext[2];
    gchar intbuf[32];
    char* strval;
    int intval;

	/** Get the hidden label which contains the pointer to the inf **/
	if (!tree) return;
	itemlist = GTK_TREE_SELECTION(tree);
	if (!itemlist) return;
	item = GTK_WIDGET (itemlist->data);
	if (!item) return;
	hbox = GTK_BOX (GTK_BIN (item)->child);
	label = GTK_LABEL (((GtkBoxChild*)(hbox->children->next->next->data))->widget);
	gtk_label_get (label, &name);
	infptr = (pStructInf)(strtol(name, NULL, 16));
	if (!infptr) return;

	/** Show the info on the right side of the window **/
	gtk_entry_set_text(GTK_ENTRY(SFE_Globals.NameEdit), infptr->Name);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(SFE_Globals.TypeCombo)->entry), infptr->UsrType);
	gtk_text_set_point(GTK_TEXT(SFE_Globals.AnnotText), 0);
	gtk_text_forward_delete(GTK_TEXT(SFE_Globals.AnnotText), gtk_text_get_length(GTK_TEXT(SFE_Globals.AnnotText)));
	annotation="";
	stAttrValue(stLookup(infptr,"annotation"), NULL, &annotation, 0);
	gtk_text_insert(GTK_TEXT(SFE_Globals.AnnotText), NULL, NULL, NULL, annotation, strlen(annotation));

	/** Build the attributes listing **/
	gtk_clist_clear(GTK_CLIST(SFE_Globals.AttrsCList));
	for(i=0;i<infptr->nSubInf;i++)
	    {
	    subinf = infptr->SubInf[i];
	    if (stStructType(subinf) == ST_T_ATTRIB)
		{
		strval = NULL;
		stAttrValue(subinf, &intval, &strval, 0);
		rowtext[0] = subinf->Name;
		if (strval)
		    {
		    rowtext[1] = strval;
		    }
		else
		    {
		    snprintf(intbuf, 32, "%d", intval);
		    rowtext[1] = intbuf;
		    }
		gtk_clist_append(GTK_CLIST(SFE_Globals.AttrsCList), rowtext);
		}
	    }

    return;
    }


void
sfe_ui_AttrDialogCancel(GtkWidget* dialog)
    {
    gtk_widget_hide(dialog);
    return;
    }


GtkWidget *
sfe_ui_CreateTabPage(GtkWidget* tabcontrol, char* labeltext)
    {
    GtkWidget* frame;
    GtkWidget* label;

	label = gtk_label_new(labeltext);
	gtk_widget_show(label);
	frame = gtk_frame_new(labeltext);
	gtk_widget_show(frame);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
	gtk_widget_set_usize(frame, 350, 220);
	gtk_notebook_append_page(GTK_NOTEBOOK(tabcontrol), frame, label);

    return frame;
    }


GtkWidget *
sfe_ui_CreateAttrDialog()
    {
    GtkWidget *dialog;
    GtkWidget *ok_button, *cancel_button;
    GtkWidget *types_tabcontrol;
    GtkWidget *tabcontrol;

	dialog = gtk_dialog_new();
	gtk_widget_set_usize(dialog, 400, 300);
	gtk_signal_connect(GTK_OBJECT(dialog), "delete_event", 
		GTK_SIGNAL_FUNC(sfe_ui_AttrDialogCancel), dialog);
	ok_button = gtk_button_new_with_label("OK");
	cancel_button = gtk_button_new_with_label("Cancel");
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), ok_button);
	gtk_widget_show(ok_button);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), cancel_button);
	gtk_widget_show(cancel_button);

	tabcontrol = gtk_notebook_new();
	gtk_widget_set_usize(tabcontrol, 370,240);
	sfe_ui_CreateTabPage(tabcontrol, "Integer");
	sfe_ui_CreateTabPage(tabcontrol, "String");
	sfe_ui_CreateTabPage(tabcontrol, "String List");
	gtk_widget_show(tabcontrol);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), tabcontrol, FALSE, FALSE, 0);

    return dialog;
    }


void
start(void* v)
    {
    GtkWidget *scrolled_win, *tree, *hpanes, *vbox, *menu, *list;
    /*static gchar *itemnames[] = {"One", "Two", "Three", "Four", "Five"};*/
    GtkItemFactory *item_factory;
    GtkAccelGroup *accel_group;
    gint nmenu_items;
    gint i;
    GtkWidget *table;
    GtkWidget *name_label, *info_vbox;
    GtkWidget *type_label;
    GtkWidget *annot_label, *annot_hbox, *annot_vscroll;
    GtkWidget *sep1;
    GtkWidget *props_hbox, *props_scrollwin;
    GtkWidget *menu_item;

	gtk_init (&SFE_Globals.ArgC, &SFE_Globals.ArgV);
	SFE_Globals.Data = stCreateStruct("new","x-unknown/x-unknown");
	SFE_Globals.Modified = 0;
	SFE_Globals.HideColTitles = 0;

	/* a generic toplevel window */
	SFE_Globals.Window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(SFE_Globals.Window), "Centrallix Structure File Editor");
	gtk_signal_connect (GTK_OBJECT(SFE_Globals.Window), "delete_event", GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
	gtk_widget_set_usize(SFE_Globals.Window, 600, 350);

	/** Build the Open File... dialog box **/
	SFE_Globals.OpenDialog = gtk_file_selection_new("Open File...");
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(SFE_Globals.OpenDialog)->cancel_button),
		"clicked", (GtkSignalFunc)sfe_ui_FileOpenCancel, GTK_OBJECT(SFE_Globals.OpenDialog));
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(SFE_Globals.OpenDialog)->ok_button),
		"clicked", (GtkSignalFunc)sfe_ui_FileOpenOk, GTK_OBJECT(SFE_Globals.OpenDialog));

	/* vertical box organizing the menu vs. rest of app */
	vbox = gtk_vbox_new(FALSE,1);
	gtk_container_add(GTK_CONTAINER(SFE_Globals.Window), vbox);
	gtk_widget_show(vbox);

	/* menu */
	nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);
	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);
	gtk_window_add_accel_group(GTK_WINDOW(SFE_Globals.Window), accel_group);
	menu = gtk_item_factory_get_widget(item_factory, "<main>");
	gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, TRUE, 0);
	/*menu_item = gtk_item_factory_get_widget(GTK_ITEM_FACTORY(item_factory),"/View/Column Titles");
	gtk_menu_item_activate(GTK_MENU_ITEM(menu_item));*/
	gtk_widget_show(menu);

	/* horizontal layout box organizing the treeview and the data view pane */
	hpanes = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(vbox), hpanes);
	gtk_container_set_border_width (GTK_CONTAINER(hpanes), 5);
	gtk_paned_set_gutter_size(GTK_PANED(hpanes), 16);
	gtk_widget_show(hpanes);

	/* A generic scrolled window - for the treeview. */
	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC);
	gtk_widget_set_usize (scrolled_win, 150, 200);
	gtk_container_add (GTK_CONTAINER(hpanes), scrolled_win);
	gtk_widget_show (scrolled_win);
  
	/* Create the root tree and add it to the scrolled window */
	tree = gtk_tree_new();
	gtk_signal_connect (GTK_OBJECT(tree), "select_child",
		GTK_SIGNAL_FUNC(cb_select_child), tree);
	gtk_signal_connect (GTK_OBJECT(tree), "unselect_child",
		GTK_SIGNAL_FUNC(cb_unselect_child), tree);
	gtk_signal_connect (GTK_OBJECT(tree), "selection_changed",
		GTK_SIGNAL_FUNC(sfe_ui_TreeSelectionChanged), tree);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), tree);
	gtk_tree_set_selection_mode (GTK_TREE(tree), GTK_SELECTION_SINGLE);
	gtk_tree_set_view_mode(GTK_TREE(tree), GTK_TREE_VIEW_ITEM);
	gtk_widget_show (tree);
	SFE_Globals.TreeView = tree;

	/** build the item name section **/
	info_vbox = gtk_vbox_new(FALSE,1);
	gtk_container_add(GTK_CONTAINER(hpanes), info_vbox);
	gtk_widget_show(info_vbox);
	table = gtk_table_new(3,2,FALSE);
	gtk_box_pack_start(GTK_BOX(info_vbox), table, FALSE, TRUE, 0);
	gtk_widget_show(table);
	name_label = gtk_label_new("Name:");
	gtk_label_set_justify(GTK_LABEL(name_label), GTK_JUSTIFY_LEFT);
	gtk_table_attach(GTK_TABLE(table), name_label, 0, 1, 0, 1, 0, 0, 0, 0);
	gtk_widget_show(name_label);
	SFE_Globals.NameEdit = gtk_entry_new_with_max_length(63);
	gtk_table_attach(GTK_TABLE(table), SFE_Globals.NameEdit, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0, 0);
	gtk_widget_show(SFE_Globals.NameEdit);

	/** Item type **/
	type_label = gtk_label_new("Type:");
	gtk_label_set_justify(GTK_LABEL(type_label), GTK_JUSTIFY_LEFT);
	gtk_table_attach(GTK_TABLE(table), type_label, 0, 1, 1, 2, 0, 0, 0, 0);
	gtk_widget_show(type_label);
	SFE_Globals.TypeCombo = gtk_combo_new();
	gtk_table_attach(GTK_TABLE(table), SFE_Globals.TypeCombo, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0, 8);
	gtk_widget_show(SFE_Globals.TypeCombo);

	/** Annotation/description/comments section **/
	annot_label = gtk_label_new("Desc:");
	gtk_label_set_justify(GTK_LABEL(annot_label), GTK_JUSTIFY_LEFT);
	gtk_table_attach(GTK_TABLE(table), annot_label, 0, 1, 2, 3, 0, 0, 0, 0);
	gtk_widget_show(annot_label);
	annot_hbox = gtk_hbox_new(FALSE,1);
	gtk_table_attach(GTK_TABLE(table), annot_hbox, 1, 2, 2, 3, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0, 0);
	gtk_widget_show(annot_hbox);
	SFE_Globals.AnnotText = gtk_text_new(NULL,NULL);
	gtk_text_set_editable(GTK_TEXT(SFE_Globals.AnnotText), TRUE);
	gtk_widget_set_usize(SFE_Globals.AnnotText, 100, 48);
	gtk_container_add(GTK_CONTAINER(annot_hbox), SFE_Globals.AnnotText);
	gtk_widget_show(SFE_Globals.AnnotText);
	annot_vscroll = gtk_vscrollbar_new(GTK_TEXT(SFE_Globals.AnnotText)->vadj);
	gtk_box_pack_end(GTK_BOX(annot_hbox), annot_vscroll, FALSE, TRUE, 0);
	gtk_widget_show(annot_vscroll);
	sep1 = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(info_vbox), sep1, FALSE, TRUE, 8);
	gtk_widget_show(sep1);

	/** Add a columnar list box for the attributes **/
	props_scrollwin = gtk_scrolled_window_new(NULL,NULL);
	gtk_container_add(GTK_CONTAINER(info_vbox), props_scrollwin);
	gtk_widget_show(props_scrollwin);
	SFE_Globals.AttrsCList = gtk_clist_new_with_titles(2, props_clist_titles);
	gtk_clist_set_selection_mode(GTK_CLIST(SFE_Globals.AttrsCList), GTK_SELECTION_SINGLE);
	gtk_container_add(GTK_CONTAINER(props_scrollwin), SFE_Globals.AttrsCList);
	gtk_widget_show(SFE_Globals.AttrsCList);
	gtk_signal_connect (GTK_OBJECT(SFE_Globals.AttrsCList), "select_row",
		GTK_SIGNAL_FUNC(sfe_ui_CListAttrSelected), SFE_Globals.AttrsCList);

#if 0
	/** Put some cruft in the treeview **/
	for (i = 0; i < 5; i++)
	    {
	    GtkWidget *subtree, *item;
	    gint j;

	    /* Create a tree item */
	    item = gtk_tree_item_new_with_label (itemnames[i]);

	    /* Add it to the parent tree */
	    gtk_tree_append (GTK_TREE(tree), item);
	    gtk_widget_show (item);
	    subtree = gtk_tree_new();

	    /* This is still necessary if you want these signals to be called
	     * for the subtree's children.  Note that selection_change will be 
	     * signalled for the root tree regardless. 
	     */
	    gtk_signal_connect (GTK_OBJECT(subtree), "select_child",
			GTK_SIGNAL_FUNC(cb_select_child), subtree);
	    gtk_signal_connect (GTK_OBJECT(subtree), "unselect_child",
			GTK_SIGNAL_FUNC(cb_unselect_child), subtree);
    
	    /* Set this item's subtree - note that you cannot do this until
	     * AFTER the item has been added to its parent tree! 
	     */
	    gtk_tree_item_set_subtree (GTK_TREE_ITEM(item), subtree);

	    for (j = 0; j < 5; j++)
		{
		GtkWidget *subitem;

		/* Create a subtree item, in much the same way */
		subitem = gtk_tree_item_new_with_label (itemnames[j]);
		gtk_tree_append (GTK_TREE(subtree), subitem);
		gtk_widget_show (subitem);
		}
	    }
#endif

	/** Open the main window **/
	gtk_widget_show (SFE_Globals.Window);

	/** Load any file specified on the command line **/
	if (SFE_Globals.ArgC == 2 && SFE_Globals.ArgV[1]) 
	    sfeLoadFile(SFE_Globals.ArgV[1]);
	else
	    sfeRebuildUI(SFE_Globals.Data, GTK_TREE(tree));

	/** Create the attribute editing window **/
	SFE_Globals.AttrEditWindow = sfe_ui_CreateAttrDialog();

	/** Enter the event loop for GTK **/
	gtk_main();

    thExit();
    }

int
main(int argc, char* argv[])
    {
    SFE_Globals.ArgC = argc;
    SFE_Globals.ArgV = argv;
    mtInitialize(0,start);
    return 0;
    }

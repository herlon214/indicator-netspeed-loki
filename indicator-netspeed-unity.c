/*

This is an Ubuntu appindicator that displays the current network traffic.

Based on indicator-netspeed.c from https://gist.github.com/982939 and indicator-netspeed-unity.c from https://github.com/mgedmin/indicator-netspeed

License: this software is in the public domain.

*/

#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#include <glibtop.h>
#include <glibtop/netload.h>
#include <glibtop/netlist.h>
#include <pango/pango.h>
#include <gio/gio.h>
#include <stdbool.h>
#include <glib/gi18n.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>

#define GETTEXT_PACKAGE "indicator-netspeed-unity"
#define LOCALEDIR "/usr/share/locale"

#define GSTR_INIT_TEXT(a,b) ((a) = g_string_new((b)))
#define GSTR_INIT(a) ((a) = g_string_new(""))
#define GSTR_FREE(a) (g_string_free((a), TRUE))
#define GSTR_GET(a) ((a)->str)
#define GSTR_SET(a,b) (g_string_assign((a),(b)))
#define die(e) do { fprintf(stderr, "%s", e); exit(EXIT_FAILURE); } while (0);

/* update period in seconds */
gint period = 1;
gint posit_item = 0;
gboolean first_run = TRUE;

GSettings *settings = NULL;
gboolean show_settings = TRUE;
AppIndicator *indicator = NULL;
GtkWidget *indicator_menu = NULL;
GtkWidget *interfaces_menu = NULL;

GString * text_All = NULL;
GString * selected_if_name = NULL;
gboolean pictures_of_the_current_theme = FALSE;
gboolean padding_indicator = FALSE;
gboolean ping_indicator = TRUE;
GString * pic_network_transmit_receive = NULL;
GString * pic_network_receive = NULL;
GString * pic_network_transmit = NULL;
GString * pic_network_idle = NULL;

GtkWidget *if_chosen = NULL;
GList* if_items_list = NULL;
GtkWidget *net_down_item = NULL;
GtkWidget *net_up_item = NULL;
GtkWidget *net_total_item = NULL;
GtkWidget *net_down_icon = NULL;
GtkWidget *net_up_icon = NULL;
GtkWidget *net_total_icon = NULL;

GtkWidget *settings_item = NULL;
GtkWidget *settings_sub = NULL;
GtkWidget *prefixes_item = NULL;
GtkWidget *prefixes_sub = NULL;
GSList *group_prefixes_sub = NULL;
GtkWidget *prefixes_binary_item = NULL;
GtkWidget *prefixes_decimal_item = NULL;
GtkWidget *prefixes_bits_item = NULL;
GtkWidget *theme_item = NULL;
GtkWidget *theme_sub = NULL;
GSList *group_theme_sub = NULL;
GtkWidget *theme_dark_item = NULL;
GtkWidget *theme_light_item = NULL;
GtkWidget *theme_current_item = NULL;
GtkWidget *view_item = NULL;
GtkWidget *view_sub = NULL;
GSList *group_view_sub = NULL;
GtkWidget *view_normal_item = NULL;
GtkWidget *view_compact_item = NULL;
GtkWidget *view_minimum_item = NULL;
GtkWidget *padding_item = NULL;
GtkWidget *ping_item = NULL;
GtkWidget *tools_item = NULL;
GtkWidget *tools_sub = NULL;
GList* tools_items_list = NULL;

GtkWidget *quit_item = NULL;

gchar **old_interfaces = NULL;
gint count_old_if = 0;

gint show_bin_dec_bit = 0;

gint view_mode = 0;

void if_signal_select(GtkMenuItem *menu_item, gpointer data);
gboolean update();

char* do_ping() {
  char * value;
  int statval;
  int link[2];
  pid_t pid;
  char foo[4096];

  if (pipe(link)==-1)
    die("pipe");

  if ((pid = fork()) == -1)
    die("fork");

  if(pid == 0) {

    dup2 (link[1], STDOUT_FILENO);
    close(link[0]);
    close(link[1]);
    execl("/bin/ping", "ping", "8.8.8.8", "-c", "1", NULL);
    die("execl");

  } else {

    close(link[1]);
    read(link[0], foo, sizeof(foo));
    value = strtok (foo, "=");
    value = strtok (NULL, "=");
    value = strtok (NULL, "=");
    value = strtok (NULL, "=");
    value = strtok (value, " ");

    wait(NULL);

    return strcat(value, " ms");
    
  }
}

/**
 * Set ping response 
 */
GString * get_ms() {
  GString * ms;
  GSTR_INIT( ms );

  if(ping_indicator) {
    GSTR_SET( ms, _(do_ping()) );
  }
  

  return ms;
}

GString * format_net_label(const gchar *in_begin, double bytes, gint show_as, gint view_as, gboolean less_kilo, gboolean padding)
{
	GString * format;
	GString * unit;
        GSTR_INIT( format );
        GSTR_INIT( unit );
	guint64 kilo; /* no really a kilo : a kilo or kibi */

	if (show_as == 2) { //Bits
		bytes *= 8;
		kilo = 1000;
	} else if (show_as == 1) //Decimal
		kilo = 1000;
	  else //Binary
		kilo = 1024;

	if (less_kilo && (bytes < kilo)) {
		GSTR_SET( format, "%s%.0f %s" );
	        if (show_as == 2)      GSTR_SET( unit, _("b") );
	        else if (show_as == 1) GSTR_SET( unit, _("B") );
	        else                   GSTR_SET( unit, _("B") );

	} else if (bytes < (kilo * kilo)) {
		GSTR_SET( format, (bytes < (100 * kilo)) ? "%s%.1f %s" : "%s%.0f %s" );
		bytes /= kilo;
	        if (show_as == 2)      GSTR_SET( unit, _("kb") );
	        else if (show_as == 1) GSTR_SET( unit, _("kB") );
	        else                   GSTR_SET( unit, _("KiB") );

	} else  if (bytes < (kilo * kilo * kilo)) {
		GSTR_SET( format, "%s%.1f %s" );
		bytes /= kilo * kilo;
	        if (show_as == 2)      GSTR_SET( unit, _("Mb") );
	        else if (show_as == 1) GSTR_SET( unit, _("MB") );
	        else                   GSTR_SET( unit, _("MiB") );

	} else  if (bytes < (kilo * kilo * kilo * kilo)) {
		GSTR_SET( format, "%s%.3f %s" );
		bytes /= kilo * kilo * kilo;
	        if (show_as == 2)      GSTR_SET( unit, _("Gb") );
	        else if (show_as == 1) GSTR_SET( unit, _("GB") );
	        else                   GSTR_SET( unit, _("GiB") );

	} else  if (bytes < (kilo * kilo * kilo * kilo * kilo)) {
		GSTR_SET( format, "%s%.3f %s" );
		bytes /= kilo * kilo * kilo * kilo;
	        if (show_as == 2)      GSTR_SET( unit, _("Tb") );
	        else if (show_as == 1) GSTR_SET( unit, _("TB") );
	        else                   GSTR_SET( unit, _("TiB") );

	} else {
		GSTR_SET( format, "%s%.3f %s" );
		bytes /= kilo * kilo * kilo * kilo * kilo;
	        if (show_as == 2)      GSTR_SET( unit, _("Pb") );
	        else if (show_as == 1) GSTR_SET( unit, _("PB") );
	        else                   GSTR_SET( unit, _("PiB") );

	}

    static GString * string = NULL;
    if(!string) GSTR_INIT( string );

    if(view_as == 0) {
      g_string_printf( string, GSTR_GET(format), in_begin, bytes, GSTR_GET(unit) );
    }
    else if(view_as == 1) {
      g_string_printf( string, GSTR_GET(format), in_begin, bytes, GSTR_GET(unit) );
    }
    else {
      gchar *tmp_s1;
      tmp_s1 = g_strdup( GSTR_GET(unit) );
      if(g_utf8_validate(GSTR_GET(unit),-1,NULL) == TRUE) {
        g_utf8_strncpy( tmp_s1, GSTR_GET(unit), 1 );
      }
      g_string_printf( string, GSTR_GET(format), in_begin, bytes, tmp_s1 );
      g_free( tmp_s1 );
    }

    GSTR_FREE( format );
    GSTR_FREE( unit );
    
    if(padding)
    {
        //render string and get its pixel width
        gint width = 0;
        static gint maxWidth = 20;   //max width for label in pixels

        //TODO: should be determined from current panel font type and size
        gint spaceWidth = 4;  //width of one space char in pixels,

        PangoContext* context = gtk_widget_get_pango_context( indicator_menu );
        PangoLayout* layout = pango_layout_new( context );
        pango_layout_set_text( layout, GSTR_GET(string), string->allocated_len );
        pango_layout_get_pixel_size( layout, &width, NULL );
        // frees the layout object, do not use after this point
        g_object_unref( layout );

        //push max size up as needed
        if (width > maxWidth) maxWidth = width + spaceWidth;

        //fill up with spaces
        GString * string_for_spaces;
        GSTR_INIT_TEXT( string_for_spaces, GSTR_GET(string) );
        g_string_printf( string, "%*s%s", (gint)((maxWidth-width)/spaceWidth), " ", GSTR_GET(string_for_spaces) );
        GSTR_FREE( string_for_spaces );
    }
    
    return string;
}

void if_net_down_item_activate(GtkMenuItem *menu_item, gpointer data) {
    posit_item = 1;
    g_settings_set_int( settings, "state", posit_item );
}

void if_net_up_item_activate(GtkMenuItem *menu_item, gpointer data) {
    posit_item = 2;
    g_settings_set_int( settings, "state", posit_item );
}

void if_net_total_item_activate(GtkMenuItem *menu_item, gpointer data) {
    if (posit_item == 0)
      posit_item = 3;
    else posit_item = 0;
    g_settings_set_int( settings, "state", posit_item );
}

void padding_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
    padding_indicator = gtk_check_menu_item_get_active( menu_item );
    g_settings_set_boolean( settings, "padding-indicator", padding_indicator );
}

void ping_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
  ping_indicator = gtk_check_menu_item_get_active( menu_item );
  g_settings_set_boolean( settings, "ping-indicator", ping_indicator);
}

void prefixes_binary_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
    if (gtk_check_menu_item_get_active( menu_item )) {
      show_bin_dec_bit = 0;
      g_settings_set_int( settings, "show-bin-dec-bit", show_bin_dec_bit );
    }
}

void prefixes_decimal_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
    if (gtk_check_menu_item_get_active( menu_item )) {
      show_bin_dec_bit = 1;
      g_settings_set_int( settings, "show-bin-dec-bit", show_bin_dec_bit );
    }
}

void prefixes_bits_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
    if (gtk_check_menu_item_get_active( menu_item )) {
      show_bin_dec_bit = 2;
      g_settings_set_int( settings, "show-bin-dec-bit", show_bin_dec_bit );
    }
}

void theme_dark_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
  if (gtk_check_menu_item_get_active( menu_item )) {
    GSTR_SET( pic_network_receive, "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-receive-d.svg" );
    GSTR_SET( pic_network_transmit, "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-d.svg" );
    GSTR_SET( pic_network_transmit_receive, "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-receive-d.svg" );
    GSTR_SET( pic_network_idle, "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-d.svg" );

    g_settings_set_string( settings, "pic-file-network-receive", GSTR_GET(pic_network_receive) );
    g_settings_set_string( settings, "pic-file-network-transmit", GSTR_GET(pic_network_transmit) );
    g_settings_set_string( settings, "pic-file-network-transmit-receive", GSTR_GET(pic_network_transmit_receive) );
    g_settings_set_string( settings, "pic-file-network-idle", GSTR_GET(pic_network_idle) );

    pictures_of_the_current_theme = FALSE;
    g_settings_set_boolean( settings, "pictures-of-the-current-theme", pictures_of_the_current_theme );

    gtk_image_set_from_file( GTK_IMAGE(net_down_icon), GSTR_GET(pic_network_receive) );
    gtk_image_set_from_file( GTK_IMAGE(net_up_icon), GSTR_GET(pic_network_transmit) );
    gtk_image_set_from_file( GTK_IMAGE(net_total_icon), GSTR_GET(pic_network_transmit_receive) );
  }
}

void theme_light_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
  if (gtk_check_menu_item_get_active( menu_item )) {
    GSTR_SET( pic_network_receive, "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-receive-l.svg" );
    GSTR_SET( pic_network_transmit, "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-l.svg" );
    GSTR_SET( pic_network_transmit_receive, "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-receive-l.svg" );
    GSTR_SET( pic_network_idle, "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-l.svg" );

    g_settings_set_string( settings, "pic-file-network-receive", GSTR_GET(pic_network_receive) );
    g_settings_set_string( settings, "pic-file-network-transmit", GSTR_GET(pic_network_transmit) );
    g_settings_set_string( settings, "pic-file-network-transmit-receive", GSTR_GET(pic_network_transmit_receive) );
    g_settings_set_string( settings, "pic-file-network-idle", GSTR_GET(pic_network_idle) );

    pictures_of_the_current_theme = FALSE;
    g_settings_set_boolean( settings, "pictures-of-the-current-theme", pictures_of_the_current_theme );

    gtk_image_set_from_file( GTK_IMAGE(net_down_icon), GSTR_GET(pic_network_receive) );
    gtk_image_set_from_file( GTK_IMAGE(net_up_icon), GSTR_GET(pic_network_transmit) );
    gtk_image_set_from_file( GTK_IMAGE(net_total_icon), GSTR_GET(pic_network_transmit_receive) );
  }
}

void theme_current_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
  if (gtk_check_menu_item_get_active( menu_item )) {
    pictures_of_the_current_theme = TRUE;
    g_settings_set_boolean( settings, "pictures-of-the-current-theme", pictures_of_the_current_theme );

    GSTR_SET( pic_network_receive, g_settings_get_string(settings, "pic-theme-network-receive") );
    gtk_image_set_from_icon_name( GTK_IMAGE(net_down_icon), GSTR_GET(pic_network_receive), GTK_ICON_SIZE_MENU );
    GSTR_SET( pic_network_transmit, g_settings_get_string(settings, "pic-theme-network-transmit") );
    gtk_image_set_from_icon_name( GTK_IMAGE(net_up_icon), GSTR_GET(pic_network_transmit), GTK_ICON_SIZE_MENU );
    GSTR_SET( pic_network_transmit_receive, g_settings_get_string(settings, "pic-theme-network-transmit-receive") );
    gtk_image_set_from_icon_name( GTK_IMAGE(net_total_icon), GSTR_GET(pic_network_transmit_receive), GTK_ICON_SIZE_MENU );
    GSTR_SET( pic_network_idle, g_settings_get_string(settings, "pic-theme-network-idle") );
  }
}

void view_normal_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
    if (gtk_check_menu_item_get_active( menu_item )) {
      view_mode = 0;
      g_settings_set_int( settings, "view-mode", view_mode );
    }
}

void view_compact_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
    if (gtk_check_menu_item_get_active( menu_item )) {
      view_mode = 1;
      g_settings_set_int( settings, "view-mode", view_mode );
    }
}

void view_minimum_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
    if (gtk_check_menu_item_get_active( menu_item )) {
      view_mode = 2;
      g_settings_set_int( settings, "view-mode", view_mode );
    }
}

void if_signal_select(GtkMenuItem *menu_item, gpointer data) {
    //set currently selected interface from user selection

    GSTR_SET( selected_if_name, gtk_widget_get_name(GTK_WIDGET(menu_item)) );
    if (strcmp( _(GSTR_GET(text_All)), GSTR_GET(selected_if_name) ) == 0) {
      GSTR_SET( selected_if_name, GSTR_GET(text_All) );
      gtk_menu_item_set_label( GTK_MENU_ITEM(if_chosen), _(GSTR_GET(text_All)) );
    } else gtk_menu_item_set_label( GTK_MENU_ITEM(if_chosen), GSTR_GET(selected_if_name) );
    g_settings_set_string( settings, "if-name", GSTR_GET(selected_if_name) );

    first_run = TRUE;
    update();
}

void if_tools_nethogs_activate(GtkMenuItem *menu_item, gpointer data) {
    //run in terminal "sudo nethogs xxxx"
    GAppInfo *appinfo = NULL;
    GString * selected_commandline;
    GSTR_INIT( selected_commandline );
    GFile *file_nethogs = NULL;
    file_nethogs = g_file_new_for_path( "/usr/sbin/nethogs" );

    if (g_file_query_exists( file_nethogs, NULL )) {
       GSTR_SET( selected_commandline, gtk_menu_item_get_label(menu_item) );
    } else {
       g_string_printf( selected_commandline, "/usr/share/indicator-netspeed-unity/install_nethogs.sh %s", gtk_widget_get_name(GTK_WIDGET(menu_item)) );
    }

    appinfo = g_app_info_create_from_commandline( GSTR_GET(selected_commandline), NULL, G_APP_INFO_CREATE_NEEDS_TERMINAL, NULL );
    g_app_info_launch( appinfo, NULL, NULL, NULL );

    GSTR_FREE( selected_commandline );
    g_object_unref( file_nethogs );
}

void add_netifs() {
    //populate list of interfaces
    glibtop_netlist netlist;
    gchar **interfaces = glibtop_get_netlist( &netlist );
    g_strfreev( old_interfaces );
    old_interfaces  = glibtop_get_netlist( &netlist );
    count_old_if = netlist.number;
    GtkWidget *if_item;
    GtkWidget *if_tools_item;

    for(gint i = 0; i < netlist.number; i++) {
        if (strcmp( "lo", interfaces[i] ) == 0)
            continue;

        GString * tmp_str;
        GSTR_INIT_TEXT( tmp_str, interfaces[i] );
        if_item = gtk_menu_item_new_with_label( interfaces[i] );
        gtk_widget_set_name( if_item, GSTR_GET(tmp_str) );
        gtk_menu_shell_append( GTK_MENU_SHELL(interfaces_menu), if_item );
        g_signal_connect( G_OBJECT(if_item), "activate", G_CALLBACK(if_signal_select), NULL );
        
        if (show_settings) {
          GString * tmp_str2;
          GSTR_INIT( tmp_str2 );
          g_string_printf( tmp_str2, "%s %s", "sudo nethogs", GSTR_GET(tmp_str) );
          if_tools_item = gtk_menu_item_new_with_label( GSTR_GET(tmp_str2) );
          GSTR_FREE( tmp_str2 );
          gtk_widget_set_name( if_tools_item, GSTR_GET(tmp_str) );
          gtk_menu_shell_append( GTK_MENU_SHELL(tools_sub), if_tools_item );
          g_signal_connect( G_OBJECT(if_tools_item), "activate", G_CALLBACK(if_tools_nethogs_activate), NULL );
        }
        GSTR_FREE( tmp_str );
    }

    if_item = gtk_menu_item_new_with_label( _(GSTR_GET(text_All)) );
    gtk_widget_set_name( if_item, _(GSTR_GET(text_All)) );
    gtk_menu_shell_append( GTK_MENU_SHELL(interfaces_menu), if_item );
    g_signal_connect( G_OBJECT(if_item), "activate", G_CALLBACK(if_signal_select), NULL );

    g_strfreev( interfaces );
    if_items_list = gtk_container_get_children( GTK_CONTAINER(interfaces_menu) );
    if (show_settings)
       tools_items_list = gtk_container_get_children( GTK_CONTAINER(tools_sub) );
}

void upd_netifs() {
    GList* l;
    for(l = if_items_list; l; l = l->next) {
       GtkWidget* item_menu = l->data;
       if (item_menu) {
         gtk_widget_destroy( item_menu );
       }
    }
    g_list_free( if_items_list );

    if (show_settings) {
      for(l = tools_items_list; l; l = l->next) {
         GtkWidget* item_menu = l->data;
         if (item_menu) {
           gtk_widget_destroy( item_menu );
         }
      }
      g_list_free( tools_items_list );
    }

    add_netifs();
    gtk_widget_show_all( interfaces_menu );
    gtk_widget_show_all( tools_sub );
}

void get_net(guint64 traffic[2])
{
    static guint64 bytes_in_old = 0;
    static guint64 bytes_out_old = 0;
    glibtop_netload netload;
    glibtop_netlist netlist;
    guint64 bytes_in = 0;
    guint64 bytes_out = 0;
    GString * tmp_s;
    GString * tmp_s1;
    GString * tmp_s2;
    GSTR_INIT( tmp_s1 );
    GSTR_INIT( tmp_s2 );
    gchar **interfaces = glibtop_get_netlist( &netlist );

    bool update_interfaces = FALSE;
    if (count_old_if != netlist.number) {
      update_interfaces = TRUE;
    }
    else {
      for(int i = 0; i < netlist.number; i++) {
         if (strcmp( interfaces[i], old_interfaces[i] ) != 0) {
           update_interfaces = TRUE;
           break;
         }
      }
    }

    if (update_interfaces) {
       upd_netifs();
    }

    for(gint i = 0; i < netlist.number; i++) {
        if (strcmp( "lo", interfaces[i] ) == 0) {
            continue;
        }

        glibtop_get_netload( &netload, interfaces[i] );

        if (strcmp( GSTR_GET(text_All), GSTR_GET(selected_if_name) ) == 0 || strcmp( GSTR_GET(selected_if_name), interfaces[i] ) == 0) {
            bytes_in += netload.bytes_in;
            bytes_out += netload.bytes_out;
        }

        GList* l;
        for(l = if_items_list; l; l = l->next) {
            GtkWidget* item_menu = l->data;
            if (item_menu && (0 == strcmp( gtk_widget_get_name(item_menu), interfaces[i]) )) {
              GString * strtmp;
              GSTR_INIT( strtmp );
              tmp_s = format_net_label("↓:", (double)netload.bytes_in, show_bin_dec_bit, 0, true, false);
              GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
              tmp_s = format_net_label("↑:", (double)netload.bytes_out, show_bin_dec_bit, 0, true, false);
              GSTR_SET( tmp_s2, GSTR_GET(tmp_s) );
              g_string_printf( strtmp, "%s: %s, %s", gtk_widget_get_name(item_menu), GSTR_GET(tmp_s1), GSTR_GET(tmp_s2) );
              gtk_menu_item_set_label( GTK_MENU_ITEM(item_menu), GSTR_GET(strtmp) );
              GSTR_FREE( strtmp );
            }
        }

    }

    g_strfreev( interfaces );

    if (first_run) {
        bytes_in_old = bytes_in;
        bytes_out_old = bytes_out;
        first_run = FALSE;
    }

    traffic[0] = (bytes_in - bytes_in_old) / period;
    traffic[1] = (bytes_out - bytes_out_old) / period;

    bytes_in_old = bytes_in;
    bytes_out_old = bytes_out;

    GSTR_FREE( tmp_s1 );
    GSTR_FREE( tmp_s2 );
}

gboolean update() {
    //get sum of up and down net traffic and separate values
    //and refresh labels of current interface
    guint64 net_traffic[2] = {0, 0};
    get_net( net_traffic );
    guint64 net_down = net_traffic[0];
    guint64 net_up = net_traffic[1];
    guint64 net_total = net_down + net_up;

    GString * label_guide;
    GSTR_INIT( label_guide ); //("↓:%s %s ↑:%s %s", "10000.00 ", _("MiB/s"), "10000.00 ", _("MiB/s"));   //maximum length label text, doesn't really work atm
    GString * indicator_label;
    GSTR_INIT( indicator_label );

    GString * tmp_s = NULL;
    GString * tmp_s1;
    GString * tmp_s2;
    GString * tmp_s3;
    GSTR_INIT( tmp_s1 );
    GSTR_INIT( tmp_s2 );
    GSTR_INIT( tmp_s3 );

    if (posit_item == 0) {
      if( view_mode == 0) {
        tmp_s = format_net_label( "↓:", (double)net_down, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        tmp_s = format_net_label( "↑:", (double)net_up, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s2, GSTR_GET(tmp_s) );
        tmp_s = get_ms();
        GSTR_SET( tmp_s3, GSTR_GET(tmp_s) );
        g_string_printf( indicator_label, "%s/%s %s/%s - %s", GSTR_GET(tmp_s1), _("s"), GSTR_GET(tmp_s2), _("s"), GSTR_GET(tmp_s3), _("s") );
      }
      else if( view_mode == 1) {
        tmp_s = format_net_label( "↓", (double)net_down, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        tmp_s = format_net_label( "↑", (double)net_up, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s2, GSTR_GET(tmp_s) );
        tmp_s = get_ms();
        GSTR_SET( tmp_s3, GSTR_GET(tmp_s) );
        g_string_printf( indicator_label, "%s %s %s - %s", GSTR_GET(tmp_s1), GSTR_GET(tmp_s2), GSTR_GET(tmp_s3) );
      }
      else {
        tmp_s = format_net_label( "", (double)net_down, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        tmp_s = format_net_label( "", (double)net_up, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s2, GSTR_GET(tmp_s) );
        tmp_s = get_ms();
        GSTR_SET( tmp_s3, GSTR_GET(tmp_s) );
        g_string_printf( indicator_label, "%s %s %s - %s", GSTR_GET(tmp_s1), GSTR_GET(tmp_s2), GSTR_GET(tmp_s3));
      }
    }
    else if (posit_item == 1)
    {
      if( view_mode == 0) {
        tmp_s = format_net_label( "↓:", (double)net_down, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        g_string_printf( indicator_label, "%s/%s", GSTR_GET(tmp_s1), _("s") );
      }
      else if( view_mode == 1) {
        tmp_s = format_net_label( "↓", (double)net_down, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        g_string_printf( indicator_label, "%s", GSTR_GET(tmp_s1) );
      }
      else {
        tmp_s = format_net_label( "", (double)net_down, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        g_string_printf( indicator_label, "%s", GSTR_GET(tmp_s1) );
      }
    }
    else if (posit_item == 2)
    {
      if( view_mode == 0) {
        tmp_s = format_net_label( "↑:", (double)net_up, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        g_string_printf( indicator_label, "%s/%s", GSTR_GET(tmp_s1), _("s") );
      }
      else if( view_mode == 1) {
        tmp_s = format_net_label( "↑", (double)net_up, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        g_string_printf( indicator_label, "%s", GSTR_GET(tmp_s1) );
      }
      else {
        tmp_s = format_net_label( "", (double)net_up, show_bin_dec_bit, view_mode, false, padding_indicator );
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        g_string_printf( indicator_label, "%s", GSTR_GET(tmp_s1) );
      }
    }
    else if (posit_item == 3)
    {
      if( view_mode == 0) {
        tmp_s = format_net_label("", (double)net_total, show_bin_dec_bit, view_mode, false, padding_indicator);
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        g_string_printf( indicator_label, "%s/%s", GSTR_GET(tmp_s1), _("s") );
      }
      else {
        tmp_s = format_net_label("", (double)net_total, show_bin_dec_bit, view_mode, false, padding_indicator);
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        g_string_printf( indicator_label, "%s", GSTR_GET(tmp_s1) );
      }
    }

    app_indicator_set_label( indicator, GSTR_GET(indicator_label), GSTR_GET(label_guide) );
    GSTR_FREE( indicator_label );
    GSTR_FREE( label_guide );

    GString * net_down_label;
    GSTR_INIT( net_down_label );
    tmp_s = format_net_label( "", (double)net_down, show_bin_dec_bit, 0, true, false );
    GSTR_SET( net_down_label, GSTR_GET(tmp_s) );
    gtk_menu_item_set_label( GTK_MENU_ITEM(net_down_item), GSTR_GET(net_down_label) );
    GSTR_FREE( net_down_label );

    GString * net_up_label;
    GSTR_INIT( net_up_label );
    tmp_s = format_net_label( "", (double)net_up, show_bin_dec_bit, 0, true, false );
    GSTR_SET( net_up_label, GSTR_GET(tmp_s) );
    gtk_menu_item_set_label( GTK_MENU_ITEM(net_up_item), GSTR_GET(net_up_label) );
    GSTR_FREE( net_up_label );

    if (posit_item == 0) {
        GString * net_total_label;
        GSTR_INIT( net_total_label );
        tmp_s = format_net_label( "", (double)net_total, show_bin_dec_bit, 0, true, false );
        GSTR_SET( net_total_label, GSTR_GET(tmp_s) );
        gtk_menu_item_set_label( GTK_MENU_ITEM(net_total_item), GSTR_GET(net_total_label) );
        GSTR_FREE( net_total_label );
    }
    else {
        tmp_s = format_net_label( "↓:", (double)net_down, show_bin_dec_bit, 0, true, false );
        GSTR_SET( tmp_s1, GSTR_GET(tmp_s) );
        tmp_s = format_net_label( "↑:", (double)net_up, show_bin_dec_bit, 0, true, false );
        GSTR_SET( tmp_s2, GSTR_GET(tmp_s) );

        GString * net_total_label;
        GSTR_INIT( net_total_label );
        g_string_printf( net_total_label, "%s/%s %s/%s", GSTR_GET(tmp_s1), _("s"), GSTR_GET(tmp_s2), _("s") );
        gtk_menu_item_set_label( GTK_MENU_ITEM(net_total_item), GSTR_GET(net_total_label) );
        GSTR_FREE( net_total_label );
    }

    if (net_down && net_up) {
        app_indicator_set_icon( indicator, GSTR_GET(pic_network_transmit_receive) );
    }
    else if (net_down) {
        app_indicator_set_icon( indicator, GSTR_GET(pic_network_receive) );
    }
    else if (net_up) {
        app_indicator_set_icon( indicator, GSTR_GET(pic_network_transmit) );
    }
    else {
        app_indicator_set_icon( indicator, GSTR_GET(pic_network_idle) );
    }

    GSTR_FREE( tmp_s1 );
    GSTR_FREE( tmp_s2 );
    GSTR_FREE( tmp_s3 );
    return TRUE;
}

gint main (gint argc, char **argv)
{
    bindtextdomain( GETTEXT_PACKAGE, LOCALEDIR );
    textdomain( GETTEXT_PACKAGE );

    gtk_init ( &argc, &argv );

    GSTR_INIT_TEXT( text_All, "All" );

    settings = g_settings_new( "apps.indicators.netspeed-unity" );
    GSTR_INIT_TEXT( selected_if_name, g_settings_get_string(settings, "if-name") );

    pictures_of_the_current_theme = g_settings_get_boolean( settings, "pictures-of-the-current-theme" );
    padding_indicator = g_settings_get_boolean( settings, "padding-indicator" );
    show_settings = g_settings_get_boolean( settings, "show-settings" );
    if (pictures_of_the_current_theme) {
      GSTR_INIT_TEXT( pic_network_receive, g_settings_get_string(settings, "pic-theme-network-receive") );
      net_down_icon = gtk_image_new_from_icon_name( GSTR_GET(pic_network_receive), GTK_ICON_SIZE_MENU );
      GSTR_INIT_TEXT( pic_network_transmit, g_settings_get_string(settings, "pic-theme-network-transmit") );
      net_up_icon = gtk_image_new_from_icon_name( GSTR_GET(pic_network_transmit), GTK_ICON_SIZE_MENU );
      GSTR_INIT_TEXT( pic_network_transmit_receive, g_settings_get_string(settings, "pic-theme-network-transmit-receive") );
      net_total_icon = gtk_image_new_from_icon_name( GSTR_GET(pic_network_transmit_receive), GTK_ICON_SIZE_MENU );
      GSTR_INIT_TEXT( pic_network_idle, g_settings_get_string(settings, "pic-theme-network-idle") );
    } else {
      GSTR_INIT_TEXT( pic_network_receive, g_settings_get_string(settings, "pic-file-network-receive") );
      net_down_icon = gtk_image_new_from_file( GSTR_GET(pic_network_receive) );
      GSTR_INIT_TEXT( pic_network_transmit, g_settings_get_string(settings, "pic-file-network-transmit") );
      net_up_icon = gtk_image_new_from_file( GSTR_GET(pic_network_transmit) );
      GSTR_INIT_TEXT( pic_network_transmit_receive, g_settings_get_string(settings, "pic-file-network-transmit-receive") );
      net_total_icon = gtk_image_new_from_file( GSTR_GET(pic_network_transmit_receive) );
      GSTR_INIT_TEXT( pic_network_idle, g_settings_get_string(settings, "pic-file-network-idle") );
    }
    posit_item = g_settings_get_int( settings, "state" );
    show_bin_dec_bit = g_settings_get_int( settings, "show-bin-dec-bit" );
    view_mode = g_settings_get_int( settings, "view-mode" );

    indicator_menu = gtk_menu_new();

    //add interfaces menu
    interfaces_menu = gtk_menu_new();

    //add interface names
    if (strcmp( GSTR_GET(text_All), GSTR_GET(selected_if_name) ) == 0) {
      if_chosen = gtk_menu_item_new_with_label( _(GSTR_GET(text_All)) );
    } else if_chosen = gtk_menu_item_new_with_label( GSTR_GET(selected_if_name) );
    gtk_menu_shell_append( GTK_MENU_SHELL(indicator_menu), if_chosen );
    gtk_menu_item_set_submenu( GTK_MENU_ITEM(if_chosen), interfaces_menu );

    //separator
    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL(indicator_menu), sep );

    //add menu entries for up and down speed
    net_down_item = gtk_image_menu_item_new_with_label( "" );
    gtk_image_menu_item_set_image( GTK_IMAGE_MENU_ITEM(net_down_item), net_down_icon );
    gtk_image_menu_item_set_always_show_image( GTK_IMAGE_MENU_ITEM(net_down_item), TRUE );
    gtk_menu_shell_append( GTK_MENU_SHELL(indicator_menu), net_down_item );
    g_signal_connect( G_OBJECT(net_down_item), "activate", G_CALLBACK(if_net_down_item_activate), NULL );

    net_up_item = gtk_image_menu_item_new_with_label("");
    gtk_image_menu_item_set_image( GTK_IMAGE_MENU_ITEM(net_up_item), net_up_icon );
    gtk_image_menu_item_set_always_show_image( GTK_IMAGE_MENU_ITEM(net_up_item), TRUE );
    gtk_menu_shell_append( GTK_MENU_SHELL(indicator_menu), net_up_item );
    g_signal_connect( G_OBJECT(net_up_item), "activate", G_CALLBACK(if_net_up_item_activate), NULL );

    net_total_item = gtk_image_menu_item_new_with_label( "" );
    gtk_image_menu_item_set_image( GTK_IMAGE_MENU_ITEM(net_total_item), net_total_icon );
    gtk_image_menu_item_set_always_show_image( GTK_IMAGE_MENU_ITEM(net_total_item), TRUE );
    gtk_menu_shell_append( GTK_MENU_SHELL(indicator_menu), net_total_item );
    g_signal_connect( G_OBJECT(net_total_item), "activate", G_CALLBACK(if_net_total_item_activate), NULL );

    //separator
    sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL(indicator_menu), sep );

    //add settings
 if (show_settings) {
    settings_item = gtk_menu_item_new_with_label( _("Settings") );
    gtk_menu_shell_append( GTK_MENU_SHELL(indicator_menu), settings_item );
    settings_sub = gtk_menu_new();
    gtk_menu_item_set_submenu( GTK_MENU_ITEM(settings_item), settings_sub );
//---
     prefixes_item = gtk_menu_item_new_with_label( _("Prefixes") );
     gtk_menu_shell_append( GTK_MENU_SHELL(settings_sub), prefixes_item );
//----
      prefixes_sub = gtk_menu_new();
      gtk_menu_item_set_submenu( GTK_MENU_ITEM(prefixes_item), prefixes_sub );
//-----
       prefixes_binary_item = gtk_radio_menu_item_new_with_label( group_prefixes_sub, _("Binary") );
       group_prefixes_sub = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(prefixes_binary_item) );
       gtk_menu_shell_append(GTK_MENU_SHELL(prefixes_sub), prefixes_binary_item);

       prefixes_decimal_item = gtk_radio_menu_item_new_with_label( group_prefixes_sub, _("Decimal") );
       group_prefixes_sub = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(prefixes_decimal_item) );
       gtk_menu_shell_append( GTK_MENU_SHELL(prefixes_sub), prefixes_decimal_item );

       prefixes_bits_item = gtk_radio_menu_item_new_with_label( group_prefixes_sub, _("Bits") );
       group_prefixes_sub = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(prefixes_bits_item) );
       gtk_menu_shell_append( GTK_MENU_SHELL(prefixes_sub), prefixes_bits_item );

       if (show_bin_dec_bit == 2)      gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(prefixes_bits_item), TRUE );
       else if (show_bin_dec_bit == 1) gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(prefixes_decimal_item), TRUE );
       else                            gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(prefixes_binary_item), TRUE );

       g_signal_connect( G_OBJECT(prefixes_binary_item), "toggled", G_CALLBACK(prefixes_binary_item_toggled), NULL );
       g_signal_connect( G_OBJECT(prefixes_decimal_item), "toggled", G_CALLBACK(prefixes_decimal_item_toggled), NULL );
       g_signal_connect( G_OBJECT(prefixes_bits_item), "toggled", G_CALLBACK(prefixes_bits_item_toggled), NULL );
//---
     theme_item = gtk_menu_item_new_with_label( _("Theme") );
     gtk_menu_shell_append( GTK_MENU_SHELL(settings_sub), theme_item );
//----
      theme_sub = gtk_menu_new();
      gtk_menu_item_set_submenu( GTK_MENU_ITEM(theme_item), theme_sub );
//-----
       theme_dark_item = gtk_radio_menu_item_new_with_label( group_theme_sub, _("Dark") );
       group_theme_sub = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(theme_dark_item) );
       gtk_menu_shell_append( GTK_MENU_SHELL(theme_sub), theme_dark_item );

       theme_light_item = gtk_radio_menu_item_new_with_label( group_theme_sub, _("Light") );
       group_theme_sub = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(theme_light_item) );
       gtk_menu_shell_append( GTK_MENU_SHELL(theme_sub), theme_light_item );

       theme_current_item = gtk_radio_menu_item_new_with_label( group_theme_sub, _("Current") );
       group_theme_sub = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(theme_current_item) );
       gtk_menu_shell_append( GTK_MENU_SHELL(theme_sub), theme_current_item );

       if (pictures_of_the_current_theme) gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(theme_current_item), TRUE );
       else if (strcmp( GSTR_GET(pic_network_idle),
                        "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-d.svg" ) == 0)
              gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(theme_dark_item), TRUE );
       else if (strcmp( GSTR_GET(pic_network_idle),
                        "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-l.svg" ) == 0)
              gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(theme_light_item), TRUE );

       g_signal_connect( G_OBJECT(theme_dark_item), "toggled", G_CALLBACK(theme_dark_item_toggled), NULL );
       g_signal_connect( G_OBJECT(theme_light_item), "toggled", G_CALLBACK(theme_light_item_toggled), NULL );
       g_signal_connect( G_OBJECT(theme_current_item), "toggled", G_CALLBACK(theme_current_item_toggled), NULL );

//---
     view_item = gtk_menu_item_new_with_label( _("View") );
     gtk_menu_shell_append( GTK_MENU_SHELL(settings_sub), view_item );
//----
      view_sub = gtk_menu_new();
      gtk_menu_item_set_submenu( GTK_MENU_ITEM(view_item), view_sub );
//-----
       view_normal_item = gtk_radio_menu_item_new_with_label( group_view_sub, _("Normal") );
       group_view_sub = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(view_normal_item) );
       gtk_menu_shell_append( GTK_MENU_SHELL(view_sub), view_normal_item );

       view_compact_item = gtk_radio_menu_item_new_with_label( group_view_sub, _("Compact") );
       group_view_sub = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(view_compact_item) );
       gtk_menu_shell_append( GTK_MENU_SHELL(view_sub), view_compact_item );

       view_minimum_item = gtk_radio_menu_item_new_with_label( group_view_sub, _("Minimum") );
       group_view_sub = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(view_minimum_item) );
       gtk_menu_shell_append( GTK_MENU_SHELL(view_sub), view_minimum_item );

       if (view_mode == 2)      gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(view_minimum_item), TRUE );
       else if (view_mode == 1) gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(view_compact_item), TRUE );
       else                            gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(view_normal_item), TRUE );

       g_signal_connect( G_OBJECT(view_normal_item), "toggled", G_CALLBACK(view_normal_item_toggled), NULL );
       g_signal_connect( G_OBJECT(view_compact_item), "toggled", G_CALLBACK(view_compact_item_toggled), NULL );
       g_signal_connect( G_OBJECT(view_minimum_item), "toggled", G_CALLBACK(view_minimum_item_toggled), NULL );

//---
     padding_item = gtk_check_menu_item_new_with_label( _("Padding") );
     gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(padding_item), padding_indicator );
     gtk_menu_shell_append( GTK_MENU_SHELL(settings_sub), padding_item );
     g_signal_connect( G_OBJECT(padding_item), "toggled", G_CALLBACK(padding_item_toggled), NULL );
//---
    ping_item = gtk_check_menu_item_new_with_label( _("Ping") );
    gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(ping_item), ping_indicator );
    gtk_menu_shell_append( GTK_MENU_SHELL(settings_sub), ping_item );
    g_signal_connect( G_OBJECT(ping_item), "toggled", G_CALLBACK(ping_item_toggled), NULL );
//---
     tools_item = gtk_menu_item_new_with_label( _("Tools") );
     gtk_menu_shell_append( GTK_MENU_SHELL(settings_sub), tools_item );
//----
      tools_sub = gtk_menu_new();
      gtk_menu_item_set_submenu( GTK_MENU_ITEM(tools_item), tools_sub );
//--

    //separator
    sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append( GTK_MENU_SHELL(indicator_menu), sep );
 }

    //add interface names (continue)
    add_netifs();

    //quit item
    quit_item = gtk_image_menu_item_new_from_stock( GTK_STOCK_QUIT, NULL );
    gtk_menu_shell_append( GTK_MENU_SHELL(indicator_menu), quit_item );
    g_signal_connect( quit_item, "activate", G_CALLBACK(gtk_main_quit), NULL );

    gtk_widget_show_all( indicator_menu );

    //create app indicator
    indicator = app_indicator_new( "netspeed", GSTR_GET(pic_network_idle), APP_INDICATOR_CATEGORY_SYSTEM_SERVICES );
    app_indicator_set_status( indicator, APP_INDICATOR_STATUS_ACTIVE );
    app_indicator_set_label( indicator, "netspeed", "netspeed" );
    app_indicator_set_menu( indicator, GTK_MENU(indicator_menu) );

    //set indicator position. default: all the way left
    //TODO: make this optional so placement can be automatic
    guint32 ordering_index = g_settings_get_int( settings, "ordering-index" );
    app_indicator_set_ordering_index( indicator, ordering_index );

    update();

    /* update period in milliseconds */
    g_timeout_add( 1000*period, update, NULL );

    gtk_main ();

    return 0;
}

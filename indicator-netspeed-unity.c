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

#define GETTEXT_PACKAGE "indicator-netspeed-unity"
#define LOCALEDIR "/usr/share/locale"

/* update period in seconds */
int period = 1;
int posit_item = 0;
gboolean first_run = TRUE;

GSettings *settings = NULL;
gboolean show_settings = TRUE;
AppIndicator *indicator = NULL;
GtkWidget *indicator_menu = NULL;
GtkWidget *interfaces_menu = NULL;

const gchar *text_All = "All";
gchar *selected_if_name = NULL;
gboolean pictures_of_the_current_theme = FALSE;
gboolean padding_indicator = FALSE;
const gchar *pic_network_transmit_receive = NULL;
const gchar *pic_network_receive = NULL;
const gchar *pic_network_transmit = NULL;
const gchar *pic_network_idle = NULL;

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
GtkWidget *padding_item = NULL;
GtkWidget *tools_item = NULL;
GtkWidget *tools_sub = NULL;
GList* tools_items_list = NULL;

GtkWidget *quit_item = NULL;

gchar **old_interfaces = NULL;
int count_old_if = 0;

int show_bin_dec_bit = 0;

void if_signal_select(GtkMenuItem *menu_item, gpointer data);
gboolean update();


gchar* format_net_label(const gchar *in_begin, double bytes, int show_as, gboolean less_kilo, gboolean padding)
{
	const gchar *format;
	const gchar *unit;
	guint64 kilo; /* no really a kilo : a kilo or kibi */

	if (show_as == 2) { //Bits
		bytes *= 8;
		kilo = 1000;
	} else if (show_as == 1) //Decimal
		kilo = 1000;
	  else //Binary
		kilo = 1024;

	if (less_kilo && (bytes < kilo)) {
		format = "%s%.0f %s";
	        if (show_as == 2)      unit = _("b");
	        else if (show_as == 1) unit = _("B");
	        else                   unit = _("B");

	} else if (bytes < (kilo * kilo)) {
		format = (bytes < (100 * kilo)) ? "%s%.1f %s" : "%s%.0f %s";
		bytes /= kilo;
	        if (show_as == 2)      unit = _("kb");
	        else if (show_as == 1) unit = _("kB");
	        else                   unit = _("KiB");

	} else  if (bytes < (kilo * kilo * kilo)) {
		format = "%s%.1f %s";
		bytes /= kilo * kilo;
	        if (show_as == 2)      unit = _("Mb");
	        else if (show_as == 1) unit = _("MB");
	        else                   unit = _("MiB");

	} else  if (bytes < (kilo * kilo * kilo * kilo)) {
		format = "%s%.3f %s";
		bytes /= kilo * kilo * kilo;
	        if (show_as == 2)      unit = _("Gb");
	        else if (show_as == 1) unit = _("GB");
	        else                   unit = _("GiB");

	} else  if (bytes < (kilo * kilo * kilo * kilo * kilo)) {
		format = "%s%.3f %s";
		bytes /= kilo * kilo * kilo * kilo;
	        if (show_as == 2)      unit = _("Tb");
	        else if (show_as == 1) unit = _("TB");
	        else                   unit = _("TiB");

	} else {
		format = "%s%.3f %s";
		bytes /= kilo * kilo * kilo * kilo * kilo;
	        if (show_as == 2)      unit = _("Pb");
	        else if (show_as == 1) unit = _("PB");
	        else                   unit = _("PiB");

	}

    gchar *string = g_strdup_printf( format, in_begin, bytes, unit );

    if(padding)
    {
        //render string and get its pixel width
        int width = 0;
        static int maxWidth = 12;   //max width for label in pixels

        //TODO: should be determined from current panel font type and size
        int spaceWidth = 4;  //width of one space char in pixels,

        PangoContext* context = gtk_widget_get_pango_context( indicator_menu );
        PangoLayout* layout = pango_layout_new( context );
        pango_layout_set_text( layout, string, strlen(string) );
        pango_layout_get_pixel_size( layout, &width, NULL );
        // frees the layout object, do not use after this point
        g_object_unref( layout );

        //push max size up as needed
        if (width > maxWidth) maxWidth = width + spaceWidth;

        gchar *old_string = string;
        //fill up with spaces
        string = g_strdup_printf( "%*s%s", (int)((maxWidth-width)/spaceWidth), " ", string );
        g_free( old_string );
    }

    return string;
}

const gchar *gsettings_get_value_string (const gchar *key) {
    GVariant *tmp_variant;
    const gchar *tmp_str;
    tmp_variant = g_settings_get_value( settings, key );
    tmp_str = g_variant_get_string( tmp_variant, NULL );
    g_variant_unref( tmp_variant );
    return tmp_str;
}

gboolean gsettings_get_value_boolean (const gchar *key) {
    GVariant *tmp_variant;
    gboolean tmp_bool;
    tmp_variant = g_settings_get_value( settings, key );
    tmp_bool = g_variant_get_boolean( tmp_variant );
    g_variant_unref( tmp_variant );
    return tmp_bool;
}

gint32 gsettings_get_value_gint32 (const gchar *key) {
    GVariant *tmp_variant;
    gint32 tmp_int;
    tmp_variant = g_settings_get_value( settings, key );
    tmp_int = g_variant_get_int32( tmp_variant );
    g_variant_unref( tmp_variant );
    return tmp_int;
}

void if_net_down_item_activate(GtkMenuItem *menu_item, gpointer data) {
    posit_item = 1;
    g_settings_set_value( settings, "state", g_variant_new_int32(posit_item) );
}

void if_net_up_item_activate(GtkMenuItem *menu_item, gpointer data) {
    posit_item = 2;
    g_settings_set_value( settings, "state", g_variant_new_int32(posit_item) );
}

void if_net_total_item_activate(GtkMenuItem *menu_item, gpointer data) {
    if (posit_item == 0)
      posit_item = 3;
    else posit_item = 0;
    g_settings_set_value( settings, "state", g_variant_new_int32(posit_item) );
}

void padding_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
    padding_indicator = gtk_check_menu_item_get_active( menu_item );
    g_settings_set_value( settings, "padding-indicator", g_variant_new_boolean(padding_indicator) );
}

void prefixes_binary_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
    if (gtk_check_menu_item_get_active( menu_item )) {
      show_bin_dec_bit = 0;
      g_settings_set_value( settings, "show-bin-dec-bit", g_variant_new_int32(show_bin_dec_bit) );
    }
}

void prefixes_decimal_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
    if (gtk_check_menu_item_get_active( menu_item )) {
      show_bin_dec_bit = 1;
      g_settings_set_value( settings, "show-bin-dec-bit", g_variant_new_int32(show_bin_dec_bit) );
    }
}

void prefixes_bits_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
    if (gtk_check_menu_item_get_active( menu_item )) {
      show_bin_dec_bit = 2;
      g_settings_set_value( settings, "show-bin-dec-bit", g_variant_new_int32(show_bin_dec_bit) );
    }
}

void theme_dark_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
  if (gtk_check_menu_item_get_active( menu_item )) {
    pic_network_receive = "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-receive-d.svg";
    pic_network_transmit = "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-d.svg";
    pic_network_transmit_receive = "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-receive-d.svg";
    pic_network_idle = "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-d.svg";

    g_settings_set_value( settings, "pic-file-network-receive", g_variant_new_string(pic_network_receive) );
    g_settings_set_value( settings, "pic-file-network-transmit", g_variant_new_string(pic_network_transmit) );
    g_settings_set_value( settings, "pic-file-network-transmit-receive", g_variant_new_string(pic_network_transmit_receive) );
    g_settings_set_value( settings, "pic-file-network-idle", g_variant_new_string(pic_network_idle) );

    pictures_of_the_current_theme = FALSE;
    g_settings_set_value( settings, "pictures-of-the-current-theme", g_variant_new_boolean(pictures_of_the_current_theme) );

    gtk_image_set_from_file( GTK_IMAGE(net_down_icon), pic_network_receive );
    gtk_image_set_from_file( GTK_IMAGE(net_up_icon), pic_network_transmit );
    gtk_image_set_from_file( GTK_IMAGE(net_total_icon), pic_network_transmit_receive );
  }
}

void theme_light_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
  if (gtk_check_menu_item_get_active( menu_item )) {
    pic_network_receive = "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-receive-l.svg";
    pic_network_transmit = "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-l.svg";
    pic_network_transmit_receive = "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-transmit-receive-l.svg";
    pic_network_idle = "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-l.svg";

    g_settings_set_value( settings, "pic-file-network-receive", g_variant_new_string(pic_network_receive) );
    g_settings_set_value( settings, "pic-file-network-transmit", g_variant_new_string(pic_network_transmit) );
    g_settings_set_value( settings, "pic-file-network-transmit-receive", g_variant_new_string(pic_network_transmit_receive) );
    g_settings_set_value( settings, "pic-file-network-idle", g_variant_new_string(pic_network_idle) );

    pictures_of_the_current_theme = FALSE;
    g_settings_set_value( settings, "pictures-of-the-current-theme", g_variant_new_boolean(pictures_of_the_current_theme) );

    gtk_image_set_from_file( GTK_IMAGE(net_down_icon), pic_network_receive );
    gtk_image_set_from_file( GTK_IMAGE(net_up_icon), pic_network_transmit );
    gtk_image_set_from_file( GTK_IMAGE(net_total_icon), pic_network_transmit_receive );
  }
}

void theme_current_item_toggled(GtkCheckMenuItem *menu_item, gpointer data) {
  if (gtk_check_menu_item_get_active( menu_item )) {
    pictures_of_the_current_theme = TRUE;
    g_settings_set_value( settings, "pictures-of-the-current-theme", g_variant_new_boolean(pictures_of_the_current_theme) );

    pic_network_receive = gsettings_get_value_string( "pic-theme-network-receive" );
    gtk_image_set_from_icon_name( GTK_IMAGE(net_down_icon), pic_network_receive, GTK_ICON_SIZE_MENU );
    pic_network_transmit = gsettings_get_value_string( "pic-theme-network-transmit" );
    gtk_image_set_from_icon_name( GTK_IMAGE(net_up_icon), pic_network_transmit, GTK_ICON_SIZE_MENU );
    pic_network_transmit_receive = gsettings_get_value_string( "pic-theme-network-transmit-receive" );
    gtk_image_set_from_icon_name( GTK_IMAGE(net_total_icon), pic_network_transmit_receive, GTK_ICON_SIZE_MENU );
    pic_network_idle = gsettings_get_value_string( "pic-theme-network-idle" );
  }
}

void if_signal_select(GtkMenuItem *menu_item, gpointer data) {
    //set currently selected interface from user selection
    g_free( selected_if_name );

    selected_if_name = g_strdup( gtk_widget_get_name(GTK_WIDGET(menu_item)) );
    if (strcmp( _(text_All), selected_if_name ) == 0) {
      g_free( selected_if_name );
      selected_if_name = g_strdup( text_All );
      gtk_menu_item_set_label( GTK_MENU_ITEM(if_chosen), _(text_All) );
    } else gtk_menu_item_set_label( GTK_MENU_ITEM(if_chosen), selected_if_name );
    g_settings_set_value( settings, "if-name", g_variant_new_string(selected_if_name) );

    first_run = TRUE;
    update();
}

void if_tools_nethogs_activate(GtkMenuItem *menu_item, gpointer data) {
    //run in terminal "sudo nethogs xxxx"
    GAppInfo *appinfo = NULL;
    gchar *selected_commandline = NULL;
    GFile *file_nethogs = NULL;
    file_nethogs = g_file_new_for_path( "/usr/sbin/nethogs" );

    if (g_file_query_exists( file_nethogs, NULL )) {
       selected_commandline = g_strdup( gtk_menu_item_get_label(menu_item) );
    } else {
       selected_commandline = g_strdup_printf( "/usr/share/indicator-netspeed-unity/install_nethogs.sh %s", gtk_widget_get_name(GTK_WIDGET(menu_item)) );
    }

    appinfo = g_app_info_create_from_commandline( selected_commandline, NULL, G_APP_INFO_CREATE_NEEDS_TERMINAL, NULL );
    g_app_info_launch( appinfo, NULL, NULL, NULL );

    g_free( selected_commandline );
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

    for(int i = 0; i < netlist.number; i++) {
        if (strcmp( "lo", interfaces[i] ) == 0)
            continue;

        gchar *tmp_str = g_strdup( interfaces[i] );
        gchar *tmp_str2;
        if (show_settings)
           tmp_str2 = g_strdup( interfaces[i] );
        if_item = gtk_menu_item_new_with_label( interfaces[i] );
        gtk_widget_set_name( if_item, tmp_str );
        g_free( tmp_str );
        gtk_menu_shell_append( GTK_MENU_SHELL(interfaces_menu), if_item );
        g_signal_connect( G_OBJECT(if_item), "activate", G_CALLBACK(if_signal_select), NULL );

        if (show_settings) {
          gchar *tmp_str3 = g_strdup_printf( "%s %s", "sudo nethogs", tmp_str2 );
          if_tools_item = gtk_menu_item_new_with_label( tmp_str3 );
          g_free( tmp_str3 );
          gtk_widget_set_name( if_tools_item, tmp_str2 );
          g_free( tmp_str2 );
          gtk_menu_shell_append( GTK_MENU_SHELL(tools_sub), if_tools_item );
          g_signal_connect( G_OBJECT(if_tools_item), "activate", G_CALLBACK(if_tools_nethogs_activate), NULL );
        }
    }

    if_item = gtk_menu_item_new_with_label( _(text_All) );
    gtk_widget_set_name( if_item, _(text_All) );
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

    gchar *tmp_s1;
    gchar *tmp_s2;
    gchar *tmp_s1p;
    gchar *tmp_s2p;

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

    for(int i = 0; i < netlist.number; i++) {
        if (strcmp( "lo", interfaces[i] ) == 0) {
            continue;
        }

        glibtop_get_netload( &netload, interfaces[i] );

        if (strcmp( text_All, selected_if_name ) == 0 || strcmp( selected_if_name, interfaces[i] ) == 0) {
            bytes_in += netload.bytes_in;
            bytes_out += netload.bytes_out;
        }

        GList* l;
        for(l = if_items_list; l; l = l->next) {
            GtkWidget* item_menu = l->data;
            if (item_menu && (0 == strcmp( gtk_widget_get_name(item_menu), interfaces[i]) )) {
              gchar *strtmp = NULL;

              tmp_s1 = format_net_label( "↓:", (double)netload.bytes_in, show_bin_dec_bit, true, false );
              tmp_s2 = format_net_label( "↑:", (double)netload.bytes_out, show_bin_dec_bit, true, false );

              tmp_s1p = tmp_s1;
              tmp_s2p = tmp_s2;

              strtmp = g_strdup_printf( "%s: %s, %s", gtk_widget_get_name(item_menu), tmp_s1, tmp_s2 );
              g_free( tmp_s1p );
              g_free( tmp_s2p );
              gtk_menu_item_set_label( GTK_MENU_ITEM(item_menu), strtmp );
              g_free( strtmp );
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
}

gboolean update() {
    //get sum of up and down net traffic and separate values
    //and refresh labels of current interface
    guint64 net_traffic[2] = {0, 0};
    get_net( net_traffic );
    guint64 net_down = net_traffic[0];
    guint64 net_up = net_traffic[1];
    guint64 net_total = net_down + net_up;

    gchar *label_guide = "";//g_strdup_printf("↓:%s %s ↑:%s %s", "10000.00 ", _("MiB/s"), "10000.00 ", _("MiB/s"));   //maximum length label text, doesn't really work atm
    gchar *indicator_label;

    gchar *tmp_s1;
    gchar *tmp_s2;
    gchar *tmp_s1p;
    gchar *tmp_s2p;

    if (posit_item == 0) {
        tmp_s1 = format_net_label( "↓:", (double)net_down, show_bin_dec_bit, false, padding_indicator );
        tmp_s2 = format_net_label( "↑:", (double)net_up, show_bin_dec_bit, false, padding_indicator );

        tmp_s1p = tmp_s1;
        tmp_s2p = tmp_s2;
        indicator_label = g_strdup_printf( "%s/%s %s/%s", tmp_s1, _("s"), tmp_s2, _("s") );
        g_free( tmp_s1p );
        g_free( tmp_s2p );
    }
    else if (posit_item == 1)
    {
        tmp_s1 = format_net_label( "↓:", (double)net_down, show_bin_dec_bit, false, padding_indicator );
        tmp_s1p = tmp_s1;
        indicator_label = g_strdup_printf( "%s/%s", tmp_s1, _("s") );
        g_free( tmp_s1p );
    }
    else if (posit_item == 2)
    {
        tmp_s1 = format_net_label( "↑:", (double)net_up, show_bin_dec_bit, false, padding_indicator );
        tmp_s1p = tmp_s1;
        indicator_label = g_strdup_printf( "%s/%s", tmp_s1, _("s") );
        g_free( tmp_s1p );
    }
    else if (posit_item == 3)
    {
        indicator_label = format_net_label( "", (double)net_total, show_bin_dec_bit, false, padding_indicator );
    }

    app_indicator_set_label( indicator, indicator_label, label_guide );
    g_free( indicator_label );
    //g_free( label_guide );

    gchar *net_down_label = format_net_label ( "", (double)net_down, show_bin_dec_bit, true, false );
    gtk_menu_item_set_label( GTK_MENU_ITEM(net_down_item), net_down_label );
    g_free( net_down_label );

    gchar *net_up_label = format_net_label( "", (double)net_up, show_bin_dec_bit, true, false );
    gtk_menu_item_set_label( GTK_MENU_ITEM(net_up_item), net_up_label );
    g_free( net_up_label );

    if (posit_item == 0) {
        gchar *net_total_label = format_net_label( "", (double)net_total, show_bin_dec_bit, true, false );
        gtk_menu_item_set_label( GTK_MENU_ITEM(net_total_item), net_total_label );
        g_free( net_total_label );
    }
    else {
        tmp_s1 = format_net_label( "↓:", (double)net_down, show_bin_dec_bit, true, false );
        tmp_s2 = format_net_label( "↑:", (double)net_up, show_bin_dec_bit, true, false );

        tmp_s1p = tmp_s1;
        tmp_s2p = tmp_s2;
        gchar *net_total_label = g_strdup_printf( "%s/%s %s/%s", tmp_s1, _("s"), tmp_s2, _("s") );
        gtk_menu_item_set_label( GTK_MENU_ITEM(net_total_item), net_total_label );
        g_free( tmp_s1p );
        g_free( tmp_s2p );
        g_free( net_total_label );
    }

    if (net_down && net_up) {
        app_indicator_set_icon( indicator, pic_network_transmit_receive );
    }
    else if (net_down) {
        app_indicator_set_icon( indicator, pic_network_receive );
    }
    else if (net_up) {
        app_indicator_set_icon( indicator, pic_network_transmit );
    }
    else {
        app_indicator_set_icon( indicator, pic_network_idle );
    }

    return TRUE;
}

int main (int argc, char **argv)
{
    bindtextdomain( GETTEXT_PACKAGE, LOCALEDIR );
    textdomain( GETTEXT_PACKAGE );

    gtk_init ( &argc, &argv );

    settings = g_settings_new( "apps.indicators.netspeed-unity" );
    selected_if_name = g_strdup( gsettings_get_value_string( "if-name") );

    pictures_of_the_current_theme = gsettings_get_value_boolean( "pictures-of-the-current-theme" );
    padding_indicator = gsettings_get_value_boolean( "padding-indicator" );
    show_settings = gsettings_get_value_boolean( "show-settings" );
    if (pictures_of_the_current_theme) {
      pic_network_receive = gsettings_get_value_string( "pic-theme-network-receive" );
      net_down_icon = gtk_image_new_from_icon_name( pic_network_receive, GTK_ICON_SIZE_MENU );
      pic_network_transmit = gsettings_get_value_string( "pic-theme-network-transmit" );
      net_up_icon = gtk_image_new_from_icon_name( pic_network_transmit, GTK_ICON_SIZE_MENU );
      pic_network_transmit_receive = gsettings_get_value_string( "pic-theme-network-transmit-receive" );
      net_total_icon = gtk_image_new_from_icon_name( pic_network_transmit_receive, GTK_ICON_SIZE_MENU );
      pic_network_idle = gsettings_get_value_string( "pic-theme-network-idle" );
    } else {
      pic_network_receive = gsettings_get_value_string( "pic-file-network-receive" );
      net_down_icon = gtk_image_new_from_file( pic_network_receive);
      pic_network_transmit = gsettings_get_value_string( "pic-file-network-transmit" );
      net_up_icon = gtk_image_new_from_file( pic_network_transmit );
      pic_network_transmit_receive = gsettings_get_value_string( "pic-file-network-transmit-receive" );
      net_total_icon = gtk_image_new_from_file( pic_network_transmit_receive );
      pic_network_idle = gsettings_get_value_string( "pic-file-network-idle" );
    }
    posit_item = gsettings_get_value_gint32( "state" );
    show_bin_dec_bit = gsettings_get_value_gint32( "show-bin-dec-bit" );

    indicator_menu = gtk_menu_new();

    //add interfaces menu
    interfaces_menu = gtk_menu_new();

    //add interface names
    if (strcmp( text_All, selected_if_name ) == 0) {
      if_chosen = gtk_menu_item_new_with_label( _(text_All) );
    } else if_chosen = gtk_menu_item_new_with_label( selected_if_name );
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
       else if (strcmp( pic_network_idle, "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-d.svg" ) == 0)
              gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(theme_dark_item), TRUE );
       else if (strcmp( pic_network_idle, "/usr/share/pixmaps/indicator-netspeed-unity/indicator-netspeed-idle-l.svg" ) == 0)
              gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(theme_light_item), TRUE );

       g_signal_connect( G_OBJECT(theme_dark_item), "toggled", G_CALLBACK(theme_dark_item_toggled), NULL );
       g_signal_connect( G_OBJECT(theme_light_item), "toggled", G_CALLBACK(theme_light_item_toggled), NULL );
       g_signal_connect( G_OBJECT(theme_current_item), "toggled", G_CALLBACK(theme_current_item_toggled), NULL );
//---
     padding_item = gtk_check_menu_item_new_with_label( _("Padding") );
     gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(padding_item), padding_indicator );
     gtk_menu_shell_append( GTK_MENU_SHELL(settings_sub), padding_item );
     g_signal_connect( G_OBJECT(padding_item), "toggled", G_CALLBACK(padding_item_toggled), NULL );
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
    indicator = app_indicator_new( "netspeed", pic_network_idle, APP_INDICATOR_CATEGORY_SYSTEM_SERVICES );
    app_indicator_set_status( indicator, APP_INDICATOR_STATUS_ACTIVE );
    app_indicator_set_label( indicator, "netspeed", "netspeed" );
    app_indicator_set_menu( indicator, GTK_MENU(indicator_menu) );

    //set indicator position. default: all the way left
    //TODO: make this optional so placement can be automatic
    guint32 ordering_index = gsettings_get_value_gint32( "ordering-index" );
    app_indicator_set_ordering_index( indicator, ordering_index );

    update();

    /* update period in milliseconds */
    g_timeout_add( 1000*period, update, NULL );

    gtk_main ();

    return 0;
}

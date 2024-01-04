#include "glib.h"
#include "gtk/gtk.h"

#include "mpvradio-common.h"
#include "mpvradio-banner.h"

extern GHashTable *playlist_table;

/*
 * flow_box の選択された要素上で何か起きた
 */
static gboolean child_selected_change = FALSE;

/*
 * 子ウィジェットが GtkLabel だったら、再生する
 */
static void
checkchild (GtkWidget *widget, gpointer data)
{
    gpointer url;
    //~ if (GTK_IS_LABEL (widget)) {
    if (MPVRADIO_IS_BANNER (widget)) {
        //~ url = g_hash_table_lookup (playlist_table, gtk_label_get_text (widget));
        url = g_hash_table_lookup (playlist_table, mpvradio_banner_get_name (widget));
        //~ g_print ("label : %s   url : %s\n",
                    //~ gtk_label_get_text (widget), (gchar*)url);
        //~ g_idle_add (mpvradio_common_mpv_play, url);
        mpvradio_common_mpv_play (url);
    }
}

static void
child_activated_cb (GtkFlowBox      *box,
                           GtkFlowBoxChild *child,
                           gpointer         user_data)
{
    // フラグを参照して同一局の連続呼び出しを避ける
    if (child_selected_change == TRUE ||
            mpvradio_common_check_mpv_is_play () == FALSE) {
        child_selected_change = FALSE;
        gtk_container_foreach (GTK_CONTAINER(child), checkchild, NULL);
    }
}

static void
selected_children_changed_cb (GtkFlowBox      *box,
                           gpointer         user_data)
{
    // このイベントはchild_activatedに先行する
    //~ g_print ("selected_children_changed detect. \n");
    child_selected_change = TRUE;
}


/*
 * 選局ボタンを並べたgtk_flow_boxを返す
 */
GtkWidget *
mpvradio_radiopanel_new (void)
{
    GtkWidget *btn, *grid;
    gpointer station, url;

    grid = gtk_flow_box_new ();
    gtk_flow_box_set_selection_mode (GTK_FLOW_BOX(grid),GTK_SELECTION_SINGLE);
    gtk_flow_box_set_homogeneous (GTK_FLOW_BOX(grid), TRUE);
    gtk_flow_box_set_activate_on_single_click (GTK_FLOW_BOX(grid), TRUE);
    gtk_flow_box_set_row_spacing (GTK_FLOW_BOX(grid), 2);
    gtk_flow_box_set_column_spacing (GTK_FLOW_BOX(grid), 2);
	gtk_flow_box_set_max_children_per_line (GTK_FLOW_BOX(grid), 6);
    // ラベルにてボタンクリックと等価の動作を行うための準備
    g_signal_connect (G_OBJECT(grid), "child-activated",
                G_CALLBACK(child_activated_cb), NULL);

    // カーソルキーで移動する毎に生じるイベント
    g_signal_connect (G_OBJECT(grid), "selected-children-changed",
                G_CALLBACK(selected_children_changed_cb), NULL);

    /*
     * playlist_table をチェックして選局ボタンを並べる
     */
    GList *playlist_sorted, *curr;
    int i = 0;
    playlist_sorted = g_hash_table_get_keys (playlist_table);
    playlist_sorted = curr = g_list_sort (playlist_sorted, strcmp);
    for (;curr != NULL;curr = g_list_next (curr)) {
        if (curr->data != NULL) {
            url = g_hash_table_lookup (playlist_table, curr->data);
            
            //~ btn = gtk_label_new (curr->data);
            //~ gtk_label_set_width_chars (GTK_LABEL(btn), 10);
            //~ gtk_label_set_max_width_chars (GTK_LABEL(btn), 20);
            //~ gtk_label_set_line_wrap (GTK_LABEL(btn), TRUE);

            btn = mpvradio_banner_new_with_data (GTK_ORIENTATION_VERTICAL,2,curr->data,NULL);// url not use
            gtk_flow_box_insert (GTK_FLOW_BOX(grid), btn, -1);
        }
    }
    g_list_free (playlist_sorted);
    return grid;
}


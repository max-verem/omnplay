## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@GTHREAD_CFLAGS@ @GTK_CFLAGS@ @OM_CFLAGS@ @CURL_CFLAGS@

bin_PROGRAMS = omnplay

omnplay_SOURCES = \
	main.c \
	support.c support.h \
	ui.c ui.h ui_utils.h \
	ui_buttons.c ui_buttons.h \
	opts.c opts.h \
	playlist.c \
	library.c \
	whois.c \
	timecode.c timecode.h \
	omnplay.cpp omnplay.h

omnplay_LDADD = @GTHREAD_LIBS@ @GTK_LIBS@ @OM_LIBS@ @CURL_LIBS@ -lpthread

pixmap2_internal = \
    Axialis_Team_item_add_16x116_png            ../pixmaps/Axialis_Team_item_add_16x116.png \
    Axialis_Team_item_add_from_library_16x16_png ../pixmaps/Axialis_Team_item_add_from_library_16x16.png \
    Axialis_Team_item_block_16x116_png          ../pixmaps/Axialis_Team_item_block_16x116.png \
    Axialis_Team_item_delete_16x116_png         ../pixmaps/Axialis_Team_item_delete_16x116.png \
    Axialis_Team_item_edit_16x116_png           ../pixmaps/Axialis_Team_item_edit_16x116.png \
    Axialis_Team_item_loop_16x116_png           ../pixmaps/Axialis_Team_item_loop_16x116.png \
    Axialis_Team_item_move_down_16x116_png      ../pixmaps/Axialis_Team_item_move_down_16x116.png \
    Axialis_Team_item_move_up_16x116_png        ../pixmaps/Axialis_Team_item_move_up_16x116.png \
    Axialis_Team_library_refresh_16x16_png      ../pixmaps/Axialis_Team_library_refresh_16x16.png \
    Axialis_Team_player_cue_32x32_png           ../pixmaps/Axialis_Team_player_cue_32x32.png \
    Axialis_Team_player_pause_32x32_png         ../pixmaps/Axialis_Team_player_pause_32x32.png \
    Axialis_Team_player_play_64x32_png          ../pixmaps/Axialis_Team_player_play_64x32.png \
    Axialis_Team_player_stop_32x32_png          ../pixmaps/Axialis_Team_player_stop_32x32.png \
    Axialis_Team_playlist_open_16x16_png        ../pixmaps/Axialis_Team_playlist_open_16x16.png \
    Axialis_Team_playlist_relink_16x16_png      ../pixmaps/Axialis_Team_playlist_relink_16x16.png \
    Axialis_Team_playlist_save_16x16_png        ../pixmaps/Axialis_Team_playlist_save_16x16.png \
    Axialis_Team_search_in_library_16x16_png    ../pixmaps/Axialis_Team_search_in_library_16x16.png \
    Axialis_Team_search_next_16x16_png          ../pixmaps/Axialis_Team_search_next_16x16.png \
    block_type_block_end_16x16_png              ../pixmaps/block_type_block_end_16x16.png \
    block_type_block_loop_16x16_png             ../pixmaps/block_type_block_loop_16x16.png \
    block_type_block_middle_16x16_png           ../pixmaps/block_type_block_middle_16x16.png \
    block_type_block_single_16x16_png           ../pixmaps/block_type_block_single_16x16.png \
    block_type_block_start_16x16_png            ../pixmaps/block_type_block_start_16x16.png \
    block_type_loop_end_16x16_png               ../pixmaps/block_type_loop_end_16x16.png \
    block_type_loop_middle_16x16_png            ../pixmaps/block_type_loop_middle_16x16.png \
    block_type_loop_start_16x16_png             ../pixmaps/block_type_loop_start_16x16.png

CLEANFILES = \
    pixmap2-internal.h

support.c: pixmap2-internal.h

pixmap2-internal.h:
	gdk-pixbuf-csource --raw --build-list $(pixmap2_internal) > pixmap2-internal.h

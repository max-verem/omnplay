## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@GTHREAD_CFLAGS@ @GTK_CFLAGS@ @OM_CFLAGS@

bin_PROGRAMS = omnplay

omnplay_SOURCES = \
	main.c \
	support.c support.h \
	ui.c ui.h ui_utils.h \
	ui_buttons.c ui_buttons.h \
	omnplay.cpp omnplay.h

omnplay_LDADD = @GTHREAD_LIBS@ @GTK_LIBS@ @OM_LIBS@

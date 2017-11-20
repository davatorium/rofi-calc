ACLOCAL_AMFLAGS=-I m4
plugindir=${libdir}/rofi/

plugin_LTLIBRARIES = rofi_calc.la

rofi_calc_la_SOURCES=\
	src/myplugin.cxx

rofi_calc_la_CXXFLAGS= @glib_CFLAGS@ @rofi_CFLAGS@
rofi_calc_la_LIBADD= @glib_LIBS@ @rofi_LIBS@
rofi_calc_la_LDFLAGS= -module -avoid-version

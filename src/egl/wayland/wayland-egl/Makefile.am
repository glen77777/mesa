pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = wayland-egl.pc

AM_CFLAGS = $(DEFINES) \
	    $(VISIBILITY_CFLAGS) \
	    $(WAYLAND_CFLAGS)

lib_LTLIBRARIES = libwayland-egl.la
noinst_HEADERS = wayland-egl-priv.h
libwayland_egl_la_SOURCES = wayland-egl.c
libwayland_egl_la_LDFLAGS = -version-info 1

TESTS = wayland-egl-symbols-check

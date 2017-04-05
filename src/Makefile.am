AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"
libnautilus_share_la_CFLAGS =				\
	-Wall						\
	-DG_LOG_DOMAIN=\"Nautilus-Share\"		\
	-DINTERFACES_DIR=\"$(interfacesdir)\"		\
	-DDATADIR=\"$(datadir)\"			\
	-I$(top_srcdir)					\
	-I$(top_builddir)	\
	$(WARN_CFLAGS)                                  \
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(NAUTILUS_CFLAGS)				\
	$(GLIB_CFLAGS)

nautilus_extension_LTLIBRARIES=libnautilus-share.la

libnautilus_share_la_SOURCES =  \
	nautilus-extension.c        \
	nautilus-extension.h      \
	shares.c \
	shares.h \
	property-page.c \
	property-page.h \
	nautilus-info-provider.c \
	nautilus-info-provider.h \
	installSamba.c \
	installSamba.h \
	nautilus-permissions.c \
	nautilus-permissions.h \
    property-page-class.h  \
    rule_user.c  \
    rule_user.h


libnautilus_share_la_LDFLAGS = -module -avoid-version
libnautilus_share_la_LIBADD  = $(GLIB_LIBS) $(NAUTILUS_LIBS)
LDADD = $(LIBINTL)

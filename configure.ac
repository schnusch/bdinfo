AC_INIT([bdinfo], [1.2], [https://github.com/Schnusch/bdinfo/])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects tar-ustar dist-xz])
AM_SILENT_RULES([yes])

AC_ARG_ENABLE([clipnames],      [AS_HELP_STRING([--disable-clipnames],
		[build modified static libbluray to extract clipnames from playlists])])
AC_ARG_ENABLE([manpage],        [AS_HELP_STRING([--disable-manpage],
		[do not install manpage])])
AC_ARG_ENABLE([required-langs], [AS_HELP_STRING([--enable-required-langs],
		[make language argument for extraction a required one])])
AC_ARG_WITH([objcopy],          [AS_HELP_STRING([--with-objcopy=<OBJCOPY>],
		[use OBJCOPY to renames symbols])])
AC_ARG_WITH([system-libbluray], [AS_HELP_STRING([--without-system-libbluray],
		[do not use system's libbluray])])

AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_INSTALL

AS_IF([test "$enable_clipnames" != "no"], [
	enable_clipnames=yes
	AC_DEFINE([GET_CLIPNAMES],, [enable extraction of clipnames])
])

AS_IF([test "$enable_manpage" != "no"], [
	enable_manpage=yes
])

AS_IF([test "$enable_required_langs" = "yes"], [
	AC_DEFINE([REQUIRE_LANGS],, [make LANGUAGES a required argument for -f and -x])
], [
	enable_required_langs=no
])

AS_IF([test "$with_system_libbluray" != "no"], [
	PKG_CHECK_MODULES([LIBBLURAY], [libbluray >= 0.9.0], [
		dnl system's libbluray detected and use automatically
		with_system_libbluray=yes
	], [
		AS_IF([test "$with_system_libbluray" = "yes"], [
			AC_MSG_ERROR([no compatible libbluray found])
		])
		dnl no compatible libbluray version detected, use internal one
		with_system_libbluray=no
	])
])

AS_IF([test "$enable_clipnames" = "yes" -o "$with_system_libbluray" = "no"], [
	AS_IF([test "$with_system_libbluray" = "yes"], [
		dnl use user-specified objcopy
		AS_IF([test "$with_objcopy" = "no" ], [
			AC_MSG_ERROR([objcopy is needed to rename symbols in the static libbluray but was disabled])
		])
		AS_IF([test "$with_objcopy" = "" ], [
			with_objcopy=objcopy
		])
		AC_CHECK_PROG(with_objcopy, [$with_objcopy], [$with_objcopy], no)
		AS_IF([test "$with_objcopy" = "no" ], [
			AC_MSG_ERROR([objcopy not found])
		])
		AC_SUBST([OBJCOPY], [$with_objcopy])
	])

	dnl libraries for modified static libbluray
	AC_CHECK_LIB([dl], [dlopen],, [
		AC_MSG_ERROR([dlopen not found])
	])
	PKG_CHECK_MODULES([LIBXML2], [libxml-2.0],, [
		AC_MSG_ERROR([libxml2 not found])
	])

	dnl TODO with-... for commands patch and git
	AC_CHECK_PROGS(gpatch, [gpatch patch], no)
	AS_IF([test "$gpatch" = "no"], [
		AC_MSG_ERROR([no supported patch found])
	])
	AC_CHECK_PROG(git, git, git, no)
	AS_IF([test "$git" = "no"], [
		AC_MSG_ERROR([git not found])
	])

	dnl init libbluray
	AC_MSG_NOTICE([preparing src/libbluray...])
	AS_IF([$git --git-dir=src/libbluray/.git reset --hard HEAD],, [
		AC_MSG_ERROR([failed to reset modified static libbluray])
	])
	AS_IF([$gpatch -Np1 -dsrc < mpls_parse.patch],, [
		AC_MSG_ERROR([failed to patch modified static libbluray])
	])
	AS_IF([(cd src/libbluray && ./bootstrap && ./configure \
			--disable-shared --enable-static --disable-udf --disable-bdjava \
			--disable-doxygen-doc --disable-examples --without-freetype \
			--without-fontconfig)],, [
		AC_MSG_ERROR([failed to configure modified static libbluray])
	])
])

AM_CONDITIONAL([ENABLE_CLIPNAMES],      [test "$enable_clipnames"      = "yes"])
AM_CONDITIONAL([ENABLE_MANPAGE],        [test "$enable_manpage"        = "yes"])
AM_CONDITIONAL([REQUIRE_LANGS],         [test "$enable_required_langs" = "yes"])
AM_CONDITIONAL([WITH_SYSTEM_LIBBLURAY], [test "$with_system_libbluray" = "yes"])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_MSG_NOTICE([  Summary:])
AC_MSG_NOTICE([  --------])
AC_MSG_NOTICE([  extract clipnames:         $enable_clipnames])
AC_MSG_NOTICE([  install manpage:           $enable_manpage])
AC_MSG_NOTICE([  require language argument: $enable_required_langs])
AC_MSG_NOTICE([  use system's libbluray:    $with_system_libbluray])
AS_IF([test "$with_system_libbluray" = "yes" -a "$enable_clipnames" = "yes"], [
	AC_MSG_NOTICE([  objcopy:                   $with_objcopy])
])

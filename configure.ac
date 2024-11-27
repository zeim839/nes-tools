AC_PREREQ([2.71])
AC_INIT([nes-tools], [0.1])
AC_CONFIG_SRCDIR([src/cpu6502.c])

AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# User-specified SDL2 headers path.
AC_ARG_WITH([sdl-headers],
  [AS_HELP_STRING([--with-sdl-headers=DIR],
                  [Specify the directory containing SDL2 headers])],
  [sdl_headers=$withval],
  [sdl_headers=""])

if test -n "$sdl_headers"; then
  CFLAGS="$CFLAGS -I$sdl_headers"
fi

# User-specified SDL2 libraries path.
AC_ARG_WITH([sdl-lib],
  [AS_HELP_STRING([--with-sdl-lib=DIR],
                  [Specify the directory containing the SDL2 library])],
  [sdl_lib=$withval],
  [sdl_lib=""])

if test -n "$sdl_lib"; then
  LDFLAGS="$LDFLAGS -L$sdl_lib"
fi

# User-specified SDL2_ttf headers path.
AC_ARG_WITH([sdl-ttf-headers],
  [AS_HELP_STRING([--with-sdl-ttf-headers=DIR],
                  [Specify the directory containing SDL2_ttf headers])],
  [sdl_ttf_headers=$withval],
  [sdl_ttf_headers=""])

if test -n "$sdl_ttf_headers"; then
  CFLAGS="$CFLAGS -I$sdl_ttf_headers"
fi

# User-specified SDL2 libraries path.
AC_ARG_WITH([sdl-ttf-lib],
  [AS_HELP_STRING([--with-sdl-ttf-lib=DIR],
                  [Specify the directory containing the SDL2_ttf library])],
  [sdl_ttf_lib=$withval],
  [sdl_ttf_lib=""])

if test -n "$sdl_ttf_lib"; then
  LDFLAGS="$LDFLAGS -L$sdl_ttf_lib"
fi

# Validate SDL2 installation.
AC_CHECK_HEADER([SDL_ttf.h],
  [AC_MSG_NOTICE([SDL2_ttf headers found])],
  [AC_MSG_ERROR([SDL2_ttf headers not found in $sdl_ttf_headers])])

AC_CHECK_LIB([SDL2_ttf], [TTF_Init],
  [AC_MSG_NOTICE([SDL2_ttf library found])],
  [AC_MSG_ERROR([SDL2_ttf library not found in $sdl_ttf_lib])])

AC_CHECK_HEADER([SDL.h],
  [AC_MSG_NOTICE([SDL2 headers found])],
  [AC_MSG_ERROR([SDL2 headers not found in $sdl_headers])])

AC_CHECK_LIB([SDL2], [SDL_Init],
  [AC_MSG_NOTICE([SDL2 library found])],
  [AC_MSG_ERROR([SDL2 library not found in $sdl_lib])])

# Checks for header files.
AC_CHECK_HEADERS([
        stdlib.h stdint.h time.h
        SDL.h SDL_ttf.h
        stdarg.h stdio.h
        ])

AC_CONFIG_FILES([
        Makefile
        src/Makefile
        src/audio/Makefile
        ])

AC_OUTPUT

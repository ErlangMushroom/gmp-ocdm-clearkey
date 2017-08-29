dnl Process this file with autoconf to produce a configure script.
AC_INIT(linaro-cdmi, 1.0, zoltan.kuscsik@linaro.org)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(cdmi/service.cpp)

AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror])

AC_PROG_CXX
AC_PROG_CC

AM_PROG_AR

AC_PROG_RANLIB


AC_CHECK_PROG(RPCGEN,rpcgen,rpcgen)

AX_CXX_COMPILE_STDCXX_11([noext])

AX_CHECK_COMPILE_FLAG([-std=c++11], [
                        CXXFLAGS="$CXXFLAGS -std=c++11"])
AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
[case "${enableval}" in
             yes) debug=true ;;
             no)  debug=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])

if  test x"$debug" = x"true"; then
  AC_SUBST(AM_CXXFLAGS, "-g3 -O0 -DDEBUG -Werror")
  AC_SUBST(AM_CFLAGS, "-g3 -O0 -DDEBUG -Werror")
else
  AC_SUBST(AM_CXXFLAGS, "-O2 -Werror")
  AC_SUBST(AM_CFLAGS, "-O2 -Werror")
fi


AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

AC_ARG_ENABLE(threads, AC_HELP_STRING([--enable-threads],
                      [make a threading prog (default is yes)]),
                    ac_enable_threads=$enableval,
                    ac_enable_threads=yes)

if test "$ac_enable_threads" = yes; then
  AC_DEFINE(USE_THREADS, 1, [use threads])
fi

AC_ARG_ENABLE(openssl, AC_HELP_STRING([--enable-openssl],
                      [enable openssl (default is yes)]),
                    ac_enable_openssl=$enableval,
                    ac_enable_openssl=yes)

if test "$ac_enable_openssl" = yes; then
  AC_DEFINE(USE_OPENSSL, 1, [use openssl])
fi

AM_CONDITIONAL(OPENSSL, test $ac_enable_openssl = yes)
AM_CONDITIONAL(THREADS, test $ac_enable_threads = yes)


AC_OUTPUT(Makefile cdmi-stub/Makefile rpc/Makefile cdmi/Makefile)

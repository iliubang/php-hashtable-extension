$Id$

PHP_ARG_ENABLE(linger_hashtable, whether to enable linger_hashtable support,
[  --enable-linger_hashtable           Enable linger_hashtable support])

if test "$PHP_LINGER_HASHTABLE" != "no"; then

  dnl # --with-linger_hashtable -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/linger_hashtable.h"  # you most likely want to change this
  dnl if test -r $PHP_LINGER_HASHTABLE/$SEARCH_FOR; then # path given as parameter
  dnl   LINGER_HASHTABLE_DIR=$PHP_LINGER_HASHTABLE
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for linger_hashtable files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       LINGER_HASHTABLE_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$LINGER_HASHTABLE_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the linger_hashtable distribution])
  dnl fi

  dnl # --with-linger_hashtable -> add include path
  dnl PHP_ADD_INCLUDE($LINGER_HASHTABLE_DIR/include)

  dnl # --with-linger_hashtable -> check for lib and symbol presence
  dnl LIBNAME=linger_hashtable # you may want to change this
  dnl LIBSYMBOL=linger_hashtable # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $LINGER_HASHTABLE_DIR/$PHP_LIBDIR, LINGER_HASHTABLE_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_LINGER_HASHTABLELIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong linger_hashtable lib version or lib not found])
  dnl ],[
  dnl   -L$LINGER_HASHTABLE_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  PHP_SUBST(LINGER_HASHTABLE_SHARED_LIBADD)
  PHP_NEW_EXTENSION(linger_hashtable, linger_hashtable.c, $ext_shared)
fi

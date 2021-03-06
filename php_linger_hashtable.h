/*
  +----------------------------------------------------------------------+
  | linger_hashtable                                                     |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: liubang <it.liubang@gmail.com>                               |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_LINGER_HASHTABLE_H
#define PHP_LINGER_HASHTABLE_H

extern zend_module_entry linger_hashtable_module_entry;
#define phpext_linger_hashtable_ptr &linger_hashtable_module_entry

#define PHP_LINGER_HASHTABLE_VERSION "1.0"

#ifdef PHP_WIN32
#	define PHP_LINGER_HASHTABLE_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_LINGER_HASHTABLE_API __attribute__ ((visibility("default")))
#else
#	define PHP_LINGER_HASHTABLE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define LINGER_HASHTABLE_PROPERTIES_NAME "_hashtable"
#define linger_efree(ptr) if(ptr) efree(ptr)

#if PHP_MAJOR_VERSION < 7
#	define LINGER_MAKE_STD_ZVAL(p)		MAKE_STD_ZVAL(p)
#	define LINGER_ALLOC_INIT_ZVAL		ALLOC_INIT_ZVAL
#	define linger_zval_ptr_dtor			zval_ptr_dtor
#	define linger_zval_add_ref_p		Z_ADDREF_P
#	define LINGER_ZVAL_STRINGL			ZVAL_STRINGL
#	define LINGER_ZVAL_STRING			ZVAL_STRING
#	define LINGER_RETURN_STRINGL		RETURN_STRINGL
#	define LINGER_RETURN_STRING			RETURN_STRING
#	define LINGER_RETVAL_STRINGL		RETVAL_STRINGL
#	define linger_get_this				getThis
#	define linger_call_user_function_ex call_user_function_ex
#else
#	define LINGER_MAKE_STD_ZVAL(p)		zval _stack_zval_##p; p = &(_stack_zval_##p)
#	define LINGER_ALLOC_INIT_ZVAL(p)	do{p = (zval *)emalloc(sizeof(zval)); bzero(p, sizeof(zval));}while(0)
#	define linger_zval_ptr_dtor(p)		zval_ptr_dtor(*p)
#	define linger_zval_add_ref_p(p)		Z_TRY_ADDREF_P(p)
#	define LINGER_ZVAL_STRINGL(z, s, l, dup)	ZVAL_STRINGL(z, s, l)
#	define LINGER_ZVAL_STRING(z, s, dup)		ZVAL_STRING(z, s)
#	define LINGER_RETURN_STRINGL(s, l, dup)		RETURN_STRINGL(z, l)
#	define LINGER_RETURN_STRING(s, dup)		RETURN_STRING(s)
#	define LINGER_RETVAL_STRINGL(s, l, dup)		RETVAL_STRINGL(s, l)
#	define linger_get_this()			Z_OBJ_P(getThis())
#	define LINGER_PHP_MAX_PARAMS_NUM 	20

static inline int linger_call_user_function_ex(HashTable *function_table, zval** object_pp, zval *function_name, zval **retval_ptr_ptr, uint32_t param_count, zval ***params, int no_separation, HashTable* symbol_table)
{
    zval real_params[LINGER_PHP_MAX_PARAMS_NUM];
    int i = 0;
    for (; i < param_count; i++) {
        real_params[i] = **params[i];
    }
    zval phpng_retval;
    *retval_ptr_ptr = &phpng_retval;
    zval *object_p = (object_pp == NULL) ? NULL : *object_pp;
    return call_user_function_ex(function_table, object_p, function_name, &phpng_retval, param_count, real_params, no_separation, NULL);
}

#endif

/*
  	Declare any global variables you may need between the BEGIN
	and END macros here:

ZEND_BEGIN_MODULE_GLOBALS(linger_hashtable)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(linger_hashtable)
*/

/* In every utility function you add that needs to use variables
   in php_linger_hashtable_globals, call TSRMLS_FETCH(); after declaring other
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as LINGER_HASHTABLE_G(variable).  You are
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define LINGER_HASHTABLE_G(v) TSRMG(linger_hashtable_globals_id, zend_linger_hashtable_globals *, v)
#else
#define LINGER_HASHTABLE_G(v) (linger_hashtable_globals.v)
#endif

#endif	/* PHP_LINGER_HASHTABLE_H */



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_linger_hashtable.h"

static int le_linger_hashtable;

zend_class_entry *hashtable_ce;
static int le_hashtable_descriptor;
static int freed = 0;
#define PHP_HASHTABLE_DESCRIPTOR_NAME "linger hashtable descriptor"
#define LINGER_HASHTABLE_PROPERTIES_NAME "_hashtable"

#define linger_efree(ptr) if(ptr) efree(ptr)

typedef struct entry_s {
	char *key;
	zval *value;
	struct entry_s *next;
} entry_t;

typedef struct hashtable_s {
	long size;
	long count;
	entry_t **table;
} hashtable_t;


/* Create hashtable */
static hashtable_t *ht_create(long size)
{
	hashtable_t *hashtable = NULL;
	long i;

	if (size < 1) {
		return NULL;
	}

	if ((hashtable = emalloc(sizeof(hashtable_t))) == NULL) {
		return NULL;
	}

	if ((hashtable->table = emalloc(sizeof(entry_t *) * size)) == NULL) {
		linger_efree(hashtable);
		return NULL;
	}

	for (i = 0; i < size; i++) {
		hashtable->table[i] = NULL;	
	}

	hashtable->size = size;
	hashtable->count = 0;
	return hashtable;
}

static inline long ht_hash(hashtable_t *hashtable, char *key)
{
	unsigned long hashval = 0;
	int i = 0;
	while (hashval < ULONG_MAX && i < strlen(key)) {
		hashval = hashval << 8;
		hashval += key[i];
		i++;
	}
	return hashval % hashtable->size;
}

static entry_t *ht_newpair(char *key, zval *value)
{
	entry_t *newpair;
	if ((newpair = emalloc(sizeof(entry_t))) == NULL) {
		return NULL;
	}

	if ((newpair->key = estrdup(key)) == NULL) {
		linger_efree(newpair);
		return NULL;	
	}

	MAKE_STD_ZVAL(newpair->value);
	ZVAL_ZVAL(newpair->value, value, 1, 0);
	newpair->next = NULL;
	return newpair;
}

static void ht_set(hashtable_t *hashtable, char *key, zval *value)
{
	int bin = 0;
	entry_t *newpair = NULL;
	entry_t *next = NULL;
	entry_t *last = NULL;

	bin = ht_hash(hashtable, key);
	next = hashtable->table[bin];
	while (next != NULL && next->key != NULL && strcmp(key, next->key)) {
		last = next;
		next = next->next;
	}

	if (next != NULL && next->key != NULL && strcmp(key, next->key) == 0) {
		linger_efree(next->value);
		next->value = estrdup(value);
	} else {
		newpair = ht_newpair(key, value);
		if (next == hashtable->table[bin]) {
			newpair->next = NULL;
			hashtable->table[bin] = newpair;
		} else if (next == NULL) {
			last->next = newpair;
		} else {
			newpair->next = next;
			last->next = newpair;
		}
		hashtable->count++;
	}
}

static zval *ht_get(hashtable_t *hashtable, char *key)
{
	int bin = 0;
	entry_t *pair;
	bin = ht_hash(hashtable, key);
	pair = hashtable->table[bin];
	while (pair != NULL && pair->key != NULL && strcmp(key, pair->key) > 0) {
		pair = pair->next;
	}

	if (pair == NULL || pair->key == NULL || strcmp(key, pair->key) != 0) {
		return NULL;
	}

	return pair->value;
}

static int ht_isset(hashtable_t *hashtable, char *key)
{
	int bin = 0;
	entry_t *pair;
	bin = ht_hash(hashtable, key);
	pair = hashtable->table[bin];
	while (pair != NULL && pair->key != NULL && strcmp(key, pair->key) > 0) {
		pair = pair->next;
	}
	if (pair == NULL || pair->key == NULL || strcmp(key, pair->key) != 0) {
		return -1;
	}
	return 0;
}

static int ht_del(hashtable_t *hashtable, char *key)
{
	if (hashtable->count <= 0) {
		return -1;
	}
	long bin = 0;
	entry_t *pair, *pre;
	bin = ht_hash(hashtable, key);
	pair = hashtable->table[bin];
	pre = pair;
	while (pair != NULL && pair->key != NULL && strcmp(key, pair->key) > 0) {
		pre = pair;
		pair = pair->next;
	}

	if (pair == NULL || pair->key == NULL || strcmp(key, pair->key) != 0) {
		return -1;
	} else {
		/* the head */
		if (pre == pair) {
			hashtable->table[bin] = pair->next;
		} else {
			pre->next = pair->next;	
		}
		linger_efree(pair->key);
		zval_ptr_dtor(&pair->value);
		linger_efree(pair);
		hashtable->count--;
		return 0;
	}
}

static void ht_destroy(hashtable_t *hashtable)
{
	entry_t *curr, *next;
	if (hashtable->count > 0) {
		for (long i = 0; i < hashtable->size; i++) {
			curr = hashtable->table[i];
			while (curr != NULL) {
				next = curr->next;
				linger_efree(curr->key);
				zval_ptr_dtor(&curr->value);
				linger_efree(curr);
				hashtable->count--;
				curr = next;
			}
		}
	}
	linger_efree(hashtable);
}

static void php_hashtable_descriptor_dotr(zend_rsrc_list_entry *rsrc TSRMLS_CC)
{
	if (!freed) {
		hashtable_t *hashtable = (hashtable_t *)rsrc->ptr;
		ht_destroy(hashtable);
		freed = 1;
	}
}


PHP_METHOD(linger_hashtable, __construct)
{
	hashtable_t *hashtable;
	long size;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &size) == FAILURE) {
		RETURN_FALSE;
	}
	hashtable = ht_create(size);
	if (hashtable == NULL) {
		RETURN_FALSE;
	}
	zval *hashtable_res;
	MAKE_STD_ZVAL(hashtable_res);
	ZEND_REGISTER_RESOURCE(hashtable_res, hashtable, le_hashtable_descriptor);
	zend_update_property(hashtable_ce, getThis(), ZEND_STRL(LINGER_HASHTABLE_PROPERTIES_NAME), hashtable_res TSRMLS_CC);
}

PHP_METHOD(linger_hashtable, set)
{
	char *key;
	long key_len;
	zval *value, *hrc;
	hashtable_t *hashtable;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &key, &key_len, &value) == FAILURE) {
		RETURN_FALSE;
	}
	hrc = zend_read_property(hashtable_ce, getThis(), ZEND_STRL(LINGER_HASHTABLE_PROPERTIES_NAME), 0 TSRMLS_CC);	
	ZEND_FETCH_RESOURCE(hashtable, hashtable_t *, &hrc, -1, PHP_HASHTABLE_DESCRIPTOR_NAME, le_hashtable_descriptor);
	if (!hashtable) {
		RETURN_FALSE;
	}
	ht_set(hashtable, key, value);
	RETURN_TRUE;	
}

PHP_METHOD(linger_hashtable, get)
{
	char *key;
	long key_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE) {
		RETURN_FALSE;
	}
	zval *hrc;
	hashtable_t *hashtable;
	hrc = zend_read_property(hashtable_ce, getThis(), ZEND_STRL(LINGER_HASHTABLE_PROPERTIES_NAME), 0 TSRMLS_CC);
	ZEND_FETCH_RESOURCE(hashtable, hashtable_t *, &hrc, -1, PHP_HASHTABLE_DESCRIPTOR_NAME, le_hashtable_descriptor);
	if (!hashtable) {
		RETURN_FALSE;
	}
	zval *retval;
	retval = ht_get(hashtable, key);
	if (retval == NULL) {
		RETURN_NULL();
	} else {
		RETURN_ZVAL(retval, 1, 0);
	}
}

PHP_METHOD(linger_hashtable, isset)
{
	char *key;
	long key_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE) {
		RETURN_FALSE;
	}
	zval *hrc;
	hashtable_t *hashtable;
	hrc = zend_read_property(hashtable_ce, getThis(), ZEND_STRL(LINGER_HASHTABLE_PROPERTIES_NAME), 0 TSRMLS_CC);
	ZEND_FETCH_RESOURCE(hashtable, hashtable_t *, &hrc, -1, PHP_HASHTABLE_DESCRIPTOR_NAME, le_hashtable_descriptor);
	if (!hashtable) {
		RETURN_FALSE;
	}
	if (ht_isset(hashtable, key) == 0) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

PHP_METHOD(linger_hashtable, del)
{
	char *key;
	long key_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE) {
		RETURN_FALSE;
	}
	zval *hrc;
	hashtable_t *hashtable;
	hrc = zend_read_property(hashtable_ce, getThis(), ZEND_STRL(LINGER_HASHTABLE_PROPERTIES_NAME), 0 TSRMLS_CC);
	ZEND_FETCH_RESOURCE(hashtable, hashtable_t *, &hrc, -1, PHP_HASHTABLE_DESCRIPTOR_NAME, le_hashtable_descriptor);
	if (!hashtable) {
		RETURN_FALSE;
	}
	if (ht_del(hashtable, key) == 0) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

PHP_METHOD(linger_hashtable, getCount)
{
	zval *hrc;
	hashtable_t *hashtable;
	hrc = zend_read_property(hashtable_ce, getThis(), ZEND_STRL(LINGER_HASHTABLE_PROPERTIES_NAME), 0 TSRMLS_CC);
	ZEND_FETCH_RESOURCE(hashtable, hashtable_t *, &hrc, -1, PHP_HASHTABLE_DESCRIPTOR_NAME, le_hashtable_descriptor);
	if (!hashtable) {
		RETURN_FALSE;
	}
	RETURN_LONG(hashtable->count);
}

PHP_METHOD(linger_hashtable, getSize)
{
	zval *hrc;
	hashtable_t *hashtable;
	hrc = zend_read_property(hashtable_ce, getThis(), ZEND_STRL(LINGER_HASHTABLE_PROPERTIES_NAME), 0 TSRMLS_CC);
	ZEND_FETCH_RESOURCE(hashtable, hashtable_t *, &hrc, -1, PHP_HASHTABLE_DESCRIPTOR_NAME, le_hashtable_descriptor);
	if (!hashtable) {
		RETURN_FALSE;
	}
	RETURN_LONG(hashtable->size);
}

PHP_METHOD(linger_hashtable, __destruct)
{

}

static zend_function_entry hashtable_method[] = {
	PHP_ME(linger_hashtable, __construct, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(linger_hashtable, __destruct, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_DTOR)
	PHP_ME(linger_hashtable, set, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(linger_hashtable, get, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(linger_hashtable, del, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(linger_hashtable, isset, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(linger_hashtable, getSize, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(linger_hashtable, getCount, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(linger_hashtable)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "Linger\\Hashtable", hashtable_method);
	hashtable_ce = zend_register_internal_class(&ce TSRMLS_CC);
	zend_declare_property_null(hashtable_ce, ZEND_STRL(LINGER_HASHTABLE_PROPERTIES_NAME), ZEND_ACC_PRIVATE TSRMLS_CC);
	le_hashtable_descriptor = zend_register_list_destructors_ex(
			php_hashtable_descriptor_dotr,
			NULL,
			PHP_HASHTABLE_DESCRIPTOR_NAME,
			module_number
			);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(linger_hashtable)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(linger_hashtable)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(linger_hashtable)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(linger_hashtable)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "linger_hashtable support", "enabled");
	php_info_print_table_end();

}
/* }}} */

/* }}} */

/* {{{ linger_hashtable_module_entry
 */
zend_module_entry linger_hashtable_module_entry = {
	STANDARD_MODULE_HEADER,
	"linger_hashtable",
	NULL,
	PHP_MINIT(linger_hashtable),
	PHP_MSHUTDOWN(linger_hashtable),
	PHP_RINIT(linger_hashtable),		
	PHP_RSHUTDOWN(linger_hashtable),	
	PHP_MINFO(linger_hashtable),
	PHP_LINGER_HASHTABLE_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_LINGER_HASHTABLE
ZEND_GET_MODULE(linger_hashtable)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

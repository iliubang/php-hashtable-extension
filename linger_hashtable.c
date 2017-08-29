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

static zend_object_handlers hashtable_object_handlers;

typedef struct entry_s {
    char *key;
    zval *value;
    struct entry_s *next;
	struct entry_s *last;
	struct entry_s *listNext;
	struct entry_s *listLast;
} entry_t;

typedef struct hashtable_s {
    long size;
    long count;
    entry_t **table;
	entry_t *head;
	entry_t *tail;
} hashtable_t;

/* Define hashtable object struct */
typedef struct _hashtable_object {
    zend_object std;
    hashtable_t *hashtable;
} hashtable_object;


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
	hashtable->head = NULL;
	hashtable->tail = NULL;
    return hashtable;
}

static inline unsigned int ht_hash(hashtable_t *hashtable, char *key)
{
    const char *ptr;
    unsigned int val;
    val = 0;
    ptr = key;
    while (*ptr != '\0') {
        unsigned int tmp;
        val = (val << 4) + (*ptr);
        if (tmp = (val & 0xf0000000)) {
            val = val ^ (tmp >> 24);
            val = val ^ tmp;
        }
        ptr++;
    }
    return val % hashtable->size;
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
	newpair->last = NULL;
	newpair->listNext = NULL;
	newpair->listLast = NULL;
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
		/* reset a value */
        linger_efree(next->value);
        next->value = estrdup(value);
    } else {
		/* insert a new element */
        newpair = ht_newpair(key, value);
		if (hashtable->head == NULL) {
			hashtable->head = newpair;
		}
		if (hashtable->tail == NULL) {
			hashtable->tail = newpair;
		} else {
			newpair->listLast = hashtable->tail;
			hashtable->tail->listNext = newpair;
			hashtable->tail = newpair;
		}
        if (next == hashtable->table[bin]) {
            newpair->next = NULL;
            hashtable->table[bin] = newpair;
        } else if (next == NULL) {
			newpair->last = last;
            last->next = newpair;
        } else {
            newpair->next = next;
			newpair->last = last;
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

static void hashtable_free_object_storage_handler(hashtable_object *ht_object TSRMLS_DC)
{
    zend_object_std_dtor(&ht_object->std TSRMLS_CC);
    ht_destroy(ht_object->hashtable);
    efree(ht_object);
}

zend_object_value hashtable_create_object_handler(zend_class_entry *class_type TSRMLS_DC)
{
    zend_object_value retval;
    hashtable_object *ht_object = emalloc(sizeof(hashtable_object));
    memset(ht_object, 0, sizeof(hashtable_object));
    ht_object->hashtable = ht_create(655350);
    zend_object_std_init(&ht_object->std, class_type TSRMLS_CC);
    retval.handle = zend_objects_store_put(
                        ht_object,
                        (zend_objects_store_dtor_t) zend_objects_destroy_object,
                        (zend_objects_free_object_storage_t) hashtable_free_object_storage_handler,
                        NULL
                        TSRMLS_CC
                    );
    retval.handlers = &hashtable_object_handlers;
    return retval;
}

static char *get_string_from_zval(zval *val)
{
	if (Z_TYPE_P(val) == IS_STRING) {
		return Z_STRVAL_P(val);
	} else {
		zval tmp = *val;
		zval_copy_ctor(&tmp);
		convert_to_string(&tmp);
		return Z_STRVAL(tmp);
	}
}

static zval *linger_hashtable_read_dimension(zval *object, zval *zv_offset, int type TSRMLS_DC)
{
    hashtable_object *intern = zend_object_store_get_object(object TSRMLS_CC);
    if (!zv_offset) {
        zend_throw_exception(NULL, "Cannot append to a hashtable", 0 TSRMLS_CC);
        return NULL;
    }

    char *offset = get_string_from_zval(zv_offset);
    return ht_get(intern->hashtable, offset);
}

static void linger_hashtable_write_dimension(zval *object, zval *zv_offset, zval *value TSRMLS_DC)
{
    hashtable_object *intern = zend_object_store_get_object(object TSRMLS_CC);
    char *offset = get_string_from_zval(zv_offset);
    return ht_set(intern->hashtable, offset, value);
}

static int linger_hashtable_has_dimension(zval *object, zval *zv_offset, int check_empty TSRMLS_DC)
{
    hashtable_object *intern = zend_object_store_get_object(object TSRMLS_CC);
    char *offset = get_string_from_zval(zv_offset);
    if (ht_isset(intern->hashtable, offset) == 0) {
        if (check_empty) {
            zval *value = ht_get(intern->hashtable, offset);
            int retval;
            retval = zend_is_true(value);
            zval_ptr_dtor(&value);
            return retval;
        }
    } else {
        return 0;
    }
}

static void linger_hashtable_unset_dimension(zval *object, zval *zv_offset TSRMLS_DC)
{
    hashtable_object *intern = zend_object_get_store_object(object TSRMLS_CC);
    char *offset = get_string_from_zval(zv_offset);
    ht_del(intern->hashtable, offset);
}

static int linger_hashtable_count_elements(zval *object, long *count TSRMLS_DC)
{
    hashtable_object *intern = zend_object_store_get_object(object TSRMLS_CC);
    *count = intern->hashtable->count;
    return SUCCESS;
}

PHP_METHOD(linger_hashtable, set)
{
    char *key;
    long key_len;
    zval *value, *hrc;
    hashtable_object *ht_obj;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &key, &key_len, &value) == FAILURE) {
        RETURN_FALSE;
    }
    ht_obj = zend_object_store_get_object(getThis() TSRMLS_CC);
    ht_set(ht_obj->hashtable, key, value);
    RETURN_TRUE;
}

PHP_METHOD(linger_hashtable, get)
{
    char *key;
    long key_len;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }
    hashtable_object *ht_obj;
    ht_obj = zend_object_store_get_object(getThis() TSRMLS_CC);
    zval *retval;
    retval = ht_get(ht_obj->hashtable, key);
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
    hashtable_object *ht_obj;
    ht_obj = zend_object_store_get_object(getThis() TSRMLS_CC);
    if (ht_isset(ht_obj->hashtable, key) == 0) {
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
    hashtable_object *ht_obj;
    ht_obj = zend_object_store_get_object(getThis() TSRMLS_CC);
    if (ht_del(ht_obj->hashtable, key) == 0) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

PHP_METHOD(linger_hashtable, count)
{
    hashtable_object *ht_obj;
    ht_obj = zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_LONG(ht_obj->hashtable->count);
}

PHP_METHOD(linger_hashtable, foreach)
{
    zval *func;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &func) == FAILURE) {
        RETURN_FALSE;
    }
    char *func_name;
    if (!zend_is_callable(func, 0, &func_name TSRMLS_CC)) {
        php_error_dcref(NULL TSRMLS_CC, E_ERROR, "Fuction %s is not callable", func_name);
        RETURN_FALSE;
    }
    hashtable_object *ht_obj;
    hashtable_t *hashtable;
    ht_obj = zend_object_store_get_object(getThis() TSRMLS_CC);
    hashtable = ht_obj->hashtable;
    if (hashtable->count <= 0) {
        RETURN_TRUE;
    } else {
        zval **arg[2];
        zval *param1, *param2, *retval;
        MAKE_STD_ZVAL(param1);
        MAKE_STD_ZVAL(param2);
        entry_t *curr;
		curr = hashtable->head;
		while (curr != NULL) {
            ZVAL_STRING(param1, curr->key, 1);
            ZVAL_ZVAL(param2, curr->value, 1, 0);
            arg[0] = &param1;
            arg[1] = &param2;
            if (call_user_function_ex(EG(function_table), NULL, func, &retval, 2, arg, 0, NULL TSRMLS_CC) != SUCCESS) {
                php_error_docref(NULL TSRMLS_CC, E_ERROR, "call function error!");
            }
            curr = curr->listNext;
		}
        zval_ptr_dtor(&param1);
        zval_ptr_dtor(&param2);
        RETURN_TRUE;
    }
}

PHP_METHOD(linger_hashtable, __construct)
{

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
    PHP_ME(linger_hashtable, count, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(linger_hashtable, foreach, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(linger_hashtable)
{
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Linger\\Hashtable", hashtable_method);
    hashtable_ce = zend_register_internal_class(&ce TSRMLS_CC);
    hashtable_ce->create_object = hashtable_create_object_handler;
    memcpy(&hashtable_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    hashtable_object_handlers.read_dimension = linger_hashtable_read_dimension;
    hashtable_object_handlers.write_dimension = linger_hashtable_write_dimension;
    hashtable_object_handlers.has_dimension = linger_hashtable_has_dimension;
    hashtable_object_handlers.unset_dimension = linger_hashtable_unset_dimension;
    hashtable_object_handlers.count_elements = linger_hashtable_count_elements;
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
    php_info_print_table_row(2, "linger_hashtable support", "enabled");
    php_info_print_table_row(2, "version", PHP_LINGER_HASHTABLE_VERSION);
    php_info_print_table_row(2, "author", "liubang <it.liubang@gmail.com>");
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

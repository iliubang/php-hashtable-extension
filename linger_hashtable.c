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

extern zend_class_entry *zend_ce_traversable;
static int le_linger_hashtable;
zend_class_entry *hashtable_ce;
static zend_object_handlers hashtable_object_handlers;

typedef struct _bucket {
    char *key;
    zval *value;
    struct _bucket *next;
    struct _bucket *last;
    struct _bucket *listNext;
    struct _bucket *listLast;
} bucket;

typedef struct _hashtable_s {
    long size;
    long count;
    bucket **table;
    bucket *head;
    bucket *tail;
} hashtable_t;

typedef struct _hashtable_object hashtable_object;
typedef struct _hashtable_iterator hashtable_iterator;

#if PHP_MAJOR_VERSION < 7
/* Define hashtable object struct */
struct _hashtable_object {
    zend_object std;
    hashtable_t *hashtable;
};
#else
struct _hashtable_object {
    hashtable_t *hashtable;
    zend_object std;
};
#endif

struct _hashtable_iterator {
    zend_object_iterator intern;
    hashtable_t *hashtable;
    char *offset;
    zval *current;
};

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
    if ((hashtable->table = ecalloc(size, sizeof(bucket *))) == NULL) {
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

static bucket *ht_newpair(char *key, zval *value)
{
    bucket *newpair;
    if ((newpair = emalloc(sizeof(bucket))) == NULL) {
        return NULL;
    }

    if ((newpair->key = estrdup(key)) == NULL) {
        linger_efree(newpair);
        return NULL;
    }
    zval *newpair_value;
    LINGER_ALLOC_INIT_ZVAL(newpair_value);
    ZVAL_ZVAL(newpair_value, value, 1, 0);
    newpair->value = newpair_value;
    linger_zval_add_ref_p(newpair_value);
    newpair->next = NULL;
    newpair->last = NULL;
    newpair->listNext = NULL;
    newpair->listLast = NULL;
    return newpair;
}

static void ht_set(hashtable_t *hashtable, char *key, zval *value)
{
    int bin = 0;
    bucket *newpair = NULL;
    bucket *next = NULL;
    bucket *last = NULL;

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

static bucket *ht_get_bucket(hashtable_t *hashtable, char *key)
{
    int bin = 0;
    bucket *pair;
    bin = ht_hash(hashtable, key);
    pair = hashtable->table[bin];
    while (pair != NULL && pair->key != NULL && strcmp(key, pair->key) > 0) {
        pair = pair->next;
    }
    if (pair == NULL || pair->key == NULL || strcmp(key, pair->key) != 0) {
        return NULL;
    }

    return pair;
}

static zval *ht_get_zval(hashtable_t *hashtable, char *key)
{
    bucket *pair;
    if ((pair = ht_get_bucket(hashtable, key)) == NULL) {
        return NULL;
    }
    return pair->value;
}

static int ht_isset(hashtable_t *hashtable, char *key)
{
    int bin = 0;
    bucket *pair;
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
    bucket *pair, *pre;
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
        linger_zval_ptr_dtor(&pair->value);
        linger_efree(pair);
        hashtable->count--;
        return 0;
    }
}

static void ht_destroy(hashtable_t *hashtable)
{
    bucket *curr, *next;
    if (hashtable->count > 0) {
        for (long i = 0; i < hashtable->size; i++) {
            curr = hashtable->table[i];
            while (curr != NULL) {
                next = curr->next;
                linger_efree(curr->key);
                linger_zval_ptr_dtor(&curr->value);
                linger_efree(curr);
                hashtable->count--;
                curr = next;
            }
        }
    }
    //linger_efree(hashtable);
}

static void hashtable_free_object_storage_handler(hashtable_object *ht_object TSRMLS_DC)
{
    zend_object_std_dtor(&ht_object->std TSRMLS_CC);
    ht_destroy(ht_object->hashtable);
    efree(ht_object);
}

#if PHP_MAJOR_VERSION < 7

#define linger_get_object(object)   zend_object_store_get_object(object TSRMLS_CC)

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
#else

#define linger_get_object(object)  (hashtable_object *)((char *)object - XtOffsetOf(hashtable_object, std))

zend_object *hashtable_create_object_handler(zend_class_entry *class_type TSRMLS_DC)
{
    size_t size = sizeof(hashtable_object) + zend_object_properties_size(class_type);
    hashtable_object *ht_object = emalloc(size);
    memset(ht_object, 0, size);
    ht_object->hashtable = ht_create(655350);
    zend_object_std_init(&ht_object->std, class_type TSRMLS_CC);
    ht_object->std.handlers = &hashtable_object_handlers;
    return &ht_object->std;
}

static void hashtable_destroy_object_handler(zend_object *object)
{
    hashtable_object *ht_object = linger_get_object(object);
    ht_destroy(ht_object->hashtable);
    zend_objects_destroy_object(object);
}

static void hashtable_free_object_handler(zend_object *object)
{
    hashtable_object *ht_object = linger_get_object(object);
    linger_efree(ht_object->hashtable);
    zend_object_std_dtor(object);
}
#endif

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
#if PHP_MAJOR_VERSION < 7
    hashtable_object *intern = linger_get_object(object);
#else
    hashtable_object *intern = linger_get_object(Z_OBJ_P(object));
#endif
    if (!zv_offset) {
        zend_throw_exception(NULL, "Cannot append to a hashtable", 0 TSRMLS_CC);
        return NULL;
    }

    char *offset = get_string_from_zval(zv_offset);
    return ht_get_zval(intern->hashtable, offset);
}

static void linger_hashtable_write_dimension(zval *object, zval *zv_offset, zval *value TSRMLS_DC)
{
#if PHP_MAJOR_VERSION < 7
    hashtable_object *intern = linger_get_object(object);
#else
    hashtable_object *intern = linger_get_object(Z_OBJ_P(object));
#endif
    char *offset = get_string_from_zval(zv_offset);
    return ht_set(intern->hashtable, offset, value);
}

static int linger_hashtable_has_dimension(zval *object, zval *zv_offset, int check_empty TSRMLS_DC)
{
#if PHP_MAJOR_VERSION < 7
    hashtable_object *intern = linger_get_object(object);
#else
    hashtable_object *intern = linger_get_object(Z_OBJ_P(object));
#endif
    char *offset = get_string_from_zval(zv_offset);
    if (ht_isset(intern->hashtable, offset) == 0) {
        if (check_empty) {
            zval *value = ht_get_zval(intern->hashtable, offset);
            int retval;
            retval = zend_is_true(value);
            linger_zval_ptr_dtor(&value);
            return retval;
        }
        return SUCCESS;
    } else {
        return FAILURE;
    }
}

static void linger_hashtable_unset_dimension(zval *object, zval *zv_offset TSRMLS_DC)
{
#if PHP_MAJOR_VERSION < 7
    hashtable_object *intern = linger_get_object(object);
#else
    hashtable_object *intern = linger_get_object(Z_OBJ_P(object));
#endif
    char *offset = get_string_from_zval(zv_offset);
    ht_del(intern->hashtable, offset);
}

static int linger_hashtable_count_elements(zval *object, long *count TSRMLS_DC)
{
#if PHP_MAJOR_VERSION < 7
    hashtable_object *intern = linger_get_object(object);
#else
    hashtable_object *intern = linger_get_object(Z_OBJ_P(object));
#endif
    *count = intern->hashtable->count;
    return SUCCESS;
}

PHP_METHOD(linger_hashtable, set)
{
    zval *key;
    zval *value, *hrc;
    hashtable_object *ht_obj;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &key, &value) == FAILURE) {
        RETURN_FALSE;
    }
    ht_obj = linger_get_object(linger_get_this());
    char *key_s = get_string_from_zval(key);
    ht_set(ht_obj->hashtable, key_s, value);
    RETURN_TRUE;
}

PHP_METHOD(linger_hashtable, get)
{
    zval *key;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &key) == FAILURE) {
        RETURN_FALSE;
    }
    hashtable_object *ht_obj;
    ht_obj = linger_get_object(linger_get_this());
    zval *retval;
    char *key_s = get_string_from_zval(key);
    retval = ht_get_zval(ht_obj->hashtable, key_s);
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
    ht_obj = linger_get_object(linger_get_this());
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
    ht_obj = linger_get_object(linger_get_this());
    if (ht_del(ht_obj->hashtable, key) == 0) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

PHP_METHOD(linger_hashtable, count)
{
    hashtable_object *ht_obj;
    ht_obj = linger_get_object(linger_get_this());
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
    ht_obj = linger_get_object(linger_get_this());
    hashtable = ht_obj->hashtable;
    if (hashtable->count <= 0) {
        RETURN_TRUE;
    } else {
        zval **arg[2];
        zval *param1, *param2, *retval;
        LINGER_MAKE_STD_ZVAL(param1);
        LINGER_MAKE_STD_ZVAL(param2);
        bucket *curr;
        curr = hashtable->head;
        while (curr != NULL) {
            LINGER_ZVAL_STRING(param1, curr->key, 1);
            ZVAL_ZVAL(param2, curr->value, 1, 0);
            arg[0] = &param1;
            arg[1] = &param2;
            if (linger_call_user_function_ex(EG(function_table), NULL, func, &retval, 2, arg, 0, NULL) != SUCCESS) {
                php_error_docref(NULL TSRMLS_CC, E_ERROR, "call function error!");
            }
            curr = curr->listNext;
        }
        linger_zval_ptr_dtor(&param1);
        linger_zval_ptr_dtor(&param2);
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

#if PHP_MAJOR_VERSION < 7
static void linger_hashtable_iterator_dtor(zend_object_iterator *intern TSRMLS_DC)
#else
static void linger_hashtable_iterator_dtor(zend_object_iterator *intern)
#endif
{
    hashtable_iterator *iterator = (hashtable_iterator *) intern;
    if (iterator->current) {
        linger_zval_ptr_dtor(&iterator->current);
    }
    linger_zval_ptr_dtor((zval **)&intern->data);
    linger_efree(iterator);
}

#if PHP_MAJOR_VERSION < 7
static int linger_hashtable_iterator_valid(zend_object_iterator *intern TSRMLS_DC)
#else
static int linger_hashtable_iterator_valid(zend_object_iterator *intern)
#endif
{
    hashtable_iterator *iterator = (hashtable_iterator *) intern;
    if (iterator->offset == NULL) {
        return FAILURE;
    }
    bucket *pair;
    if ((pair = ht_get_bucket(iterator->hashtable, iterator->offset)) == NULL) {
        return FAILURE;
    } else {
        return SUCCESS;
    }
}

#if PHP_MAJOR_VERION < 7
static void linger_hashtable_iterator_get_current_data(zend_object_iterator *intern, zval ***data TSRMLS_DC)
#else
static zval *linger_hashtable_iterator_get_current_data(zend_object_iterator *intern)
#endif
{
    hashtable_iterator *iterator = (hashtable_iterator *) intern;
    if (iterator->current) {
        linger_zval_ptr_dtor(&iterator->current);
    }
    iterator->current = ht_get_zval(iterator->hashtable, iterator->offset);
#if PHP_MAJOR_VERSION < 7
    *data = &iterator->current;
#else
    return iterator->current;
#endif
}

#if PHP_MAJOR_VERION < 7
static void linger_hashtable_iterator_get_current_key(zend_object_iterator *intern, zval *key TSRMLS_DC)
#else
static void linger_hashtable_iterator_get_current_key(zend_object_iterator *intern, zval *key)
#endif
{
    hashtable_iterator *iterator = (hashtable_iterator *)intern;
    LINGER_ZVAL_STRING(key, iterator->offset, 0);
}

#if PHP_MAJOR_VERION < 7
static void linger_hashtable_iterator_move_forward(zend_object_iterator *intern TSRMLS_DC)
#else
static void linger_hashtable_iterator_move_forward(zend_object_iterator *intern)
#endif
{
    hashtable_iterator *iterator = (hashtable_iterator *)intern;
    int bin = 0;
    bucket *pair;
    if ((pair = ht_get_bucket(iterator->hashtable, iterator->offset)) == NULL) {
        return;
    }
    if (pair->listNext == NULL) {
        iterator->offset = NULL;
    } else {
        iterator->offset = pair->listNext->key;
    }
}

#if PHP_MAJOR_VERSION < 7
static void linger_hashtable_iterator_rewind(zend_object_iterator *intern TSRMLS_DC)
#else
static void linger_hashtable_iterator_rewind(zend_object_iterator *intern)
#endif
{
    hashtable_iterator *iterator = (hashtable_iterator *) intern;
    if (iterator->hashtable->head) {
        iterator->offset = iterator->hashtable->head->key;
    } else {
        iterator->offset = NULL;
    }
    iterator->current = NULL;
}

static zend_object_iterator_funcs linger_hashtable_iterator_funcs = {
    linger_hashtable_iterator_dtor,
    linger_hashtable_iterator_valid,
    linger_hashtable_iterator_get_current_data,
    linger_hashtable_iterator_get_current_key,
    linger_hashtable_iterator_move_forward,
    linger_hashtable_iterator_rewind,
    NULL
};

zend_object_iterator *linger_hashtable_get_iterator(zend_class_entry *ce, zval *object, int by_ref TSRMLS_DC)
{
    hashtable_iterator *iterator;
    if (by_ref) {
        zend_throw_exception(NULL, "Cannot iterate by refererce", 0 TSRMLS_CC);
        return NULL;
    }
    iterator = ecalloc(1, sizeof(hashtable_iterator));
#if PHP_MAJOR_VERSION < 7
    hashtable_object *obj = linger_get_object(object);
    iterator->intern.data = object;
    linger_zval_add_ref_p(object);
#else
    zend_iterator_init(&iterator->intern);
    hashtable_object *obj = linger_get_object(Z_OBJ_P(object));
    ZVAL_COPY(&iterator->intern.data, object);
#endif
    iterator->intern.funcs = &linger_hashtable_iterator_funcs;
    iterator->hashtable = obj->hashtable;
    if (obj->hashtable->head != NULL) {
        iterator->offset = obj->hashtable->head->key;
    } else  {
        iterator->offset = NULL;
    }
    iterator->current = NULL;
#if PHP_MAJOR_VERSION < 7
    return (zend_object_iterator *)iterator;
#else
    return &iterator->intern;
#endif
}

PHP_MINIT_FUNCTION(linger_hashtable)
{
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Linger\\Hashtable", hashtable_method);
    hashtable_ce = zend_register_internal_class(&ce TSRMLS_CC);
    hashtable_ce->create_object = hashtable_create_object_handler;
    hashtable_ce->get_iterator = linger_hashtable_get_iterator;
    hashtable_ce->iterator_funcs.funcs = &linger_hashtable_iterator_funcs;
    zend_class_implements(hashtable_ce TSRMLS_CC, 1, zend_ce_traversable);
    memcpy(&hashtable_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
#if PHP_MAJOR_VERSION >= 7
    hashtable_object_handlers.free_obj = hashtable_free_object_handler;
    hashtable_object_handlers.dtor_obj = hashtable_destroy_object_handler;
    hashtable_object_handlers.offset = XtOffsetOf(hashtable_object, std);
#endif
    hashtable_object_handlers.read_dimension = linger_hashtable_read_dimension;
    hashtable_object_handlers.write_dimension = linger_hashtable_write_dimension;
    hashtable_object_handlers.has_dimension = linger_hashtable_has_dimension;
    hashtable_object_handlers.unset_dimension = linger_hashtable_unset_dimension;
    hashtable_object_handlers.count_elements = linger_hashtable_count_elements;
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(linger_hashtable)
{
    return SUCCESS;
}

PHP_RINIT_FUNCTION(linger_hashtable)
{
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(linger_hashtable)
{
    return SUCCESS;
}

PHP_MINFO_FUNCTION(linger_hashtable)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "linger_hashtable support", "enabled");
    php_info_print_table_row(2, "version", PHP_LINGER_HASHTABLE_VERSION);
    php_info_print_table_row(2, "author", "liubang <it.liubang@gmail.com>");
    php_info_print_table_end();

}

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

#ifdef COMPILE_DL_LINGER_HASHTABLE
ZEND_GET_MODULE(linger_hashtable)
#endif


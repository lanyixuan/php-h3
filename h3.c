/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
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
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_h3.h"
#include <h3/h3api.h>

/* If you declare any globals in php_h3.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(h3)
*/

/* True global resources - no need for thread safety here */
static int le_h3;

// extension global var
H3Index current_indexed;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("h3.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_h3_globals, h3_globals)
    STD_PHP_INI_ENTRY("h3.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_h3_globals, h3_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
PHP_FUNCTION(geoToH3)
{
    zend_long resolution;
    double lat, lon;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ddl", &lat, &lon, &resolution) == FAILURE) {
        return;
    }

    GeoCoord location;
    location.lat = degsToRads(lat);
    location.lon = degsToRads(lon);

    current_indexed = geoToH3(&location, resolution);

  php_printf("geoToH3 indexed: %d\n", current_indexed);

    zend_resource *index_resource = zend_register_resource(&current_indexed, le_h3_index);

    RETURN_RES(index_resource);
}

PHP_FUNCTION(h3ToGeoBoundary)
{
    zval *index_resource_zval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &index_resource_zval) == FAILURE) {
        return;
    }

    H3Index *indexed = (H3Index *) Z_RES_VAL_P(index_resource_zval);
    // Get the vertices of the H3 index.
    GeoBoundary boundary;
    h3ToGeoBoundary(*indexed, &boundary);

    zval boundary_arr;
    ZVAL_NEW_PERSISTENT_ARR(&boundary_arr);
    zend_hash_init(Z_ARRVAL(boundary_arr), boundary.numVerts, NULL, ZVAL_PTR_DTOR, 1);
    // Indexes can have different number of vertices under some cases,
    // which is why boundary.numVerts is needed.
    for (int v = 0; v < boundary.numVerts; v++) {
        zval lat, lon;
        ZVAL_LONG(&lat, boundary.verts[v].lat);
        ZVAL_LONG(&lon, boundary.verts[v].lon);

        zval lat_lon_arr;
        ZVAL_NEW_PERSISTENT_ARR(&lat_lon_arr);
        zend_hash_init(Z_ARRVAL(lat_lon_arr), 2, NULL, ZVAL_PTR_DTOR, 1);
        zend_hash_index_add(Z_ARRVAL(lat_lon_arr), v, &lat);
        zend_hash_index_add(Z_ARRVAL(lat_lon_arr), v, &lon);

        zend_hash_index_add(Z_ARRVAL(boundary_arr), v, &lat_lon_arr);
    }

    RETURN_ARR(Z_ARRVAL(boundary_arr));
}

PHP_FUNCTION(h3ToGeo)
{
    zval *index_resource_zval;
    zend_long resolution;
    double lat, lon;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &index_resource_zval) == FAILURE) {
        return;
    }

    H3Index *indexed = (H3Index *) Z_RES_VAL_P(index_resource_zval);

    // Get the center coordinates.
    GeoCoord center;
    h3ToGeo(*indexed, &center);

    lat = radsToDegs(center.lat);
    lon = radsToDegs(center.lon);

    zval lat_zval, lon_zval;
    ZVAL_DOUBLE(&lat_zval, radsToDegs(center.lat));
    ZVAL_DOUBLE(&lon_zval, radsToDegs(center.lon));

    zval lat_lon_arr;
    ZVAL_NEW_PERSISTENT_ARR(&lat_lon_arr);
    zend_hash_init(Z_ARRVAL(lat_lon_arr), 2, NULL, ZVAL_PTR_DTOR, 1);
    zend_hash_index_add(Z_ARRVAL(lat_lon_arr), 0, &lat_zval);
    zend_hash_index_add(Z_ARRVAL(lat_lon_arr), 1, &lon_zval);

    RETURN_ARR(Z_ARRVAL(lat_lon_arr));
}

PHP_FUNCTION(h3GetResolution)
{
    zval *index_resource_zval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &index_resource_zval) == FAILURE) {
        return;
    }
    H3Index *indexed = Z_RES_VAL_P(index_resource_zval);

  php_printf("h3GetResolution indexed: %d\n", *indexed);
    int resolution = h3GetResolution(*indexed);

  RETURN_LONG(resolution);
}

PHP_FUNCTION(h3GetBaseCell)
{
    zval *index_resource_zval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &index_resource_zval) == FAILURE) {
        return;
    }

    H3Index *indexed = (H3Index *) Z_RES_VAL_P(index_resource_zval);
    int base_cell = base_cell = h3GetBaseCell(*indexed);

  RETURN_LONG(base_cell);
}

PHP_FUNCTION(stringToH3)
{
    zval *index_resource_zval;
  char *str;
  size_t str_len;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &str, &str_len) == FAILURE) {
    return;
  }

    H3Index indexed = (H3Index) stringToH3(str);

    zend_resource *index_resource;
    index_resource = zend_register_resource(&indexed, le_h3_index);

    RETURN_RES(index_resource);
}

PHP_FUNCTION(h3ToString)
{
    zval *index_resource_zval;
  char *str;
  size_t str_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rs", &index_resource_zval, &str, &str_len) == FAILURE) {
        return;
    }
    H3Index *indexed = (H3Index *) Z_RES_VAL_P(index_resource_zval);

  h3ToString(*indexed, str, str_len);

  RETURN_STRING(str);
}

PHP_FUNCTION(h3IsValid)
{
    zval *index_resource_zval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &index_resource_zval) == FAILURE) {
        return;
    }

    H3Index *indexed = (H3Index *) Z_RES_VAL_P(index_resource_zval);

  if (h3IsValid(*indexed) == 0) {
    RETURN_FALSE;
  } else {
    RETURN_TRUE;
  }
}

PHP_FUNCTION(h3IsResClassIII)
{
    zval *index_resource_zval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &index_resource_zval) == FAILURE) {
        return;
    }

    H3Index *indexed = (H3Index *) Z_RES_VAL_P(index_resource_zval);
  php_printf("h3IsResClassIII indexed: %d\n", *indexed);

  if (h3IsResClassIII(*indexed) == 0) {
    RETURN_FALSE;
  } else {
    RETURN_TRUE;
  }
}

PHP_FUNCTION(h3IsPentagon)
{
    zval *index_resource_zval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &index_resource_zval) == FAILURE) {
        return;
    }

    H3Index *indexed = (H3Index *) Z_RES_VAL_P(index_resource_zval);
  php_printf("h3IsPentagon indexed: %d\n", *indexed);
  int is_valid = h3IsPentagon(*indexed);

  if (is_valid == 0) {
    RETURN_FALSE;
  } else {
    RETURN_TRUE;
  }
}

PHP_FUNCTION(kRing)
{
    zval *index_resource_zval;
  zend_long k;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl", &index_resource_zval, &k) == FAILURE) {
        return;
    }

  int arr_count = maxKringSize(k);
  H3Index out[arr_count];
    H3Index *indexed = (H3Index *) Z_RES_VAL_P(index_resource_zval);

  kRing(*indexed, k, out);

    zend_resource *out_resource = zend_register_resource(&out, le_h3_index);

    RETURN_RES(out_resource);
}

PHP_FUNCTION(maxKringSize)
{
  zend_long k;
  zend_long size;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &k) == FAILURE) {
        return;
    }

  size = maxKringSize(k);

  RETURN_LONG(size);
}

PHP_FUNCTION(kRingDistances)
{
  zend_long k;
    zval *index_resource_zval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl", &index_resource_zval, &k) == FAILURE) {
        return;
    }

  H3Index *indexed = (H3Index *) Z_RES_VAL_P(index_resource_zval);
  int arr_count = maxKringSize(k);

  H3Index outs[arr_count];
  int distances[arr_count];
  kRingDistances(*indexed, k, outs, distances);

  zval out_zvals, distance_zvals;
  array_init(&out_zvals);
  array_init(&distance_zvals);

  for (int i = 0; i < arr_count; i++) {
    zend_resource *out_resource = zend_register_resource(&outs[i], le_h3_index);
    zval out_zval;
    zval distance_zval;

    ZVAL_LONG(&distance_zval, &distances[i]);
    ZVAL_RES(&out_zval, out_resource);

    zend_hash_index_add(Z_ARRVAL(out_zvals), i, &out_zval);
    zend_hash_index_add(Z_ARRVAL(distance_zvals), i, &distance_zval);
  }

  zval outs_and_distances;
  array_init(&outs_and_distances);
  zend_hash_index_add(Z_ARRVAL(outs_and_distances), 0, &out_zvals);
  zend_hash_index_add(Z_ARRVAL(outs_and_distances), 1, &distance_zvals);

  RETURN_ARR(Z_ARRVAL(outs_and_distances));

  char *hello = "hello";
  RETURN_STRING(hello)
}

PHP_FUNCTION(h3Distance)
{
    zval *origin_resource_zval;
    zval *h3_resource_zval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rr", &origin_resource_zval, &h3_resource_zval) == FAILURE) {
        return;
    }

    H3Index *origin = (H3Index *) Z_RES_VAL_P(origin_resource_zval);
    H3Index *h3 = (H3Index *) Z_RES_VAL_P(h3_resource_zval);

  int distance = h3Distance(*origin, *h3);

    RETURN_LONG(distance);
}

/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_h3_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_h3_init_globals(zend_h3_globals *h3_globals)
{
    h3_globals->global_value = 0;
    h3_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(h3)
{
    /* If you have INI entries, uncomment these lines
    REGISTER_INI_ENTRIES();
    */
    le_h3_index = zend_register_list_destructors_ex(NULL, NULL, "h3_index", module_number);
    le_h3_boundary = zend_register_list_destructors_ex(NULL, NULL, "h3_boundary", module_number);
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(h3)
{
    /* uncomment this line if you have INI entries
    UNREGISTER_INI_ENTRIES();
    */
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(h3)
{
#if defined(COMPILE_DL_H3) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(h3)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(h3)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "h3 support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
    DISPLAY_INI_ENTRIES();
    */
}
/* }}} */

/* {{{ h3_functions[]
 *
 * Every user visible function must have an entry in h3_functions[].
 */
const zend_function_entry h3_functions[] = {
    PHP_FE(geoToH3,    NULL)
    PHP_FE(h3ToGeoBoundary,    NULL)
    PHP_FE(h3ToGeo,    NULL)

    PHP_FE(h3GetResolution,    NULL)
    PHP_FE(h3GetBaseCell,    NULL)
    PHP_FE(h3ToString,    NULL)
    PHP_FE(h3IsValid,    NULL)
    PHP_FE(h3IsResClassIII,    NULL)
    PHP_FE(h3IsPentagon,    NULL)

    PHP_FE(kRing,    NULL)
    PHP_FE(maxKringSize,    NULL)
    PHP_FE(kRingDistances,    NULL)

    PHP_FE(h3Distance,    NULL)

    PHP_FE_END    /* Must be the last line in h3_functions[] */
};
/* }}} */

/* {{{ h3_module_entry
 */
zend_module_entry h3_module_entry = {
    STANDARD_MODULE_HEADER,
    "h3",
    h3_functions,
    PHP_MINIT(h3),
    PHP_MSHUTDOWN(h3),
    PHP_RINIT(h3),        /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(h3),    /* Replace with NULL if there's nothing to do at request end */
    PHP_MINFO(h3),
    PHP_H3_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_H3
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(h3)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

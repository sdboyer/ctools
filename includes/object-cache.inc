<?php
// $Id$
/**
 * @file
 * The non-volatile object cache is used to store an object while it is
 * being edited, so that we don't have to save until we're completely
 * done. The cache should be 'cleaned' on a regular basis, meaning to
 * remove old objects from the cache, but otherwise the data in this
 * cache must remain stable, as it includes unsaved changes.
 */

/**
 * Get an object from the non-volatile ctools cache.
 *
 * This function caches in memory as well, so that multiple calls to this
 * will not result in multiple database reads.
 *
 * @param $obj
 *   A 32 character or less string to define what kind of object is being
 *   stored; primarily this is used to prevent collisions.
 * @param $name
 *   The name of the object being stored.
 * @param $skip_cache
 *   Skip the memory cache, meaning this must be read from the db again.
 *
 * @return
 *   The data that was cached.
 */
function ctools_object_cache_get($obj, $name, $skip_cache = FALSE) {
  static $cache = array();
  $key = "$obj:$name";
  if ($skip_cache) {
    unset($cache[$key]);
  }

  if (!array_key_exists($key, $cache)) {
    $data = db_fetch_object(db_query("SELECT * FROM {ctools_object_cache} WHERE sid = '%s' AND obj = '%s' AND name = '%s'", session_id(), $obj, $name));
    if ($data) {
      $cache[$key] = unserialize(db_decode_blob($data->data));
    }
  }
  return isset($cache[$key]) ? $cache[$key] : NULL;
}

/**
 * Store an object in the non-volatile ctools cache.
 *
 * @param $obj
 *   A 32 character or less string to define what kind of object is being
 *   stored; primarily this is used to prevent collisions.
 * @param $name
 *   The name of the object being stored.
 * @param $cache
 *   The object to be cached. This will be serialized prior to writing.
 */
function ctools_object_cache_set($obj, $name, $cache) {
  ctools_object_cache_clear($obj, $name);
  db_query("INSERT INTO {ctools_object_cache} (sid, obj, name, data, updated) VALUES ('%s', '%s', '%s', %b, %d)", session_id(), $obj, $name, serialize($cache), time());
}

/**
 * Remove an object from the non-volatile ctools cache
 *
 * @param $obj
 *   A 32 character or less string to define what kind of object is being
 *   stored; primarily this is used to prevent collisions.
 * @param $name
 *   The name of the object being removed.
 */
function ctools_object_cache_clear($obj, $name) {
  db_query("DELETE FROM {ctools_object_cache} WHERE sid = '%s' AND obj = '%s' AND name = '%s'", session_id(), $obj, $name);
}

/**
 * Remove all objects in the object cache that are older than the
 * specified age.
 *
 * @param $age
 *   The minimum age of objects to remove, in seconds. For example, 86400 is
 *   one day. Defaults to 7 days.
 */
function ctools_object_cache_clean($age = NULL) {
  if (empty($age)) {
    $age = 86400 * 7; // 7 days
  }
  db_query("DELETE FROM {ctools_object_cache} WHERE updated < %d", time() - $age);
}

<?php
// $Id$

/**
 * @file
 * Contains code to make it easier to have exportable objects.
 *
 * These are objects that can live in code OR the database, and versions in
 * the database will override versions that are in code.
 *
 * This doesn't include a write routine since drupal_write_record is
 * more or less sufficient.
 */

/**
  in schema:
    'export' = array(
      'key' => 'name', // unique key to identify records
      'default hook' => '', // name of hook to get default objects
      'status' => '', // name of variable to store enabled/disabled status
      'object' => '', // name of the object to put this data on
      'sub records' => array(
        /////// sub records not yet implemented
        array(
          'table' => // table subsidiary records are in
          'parent' => // field in parent that matches
          'child' => // field in child that matches
          'array' => // name of array to store record in
          'key' => // field to use as a key to store sub records.
        ),
      ),
  */

/**
 * A bit flag used to let us know if an object is in the database.
 */
define('EXPORT_IN_DATABASE', 0x01);

/**
 * A bit flag used to let us know if an object is a 'default' in code.
 */
define('EXPORT_IN_CODE', 0x02);

/**
 * Load some number of exportable objects.
 *
 * This function will cache the objects, load subsidiary objects if necessary,
 * check default objects in code and properly set them up. It will cache
 * the results so that multiple calls to load the same objects
 * will not cause problems.
 *
 * It attempts to reduce, as much as possible, the number of queries
 * involved.
 *
 * @param $table
 *   The name of the table to be loaded from. Data is expected to be in the
 *   schema to make all this work.
 * @param $type
 *   A string to notify the loader what the argument is
 *   - all: load all items. This is the default. $args is unused.
 *   - names: $args will be an array of specific named objects to load.
 *   - condition: $args will be a keyed array of conditions. The conditions
 *       must be in the schema for this table or errors will result.
 * @param $args
 *   An array of arguments whose actual use is defined by the $type argument.
 */
function ctools_export_load_object($table, $type = 'all', $args = array()) {
  static $cache = array();
  static $cached_defaults = FALSE;
  static $cached_database = FALSE;

  $schema = ctools_export_get_schema($table);
  $export = $schema['export'];

  if (!isset($cache[$table])) {
    $cache[$table] = array();
  }

  $return = array();

  // Don't load anything we've already cached.
  if ($type == 'names' && !empty($args)) {
    foreach ($args as $name) {
      if (isset($cache[$table][$name])) {
        $return[$name] = $cache[$table][$name];
        unset($args[$name]);
      }
    }

    // If nothing left to load, return the result.
    if (empty($args)) {
      return $return;
    }
  }

  // Build the query
  $query = "SELECT * FROM {" . $table . "}";
  $conditions = array();
  $query_args = array();

  // We do not have to load anything if we have already cached everything.
  if ($type != 'all' || !$cached_database) {
    // If they passed in names, add them to the query.
    if ($type == 'names') {
      $conditions[] = "$export[key] IN (" . db_placeholders($args, $schema['fields'][$export['key']]['type']) . ")";
      $query_args = $args;
    }
    else if ($type == 'conditions') {
      foreach ($args as $key => $value) {
        $conditions[] = "$key = " . db_type_placeholder($schema['fields'][$key]['type']);
        $query_args[] = $value;
      }
    }

    // Make a string out of the conditions.
    if ($conditions) {
      $query .= " WHERE " . implode(' AND ', $conditions);
    }

    $result = db_query($query, $query_args);

    // Unpack the results of the query onto objects and cache them.
    while ($data = db_fetch_object($result)) {
      $object = ctools_export_unpack_object($schema, $data, $export['object']);
      $object->type = t('Normal');
      $object->export_type = EXPORT_IN_DATABASE;

      $cache[$table][$object->{$export['key']}] = $object;
      if ($type == 'conditions') {
        $return[$object->{$export['key']}] = $object;
      }
    }
  }

  if ($type == 'all') {
    $cached_database = TRUE;
  }
  // @todo Load subrecords.

  if ($export['default hook'] && !$cached_defaults) {
    // @todo add a method to load .inc files for this.
    $defaults = module_invoke_all($export['default hook']);
    $status = variable_get($export['status'], array());

    foreach ($defaults as $object) {
      if ($type == 'conditions') {
        // if this does not match all of our conditions, skip it.
        foreach ($args as $key => $value) {
          if (!isset($object->$key) || $object->$key != $value) {
            continue;
          }
        }
      }
      else if ($type == 'names') {
        if (!in_array($object->{$export['key']}, $args)) {
          continue;
        }
      }

      // Determine if default object is enabled or disabled.
      if (isset($status[$object->name])) {
        $object->disabled = $status[$object->name];
      }

      if (!empty($cache[$table][$object->name])) {
        $cache[$table][$object->name]->type = t('Overridden');
        $cache[$table][$object->name]->export_type |= EXPORT_IN_CODE;
        if ($type == 'conditions') {
          $return[$object->name] = $cache[$table][$object->name];
        }
      }
      else {
        $object->type = t('Default');
        $object->export_type = EXPORT_IN_CODE;
        $object->in_code_only = TRUE;

        $cache[$table][$object->name] = $object;
        if ($type == 'conditions') {
          $return[$object->name] = $object;
        }
      }

    }

    // We only actually force this when retrieving all, because we may not
    // have retrieved an object from the database and could thus incorrectly
    // identify one as being a 'default' object when it is actually
    // overridden. So we settle for a potential minor performance decrease
    // in order to get this correct.
    if ($type == 'all') {
      // Make sure we don't run that again later on.
      $cached_defaults = TRUE;
    }
  }

  // If fetching all, we've done so and we are finished.
  if ($type == 'all') {
    return $cache[$table];
  }

  if ($type == 'names') {
    foreach ($args as $name) {
      if (isset($cache[$table][$name])) {
        $return[$name] = $cache[$table][$name];
      }
    }
  }

  // For conditions,
  return $return;
}

/**
 * Get the default version of an object, if it exists.
 *
 * This function doesn't care if an object is in the database or not and
 * does not check. This means that export_type could appear to be incorrect,
 * because a version could exist in the database. However, it's not
 * incorrect for this function as it is *only* used for the default
 * in code version.
 */
function ctools_get_default_object($table, $name) {
  $schema = ctools_export_get_schema($table);
  $export = $schema['export'];

  if (!$export['default hook']) {
    return;
  }

  // @todo add a method to load .inc files for this.
  $defaults = module_invoke_all($export['default hook']);
  $status = variable_get($export['status'], array());

  if (!isset($defaults[$name])) {
    return;
  }

  $object = $defaults[$name];

  // Determine if default object is enabled or disabled.
  if (isset($status[$object->name])) {
    $object->disabled = $status[$object->name];
  }

  $object->type = t('Default');
  $object->export_type = EXPORT_IN_CODE;
  $object->in_code_only = TRUE;

  return $object;
}

/**
 * Unpack data loaded from the database onto an object.
 *
 * @param $schema
 *   The schema from drupal_get_schema().
 * @param $data
 *   The data as loaded by db_fetch_object().
 * @param $object
 *   If an object, data will be unpacked onto it. If a string
 *   an object of that type will be created.
 */
function ctools_export_unpack_object($schema, $data, $object = 'stdClass') {
  if (is_string($object)) {
    if (class_exists($object)) {
      $object = new $object;
    }
    else {
      $object = new stdClass;
    }
  }

  // Go through our schema and build correlations.
  foreach ($schema['fields'] as $field => $info) {
    $object->$field = empty($info['serialize']) ? $data->$field : unserialize($data->$field);
  }

  return $object;
}

/**
 * Export a field.
 *
 * This is a replacement for var_export(), allowing us to more nicely
 * format exports. It will recurse down into arrays and will try to
 * properly export bools when it can, though PHP has a hard time with
 * this since they often end up as strings or ints.
 */
function ctools_var_export($var, $prefix = '') {
  if (is_array($var)) {
    if (empty($var)) {
      $output = 'array()';
    }
    else {
      $output = "array(\n";
      foreach ($var as $key => $value) {
        $output .= "  '$key' => " . ctools_var_export($value, '  ') . ",\n";
      }
      $output .= ')';
    }
  }
  else if (is_bool($var)) {
    $output = $var ? 'TRUE' : 'FALSE';
  }
  else {
    $output = var_export($var, TRUE);
  }

  if ($prefix) {
    $output = str_replace("\n", "\n$prefix", $output);
  }

  return $output;
}

/**
 * Export an object into code.
 */
function ctools_export_object($table, $object, $identifier, $indent = '', $additions = array(), $additions2 = array()) {
  $schema = drupal_get_schema($table);

  $output = $indent . '$' . $identifier . ' = new ' . get_class($object) . ";\n";

  $output .= $indent . '$' . $identifier . '->disabled' . ' = FALSE; /* Edit this to true to make a default ' . $identifier . ' disabled initially */' . "\n";

  // Put top additions here:
  foreach ($additions as $field => $value) {
    $output .= $indent . '$' . $identifier . '->' . $field . ' = ' . ctools_var_export($value, $indent) . ";\n";
  }

  // Go through our schema and build correlations.
  foreach ($schema['fields'] as $field => $info) {
    if (!empty($info['no export'])) {
      continue;
    }
    if (!isset($object->$field)) {
      if (isset($info['default'])) {
        $object->$field = $info['default'];
      }
      else {
        $object->$field = '';
      }
    }

    $value = $object->$field;
    if ($info['type'] == 'int') {
      $value = (isset($info['size']) && $info['size'] == 'tiny') ? (bool) $value : (int) $value;
    }

    $output .= $indent . '$' . $identifier . '->' . $field . ' = ' . ctools_var_export($value, $indent) . ";\n";
  }

  // And bottom additions here
  foreach ($additions2 as $field => $value) {
    $output .= $indent . '$' . $identifier . '->' . $field . ' = ' . ctools_var_export($value, $indent) . ";\n";
  }

  return $output;
}

/**
 * Get the schema for a given table.
 *
 * This looks for data the export subsystem needs and applies defaults so
 * that it's easily available.
 */
function ctools_export_get_schema($table) {
  $schema = drupal_get_schema($table);

  if (!isset($schema['export'])) {
    $schema['export'] = array();
  }

  // Add some defaults
  $schema['export'] += array(
    'key' => 'name',
    'object' => 'stdClass',
    'status' => 'default_' . $table,
    'default hook' => 'default_' . $table,
  );

  return $schema;
}

/**
 * Set the status of a default $object as a variable.
 *
 * The status, in this case, is whether or not it is 'disabled'
 * and is only valid for in-code objects that do not have a database
 * equivalent. This function does not check to make sure $object actually
 * exists.
 */
function ctools_export_set_status($table, $name, $new_status = TRUE) {
  $schema = ctools_export_get_schema($table);
  $status = variable_get($schema['export']['status'], array());

  $status[$name] = $new_status;
  variable_set($schema['export']['status'], $status);
}

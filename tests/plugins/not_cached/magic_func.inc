<?php
// $Id$
/**
 * @file
 * Chaos Tools plugin include using magic function to declare a plugin.
 */

/**
 * Magic function plugin definition.
 */
function ctools_plugin_test_magic_func_ctools_plugin_test_not_cached() {
  return array(
    'function' => 'ctools_plugin_test_magic_func_not_cached_test',
    'handler' => 'ctoolsNotCachedMagicFunc',
  );
}

/**
 * Magic function function plugin.
 */
function ctools_plugin_test_magic_func_not_cached_test() {}

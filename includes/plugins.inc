<?php
// $Id$
/**
 * @file
 *
 * Contains routines to organize and load plugins. It allows a special
 * variation of the hook system so that plugins can be kept in separate
 * .inc files, and can be either loaded all at once or loaded only when
 * necessary.
 */

/**
 * Fetch a group of plugins by name.
 *
 * @param $module
 *   The name of the module that utilizes this plugin system. It will be
 *   used to call hook_ctools_plugin_$plugin() to get more data about the plugin.
 * @param $type
 *   The type identifier of the plugin.
 * @param $id
 *   If specified, return only information about plugin with this identifier.
 *   The system will do its utmost to load only plugins with this id.
 *
 * @return
 *   An array of information arrays about the plugins received. The contents
 *   of the array are specific to the plugin.
 */
function ctools_get_plugins($module, $type, $id = NULL) {
  static $plugins = array();
  static $all_hooks = array();
  static $all_files = array();
  static $info = array();

  if (!isset($info[$type])) {
    $info[$type] = ctools_plugin_get_info($module, $type);
  }

  // If the plugin info says this can be cached, check cache first.
  if ($info[$type]['cache'] && !isset($plugins['cache'])) {
    // @todo Maybe this should use our own table but free wiping
    // with content updates is convenient.
    $cache = cache_get("plugins:$module:$type");

    // if cache load successful, set $all_hooks and $all_files to true.
    if (!empty($cache->data)) {
      $plugins[$type] = $cache->data;
      $all_hooks[$type] = TRUE;
      $all_files[$type] = TRUE;
    }
    else {
      $write_cache = TRUE;
    }
  }

  // Always load all hooks if we need them.
  if (!isset($all_hooks[$type])) {
    $all_hooks[$type] = TRUE;
    $plugins[$type] = ctools_plugin_load_hooks($info[$type]);
  }

  // First, see if it's in our hooks before we even bother.
  if ($id && array_key_exists($id, $plugins[$type])) {
    return $plugins[$type][$id];
  }

  // Then see if we should load all files. We only do this if we
  // want a list of all plugins.
  if ((!$id || $info[$type]['cache']) && empty($all_files[$type])) {
    $all_files[$type] = TRUE;
    $plugins[$type] = array_merge($plugins[$type], ctools_plugin_load_includes($info[$type]));
  }

  // If we were told earlier that this is cacheable and the cache was
  // empty, give something back.
  if (!empty($write_cache)) {
    cache_set("plugins:$module:$type", $plugins[$type]);
  }

  // If no id was requested, we are finished here:
  if (!$id) {
    return $plugins[$type];
  }

  // Check to see if we need to look for the file
  if (!array_key_exists($id, $plugins[$type])) {
    $result = ctools_plugin_load_includes($info[$type], $id);
    // Set to either what was returned or NULL.
    $plugins[$type][$id] = isset($result[$id]) ? $result[$id] : NULL;
  }

  // At this point we should either have the plugin, or a NULL.
  return $plugins[$type][$id];
}

/**
 * Load plugins from a directory.
 *
 * @param $info
 *   The plugin info as returned by ctools_plugin_get_info()
 * @param $file
 *   The file to load if we're looking for just one particular plugin.
 *
 * @return
 *   An array of information created for this plugin.
 */
function ctools_plugin_load_includes($info, $file = NULL) {
  // Load all our plugins.
  $directories = ctools_plugin_get_directories($info);
  $file_list = array();
  foreach ($directories as $module => $path) {
    $file_list[$module] = drupal_system_listing("$file" . '.inc$', $path, 'name', 0);
  }
  $plugins = array();

  // Iterate through all the plugin .inc files, load them and process the hook
  // that should now be available.
  foreach (array_filter($file_list) as $module => $files) {
    foreach ($files as $file) {
      require_once './' . $file->filename;
      // .inc files have a special format for the hook identifier.
      // For example, 'foo.inc' in the module 'mogul' using the plugin
      // whose hook is named 'borg_type' should have a function named (deep breath)
      // mogul_foo_borg_type()
      $result = ctools_plugin_process($info, $module, $module . '_' . $file->name, dirname($file->filename), basename($file->filename));
      if (is_array($result)) {
        $plugins = array_merge($plugins, $result);
      }
    }
  }
  return $plugins;
}

/**
 * Get a list of directories to search for plugins of the given type.
 *
 * This utilizes hook_ctools_plugin_directory() to determine a complete list of
 * directories. Only modules that implement this hook and return a string
 * value will have their directories included.
 *
 * @param $info
 *   The $info array for the plugin as returned by ctools_plugin_get_info().
 *
 * @return array $directories
 *   An array of directories to search.
 */
function ctools_plugin_get_directories($info) {
  $directories = array();

  foreach (module_implements('ctools_plugin_directory') as $module) {
    $function = $module . '_ctools_plugin_directory';
    $result = $function($info['module'], $info['type']);
    if ($result && is_string($result)) {
      $directories[$module] = drupal_get_path('module', $module) . '/' . $result;
    }
  }
  return $directories;
}

/**
 * Load plugin info for the provided hook; this is handled separately from
 * plugins from files.
 *
 * @param $info
 *   The info array about the plugin as created by ctools_plugin_get_info()
 *
 * @return
 *   An array of info supplied by any hook implementations.
 */
function ctools_plugin_load_hooks($info) {
  $hooks = array();
  foreach (module_implements($info['hook']) as $module) {
    $result = ctools_plugin_process($info, $module, $module, drupal_get_path('module', $module));
    if (is_array($result)) {
      $hooks = array_merge($hooks, $result);
    }
  }
  return $hooks;
}

/**
 * Process a single hook implementation of a ctools plugin.
 *
 * @param $info
 *   The $info array about the plugin as returned by ctools_plugin_get_info()
 * @param $module
 *   The module that implements the plugin being processed.
 * @param $identifier
 *   The plugin identifier, which is used to create the name of the hook
 *   function being called.
 * @param $path
 *   The path where files utilized by this plugin will be found.
 * @param $file
 *   The file that was loaded for this plugin, if it exists.
 */
function ctools_plugin_process($info, $module, $identifier, $path, $file = NULL) {
  $function = $identifier . '_' . $info['hook'];
  if (!function_exists($function)) {
    return NULL;
  }
  $result = $function();
  if (!isset($result) || !is_array($result)) {
    return NULL;
  }

  // Fill in defaults.
  foreach ($result as $name => $plugin) {
    $result[$name] += array(
      'module' => $module,
      'name' => $name,
      'path' => $path,
      'file' => $file
    );

    // Fill in plugin specific defaults, if they exist.
    if ($info['defaults']) {
      $result[$name] += $info['defaults'];
    }
  }
  return $result;
}

/**
 * Ask a module for info about a particular plugin type.
 */
function ctools_plugin_get_info($module, $type) {
  $info = array();
  $function = $module . '_ctools_plugin_' . $type;
  if (function_exists($function)) {
    $info = $function();
  }

  // Apply defaults. Array addition will not overwrite pre-existing keys.
  $info += array(
    'module' => $module,
    'type' => $type,
    'cache' => FALSE,
    'defaults' => array(),
    'hook' => $module . '_' . $type,
  );

  return $info;
}

/**
 * Get a function from a plugin, if it exists. If the plugin is not already
 * loaded, try ctools_plugin_load_function() instead.
 *
 * @param $plugin
 *   The loaded plugin type.
 * @param $function_name
 *   The identifier of the function. For example, 'settings form'.
 *
 * @return
 *   The actual name of the function to call, or NULL if the function
 *   does not exist.
 */
function ctools_plugin_get_function($plugin, $function_name) {
  // If cached the .inc file may not have been loaded. require_once is quite safe
  // and fast so it's okay to keep calling it.
  if (isset($plugin['file'])) {
    if (!is_array($plugin)) { vpr_trace(); }
    require_once './' . $plugin['path'] . '/' . $plugin['file'];
  }

  if (!isset($plugin[$function_name])) {
    return;
  }

  if (is_array($plugin[$function_name]) && isset($plugin[$function_name]['function'])) {
    $function = $plugin[$function_name]['function'];
    if (isset($plugin[$function_name]['file'])) {
      $file = $plugin[$function_name]['file'];
      if (isset($plugin[$function_name]['path'])) {
        $file = $plugin[$function_name]['path'] . '/' . $file;
      }
      require_once './' . $file;
    }
  }
  else {
    $function = $plugin[$function_name];
  }

  if (function_exists($function)) {
    return $function;
  }
}

/**
 * Load a plugin and get a function name from it, returning success only
 * if the function exists.
 *
 * @param $module
 *   The module that owns the plugin type.
 * @param $type
 *   The type of plugin.
 * @param $id
 *   The id of the specific plugin to load.
 * @param $function_name
 *   The identifier of the function. For example, 'settings form'.
 *
 * @return
 *   The actual name of the function to call, or NULL if the function
 *   does not exist.
 */
function ctools_plugin_load_function($module, $type, $id, $function_name) {
  $plugin = ctools_get_plugins($module, $type, $id);
  return ctools_plugin_get_function($plugin, $function_name);
}

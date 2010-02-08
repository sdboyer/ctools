
<?php
// $Id$

/**
 * @file
 * Contains theme registry and theme implementations for the content types.
 */

/**
 * Implementation of hook_theme to load all content plugins and pass thru if
 * necessary.
 */
function ctools_stylizer_theme(&$theme) {
  ctools_include('stylizer');

  $plugins = ctools_get_style_bases();
  foreach ($plugins as $plugin) {
    if (!empty($plugin['theme'])) {
      $theme[$plugin['theme']] = array(
        'arguments' => array('settings' => NULL, 'classes' => NULL, 'content' => NULL),
        'path' => $plugin['path'],
      );

      // if no theme function exists, assume template.
      if (!function_exists("theme_$plugin[theme]")) {
        $theme[$plugin['theme']]['template'] = str_replace('_', '-', $plugin['theme']);
        $theme[$plugin['theme']]['file'] = $plugin['file']; // for preprocess.
      }
    }
  }
}


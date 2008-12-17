<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide an argument handler for a node id
 */

/**
 * Implementation of specially named hook_ctools_arguments().
 */
function ctools_perm_ctools_access() {
  $args['perm'] = array(
    'title' => t("By permission string"),
    'description' => t('Control access by permission string.'),
    'callback' => 'ctools_perm_ctools_access_check',
    'default' => array('perm' => 'access content'),
    'settings form' => 'ctools_perm_ctools_access_settings',
  );

  return $args;
}

/**
 * Settings form for the 'by perm' access plugin
 */
function ctools_perm_ctools_access_settings(&$form, &$form_state, $conf) {
  $perms = array();
  // Get list of permissions
  foreach (module_list(FALSE, FALSE, TRUE) as $module) {
    // By keeping them keyed by module we can use optgroups with the
    // 'select' type.
    if ($permissions = module_invoke($module, 'perm')) {
      $perms[$module] = drupal_map_assoc($permissions);
    }
  }

  $form['settings']['perm'] = array(
    '#type' => 'select',
    '#options' => $perms,
    '#title' => t('Permission'),
    '#default_value' => $conf['perm'],
    '#description' => t('Only users with the selected permission flag will be able to access this.'),
  );
}

/**
 * Check for access.
 */
function ctools_perm_ctools_access_check($conf, $contexts, $account) {
  return user_access($conf['perm'], $account);
}
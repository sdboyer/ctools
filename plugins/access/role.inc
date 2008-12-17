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
function ctools_role_ctools_access() {
  $args['role'] = array(
    'title' => t("By role"),
    'description' => t('Control access by role.'),
    'callback' => 'ctools_role_ctools_access_check',
    'default' => array('rids' => array()),
    'settings form' => 'ctools_role_ctools_access_settings',
    'settings form submit' => 'ctools_role_ctools_access_settings_submit',

  );

  return $args;
}

/**
 * Settings form for the 'by role' access plugin
 */
function ctools_role_ctools_access_settings(&$form, &$form_state, $conf) {
  $form['settings']['rids'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Role'),
    '#default_value' => $conf['rids'],
    '#options' => ctools_get_roles(),
    '#description' => t('Only the checked roles will be granted access.'),
  );
}

/**
 * Compress the roles allowed to the minimum.
 */
function ctools_role_ctools_access_settings_submit(&$form, &$form_state) {
  $form_state['values']['settings']['rids'] = array_keys(array_filter($form_state['values']['settings']['rids']));
}

/**
 * Check for access.
 */
function ctools_role_ctools_access_check($conf, $contexts, $account) {
  $roles = array_keys($account->roles);
  $roles[] = $account->uid ? DRUPAL_AUTHENTICATED_RID : DRUPAL_ANONYMOUS_RID;
  return array_intersect($conf['rids'], $roles);
}
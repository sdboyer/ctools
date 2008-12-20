<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide an argument handler for a user id
 */

/**
 * Implementation of specially named hook_ctools_arguments().
 */
function ctools_uid_ctools_arguments() {
  $args['uid'] = array(
    'title' => t("User ID"),
    // keyword to use for %substitution
    'keyword' => 'user',
    'description' => t('Creates a user context from a user ID argument.'),
    'context' => 'ctools_argument_uid_context',
    'criteria form' => 'ctools_argument_uid_criteria_form',
    'criteria select' => 'ctools_argument_uid_criteria_select',
    'criteria summary' => 'ctools_argument_uid_criteria_summary',
  );
  return $args;
}

/**
 * Discover if this argument gives us the user we crave.
 */
function ctools_argument_uid_context($arg = NULL, $conf = NULL, $empty = FALSE) {
  // If unset it wants a generic, unfilled context.
  if ($empty) {
    return ctools_context_create_empty('user');
  }

  if (!is_numeric($arg)) {
    return NULL;
  }

  $account = user_load(array('uid' => $arg));
  if (!$account) {
    return NULL;
  }

  return ctools_context_create('user', $account);
}

/**
 * Provide a criteria form for selecting a node.
 */
function ctools_argument_uid_criteria_form(&$form, &$form_state, $conf, $argument, $id) {
  // Ensure $conf has valid defaults:
  if (!is_array($conf)) {
    $conf = array();
  }

  $conf += array(
    'rids' => array(),
  );


  $form[$id]['rids'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Select roles for %identifier', array('%identifier' => $argument['identifier'])),
    '#default_value' => $conf['rids'],
    '#options' => ctools_get_roles(),
    '#description' => t('This item will only be selected for users having the selected roles. If no roles are selected, it will be selected for all users.'),
  );
}

/**
 * Provide a criteria form for selecting a node.
 */
function ctools_argument_uid_criteria_select($conf, $context) {
  // As far as I know there should always be a context at this point, but this
  // is safe.
  if (empty($context) || empty($context->data) || !isset($context->data->roles)) {
    return FALSE;
  }

  $rids = array_filter($conf['rids']);
  if (!$rids) {
    return TRUE;
  }

  $roles = array_keys($context->data->roles);
  $roles[] = $context->data->uid ? DRUPAL_AUTHENTICATED_RID : DRUPAL_ANONYMOUS_RID;

  return array_intersect($rids, $roles);
}

/**
 * Provide a summary of the criteria for selecting this node.
 */
function ctools_argument_uid_criteria_summary($conf, $argument) {
  if (!isset($conf['rids'])) {
    $conf['rids'] = array();
  }
  $roles = ctools_get_roles();

  $names = array();
  foreach (array_filter($conf['rids']) as $rid) {
    $names[] = check_plain($roles[$rid]);
  }

  if (empty($names)) {
    return t('@identifier can have any role', array('@identifier' => $argument['identifier']));
  }

  return format_plural(count($names), '@identifier must have role "@roles"', '@identifier can be one of "@roles"', array('@roles' => implode(', ', $names), '@identifier' => $argument['identifier']));
}

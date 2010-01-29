<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide a user context
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t("User"),
  'description' => t('A single user object.'),
  'context' => 'ctools_context_create_user',
  'settings form' => 'ctools_context_user_settings_form',
  'settings form validate' => 'ctools_context_user_settings_form_validate',
  'keyword' => 'user',
  'no ui' => TRUE,
  'context name' => 'user',
  'convert list' => 'ctools_context_user_convert_list',
  'convert' => 'ctools_context_user_convert',
);

/**
 * It's important to remember that $conf is optional here, because contexts
 * are not always created from the UI.
 */
function ctools_context_create_user($empty, $data = NULL, $conf = FALSE) {
  $context = new ctools_context('user');
  $context->plugin = 'user';

  if ($empty) {
    return $context;
  }

  if ($conf) {
    $data = user_load(array('uid' => $data['uid']));
  }

  if (!empty($data)) {
    $context->data     = $data;
    $context->title    = isset($data->name) ? $data->name : t('Anonymous');
    $context->argument = $data->uid;
    return $context;
  }
}

function ctools_context_user_settings_form($conf) {
  $form = array();
  return $form;
}

/**
 * Provide a list of replacements.
 */
function ctools_context_user_convert_list() {
  $list = array(
    'uid' => t('User ID'),
    'name' => t('User name'),
  );
  if (module_exists('token')) {
    $list += reset(token_get_list(array('user')));
  }
  return $list;
}

/**
 * Convert a context into a string.
 */
function ctools_context_user_convert($context, $type) {
  switch ($type) {
    case 'uid':
      return $context->data->uid;
    case 'name':
      return $context->data->name;
  }
  if (module_exists('token')) {
    $values = token_get_values('user', $context->data);
    if ($key = array_search($type, $values->tokens)) {
      return $values->values[$key];
    }
  }
}

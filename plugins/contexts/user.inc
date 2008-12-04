<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide a user context
 */

/**
 * Implementation of specially named hook_ctools_contexts().
 */
function ctools_user_ctools_contexts() {
  $args['user'] = array(
    'title' => t("User"),
    'description' => t('A single user object.'),
    'context' => 'ctools_context_create_user',
    'settings form' => 'ctools_context_user_settings_form',
    'settings form validate' => 'ctools_context_user_settings_form_validate',
    'keyword' => 'user',
    'no ui' => TRUE,
    'context name' => 'user',
  );
  return $args;
}

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
    $context->title    = $data->name;
    $context->argument = $data->uid;
    return $context;
  }
}

function ctools_context_user_settings_form($conf, $external = FALSE) {
  $form = array();
  return $form;
}


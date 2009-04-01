<?php
// $Id$

/**
 * @file
 * Plugin to provide access control based upon node type.
 */

/**
 * Implementation of specially named hook_ctools_arguments().
 */
function ctools_node_type_ctools_access() {
  $args['node_type'] = array(
    'title' => t("Node: Type"),
    'description' => t('Control access by node_type.'),
    'callback' => 'ctools_node_type_ctools_access_check',
    'default' => array('type' => array()),
    'settings form' => 'ctools_node_type_ctools_access_settings',
    'settings form submit' => 'ctools_node_type_ctools_access_settings_submit',
    'summary' => 'ctools_node_type_ctools_acesss_summary',
    'required context' => new ctools_context_required(t('Node'), 'node'),
  );

  return $args;
}

/**
 * Settings form for the 'by node_type' access plugin
 */
function ctools_node_type_ctools_access_settings(&$form, &$form_state, $conf) {
  $types = node_get_types();
  foreach ($types as $type => $info) {
    $options[$type] = check_plain($info->name);
  }

  $form['settings']['type'] = array(
    '#title' => t('Node type'),
    '#type' => 'checkboxes',
    '#options' => $options,
    '#description' => t('Only the checked node types will be valid.'),
    '#default_value' => $conf['type'],
  );
}

/**
 * Compress the node_types allowed to the minimum.
 */
function ctools_node_type_ctools_access_settings_submit(&$form, &$form_state) {
  $form_state['values']['settings']['type'] = array_filter($form_state['values']['settings']['type']);
}

/**
 * Check for access.
 */
function ctools_node_type_ctools_access_check($conf, $context) {
  // As far as I know there should always be a context at this point, but this
  // is safe.
  if (empty($context) || empty($context->data) || empty($context->data->type)) {
    return FALSE;
  }

  if (array_filter($conf['type']) && empty($conf['type'][$context->data->type])) {
    return FALSE;
  }

  return TRUE;
}

/**
 * Provide a summary description based upon the checked node_types.
 */
function ctools_node_type_ctools_acesss_summary($conf, $context) {
  if (!isset($conf['type'])) {
    $conf['type'] = array();
  }
  $types = node_get_types();

  $names = array();
  foreach (array_filter($conf['type']) as $type) {
    $names[] = check_plain($types[$type]->name);
  }

  if (empty($names)) {
    return t('@identifier can be any node type', array('@identifier' => $context->identifier));
  }

  return format_plural(count($names), '@identifier can be type "@types"', '@identifier can be types "@types"', array('@types' => implode(', ', $names), '@identifier' => $context->identifier));
}


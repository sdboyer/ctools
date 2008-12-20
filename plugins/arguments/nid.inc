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
function ctools_nid_ctools_arguments() {
  $args['nid'] = array(
    'title' => t("Node ID"),
    'keyword' => 'node',
    'description' => t('Creates a node context from a node ID argument.'),
    'context' => 'ctools_argument_nid_context',
    'criteria form' => 'ctools_argument_nid_criteria_form',
    'criteria select' => 'ctools_argument_nid_criteria_select',
    'criteria summary' => 'ctools_argument_nid_criteria_summary',
  );
  return $args;
}

/**
 * Discover if this argument gives us the node we crave.
 */
function ctools_argument_nid_context($arg = NULL, $conf = NULL, $empty = FALSE) {
  // If unset it wants a generic, unfilled context.
  if ($empty) {
    return ctools_context_create_empty('node');
  }

  if (!is_numeric($arg)) {
    return FALSE;
  }

  $node = node_load($arg);
  if (!$node) {
    return FALSE;
  }

  return ctools_context_create('node', $node);
}

/**
 * Provide a criteria form for selecting a node.
 */
function ctools_argument_nid_criteria_form(&$form, &$form_state, $conf, $argument, $id) {
  // Ensure $conf has valid defaults:
  if (!is_array($conf)) {
    $conf = array();
  }

  $conf += array(
    'type' => array(),
  );

  $types = node_get_types();
  foreach ($types as $type => $info) {
    $options[$type] = check_plain($info->name);
  }

  $form[$id]['type'] = array(
    '#title' => t('Select types for %identifier', array('%identifier' => $argument['identifier'])),
    '#type' => 'checkboxes',
    '#options' => $options,
    '#description' => t('This item will only be selected for nodes having the selected node types. If no node types are selected, it will be selected for all node types.'),
    '#default_value' => $conf['type'],
  );
}


/**
 * Provide a criteria form for selecting a node.
 */
function ctools_argument_nid_criteria_select($conf, $context) {
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
 * Provide a summary of the criteria for selecting this node.
 */
function ctools_argument_nid_criteria_summary($conf, $argument) {
  if (!isset($conf['type'])) {
    $conf['type'] = array();
  }
  $types = node_get_types();

  $names = array();
  foreach (array_filter($conf['type']) as $type) {
    $names[] = check_plain($types[$type]->name);
  }

  if (empty($names)) {
    return t('@identifier can be any node type', array('@identifier' => $argument['identifier']));
  }

  return format_plural(count($names), '@identifier can be type "@types"', '@identifier can be types "@types"', array('@types' => implode(', ', $names), '@identifier' => $argument['identifier']));
}

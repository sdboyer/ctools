<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide an argument handler for a Node edit form
 */

/**
 * Implementation of specially named hook_ctools_arguments().
 */
function ctools_node_edit_ctools_arguments() {
  $args['node_edit'] = array(
    'title' => t("Node edit form"),
    // keyword to use for %substitution
    'keyword' => 'node',
    'description' => t('Displays the node edit form for a node.'),
    'context' => 'ctools_node_edit_context',
    'settings form' => 'ctools_node_edit_settings_form',
    'settings form submit' => 'ctools_node_edit_settings_form_submit',
  );
  return $args;
}

/**
 * Discover if this argument gives us the node we crave.
 */
function ctools_node_edit_context($arg = NULL, $conf = NULL, $empty = FALSE) {
  // If unset it wants a generic, unfilled context.
  if ($empty) {
    return ctools_context_create_empty('node_edit_form');
  }

  if (!is_numeric($arg)) {
    return NULL;
  }

  $node = node_load($arg);
  if (!$node) {
    return NULL;
  }

  if (array_filter($conf['types']) && empty($conf['types'][$node->type])) {
    return NULL;
  }

  // This will perform a node_access check, so we don't have to.
  return ctools_context_create('node_edit_form', $node);
}

/**
 * Settings form for the argument
 */
function ctools_node_edit_settings_form($conf) {
  $options = array();
  foreach (node_get_types() as $type => $info) {
    $options[$type] = $info->name;
  }
  $form['types'] = array(
    '#title' => t('Node types'),
    '#type' => 'checkboxes',
    '#options' => $options,
    '#description' => t('You can restrict this argument to use the checked node types. Arguments from non-conforming node types will be ignored, and ctools will behave as if no argument were given. Leave all unchecked to impose no restriction.'),
    '#default_value' => $conf['types'],
    '#prefix' => '<div class="clear-block">',
    '#suffix' => '</div>',
  );

  return $form;
}

/**
 * There appears to be a bit of a bug with the way we're handling forms; it causes
 * 'checkboxes' to get invalid values added to them when empty. This takes care
 * of that.
 */
function ctools_node_edit_settings_form_submit(&$values) {
  $types = node_get_types();
  if (!empty($values['types'])) {
    foreach ($values['types'] as $type => $value) {
      if (empty($types[$type])) {
        unset($values['types'][$type]);
      }
    }
  }
}

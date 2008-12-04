<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide an argument handler for a Node add form
 */

/**
 * Implementation of specially named hook_ctools_arguments().
 */
function ctools_node_add_ctools_arguments() {
  $args['node_add'] = array(
    'title' => t("Node add form"),
    // keyword to use for %substitution
    'keyword' => 'node_type',
    'description' => t('Displays the node add form for a content type.'),
    'context' => 'ctools_node_add_context',
    'settings form' => 'ctools_node_add_settings_form',
    'settings form submit' => 'ctools_node_add_settings_form_submit',
  );
  return $args;
}

/**
 * Discover if this argument gives us the node we crave.
 */
function ctools_node_add_context($arg = NULL, $conf = NULL, $empty = FALSE) {
  // If unset it wants a generic, unfilled context.
  if (!isset($arg)) {
    return ctools_context_create_empty('node_add_form');
  }

  if (array_filter($conf['types']) && empty($conf['types'][$arg])) {
    return NULL;
  }

  return ctools_context_create('node_add_form', $arg);
}

/**
 * Settings form for the argument
 */
function ctools_node_add_settings_form($conf) {
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
function ctools_node_add_settings_form_submit(&$values) {
  $types = node_get_types();
  if (!empty($values['types'])) {
    foreach ($values['types'] as $type => $value) {
      if (empty($types[$type])) {
        unset($values['types'][$type]);
      }
    }
  }
}

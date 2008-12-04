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
    'description' => t('Restricts the argument to a node id.'),
    'context' => 'ctools_nid_context',
    'settings form' => 'ctools_nid_settings_form',
    'settings form submit' => 'ctools_nid_settings_form_submit',
  );
  return $args;
}

/**
 * Discover if this argument gives us the node we crave.
 */
function ctools_nid_context($node = NULL, $conf = NULL, $empty = FALSE) {
  // If unset it wants a generic, unfilled context.
  if ($empty) {
    return ctools_context_create_empty('node');
  }

  if (!is_object($node)) {
    return NULL;
  }

  if (array_filter($conf['types']) && empty($conf['types'][$node->type])) {
    return NULL;
  }

  return ctools_context_create('node', $node);
}

/**
 * Settings form for the argument
 */
function ctools_nid_settings_form($conf) {
  if (empty($conf)) {
    $conf = array(
      'types' => array(),
      'own_default' => FALSE,
      'displays' => array(),
    );
  }

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
function ctools_nid_settings_form_submit(&$values) {
  $types = node_get_types();
  if (!empty($values['types'])) {
    foreach ($values['types'] as $type => $value) {
      if (empty($types[$type])) {
        unset($values['types'][$type]);
      }
    }
  }
}

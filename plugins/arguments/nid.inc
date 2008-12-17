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
    'context' => 'ctools_nid_context',
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

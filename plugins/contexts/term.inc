<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide a term context
 */

/**
 * Implementation of specially named hook_ctools_contexts().
 */
function ctools_term_ctools_contexts() {
  $args['term'] = array(
    'title' => t("Taxonomy term"),
    'description' => t('A single taxonomy term object.'),
    'context' => 'ctools_context_create_term',
    'settings form' => 'ctools_context_term_settings_form',
    'settings form validate' => 'ctools_context_term_settings_form_validate',
    'keyword' => 'term',
    'no ui' => TRUE,
    'context name' => 'term',
    'convert list' => array(
      'tid' => t('Term ID'),
      'name' => t('Term name'),
      'vid' => t('Vocabulary ID'),
    ),
    'convert' => 'ctools_context_term_convert',
  );
  return $args;
}

/**
 * It's important to remember that $conf is optional here, because contexts
 * are not always created from the UI.
 */
function ctools_context_create_term($empty, $data = NULL, $conf = FALSE) {
  $context = new ctools_context('term');
  $context->plugin = 'term';

  if ($empty) {
    return $context;
  }

  if ($conf) {
    $data = taxonomy_get_term($data['tid']);
  }

  if (!empty($data)) {
    $context->data     = $data;
    $context->title    = $data->name;
    $context->argument = $data->tid;
    return $context;
  }
}

/**
 * Convert a context into a string.
 */
function ctools_context_term_convert($context, $type) {
  switch ($type) {
    case 'tid':
      return $context->data->tid;
    case 'name':
      return $context->data->name;
    case 'vid':
      return $context->data->vid;
  }
}

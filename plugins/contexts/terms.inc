<?php
// $Id$


/**
 * @file
 *
 * Plugin to provide a terms context
 */

/**
 * Implementation of specially named hook_ctools_contexts().
 */
function ctools_terms_ctools_contexts() {
  $args['terms'] = array(
    'title' => t("Taxonomy terms"),
    'description' => t('Multiple taxonomy terms, as a group.'),
    'context' => 'ctools_context_create_terms',
    'settings form' => 'ctools_context_terms_settings_form',
    'settings form validate' => 'ctools_context_terms_settings_form_validate',
    'keyword' => 'terms',
    'no ui' => TRUE,
    'context name' => 'terms',
  );
  return $args;
}

/**
 * It's important to remember that $conf is optional here, because contexts
 * are not always created from the UI.
 */
function ctools_context_create_terms($empty, $data = NULL, $conf = FALSE) {
  // The input is expected to be an object as created by ctools_break_phrase
  // which contains a group of terms.

  $context = new ctools_context(array('terms', 'term'));
  $context->plugin = 'terms';

  if ($empty) {
    return $context;
  }

  if (!empty($data) && is_object($data)) {
    $context->operator = $data->operator;
    $context->tids     = $data->value;
    if (!isset($data->term)) {
      // load the first term:
      reset($context->tids);
      $data->term = taxonomy_get_term(current($context->tids));
    }
    $context->data     = $data->term;
    $context->title    = $data->term->name;
    $context->argument = implode($context->operator == 'or' ? '+' : ',', $context->tids);
    return $context;
  }
}

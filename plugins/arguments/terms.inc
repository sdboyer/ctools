<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide an argument handler for a Taxonomy term
 */

/**
 * Implementation of specially named hook_ctools_arguments().
 */
function ctools_terms_ctools_arguments() {
  $args['terms'] = array(
    'title' => t("Taxonomy term (multiple)"),
    // keyword to use for %substitution
    'keyword' => 'term',
    'description' => t('Creates a group of taxonomy terms from a list of tids separated by a comma or a plus sign. In general the first term of the list will be used for panes.'),
    'context' => 'ctools_terms_context',
    'settings form' => 'ctools_terms_settings_form',
    'placeholder form' => array(
      '#type' => 'textfield',
      '#description' => t('Enter a term ID or a list of term IDs separated by a + or a ,'),
    ),
  );
  return $args;
}

/**
 * Discover if this argument gives us the term we crave.
 */
function ctools_terms_context($arg = NULL, $conf = NULL, $empty = FALSE) {
  // If unset it wants a generic, unfilled context.
  if ($empty) {
    return ctools_context_create_empty('terms');
  }

  $terms = ctools_break_phrase($arg);
  if (empty($terms->value) || !empty($terms->invalid_input)) {
    return FALSE;
  }

  $context = ctools_context_create('terms', $terms);
  $context->original_argument = $arg;
  return $context;
}

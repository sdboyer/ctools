<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide an argument handler for a vocabulary id
 */

/**
 * Implementation of specially named hook_ctools_arguments().
 */
function ctools_vid_ctools_arguments() {
  $args['vid'] = array(
    'title' => t("Vocabulary ID"),
    // keyword to use for %substitution
    'keyword' => 'vocabulary',
    'description' => t('Creates a vocabulary context from a vocabulary ID argument.'),
    'context' => 'ctools_vid_context',
    'placeholder form' => array(
      '#type' => 'textfield',
      '#description' => t('Enter the vocabulary ID for this argument'),
    ),
  );
  return $args;
}

/**
 * Discover if this argument gives us the vocabulary we crave.
 */
function ctools_vid_context($arg = NULL, $conf = NULL, $empty = FALSE) {
  // If unset it wants a generic, unfilled context.
  if ($empty) {
    return ctools_context_create_empty('vocabulary');
  }

  if (!is_numeric($arg)) {
    return NULL;
  }

  $vocabulary = taxonomy_vocabulary_load($arg);
  if (!$vocabulary) {
    return NULL;
  }

  return ctools_context_create('vocabulary', $vocabulary);
}


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
function ctools_term_ctools_arguments() {
  $args['term'] = array(
    'title' => t("Taxonomy term"),
    // keyword to use for %substitution
    'keyword' => 'term',
    'description' => t('Creates a single taxonomy term from a taxonomy ID or taxonomy term name.'),
    'context' => 'ctools_term_context',
    'default' => array('input_form' => 'tid'),
    'settings form' => 'ctools_term_settings_form',
  );
  return $args;
}

/**
 * Discover if this argument gives us the term we crave.
 */
function ctools_term_context($arg = NULL, $conf = NULL, $empty = FALSE) {
  // If unset it wants a generic, unfilled context.
  if ($empty) {
    return ctools_context_create_empty('term');
  }

  switch ($conf['input_form']) {
    case 'tid':
    default:
      if (!is_numeric($arg)) {
        return FALSE;
      }
      $term = taxonomy_get_term($arg);
      break;

    case 'term':
      $terms = taxonomy_get_term_by_name($arg);
      if (count($terms) != 1) {
        foreach ($terms as $potential) {
          foreach ($conf['vids'] as $vid => $active) {
            if ($active == 1 && $potential->vid == $vid) {
              $term = $potential;
              // break out of the foreaches AND the case
              break 3;
            }
          }
        }
      }
      $term = array_shift($terms);
      break;
  }

  if (empty($term)) {
    return NULL;
  }

  if (!empty($conf['vids']) && array_filter($conf['vids']) && empty($conf['vids'][$term->vid])) {
    return NULL;
  }

  $context = ctools_context_create('term', $term);
  $context->original_argument = $arg;
  return $context;
}

/**
 * Settings form for the argument
 */
function ctools_term_settings_form(&$form, &$form_state, $conf) {
  // @todo allow synonym use like Views does.
  $form['settings']['input_form'] = array(
    '#title' => t('Argument type'),
    '#type' => 'radios',
    '#options' => array('tid' => t('Term ID'), 'term' => t('Term name')),
    '#default_value' => $conf['input_form'],
    '#prefix' => '<div class="clear-block">',
    '#suffix' => '</div>',
  );
}

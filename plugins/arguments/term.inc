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
    'description' => t('Restricts the argument to a taxonomy term.'),
    'context' => 'ctools_term_context',
    'settings form' => 'ctools_term_settings_form',
    'settings form submit' => 'ctools_term_settings_form_submit',
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
function ctools_term_settings_form($conf) {
  if (empty($conf)) {
    $conf = array(
      'input_form' => 'tid',
    );
  }

  $options = array();
  foreach (taxonomy_get_vocabularies() as $vid => $vocabulary) {
    $options[$vid] = $vocabulary->name;
  }

  $form['input_form'] = array(
    '#title' => t('Argument type'),
    '#type' => 'radios',
    '#options' => array('tid' => t('Term ID'), 'term' => t('Term name')),
    '#default_value' => $conf['input_form'],
    '#prefix' => '<div class="clear-block">',
    '#suffix' => '</div>',
  );


  $form['vids'] = array(
    '#title' => t('Vocabularies'),
    '#type' => 'checkboxes',
    '#options' => $options,
    '#description' => t('You can restrict this argument to use the checked vocabularies. Arguments from non-conforming vocabularies will be ignored, and ctools will behave as if no argument were given. Leave all unchecked to impose no restriction.'),
    '#default_value' => $conf['vids'],
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
function ctools_term_settings_form_submit(&$values) {
  $vocs = taxonomy_get_vocabularies();
  if (!empty($values['vids'])) {
    foreach ($values['vids'] as $vid => $value) {
      if (empty($vocs[$vid])) {
        unset($values['vids'][$vid]);
      }
    }
  }
}

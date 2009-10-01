<?php
// $Id$

/**
 * @file
 * Plugin to provide access control based upon specific terms.
 */

/**
 * Implementation of specially named hook_ctools_access().
 */
function ctools_term_ctools_access() {
  $args = array();
  $args['term'] = array(
    'title' => t("Taxonomy: term"),
    'description' => t('Control access by a specific term.'),
    'callback' => 'ctools_term_ctools_access_check',
    'default' => array('vids' => array()),
    'settings form' => 'ctools_term_ctools_access_settings',
    'settings form validation' => 'ctools_term_ctools_access_settings_validate',
    'settings form submit' => 'ctools_term_ctools_access_settings_submit',
    'summary' => 'ctools_term_ctools_acesss_summary',
    // 'required context' => new ctools_context_required(t('Term'), array('term', 'terms')),
  );

  return $args;
}

/**
 * Settings form for the 'by term' access plugin
 */
function ctools_term_ctools_access_settings(&$form, &$form_state, $conf) {
  // If no configuration was saved before, set some defaults.
  if (empty($conf)) {
    $conf = array(
      'vid' => 0,
    );
  }
  if (!isset($conf['vid'])) {
    $conf['vid'] = 0;
  }
  
  $form['settings']['vid'] = array(
    '#title' => t('Vocabulary'),
    '#type' => 'select',
    '#options' => array(),
    '#description' => t('Select the vocabulary for this form.'),
    '#id' => 'ctools-select-vid',
    '#default_value' => $conf['vid'],
    '#required' => TRUE,
  );

  ctools_include('dependent');
  $options = array();

  // A note: Dependency works strangely on these forms as they have never been
  // updated to a more modern system so they are not individual forms of their
  // own like the content types.

  $form['settings']['#tree'] = TRUE;
  
  // Loop over each of the configured vocabularies.
  foreach (taxonomy_get_vocabularies() as $vid => $vocabulary) {
    $options[$vid] = $vocabulary->name;
    $form['settings'][$vocabulary->vid] = array(
      '#title' => t('Terms'),
      '#description' => t('Select a term or terms from @vocabulary.', array('@vocabulary' => $vocabulary->name)), //. $description,
      '#process' => array('ctools_dependent_process'),
      '#dependency' => array('ctools-select-vid' => array($vocabulary->vid)),
      '#default_value' => $conf[$vid],
      '#multiple' => TRUE,
    );

    // If it's a tag, use an autocomplete.
    if ($vocabulary->tags) {
      $form['settings'][$vocabulary->vid]['#type'] = 'textfield';
      $form['settings'][$vocabulary->vid]['#autocomplete_path'] = 'taxonomy/autocomplete/' . $vocabulary->vid;
    }

    // Other vocabs just show a list.
    else {
      $terms = array();
      foreach (taxonomy_get_tree($vid) as $x => $term) {
        $terms[$term->tid] = str_repeat('-', $term->depth) . ($term->depth ? ' ' : '') . $term->name;
      }
      $form['settings'][$vocabulary->vid]['#type'] = 'select';
      $form['settings'][$vocabulary->vid]['#options'] = $terms;
      unset($terms);
    }
  }
  $form['settings']['vid']['#options'] = $options;
}

/**
 * Check for access.
 */
function ctools_term_ctools_access_check($conf, $context) {
  // As far as I know there should always be a context at this point, but this
  // is safe.
  if (empty($context) || empty($context->data) || empty($context->data->vid) || empty($context->data->tid)) {
    return FALSE;
  }

  // Get the $vid.
  if (!isset($conf['vid'])) {
    return FALSE;
  }
  $vid = $conf['vid'];

  // Get the terms.
  if (!isset($conf[$vid])) {
    return FALSE;
  }
  $terms = array_filter($conf[$vid]);

  $return = FALSE;
  // Tags get saved as an imploded array of strings.
  if (!is_array($terms)) {
    $terms = expand(', ', $terms);
  }
  // Loop over each of the values.
  foreach ($terms as $term) {
    if ($context->data->tid == $term) {
      $return = TRUE;
    }
  }

  return $return;
}

/**
 * Provide a summary description based upon the checked terms.
 */
function ctools_term_ctools_acesss_summary($conf, $context) {
  $vocab = taxonomy_vocabulary_load($conf['vid']);
  if ($vocab->tags) {
    $terms = explode(', ', $conf[$vocab->vid]);
  }
  else {
    $terms = array();
    foreach ($conf[$vocab->vid] as $tid) {
      $term = taxonomy_get_term($tid);
      $terms[] = $term->name;
    }
  }

  return format_plural(count($terms),
    '@vocab can be the term "@terms"',
    '@vocab can be one of these terms: @terms',
    array('@terms' => implode(', ', $terms),
      '@vocab' => $vocab->name));
}

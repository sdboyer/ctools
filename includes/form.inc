<?php
// $Id$

/**
 * @file
 * CTools' replacements for Drupal's form functions.
 *
 * Primarily, ctools_build_form is meant to be a replacement for drupal_get_form().
 *
 * Instead of sending arguments through to the form builder, a form state array
 * is passed through. This form_state can contain any arguments needed, and it can
 * also be used to return data to the calling function.
 *
 * This can allow cleaner separation of the form from the storage mechanism.
 */

/**
 * Build a form, similar to drupal_get_form(). However, arguments
 * to the form builder are not sent through. Instead, the $form_state
 * can be given all the necessary data to fully utilize the form.
 */
function ctools_build_form($form_id, &$form_state) {
  if (isset($form_state['args'])) {
    $form_state['build_info']['args'] = $form_state['args'];
  }
  if (isset($form_state['wrapper callback'])) {
    $form_state['wrapper_callback'] = $form_state['wrapper callback'];
  }
  return drupal_build_form($form_id, $form_state);
}

/**
 * ctools' replacement of drupal_rebuild_form.
 *
 * This change merely respects a form's wishes not to be cached.
 */
function ctools_rebuild_form($form_id, &$form_state, $args, $form_build_id = NULL) {
  // Remove the first argument. This is $form_id.when called from
  // drupal_get_form and the original $form_state when called from some AHAH
  // callback. Neither is needed. After that, put in the current state.
  array_unshift($args, 'placeholder');
  $args[0] = &$form_state; // Replaces placeholder.
  // And the form_id.
  array_unshift($args, $form_id);

  if (isset($form_state['wrapper callback']) && function_exists($form_state['wrapper callback'])) {
    // If there is a wrapper callback, we do not use drupal_retrieve_form.
    // Instead, we call $form_id builder function directly. This means the args
    // are *different* for forms used this way, which may not be ideal but
    // is necessary right now.
    $form = array();
    $form_state['wrapper callback']($form, $form_state);
    if (function_exists($form_id)) {
      $form_id($form, $form_state);
    }
  }
  else {
    $form = call_user_func_array('drupal_retrieve_form', $args);
  }

  if (!isset($form_build_id)) {
    // We need a new build_id for the new version of the form.
    $form_build_id = 'form-' . md5(mt_rand());
  }
  $form['#build_id'] = $form_build_id;
  // Flush form ID element cache because we may be rebuilding
  // a form in a way that Drupal's FAPI isn't used to which
  // causes unnecessary form ID changes.
  form_clean_id(NULL, TRUE);
  drupal_prepare_form($form_id, $form, $form_state);

  if (empty($form['#no_cache'])) {
    // Now, we cache the form structure so it can be retrieved later for
    // validation. If $form_state['storage'] is populated, we'll also cache
    // it so that it can be used to resume complex multi-step processes.
    form_set_cache($form_build_id, $form, $form_state);
  }

  // Originally this called drupal_process_form, but all that happens there
  // is form_builder and then submission; and the rebuilt form is not
  // allowed to submit. Therefore, just do this:
  $form['#post'] = $form_state['input'];
  $form = form_builder($form_id, $form, $form_state);

  return $form;
}

/**
 * ctools' replacement for drupal_process_form that accepts commands
 * not to redirect, as well as forcing processing of 'get' method forms.
 */
function ctools_process_form($form_id, &$form, &$form_state) {
  // submitting, and handling the results returned by its submission
  // handlers. Submit handlers accumulate data in the form_state by
  // altering the $form_state variable, which is passed into them by
  // reference.
  $form_state['values'] = array();

  // With $_GET, these forms are always submitted.
  if ($form_state['method'] == 'get') {
    if (!isset($form['#post']['form_build_id'])) {
      $form['#post']['form_build_id'] = $form['#build_id'];
    }
    if (!isset($form['#post']['form_id'])) {
      $form['#post']['form_id'] = $form_id;
    }
    if (!isset($form['#post']['form_token']) && isset($form['#token'])) {
      $form['#post']['form_token'] = drupal_get_token($form['#token']);
    }
  }

  $form = form_builder($form_id, $form, $form_state);
  // Only process the form if it is programmed or the form_id coming
  // from the POST data is set and matches the current form_id.

  if ((!empty($form['#programmed'])) || (!empty($form['#post']) && (isset($form['#post']['form_id']) && ($form['#post']['form_id'] == $form_id)))) {
    ctools_validate_form($form_id, $form, $form_state);

    // form_clean_id() maintains a cache of element IDs it has seen,
    // so it can prevent duplicates. We want to be sure we reset that
    // cache when a form is processed, so scenerios that result in
    // the form being built behind the scenes and again for the
    // browser don't increment all the element IDs needlessly.
    form_clean_id(NULL, TRUE);

    if ((!empty($form_state['submitted'])) && !form_get_errors() && empty($form_state['rebuild'])) {
      $form_state['redirect'] = NULL;
      form_execute_handlers('submit', $form, $form_state);

      // We'll clear out the cached copies of the form and its stored data
      // here, as we've finished with them. The in-memory copies are still
      // here, though.
      if (variable_get('cache', CACHE_DISABLED) == CACHE_DISABLED && !empty($form_state['values']['form_build_id'])) {
        cache_clear_all('form_' . $form_state['values']['form_build_id'], 'cache_form');
        cache_clear_all('storage_' . $form_state['values']['form_build_id'], 'cache_form');
      }

      // If batches were set in the submit handlers, we process them now,
      // possibly ending execution. We make sure we do not react to the batch
      // that is already being processed (if a batch operation performs a
      // drupal_execute).
      if ($batch = &batch_get() && !isset($batch['current_set'])) {
        // The batch uses its own copies of $form and $form_state for
        // late execution of submit handers and post-batch redirection.
        $batch['form'] = $form;
        $batch['form_state'] = $form_state;
        $batch['progressive'] = !$form['#programmed'];
        batch_process();
        // Execution continues only for programmatic forms.
        // For 'regular' forms, we get redirected to the batch processing
        // page. Form redirection will be handled in _batch_finished(),
        // after the batch is processed.
      }

      // If no submit handlers have populated the $form_state['storage']
      // bundle, and the $form_state['rebuild'] flag has not been set,
      // we're finished and should redirect to a new destination page
      // if one has been set (and a fresh, unpopulated copy of the form
      // if one hasn't). If the form was called by drupal_execute(),
      // however, we'll skip this and let the calling function examine
      // the resulting $form_state bundle itself.
      if (!$form['#programmed'] && empty($form_state['rebuild']) && empty($form_state['storage'])) {
        if (!empty($form_state['no_redirect'])) {
          $form_state['executed'] = TRUE;
        }
        else {
          drupal_redirect_form($form, $form_state['redirect']);
        }
      }
    }
  }
}

/**
 * The original version of drupal_validate_form does not have an override for
 * the static check to only validate a form id once. Unfortunately, we need
 * to be able to override this.
 */
function ctools_validate_form($form_id, $form, &$form_state) {
  static $validated_forms = array();

  if (isset($validated_forms[$form_id]) && empty($form_state['must_validate'])) {
    return;
  }

  // If the session token was set by drupal_prepare_form(), ensure that it
  // matches the current user's session.
  if (isset($form['#token'])) {
    if (!drupal_valid_token($form_state['values']['form_token'], $form['#token'])) {
      // Setting this error will cause the form to fail validation.
      form_set_error('form_token', t('Validation error, please try again. If this error persists, please contact the site administrator.'));
    }
  }

  if (!empty($form_state['clicked_button']['#skip validation'])) {
    return;
  }

  _form_validate($form, $form_state, $form_id);
  $validated_forms[$form_id] = TRUE;
}


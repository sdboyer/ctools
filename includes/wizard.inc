<?php
// $Id$

/**
 * @file
 * CTools' multi-step form wizard tool.
 *
 * This tool enables the creation of multi-step forms that go from one
 * form to another. The forms themselves can allow branching if they
 * like, and there are a number of configurable options to how
 * the wizard operates.
 *
 * The wizard can also be friendly to ajax forms, such as when used
 * with the modal tool.
 *
 * TODO: should the wizard also perform object caching? We'll see
 *       as we develop this if it should happen within the wizard
 *       or outside the wizard.
 */

/**
 * Display a multi-step form.
 *
 * Aside from the addition of the $form_info which contains an array of
 * information and configuration so the multi-step wizard can do its thing,
 * this function works a lot like ctools_build_form.
 *
 * Remember that the form builders for this form will receive
 * &$form, &$form_state, NOT just &$form_state and no additional args.
 *
 * Do NOT use #required => TRUE with these forms as that validation
 * cannot be skipped for the CANCEL button.
 *
 * @param $form_info
 *   An array of form info. @todo document the array.
 * @param $step
 *   The current form step.
 * @param &$form_state
 *   The form state array; this is a reference so the caller can get back
 *   whatever information the form(s) involved left for it.
 */
function ctools_wizard_multistep_form($form_info, $step, &$form_state) {
  $form_state['step'] = $step;
  $form_state['form_info'] = $form_info;

  // Ensure we have form information for the current step.
  if (!isset($form_info['forms'][$step])) {
    return;
  }

  // Ensure that whatever include file(s) were requested by the form info are
  // actually included.
  $info = $form_info['forms'][$step];

  if (!empty($info['include'])) {
    if (is_array($info['include'])) {
      foreach ($info['include'] as $file) {
        require_once './' . $file;
      }
    }
    else {
      require_once './' . $info['include'];
    }
  }

  // This tells ctools_build_form to apply our wrapper to the form. It
  // will give it buttons and the like.
  $form_state['wrapper callback'] = 'ctools_wizard_wrapper';
  $form_state['re_render'] = FALSE;
  $form_state['no_redirect'] = TRUE;

  ctools_include('form');
  $output = ctools_build_form($info['form id'], $form_state);

  if (!$output) {
    // We use the plugins get_function format because it's powerful and
    // not limited to just functions.
    ctools_include('plugins');

    if (isset($form_state['clicked_button']['#wizard type'])) {
      $type = $form_state['clicked_button']['#wizard type'];
      // If the finish button was clicked, call the finish callback.
      if ($function = ctools_plugin_get_function($form_info, "$type callback")) {
        $function($form_state);
      }
    }

    // redirect, if one is set.
    drupal_redirect_form(array(), $form_state['redirect']);
  }

  return $output;
}

/**
 * Provide a wrapper around another form for adding multi-step information.
 */
function ctools_wizard_wrapper(&$form, &$form_state) {
  $form_info = &$form_state['form_info'];
  $info = $form_info['forms'][$form_state['step']];

  // Determine the next form from this step.
  // Create a form trail if we're supposed to have one.
  $trail = array();
  $previous = TRUE;
  foreach ($form_info['order'] as $id => $title) {
    if ($id == $form_state['step']) {
      $previous = FALSE;
      $class = 'wizard-trail-current';
    }
    elseif ($previous) {
      $not_first = TRUE;
      $class = 'wizard-trail-previous';
      $form_state['previous'] = $id;
    }
    else {
      $class = 'wizard-trail-next';
      if (!isset($form_state['next'])) {
        $form_state['next'] = $id;
      }
      if (empty($form_info['show trail'])) {
        break;
      }
    }

    if (!empty($form_info['show trail'])) {
      $trail[] = '<span class="' . $class . '">' . $title . '</span>';
    }
  }

  // Display the trail if instructed to do so.
  if (!empty($form_info['show trail'])) {
    ctools_add_css('wizard');
//    drupal_add_css(drupal_get_path('module', 'delegator') . '/css/task-handlers.css');
    $form['ctools_trail'] = array(
      '#value' => theme(array('ctools_wizard_trail__' . $form_info['id'], 'ctools_wizard_trail'), $trail),
      '#weight' => -1000,
    );
  }

  // Ensure buttons stay on the bottom.
  $form['buttons'] = array(
    '#prefix' => '<div class="clear-block">',
    '#suffix' => '</div>',
    '#weight' => 1000,
  );

  if (!empty($form_info['show back']) && isset($form_state['previous'])) {
    $form['buttons']['previous'] = array(
      '#type' => 'submit',
      '#value' => t('Back'),
      '#next' => $form_state['previous'],
      '#wizard type' => 'next',
      '#weight' => -2000,
      // hardcode the submit so that it doesn't try to save data.
      '#submit' => array('ctools_wizard_submit'),
    );
  }

  // If there is a next form, place the next button.
  if (isset($form_state['next'])) {
    $form['buttons']['next'] = array(
      '#type' => 'submit',
      '#value' => t('Continue'),
      '#next' => $form_state['next'],
      '#wizard type' => 'next',
      '#weight' => -1000,
    );
  }

  // There are two ways the return button can appear. If this is not the
  // end of the form list (i.e, there is a next) then it's "update and return"
  // to be clear. If this is the end of the path and there is no next, we
  // call it 'Finish'.

  // Even if there is no direct return path (some forms may not want you
  // leaving in the middle) the final button is always a Finish and it does
  // whatever the return action is.
  if (!empty($form_info['show return']) && !empty($form_state['next'])) {
    $form['buttons']['return'] = array(
      '#type' => 'submit',
      '#value' => t('Update and return'),
      '#wizard type' => 'return',
    );
  }
  else if (empty($form_state['next'])) {
    $form['buttons']['return'] = array(
      '#type' => 'submit',
      '#value' => t('Finish'),
      '#wizard type' => 'finish',
    );
  }

  // If we are allowed to cancel, place a cancel button.
  if (isset($form_info['cancel path'])) {
    $form['buttons']['cancel'] = array(
      '#type' => 'submit',
      '#value' => t('Cancel'),
      '#submit' => array('ctools_wizard_cancel'),
      '#wizard type' => 'cancel',
    );
  }

  // Set up our submit handler after theirs. Since putting something here will
  // skip Drupal's autodetect, we autodetect for it.

  // We make sure ours is after theirs so that they get to change #next if
  // the want to.
  $form['#submit'] = array();
  if (function_exists($info['form id'] . '_submit')) {
    $form['#submit'][] = $info['form id'] . '_submit';
  }
  $form['#submit'][] = 'ctools_wizard_submit';
}

/**
 * On a submit, go to the next form.
 */
function ctools_wizard_submit(&$form, &$form_state) {
  if (isset($form_state['clicked_button']['#wizard type'])) {
    $type = $form_state['clicked_button']['#wizard type'];
    if ($type == 'return' || $type == 'finish') {
      $form_state['redirect'] = $form_state['form_info']['return path'];
      // Do we need to do something here or just let it go?
    }
    else {
      $form_state['redirect'] = ctools_wizard_get_path($form_state['form_info'], $form_state['clicked_button']['#next']);
    }
  }
}

/**
 * Create a path from the form info and a given step.
 */
function ctools_wizard_get_path($form_info, $step) {
  return str_replace('%step', $step, $form_info['path']);
}

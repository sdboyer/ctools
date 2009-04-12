<?php
// $Id$

/**
 * @file
 * Custom content type.
 *
 * This content type is nothing more than a title and a body that is entered
 * by the user and run through standard filters. The information is stored
 * right in the config, so each custom content is unique.
 */

/**
 * Callback function to supply a list of content types.
 */
function ctools_custom_ctools_content_types() {
  return array(
    'single' => TRUE,
    'no title override' => TRUE,
    'title' => t('New custom content'),
    'icon' => 'icon_block_custom.png',
    'description' => t('Create a completely custom piece of HTML content.'),
    'category' => t('Custom'),
    'defaults' => array('title' => '', 'body' => '', 'format' => FILTER_FORMAT_DEFAULT),
    // render callback is automatically deduced:
    // 'render callback' => 'ctools_custom_content_type_render',
  );
}

/**
 * Output function for the 'custom' content type. Outputs a custom
 * based on the module and delta supplied in the configuration.
 */
function ctools_custom_content_type_render($subtype, $conf) {
  static $delta = 0;

  $block          = new stdClass();
  $block->subtype = ++$delta;
  $block->title   = filter_xss_admin($conf['title']);
  $block->content = check_markup($conf['body'], $conf['format'], FALSE);

  return $block;
}

/**
 * Callback to provide the administrative title of the custom content.
 */
function ctools_custom_content_type_admin_title($subtype, $conf) {
  $output = t('Custom');
  if (!empty($conf['title'])) {
    $output .= " (" . filter_xss_admin($conf['title']) . ")";
  }
  return $output;
}

/**
 * Callback to provide administrative info. In this case we'll render the
 * content as long as it's not PHP, which is too risky to render here.
 */
function ctools_custom_content_type_admin_info($subtype, $conf) {
  $block = new stdClass();
  $block->title = filter_xss_admin($conf['title']);
  // We don't want to render php output on preview here, because if something is
  // wrong the whole display will be borked. So we check to see if the php
  // evaluator filter is being used, and make a temporary change to the filter
  // so that we get the printed php, not the eval'ed php.
  $php_filter = FALSE;
  foreach (filter_list_format($conf['format']) as $filter) {
    if ($filter->name == 'PHP evaluator') { // TODO stupid way to check
      $php_filter = TRUE;
    }
  }
  // If a php filter is active, pass 1 to use core's most restrictive filter.
  $block->content = check_markup($conf['body'], $php_filter ? 1 : $conf['format']);
  return $block;
}

/**
 * Returns an edit form for the custom type.
 */
function ctools_custom_content_type_edit_form(&$form, &$form_state) {
  $conf = $form_state['conf'];
  $form['title'] = array(
    '#type' => 'textfield',
    '#default_value' => $conf['title'],
    '#title' => t('Title'),
  );
  $form['body'] = array(
    '#title' => t('Body'),
    '#type' => 'textarea',
    '#default_value' => $conf['body'],
  );
  $parents[] = 'format';
  $form['format'] = filter_form($conf['format'], 1, $parents);

  return $form;
}

/**
 * The submit form stores the data in $conf.
 */
function ctools_custom_content_type_edit_form_submit(&$form, &$form_state) {
  $form_state['conf'] = $form_state['values'];
}

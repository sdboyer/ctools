<?php
// $Id$

/**
 * @file
 * Plugin to handle the 'page_help' content type which allows the
 * help text of the current page to be embedded into a panel.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t('Help'),
  'single' => TRUE,
  'icon' => 'icon_page.png',
  'description' => t('Add the help text of the current page as content.'),
  'category' => t('Page elements'),
);

/**
 * Output function for the 'page_help' content type.
 *
 * Outputs the breadcrumb for the current page.
 */
function ctools_page_help_content_type_render($subtype, $conf, $panel_args) {
  $token = '<!-- ctools-page-help -->';
  ctools_set_page_token($token, 'variable', 'help');

  $block = new stdClass();
  $block->content = $token;

  return $block;
}


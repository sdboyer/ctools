<?php
// $Id$

/**
 * @file
 * Theme registry for dropdown-div tool.
 */
function ctools_dropdown_theme(&$items) {
  $items['ctools_dropdown'] = array(
    'arguments' => array('title' => NULL, 'links' => NULL),
    'file' => 'includes/dropdown.theme.inc',
  );
}

/**
 * Create a dropdown menu.
 */
function theme_ctools_dropdown($title, $links, $class = '') {
  // Provide a unique identifier for every dropdown on the page.
  static $id = 0;
  $id++;

  $class = 'ctools-dropdown-no-js ctools-dropdown' . ($class ? (' ' . $class) : '');

  ctools_add_js('dropdown');
  ctools_add_css('dropdown');

  $output = '';

  $output .= '<div class="' . $class . '" id="ctools-dropdown-' . $id . '">';
  $output .= '<a href="#" class="ctools-dropdown-link">' . check_plain($title) . '</a>';
  $output .= '<div class="ctools-dropdown-container">';
  $output .= theme('links', $links);
  $output .= '</div>'; // container
  $output .= '</div>'; // dropdown
  return $output;
}

<?php
// $Id$

/**
 * @file
 * Theme function for the collapsible div tool.
 *
 * Call theme('ctools_collapsible', $handle, $content, $collapsed) to draw the
 * div. The theme function is not necessary; you can add the classes, js and css
 * yourself if you really want to.
 */

function theme_ctools_collapsible($handle, $content, $collapsed = FALSE) {
  drupal_add_js(drupal_get_path('module', 'ctools') . '/js/collapsible-div.js');
  drupal_add_css(drupal_get_path('module', 'ctools') . '/css/collapsible-div.css');

  $class = $collapsed ? ' ctools-collapsed' : '';
  $output = '<div class="ctools-collapsible-container' . $class . '">';
  $output .=   '<div class="ctools-collapsible-handle">';
  $output .=   $handle;
  $output .=   '</div>';
  $output .=   '<div class="ctools-collapsible-content">';
  $output .=   $content;
  $output .=   '</div>';
  $output .= '</div>';

  return $output;
}
<?php
// $Id$

/**
 * @file
 * Theme registry for collapsible-div tool.
 */
function ctools_collapsible_theme($items) {
  $items['ctools_collapsible'] = array(
    'arguments' => array('handle' => NULL, 'content' => NULL, 'collapsed' => FALSE),
    'file' => 'includes/collapsible-div.inc',
  );
}

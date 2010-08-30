<?php
// $Id$

/**
 * @file
 * Contains menu item registration for the page manager page wizards tool.
 */

function ctools_page_wizard_menu(&$items) {
  if (!module_exists('page_manager')) {
    return;
  }

  $base = array(
    'access arguments' => array('use page manager'),
    'file' => 'includes/page-wizard.inc',
    'type' => MENU_CALLBACK,
  );

  $items['admin/build/pages/wizard/%'] = array(
    'title' => 'Wizard',
    'page callback' => 'page_manager_page_wizard',
    'page arguments' => array(4),
  ) + $base;
}

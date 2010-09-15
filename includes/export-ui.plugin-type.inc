<?php
// $Id$

/**
 * @file
 * Contains plugin type registration information for the context tool.
 *
 * Don't actually need to declare anything for these plugin types right now,
 * apart from the fact that they exist. So, an empty array.
 */

function ctools_export_ui_plugin_type(&$items) {
  $items['export_ui'] = array(
    'process' => 'ctools_export_ui_process',
  );
}
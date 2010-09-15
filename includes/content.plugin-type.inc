<?php
// $Id$

/**
 * @file
 * Contains plugin type registration information for the content tool.
 */

function ctools_content_plugin_type(&$items) {
  $items['content_types'] = array(
    'cache' => FALSE,
    'process' => 'ctools_content_process',
  );
}
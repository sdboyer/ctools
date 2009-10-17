<?php
// $Id$

/**
 * @file
 * Contains menu item registration for the content tool.
 *
 * The menu items registered are AJAX callbacks for the things like
 * autocomplete and other tools needed by the content types.
 */

function ctools_content_menu(&$items) {
  $base = array(
    'access arguments' => array('access content'),
    'type' => MENU_CALLBACK,
    'file' => 'includes/content.menu.inc',
  );
  $items['ctools/autocomplete/node'] = array(
    'page callback' => 'ctools_content_autocomplete_node',
  ) + $base;
}

/**
 * Helper function for autocompletion of node titles.
 */
function ctools_content_autocomplete_node($string) {
  if ($string != '') {
    $preg_matches = array();
    $match = preg_match('/\[nid: (\d+)\]/', $string, $preg_matches);
    if (!$match) {
      $match = preg_match('/^nid: (\d+)/', $string, $preg_matches);
    }
    if ($match) {
      $arg = array(':nid' => $preg_matches[1]);
      $where = "n.nid = :nid";
    }
    else {
      $arg = array(':title' => '%' . $string . '%');
      $where = "LOWER(n.title) LIKE LOWER(:title)";
    }
    $result = db_query_range('SELECT n.nid, n.title, u.name FROM {node} n INNER JOIN {users} u ON u.uid = n.uid WHERE ' . $where, 0, 10, $arg);

    $matches = array();
    foreach ($result as $node) {
      $name = empty($node->name) ? variable_get('anonymous', t('Anonymous')) : check_plain($node->name);
      $matches[$node->title . " [nid: $node->nid]"] = '<span class="autocomplete_title">' . check_plain($node->title) . '</span> <span class="autocomplete_user">(' . t('by @user', array('@user' => $name)) . ')</span>';
    }
    drupal_json_output($matches);
  }
}

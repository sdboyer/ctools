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
    // @todo verify the query logic here, it's untested.
    // Set up the query
    $query = db_select('node', 'n');
    $query->innerJoin('users', 'u', 'n.uid = u.uid');
    $query->fields('n', array('nid', 'title'))
      ->fields('u', array('name'))
      ->range(0, 10)
      ->addTag('node_access'); // ensure it's behind proper node_access logic

    $preg_matches = array();
    $match = preg_match('/\[nid: (\d+)\]/', $string, $preg_matches);
    if (!$match) {
      $match = preg_match('/^nid: (\d+)/', $string, $preg_matches);
    }
    if ($match) {
      $query->condition('n.nid', $preg_matches[1]);
    }
    else {
      $query->where('LOWER(n.title) LIKE LOWER(:title)', array(':title' => '%' . db_like($string) . '%'));
    }
    if (!user_access('administer nodes')) {
      $query->condition('n.status', 1);
    }

    $matches = array();
    foreach ($query->execute() as $nodeish) {
      $name = empty($nodeish->name) ? variable_get('anonymous', t('Anonymous')) : check_plain($nodeish->name);
      $matches[$nodeish->title . " [nid: $nodeish->nid]"] = '<span class="autocomplete_title">' . check_plain($node->title) . '</span> <span class="autocomplete_user">(' . t('by @user', array('@user' => $name)) . ')</span>';
    }
    drupal_json_output($matches);
  }
}

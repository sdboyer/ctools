<?php
// $Id$

/**
 * @file
 * General menu helper functions.
 *
 * The menu system was completely revamped in Drupal 6; as such it is not as
 * mature as some other systems and is missing some API functions. This
 * file helps smooth some edges out.
 */

/**
 * Dynamically add a tab to the current path.
 *
 * This function allows you to dynamically add tabs to the current path.
 * There are several important considerations to this:
 *
 * - First, Drupal doesn't really allow this. CTools lets this happen by
 *   overriding the theme function that displays the tabs. That means that
 *   custom themes which do not use CTools functions will not get the new
 *   tabs. You can provide instructions to your users about how to deal with
 *   this, but you should be prepared for some users not getting the new tabs
 *   and not knowing why.
 * - Second, if there is only 1 tab, Drupal will not show it. Therefore, if
 *   you are only adding one tab, you should find a way to make sure there is
 *   already tab, or instead add 2.
 * - Third, the caller is responsible for providing access control to these
 *   links.
 *
 * @param $link
 *   An array describing this link. It must contain:
 *   - 'title': The printed title of the link.
 *   - 'href': The path of the link. This is an argument to l() so it has all
 *     of those features and limitations.
 *   - 'options': Any options that go to l, including query, fragment and html
 *     options necessary.
 *   - 'weight': The weight to use in ordering the tabs.
 *   - 'type': Optional. If set to MENU_DEFAULT_LOCAL_TASK this can be used to
 *     add a fake 'default' local task, which is useful if you have to add
 *     tabs to a page that has noen.
 */
function ctools_menu_add_tab($link = NULL) {
  static $links = array();
  if (isset($link)) {
    $links[$link['href']] = $link;
  }

  return $links;
}

/**
 * CTools replacement for menu_get_menu_trail that allows us to take apart
 * the menu trail if necessary.
 */
function ctools_get_menu_trail($path = NULL) {
  $trail   = array();
  $trail[] = array('title' => t('Home'), 'href' => '<front>', 'localized_options' => array(), 'type' => 0);
  $item    = menu_get_item($path);

  // Check whether the current item is a local task (displayed as a tab).
  if ($item['tab_parent']) {
    // The title of a local task is used for the tab, never the page title.
    // Thus, replace it with the item corresponding to the root path to get
    // the relevant href and title.  For example, the menu item corresponding
    // to 'admin' is used when on the 'By module' tab at 'admin/by-module'.
    $parts = explode('/', $item['tab_root']);
    $args = arg();
    // Replace wildcards in the root path using the current path.
    foreach ($parts as $index => $part) {
      if ($part == '%') {
        $parts[$index] = $args[$index];
      }
    }
    // Retrieve the menu item using the root path after wildcard replacement.
    $root_item = menu_get_item(implode('/', $parts));
    if ($root_item && $root_item['access']) {
      $item = $root_item;
    }
  }

  $tree = ctools_menu_tree_page_data($item, menu_get_active_menu_name());
  list($key, $curr) = each($tree);

  while ($curr) {
    // Terminate the loop when we find the current path in the active trail.
    if ($curr['link']['href'] == $item['href']) {
      $trail[] = $curr['link'];
      $curr = FALSE;
    }
    else {
      // Add the link if it's in the active trail, then move to the link below.
      if ($curr['link']['in_active_trail']) {
        $trail[] = $curr['link'];
        $tree = $curr['below'] ? $curr['below'] : array();
      }
      list($key, $curr) = each($tree);
    }
  }
  // Make sure the current page is in the trail (needed for the page title),
  // but exclude tabs and the front page.
  $last = count($trail) - 1;
  if ($trail[$last]['href'] != $item['href'] && !(bool)($item['type'] & MENU_IS_LOCAL_TASK) && !drupal_is_front_page()) {
    $trail[] = $item;
  }

  return $trail;
}

/**
 * Get the data structure representing a named menu tree, based on the current page.
 *
 * The tree order is maintained by storing each parent in an individual
 * field, see http://drupal.org/node/141866 for more.
 *
 * @param $menu_name
 *   The named menu links to return
 *
 * @return
 *   An array of menu links, in the order they should be rendered. The array
 *   is a list of associative arrays -- these have two keys, link and below.
 *   link is a menu item, ready for theming as a link. Below represents the
 *   submenu below the link if there is one, and it is a subtree that has the
 *   same structure described for the top-level array.
 */
function ctools_menu_tree_page_data($item, $menu_name = 'navigation') {
  static $tree = array();

  // Generate a cache ID (cid) specific for this page.
  $cid = 'links:'. $menu_name .':page-cid:'. $item['href'] .':'. (int)$item['access'];

  if (!isset($tree[$cid])) {
    // If the static variable doesn't have the data, check {cache_menu}.
    $cache = cache_get($cid, 'cache_menu');
    if ($cache && isset($cache->data)) {
      // If the cache entry exists, it will just be the cid for the actual data.
      // This avoids duplication of large amounts of data.
      $cache = cache_get($cache->data, 'cache_menu');
      if ($cache && isset($cache->data)) {
        $data = $cache->data;
      }
    }
    // If the tree data was not in the cache, $data will be NULL.
    if (!isset($data)) {
      // Build and run the query, and build the tree.
      if ($item['access']) {
        // Check whether a menu link exists that corresponds to the current path.
        $args = array($item['href']);
        if (drupal_is_front_page()) {
          $args[] = '<front>';
        }
        $parents = db_query("SELECT p1, p2, p3, p4, p5, p6, p7, p8 FROM {menu_links} WHERE menu_name = :menu_name AND link_path IN (:href)", array(':menu_name' => $menu_name, ':href' => $args))->fetchAll(PDO::FETCH_ASSOC);

        if (empty($parents)) {
          // If no link exists, we may be on a local task that's not in the links.
          // TODO: Handle the case like a local task on a specific node in the menu.
          $parents = db_query("SELECT p1, p2, p3, p4, p5, p6, p7, p8 FROM {menu_links} WHERE menu_name = :menu_name AND link_path = :path", array(':menu_name' => $menu_name, ':path' => $item['tab_root']))->fetchAll(PDO::FETCH_ASSOC);
        }
        // We always want all the top-level links with plid == 0.
        $parents[] = '0';

        // Use array_values() so that the indices are numeric for array_merge().
        $args = $parents = array_unique(array_values($parents));
        $expanded = variable_get('menu_expanded', array());
        // Check whether the current menu has any links set to be expanded.
        if (in_array($menu_name, $expanded)) {
          // Collect all the links set to be expanded, and then add all of
          // their children to the list as well.
          do {
            $result = db_query('SELECT mlid FROM {menu_links} WHERE menu_name = :menu_name AND expanded = 1 AND has_children = 1 AND plid IN (:args) AND mlid NOT IN (:args)', array(':menu_name' => $menu_name, ':args' => $args))->fetchAll(PDO::FETCH_ASSOC);
            $num_rows = FALSE;
            while ($item = db_fetch_array($result)) {
              $args[] = $item['mlid'];
              $num_rows = TRUE;
            }
            $placeholders = implode(', ', array_fill(0, count($args), '%d'));
          } while ($num_rows);
        }
        array_unshift($args, $menu_name);
      }
      else {
        // Show only the top-level menu items when access is denied.
        $args = array($menu_name, '0');
        $placeholders = '%d';
        $parents = array();
      }
      // Select the links from the table, and recursively build the tree. We
      // LEFT JOIN since there is no match in {menu_router} for an external
      // link.
      $data['tree'] = menu_tree_data(db_query("
        SELECT m.load_functions, m.to_arg_functions, m.access_callback, m.access_arguments, m.page_callback, m.page_arguments, m.title, m.title_callback, m.title_arguments, m.type, m.description, ml.*
        FROM {menu_links} ml LEFT JOIN {menu_router} m ON m.path = ml.router_path
        WHERE ml.menu_name = '%s' AND ml.plid IN (". $placeholders .")
        ORDER BY p1 ASC, p2 ASC, p3 ASC, p4 ASC, p5 ASC, p6 ASC, p7 ASC, p8 ASC, p9 ASC", $args), $parents);
      $data['node_links'] = array();
      menu_tree_collect_node_links($data['tree'], $data['node_links']);
      // Cache the data, if it is not already in the cache.
      $tree_cid = _menu_tree_cid($menu_name, $data);
      if (!cache_get($tree_cid, 'cache_menu')) {
        cache_set($tree_cid, $data, 'cache_menu');
      }
      // Cache the cid of the (shared) data using the page-specific cid.
      cache_set($cid, $tree_cid, 'cache_menu');
    }
    // Check access for the current user to each item in the tree.
    menu_tree_check_access($data['tree'], $data['node_links']);
    $tree[$cid] = $data['tree'];
  }
  return $tree[$cid];
}

function ctools_menu_set_trail_parent($path) {
  $current = menu_get_active_trail();
  $keep = array_pop($current);

  $trail = ctools_get_menu_trail($path);
  $trail[] = $keep;

  menu_set_active_trail($trail);
}

/**
 * Re-sort menu items after we have modified them.
 */
function ctools_menu_sort($a, $b) {
  $a_weight = (is_array($a) && isset($a['#link']['weight'])) ? $a['#link']['weight'] : 0;
  $b_weight = (is_array($b) && isset($b['#link']['weight'])) ? $b['#link']['weight'] : 0;
  if ($a_weight == $b_weight) {
    $a_title = (is_array($a) && isset($a['#link']['title'])) ? $a['#link']['title'] : 0;
    $b_title = (is_array($b) && isset($b['#link']['title'])) ? $b['#link']['title'] : 0;
    if ($a_title == $b_title) {
      return 0;
    }

    return ($a_title < $b_title) ? -1 : 1;
  }

  return ($a_weight < $b_weight) ? -1 : 1;
}

function _ctools_menu_add_dynamic_items(&$data, &$router_item, &$root_path) {
  // TESTING: add hardcoded values.
  if ($additions = ctools_menu_add_tab()) {
    // If none of the static local tasks are active allow one of the dynamic
    // active tasks to be marked as such.
    $has_active = FALSE;
    if (!empty($data['tabs'][0]['output'])) {
      foreach ($data['tabs'][0]['output'] as $element) {
        if (!empty($element['#link']['#active'])) {
          $has_active = TRUE;
        }
      }
    }
    foreach ($additions as $addition) {
      $addition['localized_options'] = isset($addition['options']) ? $addition['options'] : array();
      $data['tabs'][0]['output'][] = array(
        '#theme' => 'menu_local_task',
        '#link' => $addition,
        '#active' => (!$has_active && $root_path === $addition['href']),
      );
    }
    uasort($data['tabs'][0]['output'], 'ctools_menu_sort');
    $data['tabs'][0]['count'] = count($data['tabs'][0]['output']);
  }
}

<?php
// $Id$
/**
 * @file
 * Groups tasks that provide pages and provides a common administrative UI for them.
 *
 * The page task supports any task that defines itself as type 'page' and it also
 * includes special handling for the 'page' task to provide a nice location to
 * place administer the list of pages, including adding removing and editing
 * the handlers of pages alongside the less generic tasks such as node and
 * user viewing.
 */

/**
 * Specialized implementation of hook_delegator_tasks(). See api-task.html for
 * more information.
 */
function delegator_page_delegator_task_types() {
  return array(
    'page' => array(
      'title' => t('Pages'),
      'admin path' => 'admin/build/pages',
      'hook menu' => 'delegator_page_type_menu',
    ),
  );
}

/**
 * Delegated implementation of hook_menu().
 */
function delegator_page_type_menu(&$items, $task) {
  // Set up access permissions.
  $access_callback = isset($task['admin access callback']) ? $task['admin access callback'] : 'user_access';

  // @todo use 'administer pages' perm instead?
  $access_arguments = isset($task['admin access arguments']) ? $task['admin access arguments'] : array('administer delegator');

  $base = array(
    'access callback' => $access_callback,
    'access arguments' => $access_arguments,
    'file' => 'plugins/task_types/page.admin.inc',
  );

  $items['admin/build/pages'] = array(
    'title' => 'Pages',
    'description' => 'Add, edit and remove overridden system pages and user defined pages from the system.',
    'page callback' => 'delegator_page_type_list',
  ) + $base;

  $items['admin/build/pages/list'] = array(
    'title' => 'List',
    'page callback' => 'delegator_page_type_list',
    'type' => MENU_DEFAULT_LOCAL_TASK,
    'weight' => -10,
  );
}

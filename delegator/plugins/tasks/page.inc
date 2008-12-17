<?php
// $Id$
/**
 * @file
 * Handle the 'page' task, which creates pages with arbitrary tasks and lets
 * handlers decide how they will be rendered.
 *
 * This creates subtasks and stores them in the delegator_pages table. These
 * are exportable objects, too.
 */

/**
 * Specialized implementation of hook_delegator_tasks(). See api-task.html for
 * more information.
 */
function delegator_page_delegator_tasks() {
  return array(
    'page' => array(
      'title' => t('Page'),
      'type' => 'node_view',
      'subtasks' => TRUE,
      'subtasks callback' => 'delegator_page_subtasks',
      'hook menu' => 'delegator_page_menu',
      'hook theme' => 'delegator_page_theme',
    ),
  );
}

/**
 * Return a list of all subtasks.
 */
function delegator_page_subtasks($task) {
  $subtasks = delegator_page_load_all();
  $return = array();
  foreach ($subtasks as $name => $subtask) {
    $return[$name] = array(
      'admin title' => $subtask->admin_title,
      'admin description' => t('TODO'),
      'subtask' => $subtask,
    );
  }

  return $return;
}

/**
 * Delegated implementation of hook_menu().
 */
function delegator_page_menu(&$items, $task) {
  // Set up access permissions.
  $access_callback = isset($task['admin access callback']) ? $task['admin access callback'] : 'user_access';
  $access_arguments = isset($task['admin access arguments']) ? $task['admin access arguments'] : array('administer delegator');

  $base = array(
    'access callback' => $access_callback,
    'access arguments' => $access_arguments,
    'file' => 'plugins/tasks/page.admin.inc',
  );

  $items['admin/build/delegator/page/add'] = array(
    'title' => 'Add page',
    'description' => 'Add a delegator page subtask.',
    'page callback' => 'delegator_page_add_subtask',
  ) + $base;


  // AJAX callbacks for argument love.
  $items['admin/build/delegator/argument'] = array(
    'page callback' => 'delegator_page_subtask_argument_ajax',
  ) + $base;
}

/**
 * Delegated implementation of hook_theme().
 */
function delegator_page_theme(&$items, $task) {
  $items['delegator_page_form_argument_table'] = array(
    'arguments' => array('form' => NULL),
    'file' => 'page.admin.inc',
    'path' => drupal_get_path('module', 'delegator') . '/plugins/tasks',
  );
}

// --------------------------------------------------------------------------
// Page task database info.

/**
 * Create a new page with defaults appropriately set from schema.
 */
function delegator_page_new() {
  ctools_include('export');
  return ctools_export_new_object('delegator_pages');
}

/**
 * Load a single page subtask.
 */
function delegator_page_load($name) {
  ctools_include('export');
  $result = ctools_export_load_object('delegator_pages', 'names', array($name));
  if (isset($result[$name])) {
    return $result[$name];
  }
}

/**
 * Load all page subtasks.
 */
function delegator_page_load_all() {
  ctools_include('export');
  return ctools_export_load_object('delegator_pages');
}

/**
 * Write a page subtask to the database.
 */
function delegator_page_save(&$subtask) {
  $update = (isset($subtask->did)) ? array('pid') : array();
  drupal_write_record('delegator_pages', $subtask, $update);
  return $subtask;
}

/**
 * Remove a page subtask.
 */
function delegator_page_delete($subtask) {
  db_query("DELETE FROM {delegator_pages} WHERE name = '%s'", $subtask->name);
}

/**
 * Export a page subtask.
 */
function delegator_page_export($subtask, $indent = '') {
  ctools_include('export');
  $output = ctools_export_object('delegator_pages', $subtask, 'page', $indent);
  return $output;
}


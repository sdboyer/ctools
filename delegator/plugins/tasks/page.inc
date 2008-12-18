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
      'title' => t('User pages'),
      'description' => t('Administrator created pages that have a URL path, access control and entries in the Drupal menu system.'),
      'subtasks' => TRUE,
      'subtasks callback' => 'delegator_page_subtasks',
      'hook menu' => 'delegator_page_menu',
      'hook theme' => 'delegator_page_theme',

      // page only items
      'task type' => 'page',
      'operations' => array(
        array(
          'title' => t('Import'),
          'href' => 'admin/build/pages/import',
        ),
        array(
          'title' => t('Add page'),
          'href' => 'admin/build/pages/add',
        ),
      ),
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
    $form_info = delegator_page_edit_form_info();
    $edit_links = array();
    foreach ($form_info['order'] as $form_id => $form_title) {
      $edit_links[] = array(
        'title' => $form_title,
        'href' => "admin/build/pages/edit/$name/$form_id",
      );
    }

    $operations = array();

    if (empty($subtask->disabled)) {
      $operations[] =  array(
        'title' => '<span class="text">' . t('Edit page') . '</span>' . theme('links', $edit_links),
        'html' => TRUE,
      );
      $operations[] = array(
        'title' => t('Clone'),
        'href' => "admin/build/pages/clone/$name",
      );
      $operations[] = array(
        'title' => t('Export'),
        'href' => "admin/build/pages/export/$name",
      );
      if ($subtask->export_type == (EXPORT_IN_CODE | EXPORT_IN_DATABASE)) {
        $operations[] = array(
          'title' => t('Revert'),
          'href' => "admin/build/pages/delete/$name",
        );
      }
      else if ($subtask->export_type == EXPORT_IN_CODE) {
        $operations[] = array(
          'title' => t('Disable'),
          'href' => "admin/build/pages/disable/$name",
        );
      }
      else {
        $operations[] = array(
          'title' => t('Delete'),
          'href' => "admin/build/pages/delete/$name",
        );
      }
    }
    else {
      $operations[] = array(
        'title' => t('Enable'),
        'href' => "admin/build/pages/enable/$name",
      );
    }
    $return[$name] = array(
      'name' => $name,
      'admin title' => $subtask->admin_title,
      'admin description' => t('TODO'),
      'admin path' => $subtask->path,
      'subtask' => $subtask,
      'operations' => $operations,
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

  $items['admin/build/pages/add'] = array(
    'title' => 'Add page',
    'description' => 'Add a delegator page subtask.',
    'page callback' => 'delegator_page_add_subtask',
    'type' => MENU_LOCAL_TASK,
  ) + $base;

  $form_info = delegator_page_edit_form_info();
  $default_task = FALSE;
  $weight = 0;
  foreach ($form_info['order'] as $form_id => $form_title) {
    // The first edit form is the default for tabs, so it gets a bit
    // of special treatment here.
    if (!$default_task) {
      $default_task = TRUE;
      // Add the callback for the default tab.
      $items["admin/build/pages/edit/%"] = array(
        'title' => t('Edit'),
        'page callback' => 'delegator_page_edit_subtask',
        'page arguments' => array(4, $form_id),
      ) + $base;

      // And make sure it's the default local task.
      $type = MENU_DEFAULT_LOCAL_TASK;
    }
    else {
      // This allows an empty form title to provide a hidden form
      // which is useful for doing more branch-like multi-step
      // functionality.
      $type = $form_title ? MENU_LOCAL_TASK : MENU_CALLBACK;
    }

    // Handler to edit delegator task handlers. May exist in its own UI.
    $items["admin/build/pages/edit/%/$form_id"] = array(
      'title' => $form_title,
      'page callback' => 'delegator_page_edit_subtask',
      'page arguments' => array(4, 5),
      'type' => $type,
      'weight' => $weight++,
    ) + $base;
  }

  // AJAX callbacks for argument modal.
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

/**
 * Supply information for the multi-step wizard for both edit and add subtask
 */
function delegator_page_edit_form_info() {
  return array(
    'id' => 'delegator_page',
    'show trail' => TRUE,
    'show back' => TRUE,
    'show return' => FALSE,
    'next callback' => 'delegator_page_add_subtask_next',
    'finish callback' => 'delegator_page_add_subtask_finish',
    'return callback' => 'delegator_page_add_subtask_finish',
    'cancel callback' => 'delegator_page_add_subtask_cancel',
    'order' => array(
      'basic' => t('Basic settings'),
      'argument' => t('Argument settings'),
      'access' => t('Access type'),
      'access-settings' => t('Access settings'),
      'menu' => t('Menu settings'),
    ),
    'forms' => array(
      'basic' => array(
        'form id' => 'delegator_page_form_basic'
      ),
      'access' => array(
        'form id' => 'delegator_page_form_access'
      ),
      'access-settings' => array(
        'form id' => 'delegator_page_form_access_settings'
      ),
      'menu' => array(
        'form id' => 'delegator_page_form_menu'
      ),
      'argument' => array(
        'form id' => 'delegator_page_form_argument'
      ),
    ),
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
  $update = (isset($subtask->pid)) ? array('pid') : array();
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


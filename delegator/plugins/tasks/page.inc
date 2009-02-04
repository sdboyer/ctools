<?php
// $Id$

/**
 * @file
 * Handle the 'page' task, which creates pages with arbitrary tasks and lets
 * handlers decide how they will be rendered.
 *
 * This creates subtasks and stores them in the delegator_pages table. These
 * are exportable objects, too.
 *
 * The render callback for this task type has $handler, $page, $contexts as
 * parameters.
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
      'subtask callback' => 'delegator_page_subtask',
      'subtasks callback' => 'delegator_page_subtasks',
      'hook menu' => array(
        'file' => 'page.admin.inc',
        'path' => drupal_get_path('module', 'delegator') . '/plugins/tasks',
        'function' => 'delegator_page_menu',
      ),
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
      'columns' => array(
        'storage' => array(
          'label' => t('Storage'),
          'class' => 'delegator-page-storage',
        ),
      ),
      // context only items
      'handler type' => 'context',
      'get arguments' => 'delegator_page_get_arguments',
      'get context placeholders' => 'delegator_page_get_contexts',
    ),
  );
}

/**
 * Return a list of all subtasks.
 */
function delegator_page_subtasks($task) {
  $pages = delegator_page_load_all();
  $return = array();
  foreach ($pages as $name => $page) {
    $return[$name] = delegator_page_build_subtask($task, $page);
  }

  return $return;
}

/**
 * Callback to return a single subtask.
 */
function delegator_page_subtask($task, $subtask_id) {
  $page = delegator_page_load($subtask_id);
  if ($page) {
    return delegator_page_build_subtask($task, $page);
  }
}

/**
 * Build a subtask array for a given page.
 */
function delegator_page_build_subtask($task, $page) {
  $form_info  = delegator_page_edit_form_info();
  $edit_links = array();
  $name       = $page->name;

  foreach ($form_info['order'] as $form_id => $form_title) {
    $edit_links[] = array(
      'title' => $form_title,
      'href' => "admin/build/pages/edit/$name/$form_id",
    );
  }

  $operations = array();
  $task_name = delegator_make_task_name($task['name'], $name);

  if (empty($page->disabled)) {
    if ($page->multiple) {
      $operations[] = array(
        'title' => t('Task handlers'),
        'href' => "admin/build/delegator/$task_name",
      );
    }
    else {
      $default_handlers = isset($page->default_handlers) ? $page->default_handlers : array();
      $task_handlers = delegator_load_task_handlers($task, $page->name, $default_handlers);
      if ($task_handlers) {
        $handler = array_shift($task_handlers);
        $plugin = delegator_get_task_handler($handler->handler);
        if (!empty($plugin['edit forms'])) {
          $actions = array();
          foreach ($plugin['edit forms'] as $edit_id => $title) {
            if ($title) {
              $actions[] = array(
                'title' => $title,
                'href' => "admin/build/delegator/$task_name/$handler->handler/$handler->name/$edit_id",
              );
            }
          }
          $operations[] = array(
            'title' => '<span class="text">' . t('Edit handler') . '</span>' . theme('links', $actions),
            'html' => TRUE,
          );
        }
      }
    }
    $operations[] = array(
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
    $operations[] = array(
      'title' => t('Export (with handlers)'),
      'href' => "admin/build/pages/export/$name/handlers",
    );
    if ($page->export_type == (EXPORT_IN_CODE | EXPORT_IN_DATABASE)) {
      $operations[] = array(
        'title' => t('Revert'),
        'href' => "admin/build/pages/delete/$name",
      );
    }
    else if ($page->export_type == EXPORT_IN_CODE) {
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
  $subtask = array(
    'name' => $name,
    'admin title' => $page->admin_title,
    'admin path' => $page->path,
    'subtask' => $page,
    'operations' => $operations,
    'single task' => empty($page->multiple),
    'row class' => empty($page->disabled) ? 'delegator-enabled' : 'delegator-disabled',
    'storage' => $page->type,
  );

  // default handlers may appear from a default subtask.
  if (isset($page->default_handlers)) {
    $subtask['default handlers'] = $page->default_handlers;
  }
  return $subtask;
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
      'access' => t('Access control'),
      'menu' => t('Menu settings'),
      'multiple' => t('Task handlers'),
    ),
    'forms' => array(
      'basic' => array(
        'form id' => 'delegator_page_form_basic',
      ),
      'access' => array(
        'form id' => 'delegator_page_form_access',
      ),
      'menu' => array(
        'form id' => 'delegator_page_form_menu',
      ),
      'argument' => array(
        'form id' => 'delegator_page_form_argument',
      ),
      'multiple' => array(
        'form id' => 'delegator_page_argument_form_multiple',
      ),
    ),
  );
}

// --------------------------------------------------------------------------
// Page execution functions

/**
 * Load a context from an argument for a given page task.
 *
 * @param $value
 *   The incoming argument value.
 * @param $subtask
 *   The subtask id.
 * @param $argument
 *   The numeric position of the argument in the path, counting from 0.
 *
 * @return
 *   A context item if one is configured, the argument if one is not, or
 *   FALSE if restricted or invalid.
 */
function _dp_arg_load($value, $subtask, $argument) {
  $page = delegator_page_load($subtask);
  if (!$page) {
    return FALSE;
  }

  $path = explode('/', $page->path);
  if (empty($path[$argument])) {
    return FALSE;
  }

  $keyword = substr($path[$argument], 1);
  if (empty($page->arguments[$keyword])) {
    return $value;
  }

  $page->arguments[$keyword]['keyword'] = $keyword;

  ctools_include('context');
  $context = ctools_context_get_context_from_argument($page->arguments[$keyword], $value);

  // convert false equivalents to false.
  return $context ? $context : FALSE;
}

/**
 * Execute a page task.
 *
 * This is the callback to entries in the Drupal menu system created by the
 * page task.
 *
 * @param $subtask_id
 *   The name of the page task used.
 * @param ...
 *   A number of context objects as specified by the user when
 *   creating named arguments in the path.
 */
function delegator_page_execute($subtask_id) {
  // Turn the contexts into a properly keyed array.
  $contexts = array();
  foreach (func_get_args() as $arg) {
    if (is_object($arg) && get_class($arg) == 'ctools_context') {
      $contexts[$arg->id] = $arg;
    }
  }

  $task = delegator_get_task('page');

  ctools_include('context-task-handler');
  $output = ctools_context_handler_render($task, $subtask_id, $contexts);
  if ($output === FALSE) {
    return drupal_not_found();
  }

  return $output;
}

// --------------------------------------------------------------------------
// Context type callbacks

/**
 * Return a list of arguments used by this task.
 */
function delegator_page_get_arguments($task, $subtask_id) {
  $page = delegator_page_load($subtask_id);
  return _delegator_page_get_arguments($page);
}

function _delegator_page_get_arguments($page) {
  $arguments = array();

  if (!empty($page->arguments)) {
    foreach ($page->arguments as $keyword => $argument) {
      if (isset($argument['name'])) {
        $argument['keyword'] = $keyword;
        $arguments[$keyword] = $argument;
      }
    }
  }
  return $arguments;
}

/**
 * Get a group of context placeholders for the arguments.
 */
function delegator_page_get_contexts($task, $subtask_id) {
  ctools_include('context');
  return ctools_context_get_placeholders_from_argument(delegator_page_get_arguments($task, $subtask_id));
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

  // If this was a default page we may need to write default task
  // handlers that we provided as well.
  if (!$update && isset($subtask->default_handlers)) {
    $handlers = delegator_load_task_handlers(delegator_get_task('page'), $subtask->name);
    foreach ($subtask->default_handlers as $name => $handler) {
      if (!isset($handlers[$name]) || !($handlers[$name]->export_type & EXPORT_IN_DATABASE)) {
        // Make sure this is right, as exports can wander a bit.
        $handler->subtask = $subtask->name;
        delegator_save_task_handler($handler);
      }
    }
  }
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
function delegator_page_export($subtask, $with_handlers = FALSE, $indent = '') {
  ctools_include('export');
  $output = ctools_export_object('delegator_pages', $subtask, $indent);
  if ($with_handlers) {
    $handlers = delegator_load_task_handlers(delegator_get_task('page'), $subtask->name);
    $output .= $indent . '$page->default_handlers = array();' . "\n";
    foreach ($handlers as $handler) {
      $output .= delegator_export_task_handler($handler, $indent);
      $output .= $indent . '$page->default_handlers[$handler->name] = $handler;' . "\n";
    }
  }
  return $output;
}


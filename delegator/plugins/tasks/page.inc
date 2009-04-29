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
      'title' => t('Custom pages'),
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
      'form info' => 'delegator_page_edit_form_info',
      // page only items
      'task type' => 'page',
      'operations' => array(
        array(
          'title' => t('Import'),
          'href' => 'admin/build/pages/import',
        ),
        array(
          'title' => t('Add custom page'),
          'href' => 'admin/build/pages/add',
        ),
      ),
      'columns' => array(
        'storage' => array(
          'label' => t('Storage'),
          'class' => 'delegator-page-storage',
        ),
      ),
      'page type' => 'custom',
      // What page callback will execute this page. If it has handlers then
      // delegator_page_execute should be the default.
//      'page callback' => 'delegator_page_execute',
      // context only items
      'handler type' => 'context',
      'get arguments' => 'delegator_page_get_arguments',
      'get context placeholders' => 'delegator_page_get_contexts',
      'access restrictions' => 'delegator_page_access_restrictions',
      'uses handlers' => TRUE,
    ),
  );
}

/**
 * Return a list of all subtasks.
 */
function delegator_page_subtasks($task) {
  $pages = delegator_page_load_all($task['name']);
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
  $function = ctools_plugin_get_function($task, 'form info');
  $form_info = $function();

  $edit_links = array();
  $name       = $page->name;

  foreach ($form_info['order'] as $form_id => $form_title) {

    $edit_links[] = array(
      'title' => $form_title,
      'href' => str_replace(array('%page_name', '%step'), array($name, $form_id), $form_info['edit path']),
    );
  }

  $operations = array();
  $task_name = delegator_make_task_name($task['name'], $name);

  if (empty($page->disabled)) {
    if (!empty($task['uses handlers'])) {
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
    if (!empty($task['has handlers'])) {
      $operations[] = array(
        'title' => t('Export (with handlers)'),
        'href' => "admin/build/pages/export/$name/handlers",
      );
    }
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
  $base = array(
    'file' => 'page.admin.inc',
    'path' => drupal_get_path('module', 'delegator') . '/plugins/tasks',
  );
  $items['delegator_page_form_argument_table'] = $base + array(
    'arguments' => array('form' => NULL),
  );
  $items['delegator_page_lock'] = $base + array(
    'arguments' => array('lock' => array(), 'task_name' => NULL),
  );
  $items['delegator_page_changed'] = $base + array(
    'arguments' => array(),
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
    // Items specific to the 'add' routines that will get moved over:
    'add path' => 'admin/build/pages/add/%step',
    // Items specific to the 'edit' routines that will get moved over:
    'edit path' => 'admin/build/pages/edit/%page_name/%step',
  );
}

// --------------------------------------------------------------------------
// Page execution functions

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
  $page = delegator_page_load($subtask_id);
  $task = delegator_get_task($page->task);

  // Turn the contexts into a properly keyed array.
  $contexts = array();
  $args = array();
  foreach (func_get_args() as $count => $arg) {
    if (is_object($arg) && get_class($arg) == 'ctools_context') {
      $contexts[$arg->id] = $arg;
      $args[] = $arg->original_argument;
    }
    else if ($count) {
      $args[] = $arg;
    }
  }

  $count = 0;
  $names = delegator_page_get_named_arguments($page->path);
  $bits = explode('/', $page->path);

  foreach ($page->arguments as $name => $argument) {
    // Optional arguments must be converted to contexts too, if they exist.
    if ($bits[$names[$name]][0] == '!' && isset($args[$count])) {
      $argument['keyword'] = $name;
      ctools_include('context');
      $context = ctools_context_get_context_from_argument($argument, $args[$count]);
      $contexts[$context->id] = $context;
    }
    $count++;
  }

  if ($function = ctools_plugin_get_function($task, 'page callback')) {
    return call_user_func_array($function, array($page, $contexts, $args));
  }

  ctools_include('context-task-handler');
  $output = ctools_context_handler_render($task, $subtask_id, $contexts, $args);
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

/**
 * Return a list of arguments used by this task.
 */
function delegator_page_access_restrictions($task, $subtask_id, $contexts) {
  $page = delegator_page_load($subtask_id);
  return ctools_access_add_restrictions($page->access, $contexts);
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
function delegator_page_load_all($task = NULL) {
  ctools_include('export');

  if (empty($task)) {
    return ctools_export_load_object('delegator_pages');
  }
  else {
    return ctools_export_load_object('delegator_pages', 'conditions', array('task' => $task));
  }
}

/**
 * Write a page subtask to the database.
 */
function delegator_page_save(&$page) {
  $update = (isset($page->pid)) ? array('pid') : array();
  $task = delegator_get_task($page->task);

  if ($function = ctools_plugin_get_function($task, 'save')) {
    $function($page, $update);
  }
  drupal_write_record('delegator_pages', $page, $update);

  // If this was a default page we may need to write default task
  // handlers that we provided as well.
  if (!$update && isset($page->default_handlers)) {
    $handlers = delegator_load_task_handlers(delegator_get_task('page'), $page->name);
    foreach ($page->default_handlers as $name => $handler) {
      if (!isset($handlers[$name]) || !($handlers[$name]->export_type & EXPORT_IN_DATABASE)) {
        // Make sure this is right, as exports can wander a bit.
        $handler->subtask = $page->name;
        delegator_save_task_handler($handler);
      }
    }
  }
  return $page;
}

/**
 * Remove a page subtask.
 */
function delegator_page_delete($page) {
  $task = delegator_get_task($page->task);
  if ($function = ctools_plugin_get_function($task, 'delete')) {
    $function($page);
  }
  if (!empty($task['uses handlers'])) {
    $handlers = delegator_load_task_handlers($task, $page->name);
    foreach ($handlers as $handler) {
      delegator_delete_task_handler($handler);
    }
  }
  db_query("DELETE FROM {delegator_pages} WHERE name = '%s'", $page->name);
}

/**
 * Export a page subtask.
 */
function delegator_page_export($page, $with_handlers = FALSE, $indent = '') {
  $task = delegator_get_task($page->task);
  if ($function = ctools_plugin_get_function($task, 'export')) {
    $append = $function($page, $indent);
  }

  ctools_include('export');
  $output = ctools_export_object('delegator_pages', $page, $indent);
  $output .= $append;

  if ($with_handlers) {
    $handlers = delegator_load_task_handlers(delegator_get_task('page'), $page->name);
    $output .= $indent . '$page->default_handlers = array();' . "\n";
    foreach ($handlers as $handler) {
      $output .= delegator_export_task_handler($handler, $indent);
      $output .= $indent . '$page->default_handlers[$handler->name] = $handler;' . "\n";
    }
  }
  return $output;
}

/**
 * Get a list of named arguments in a delegator page path.
 *
 * @param $path
 *   A normal Drupal path.
 *
 * @return
 *   An array of % marked variable arguments, keyed by the argument's name.
 *   The value will be the position of the argument so that it can easily
 *   be found. Items with a position of -1 have multiple positions.
 */
function delegator_page_get_named_arguments($path) {
  $arguments = array();
  $bits = explode('/', $path);
  foreach ($bits as $position => $bit) {
    if ($bit && ($bit[0] == '%' || $bit[0] == '!')) {
      // special handling for duplicate path items and substr to remove the %
      $arguments[substr($bit, 1)] = isset($arguments[$bit]) ? -1 : $position;
    }
  }

  return $arguments;
}

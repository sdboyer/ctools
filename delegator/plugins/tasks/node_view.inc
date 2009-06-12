<?php
// $Id$

/**
 * @file
 * Handle the 'node view' override task.
 *
 * This plugin overrides node/%node and reroutes it to the delegator, where
 * a list of tasks can be used to service this request based upon criteria
 * supplied by access plugins.
 */

/**
 * Specialized implementation of hook_delegator_tasks(). See api-task.html for
 * more information.
 */
function delegator_node_view_delegator_tasks() {
  return array(
    // This is a 'page' task and will fall under the page admin UI
    'task type' => 'page',

    'title' => t('Node view'),
    'description' => t('The node view task allows you to control what handler will handle the job of rendering a node view at the path <em>node/%node</em>. If no handler is set or matches the criteria, the default Drupal node renderer will be used.'),
    'admin title' => 'Node view', // translated by menu system
    'admin description' => 'Overrides for the built in node view handler at <em>node/%node</em>.',
    'admin path' => 'node/%node',

    // Callback to add items to the delegator task administration form:
    'task admin' => 'delegator_node_view_task_admin',

    // Menu hooks so that we can alter the node/%node menu entry to point to us.
    'hook menu' => 'delegator_node_view_menu',
    'hook menu alter' => 'delegator_node_view_menu_alter',

    // This is task uses 'context' handlers and must implement these to give the
    // handler data it needs.
    'handler type' => 'context',
    'get arguments' => 'delegator_node_view_get_arguments',
    'get context placeholders' => 'delegator_node_view_get_contexts',
  );
}

/**
 * Callback defined by delegator_node_view_delegator_tasks().
 *
 * Alter the node view input so that node view comes to us rather than the
 * normal node view process.
 */
function delegator_node_view_menu_alter(&$items, $task) {
  // Override the node view handler for our purpose.
  if ($items['node/%node']['page callback'] == 'node_page_view' || variable_get('delegator_override_anyway', FALSE)) {
    $items['node/%node']['page callback'] = 'delegator_node_view';
    $items['node/%node']['file path'] = $task['path'];
    $items['node/%node']['file'] = $task['file'];
  }
  // @todo override node revision handler as well?
}

/**
 * Warn if we are unable to override the taxonomy term page.
 */
function delegator_node_view_task_admin(&$form, &$form_state) {
  $callback = db_result(db_query("SELECT page_callback FROM {menu_router} WHERE path = 'node/%'"));
  if ($callback != 'delegator_node_view') {
    drupal_set_message(t('Delegator module is unable to override node/%node because some other module already has overridden with %callback. Delegator will not be able to handle this page.', array('%callback' => $callback)), 'warning');
  }
}

/**
 * Entry point for our overridden node view.
 *
 * This function asks its assigned handlers who, if anyone, would like
 * to run with it. If no one does, it passes through to Drupal core's
 * node view, which is node_page_view().
 */
function delegator_node_view($node) {
  // Load my task plugin
  $task = delegator_get_task('node_view');

  // Load the node into a context.
  ctools_include('context');
  ctools_include('context-task-handler');
  $contexts = ctools_context_handler_get_task_contexts($task, '', array($node));

  $output = ctools_context_handler_render($task, '', $contexts, array($node->nid));
  if ($output !== FALSE) {
    node_tag_new($node->nid);
    return $output;
  }

  $function = 'node_page_view';
  foreach (module_implements('delegator_override') as $module) {
    $call = $module . '_delegator_override';
    if ($rc = $call('node_view') && function_exists($rc)) {
      $function = $rc;
      break;
    }
  }

  // Otherwise, fall back.
  return $function($node);
}

/**
 * Callback to get arguments provided by this task handler.
 *
 * Since this is the node view and there is no UI on the arguments, we
 * create dummy arguments that contain the needed data.
 */
function delegator_node_view_get_arguments($task, $subtask_id) {
  return array(
    array(
      'keyword' => 'node',
      'identifier' => t('Node being viewed'),
      'id' => 1,
      'name' => 'nid',
      'settings' => array(),
    ),
  );
}

/**
 * Callback to get context placeholders provided by this handler.
 */
function delegator_node_view_get_contexts($task, $subtask_id) {
  return ctools_context_get_placeholders_from_argument(delegator_node_view_get_arguments($task, $subtask_id));
}


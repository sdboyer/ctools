<?php
// $Id$

/**
 * Specialized implementation of hook_delegator_tasks(). See api-task.html for
 * more information.
 */
function delegator_node_edit_delegator_tasks() {
  return array(
    'node_edit' => array(
      // This is a 'page' task and will fall under the page admin UI
      'task type' => 'page',

      'title' => t('Node edit'),
      'description' => t('The node edit task allows you to control what handler will handle the job of editing a node at the path <em>node/%node/edit</em>. It also handles the node add page at node/add/%type. If no handler is set or matches the criteria, the default Drupal node renderer will be used. Please note that some modules sufficiently customize their node add and edit procedure that this may not successfully override adding or editing of all types.'),

      'admin title' => 'Node edit', // translated by menu system
      'admin description' => 'Overrides for the built in node edit handler at <em>node/%node/edit</em>.',
      'admin path' => 'node/%node/edit',

      // Menu hooks so that we can alter the node/%node menu entry to point to us.
      'hook menu' => 'delegator_node_edit_menu',
      'hook menu alter' => 'delegator_node_edit_menu_alter',

      // This is task uses 'context' handlers and must implement these to give the
      // handler data it needs.
      'handler type' => 'context', // handler type -- misnamed
      'get arguments' => 'delegator_node_edit_get_arguments',
      'get context placeholders' => 'delegator_node_edit_get_contexts',
    ),
  );
}

/**
 * Callback defined by delegator_node_edit_delegator_tasks().
 *
 * Alter the node edit input so that node edit comes to us rather than the
 * normal node edit process.
 */
function delegator_node_edit_menu_alter(&$items, $task) {
  // Override the node edit handler for our purpose.
  $items['node/%node/edit']['page callback'] = 'delegator_node_edit';
  $items['node/%node/edit']['file path'] = $task['path'];
  $items['node/%node/edit']['file'] = $task['file'];

  // Also catch node/add handling:
  foreach (node_get_types('types', NULL, TRUE) as $type) {
    $path = 'node/add/' . str_replace('_', '-', $type->type);
    if ($items[$path]['page callback'] != 'node_add') {
      continue;
    }

    $items[$path]['page callback'] = 'delegator_node_add';
    $items[$path]['file path'] = $task['path'];
    $items[$path]['file'] = $task['file'];
    // Why str_replace things back?
    $items[$path]['page arguments'] = array($type->type);
  }
}

/**
 * Entry point for our overridden node edit.
 *
 * This function asks its assigned handlers who, if anyone, would like
 * to run with it. If no one does, it passes through to Drupal core's
 * node edit, which is node_page_edit().
 */
function delegator_node_edit($node) {
  // Load my task plugin
  $task = delegator_get_task('node_edit');

  // Load the node into a context.
  ctools_include('context');
  ctools_include('context-task-handler');
  $contexts = ctools_context_handler_get_task_contexts($task, '', array($node));

  $output = ctools_context_handler_render($task, '', $contexts);
  if ($output === FALSE) {
    // Fall back!
    module_load_include('inc', 'node', 'node.pages');
    $output = drupal_get_form($node->type . '_node_form', $node);
  }

  return $output;
}

/**
 * Callback to handle the process of adding a node.
 *
 * This creates a basic $node and passes that off to delegator_node_edit().
 * It is modeled after Drupal's node_add() function.
 *
 * Unlike node_add() we do not need to check node_access because that was
 * already checked by the menu system.
 */
function delegator_node_add($type) {
  global $user;

  $types = node_get_types();

  // Initialize settings:
  $node = (object) array(
    'uid' => $user->uid,
    'name' => (isset($user->name) ? $user->name : ''),
    'type' => $type,
    'language' => ''
  );

  drupal_set_title(t('Create @name', array('@name' => $types[$type]->name)));
  return delegator_node_edit($node);
}

/**
 * Callback to get arguments provided by this task handler.
 *
 * Since this is the node edit and there is no UI on the arguments, we
 * create dummy arguments that contain the needed data.
 */
function delegator_node_edit_get_arguments($task, $subtask_id) {
  return array(
    array(
      'keyword' => 'node',
      'identifier' => t('Node being edited'),
      'id' => 0,
      'name' => 'node_edit',
      'settings' => array(),
    ),
  );
}

/**
 * Callback to get context placeholders provided by this handler.
 */
function delegator_node_edit_get_contexts($task, $subtask_id) {
  return ctools_context_get_placeholders_from_argument(delegator_node_edit_get_arguments($task, $subtask_id));
}

<?php
// $Id$

/**
 * Specialized implementation of hook_delegator_tasks(). See api-task.html for
 * more information.
 */
function delegator_node_view_delegator_tasks() {
  return array(
    'node_view' => array(
       'title' => t('Node view'),
       'description' => t('The node view task allows you to control which functions will handle the actual job of rendering a node view. The first task that matches the node will be used to display the node. If there are no task handlers listed, or no task handlers are configured to handle a given node, the default Drupal node view mechanism will be used.'),
       'admin title' => 'Node view', // translated by menu system
       'admin description' => 'Overrides for the built in node view handler which allows customized node output.',
       'hook menu' => 'delegator_node_view_menu',
       'hook menu alter' => 'delegator_node_view_menu_alter',
    ),
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
  $items['node/%node']['page callback'] = 'delegator_node_view';
  $items['node/%node']['file path'] = $task['path'];
  $items['node/%node']['file'] = $task['file'];

//  @todo override node revision handler as well?
}

/**
 * Entry point for our overridden node view.
 *
 * This function asks its assigned handlers who, if anyone, would like
 * to run with it. If no one does, it passes through to Drupal core's
 * node view, which is node_page_view().
 */
function delegator_node_view($node) {
  // Load my task plugin:
  $task = delegator_get_task('node_view');
  $handlers = delegator_load_sorted_handlers($task);

  // Try each handler.
  foreach ($handlers as $handler) {
    if ($function = ctools_plugin_load_function('delegator', 'task_handlers', $handler->handler, 'render')) {
      $output = $function($handler, $node);
      if ($output) {
        return $output;
      }
    }
  }

  // Fall back!
  return node_page_view($node);
}
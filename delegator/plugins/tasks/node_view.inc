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
       'description' => t('The node view task allows you to control what handler will handle the job of rendering a node view at the path <em>node/%node</em>. If no handler is set or matches the criteria, the default Drupal node renderer will be used.'),
       'type' => 'node_view', // handler type -- misnamed
       'admin title' => 'Node view', // translated by menu system
       'admin description' => 'Overrides for the built in node view handler at <em>node/%node</em>.',
       'admin path' => 'node/%node',
       'hook menu' => 'delegator_node_view_menu',
       'hook menu alter' => 'delegator_node_view_menu_alter',
       'task type' => 'page',
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
        // Since we're not using node_show() we need to emulate what it used to do.
        // Update the history table, stating that this user viewed this node.
        node_tag_new($node->nid);

        // TRUE is a special return used to let us know that it handled the
        // task but does not wish us to render anything, as it already did.
        // This is needed for the 'no blocks' functionality.
        if ($output === TRUE) {
          return;
        }
        return $output;
      }
    }
  }

  // Fall back!
  return node_page_view($node);
}
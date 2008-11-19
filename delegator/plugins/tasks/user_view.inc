<?php
// $Id$

/**
 * Specialized implementation of hook_delegator_tasks(). See api-task.html for
 * more information.
 */
function delegator_user_view_delegator_tasks() {
  return array(
    'user_view' => array(
       'title' => t('User view'),
       'description' => t('The user view task allows you to control which modules serve requests made to user/%. By default, the core user module will show the user account page. The first task that matches the user will be used to display the user. If no task handlers exist, or if none of the existing task handlers are configured to handle the currently requested user, then the request falls back to the default Drupal user view mechanism.'),
       'admin title' => 'User view', // translated by menu system
       'admin description' => 'Overrides for the built in user handler, allowing customized user output.',
       'hook menu' => 'delegator_user_view_menu',
       'hook menu alter' => 'delegator_user_view_menu_alter',
    ),
  );
}

/**
 * Callback defined by delegator_user_view_delegator_tasks().
 *
 * Alter the user view input so that user view comes to us rather than the
 * normal user view process.
 */
function delegator_user_view_menu_alter(&$items, $task) {
  // Override the user view handler for our purpose.
  $items['user/%user_uid_optional']['page callback'] = 'delegator_user_view';
  $items['user/%user_uid_optional']['file path'] = $task['path'];
  $items['user/%user_uid_optional']['file'] = $task['file'];

//  @todo override user revision handler as well?
}

/**
 * Entry point for our overridden user view.
 *
 * This function asks its assigned handlers who, if anyone, would like
 * to run with it. If no one does, it passes through to Drupal core's
 * user view, which is user_page_view().
 */
function delegator_user_view($account) {
  // Load my task plugin:
  $task = delegator_get_task('user_view');
  $handlers = delegator_load_sorted_handlers($task);

  // Try each handler.
  foreach ($handlers as $handler) {
    if ($function = ctools_plugin_load_function('delegator', 'task_handlers', $handler->handler, 'render')) {
      $output = $function($handler, $account);
      if ($output) {
        return $output;
      }
    }
  }

  // Fall back!
  return user_view($account);
}
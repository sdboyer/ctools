<?php
// $Id$

/**
 * Specialized implementation of hook_delegator_tasks(). See api-task.html for
 * more information.
 */
function delegator_user_view_delegator_tasks() {
  return array(
    'user_view' => array(
      // This is a 'page' task and will fall under the page admin UI
      'task type' => 'page',

      'title' => t('User view'),
      'description' => t('The user view task allows you to control which modules serve requests made to user/%. By default, the core user module will show the user account page. The first task that matches the user will be used to display the user. If no task handlers exist, or if none of the existing task handlers are configured to handle the currently requested user, then the request falls back to the default Drupal user view mechanism.'),

      'admin title' => 'User view', // translated by menu system
      'admin description' => 'Overrides for the built in user handler, allowing customized user output.',
      'admin path' => 'user/%user',

      'hook menu' => 'delegator_user_view_menu',
      'hook menu alter' => 'delegator_user_view_menu_alter',

      // This is task uses 'context' handlers and must implement these to give the
      // handler data it needs.
      'handler type' => 'context', // handler type -- misnamed

      'get arguments' => 'delegator_user_view_get_arguments',
      'get context placeholders' => 'delegator_user_view_get_contexts',

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

  // Load the account into a context.
  ctools_include('context');
  ctools_include('context-task-handler');
  $contexts = ctools_context_handler_get_task_contexts($task, '', array($account));

  // Load the landlers.
  $handlers = delegator_load_sorted_handlers($task, '', TRUE);

  // Try each handler.
  foreach ($handlers as $handler) {
    if ($function = delegator_get_renderer($handler)) {
      $output = $function($handler, $contexts);
      if ($output) {
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
  require_once './' . drupal_get_path('module', 'user') . '/user.pages.inc';
  return user_view($account);
}

/**
 * Callback to get arguments provided by this task handler.
 *
 * Since this is the node view and there is no UI on the arguments, we
 * create dummy arguments that contain the needed data.
 */
function delegator_user_view_get_arguments($task, $subtask_id) {
  return array(
    array(
      'keyword' => 'user',
      'identifier' => t('User being viewed'),
      'id' => 0,
      'name' => 'uid',
      'settings' => array(),
    ),
  );
}

/**
 * Callback to get context placeholders provided by this handler.
 */
function delegator_user_view_get_contexts($task, $subtask_id) {
  return ctools_context_get_placeholders_from_argument(delegator_user_view_get_arguments($task, $subtask_id));
}

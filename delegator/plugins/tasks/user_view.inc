<?php
// $Id$

/**
 * Specialized implementation of hook_delegator_tasks(). See api-task.html for
 * more information.
 */
function delegator_user_view_delegator_tasks() {
  return array(
    // This is a 'page' task and will fall under the page admin UI
    'task type' => 'page',
    'title' => t('User view'),
    'description' => t('The user view task allows you to control which modules serve requests made to user/%. By default, the core user module will show the user account page. The first task that matches the user will be used to display the user. If no task handlers exist, or if none of the existing task handlers are configured to handle the currently requested user, then the request falls back to the default Drupal user view mechanism.'),
    'admin title' => 'User view', // translated by menu system
    'admin description' => 'Overrides for the built in user handler, allowing customized user output.',
    'admin path' => 'user/%user',

    // Callback to add items to the delegator task administration form:
    'task admin' => 'delegator_term_view_task_admin',

    'hook menu' => 'delegator_user_view_menu',
    'hook menu alter' => 'delegator_user_view_menu_alter',

    // This is task uses 'context' handlers and must implement these to give the
    // handler data it needs.
    'handler type' => 'context', // handler type -- misnamed
    'get arguments' => 'delegator_user_view_get_arguments',
    'get context placeholders' => 'delegator_user_view_get_contexts',
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
  if ($items['user/%user_uid_optional']['page callback'] == 'user_view' || variable_get('delegator_override_anyway', FALSE)) {
    $items['user/%user_uid_optional']['page callback'] = 'delegator_user_view';
    $items['user/%user_uid_optional']['file path'] = $task['path'];
    $items['user/%user_uid_optional']['file'] = $task['file'];
  }
}

/**
 * Warn if we are unable to override the taxonomy term page.
 */
function delegator_user_view_task_admin(&$form, &$form_state) {
  $callback = db_result(db_query("SELECT page_callback FROM {menu_router} WHERE path = 'user/%'"));
  if ($callback != 'delegator_user_view') {
    drupal_set_message(t('Delegator module is unable to override user/%user because some other module already has overridden with %callback. Delegator will not be able to handle this page.', array('%callback' => $callback)), 'warning');
  }
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

  $output = ctools_context_handler_render($task, '', $contexts, array($account->uid));
  if ($output === FALSE) {
    // Fall back!
    module_load_include('inc', 'user', 'user.pages');
    $output = user_view($account);
  }

  return $output;
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
      'id' => 1,
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


<?php
// $Id$

/**
 * @file
 * Handle the 'term view' override task.
 *
 * This plugin overrides term/%term and reroutes it to the delegator, where
 * a list of tasks can be used to service this request based upon criteria
 * supplied by access plugins.
 */

/**
 * Specialized implementation of hook_delegator_tasks(). See api-task.html for
 * more information.
 */
function delegator_term_view_delegator_tasks() {
  return array(
    'term_view' => array(
      // This is a 'page' task and will fall under the page admin UI
      'task type' => 'page',

      'title' => t('Taxonomy term view'),
      'description' => t('Control what handles the job of displaying a term at taxonomy/term/%term.'),
      'admin title' => 'Taxonomy term view', // translated by menu system
      'admin description' => 'Overrides for the built in taxonomy term handler at <em>taxonomy/term/%term</em>.',
      'admin path' => 'taxonomy/term/%term',

      // Menu hooks so that we can alter the term/%term menu entry to point to us.
      'hook menu' => 'delegator_term_view_menu',
      'hook menu alter' => 'delegator_term_view_menu_alter',

      // This is task uses 'context' handlers and must implement these to give the
      // handler data it needs.
      'handler type' => 'context',
      'get arguments' => 'delegator_term_view_get_arguments',
      'get context placeholders' => 'delegator_term_view_get_contexts',

      // Allow additional operations
      'operations' => array(
        array(
          'title' => t('Task handlers'),
          'href' => "admin/build/delegator/term_view",
        ),
/*
        array(
          'title' => t('Settings'),
          'href' => "admin/build/delegator/term_view/settings",
        ),
*/
      ),
    ),
  );
}

/**
 * Callback defined by delegator_term_view_delegator_tasks().
 *
 * Alter the term view input so that term view comes to us rather than the
 * normal term view process.
 */
function delegator_term_view_menu_alter(&$items, $task) {
  // Override the term view handler for our purpose.
  $items['taxonomy/term/%']['page callback'] = 'delegator_term_view';
  $items['taxonomy/term/%']['file path'] = $task['path'];
  $items['taxonomy/term/%']['file'] = $task['file'];
}

/**
 * Entry point for our overridden term view.
 *
 * This function asks its assigned handlers who, if anyone, would like
 * to run with it. If no one does, it passes through to Drupal core's
 * term view, which is term_page_view().
 */
function delegator_term_view($terms, $depth = 0, $op = 'page') {
  // While we ordinarily should never actually get feeds through here,
  // just in case
  if ($op != 'feed') {
    // Load my task plugin
    $task = delegator_get_task('term_view');

    // Load the term into a context.
    ctools_include('context');
    ctools_include('context-task-handler');
    $contexts = ctools_context_handler_get_task_contexts($task, '', array($terms, $depth));

    $output = ctools_context_handler_render($task, '', $contexts);
    if ($output !== FALSE) {
      return $output;
    }
  }

  // Otherwise, fall back.
  module_load_include('inc', 'taxonomy', 'taxonomy.pages');
  return taxonomy_term_page($terms, $depth, $op);
}

/**
 * Callback to get arguments provided by this task handler.
 *
 * Since this is the term view and there is no UI on the arguments, we
 * create dummy arguments that contain the needed data.
 */
function delegator_term_view_get_arguments($task, $subtask_id) {
  return array(
    array(
      'keyword' => 'term',
      'identifier' => t('Term(s) being viewed'),
      'id' => 0,
      'name' => 'terms',
      'settings' => array(),
    ),
    array(
      'keyword' => 'depth',
      'identifier' => t('Depth'),
      'id' => 0,
      'name' => 'string',
      'settings' => array(),
    ),
  );
}

/**
 * Callback to get context placeholders provided by this handler.
 */
function delegator_term_view_get_contexts($task, $subtask_id) {
  return ctools_context_get_placeholders_from_argument(delegator_term_view_get_arguments($task, $subtask_id));
}


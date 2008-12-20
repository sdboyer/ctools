<?php
// $Id$
/**
 * @file
 * Support for creating 'context' type task handlers.
 *
 * Context task handlers expect the task to provide 0 or more contexts. The
 * task handler should use those contexts as selection criteria, as well as
 * rendering with them.
 *
 * The functions and forms in this file should be common to every context type
 * task handler made.
 *
 * Forms:
 * - ...
 */

/**
 * Compare arguments to contexts for selection purposes.
 *
 * @param $handler
 *   The handler in question.
 * @param $contexts
 *   The context objects provided by the task.
 * @return
 *   TRUE if these contexts match the selection criteria. NULL or FALSE
 *   otherwise.
 */
function ctools_context_handler_select($handler, $contexts) {
  $task = delegator_get_task($handler->task);
  if ($function = ctools_plugin_get_function($task, 'get arguments')) {
    $arguments = $function($task, $handler->subtask);
    foreach ($arguments as $argument) {
      $id = ctools_context_id($argument, 'argument');
      if (empty($handler->conf[$id]) || empty($contexts[$id])) {
        return FALSE;
      }

      if ($function = ctools_plugin_load_function('ctools', 'arguments', $argument['name'], 'criteria select')) {
        if (!$function($handler->conf[$id], $contexts[$id])) {
          return FALSE;
        }
      }
    }
  }
  // Either there are no arguments or all arguments passed.
  return TRUE;
}

/**
 * Get the array of summary strings for the arguments.
 *
 * These summary strings are used to communicate to the user what
 * arguments the task handlers are selecting.
 *
 * @param $task
 *   The loaded task plugin.
 * @param $subtask_id
 *   The subtask id.
 * @param $conf
 *   The configuration array that will contain the various argument settings,
 *   keyed by argument ID.
 */
function ctools_context_handler_summary($task, $subtask_id, $conf) {
  $strings = array();
  if ($function = ctools_plugin_get_function($task, 'get arguments')) {
    $arguments = $function($task, $subtask_id);
    ctools_include('context');
    foreach ($arguments as $argument) {
      if ($function = ctools_plugin_load_function('ctools', 'arguments', $argument['name'], 'criteria summary')) {
        $id = ctools_context_id($argument, 'argument');
        if (!isset($conf[$id])) {
          $conf[$id] = array();
        }
        $strings[] = $function($conf[$id], $argument);
      }
    }
  }

  return $strings;
}

/**
 * Get empty contexts for use with the delegator panels pages.
 */
function ctools_context_handler_placeholders($task, $subtask_id) {
  if ($function = ctools_plugin_get_function($task, 'get context placeholders')) {
    return $function($task, $subtask_id);
  }

  return array();
}

/**
 * Form to choose context based selection criteria for a task handler.
 *
 * The configuration will be assumed to go simply in $handler->conf and
 * will be keyed by the argument ID.
 */
function ctools_context_handler_edit_criteria(&$form, &$form_state) {
  // All 'context' type tasks are required to implement this function.
  $form_state['arguments'] = array();
  if ($function = ctools_plugin_get_function($form_state['task'], 'get arguments')) {
    $arguments = $function($form_state['task'], $form_state['subtask_id']);

    ctools_include('context');
    foreach ($arguments as $argument) {
      if ($function = ctools_plugin_load_function('ctools', 'arguments', $argument['name'], 'criteria form')) {
        $id = ctools_context_id($argument, 'argument');
        if (!isset($form_state['handler']->conf[$id])) {
          $form_state['handler']->conf[$id] = array();
        }

        $form[$id] = array('#tree' => TRUE);
        $function($form, $form_state, $form_state['handler']->conf[$id], $argument, $id);
      }
    }
    $form_state['arguments'] = $arguments;
  }
}

/**
 * Validate handler for criteria selection
 */
function ctools_context_handler_edit_criteria_validate(&$form, &$form_state) {
  foreach ($form_state['arguments'] as $argument) {
    if ($function = ctools_plugin_load_function('ctools', 'arguments', $argument['name'], 'criteria form validate')) {
      $id = ctools_context_id($argument, 'argument');
      $function($form, $form_state, $form_state['handler']->conf[$id], $argument, $id);
    }
  }
}

/**
 * Submit handler for criteria selection
 */
function ctools_context_handler_edit_criteria_submit(&$form, &$form_state) {
  foreach ($form_state['arguments'] as $argument) {
    $id = ctools_context_id($argument, 'argument');
    if ($function = ctools_plugin_load_function('ctools', 'arguments', $argument['name'], 'criteria form submit')) {
      $function($form, $form_state, $form_state['handler']->conf[$id], $argument, $id);
    }

    if (isset($form_state['values'][$id])) {
      $form_state['handler']->conf[$id] = $form_state['values'][$id];
    }
  }
}

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
  if (empty($handler->conf['access'])) {
    return TRUE;
  }

  ctools_include('context');
  return ctools_access($handler->conf['access'], $contexts);
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
 * @param $handler
 *   The handler to be checked.
 */
function ctools_context_handler_summary($task, $subtask_id, $handler) {
  if (empty($handler->conf['access']['plugins'])) {
    return array();
  }

  ctools_include('context');
  $strings = array();
  $contexts = ctools_context_handler_get_all_contexts($task, $subtask_id, $handler);

  foreach ($handler->conf['access']['plugins'] as $test) {
    $plugin = ctools_get_access_plugin($test['name']);
    if ($string = ctools_access_summary($plugin, $contexts, $test)) {
      $strings[] = $string;
    }
  }

  return $strings;
}

// --------------------------------------------------------------------------
// Tasks and Task handlers can both have their own sources of contexts.
// Sometimes we need all of these contexts at once (when editing
// the task handler, for example) but sometimes we need them separately
// (when a task has contexts loaded and is trying out the task handlers,
// for example). Therefore there are two paths we can take to getting contexts.

/**
 * Load the contexts for a task, using arguments.
 *
 * This creates the base array of contexts, loaded from arguments, suitable
 * for use in rendering.
 */
function ctools_context_handler_get_task_contexts($task, $subtask_id, $args) {
  $contexts = array();

  $arguments = ctools_context_handler_get_task_arguments($task, $subtask_id);
  ctools_context_get_context_from_arguments($arguments, $contexts, $args);

  return $contexts;
}

/**
 * Load the contexts for a task handler.
 *
 * This expands a base set of contexts passed in from a task with the
 * contexts defined on the task handler. The contexts from the task
 * must already have been loaded.
 */
function ctools_context_handler_get_handler_contexts($contexts, $handler) {
  $object = ctools_context_handler_get_handler_object($handler);
  return ctools_context_load_contexts($object, FALSE, $contexts);
}

/**
 * Load the contexts for a task and task handler together.
 *
 * This pulls the arguments from a task and everything else from a task
 * handler and loads them as a group. Since there is no data, this loads
 * the contexts as placeholders.
 */
function ctools_context_handler_get_all_contexts($task, $subtask_id, $handler) {
  $object = ctools_context_handler_get_task_object($task, $subtask_id, $handler);
  return ctools_context_load_contexts($object, TRUE);
}

/**
 * Create an object suitable for use with the context system that kind of
 * expects things in a certain, kind of clunky format.
 */
function ctools_context_handler_get_handler_object($handler) {
  $object = new stdClass;
  $object->name = $handler->name;
  $object->contexts = isset($handler->conf['contexts']) ? $handler->conf['contexts'] : array();
  $object->relationships = isset($handler->conf['relationships']) ? $handler->conf['relationships'] : array();

  return $object;
}

/**
 * Create an object suitable for use with the context system that kind of
 * expects things in a certain, kind of clunky format. This one adds in
 * arguments from the task.
 */
function ctools_context_handler_get_task_object($task, $subtask_id, $handler) {
  $object = new stdClass;
  $object->name = $handler->name;
  $object->arguments = ctools_context_handler_get_task_arguments($task, $subtask_id);
  $object->contexts = isset($handler->conf['contexts']) ? $handler->conf['contexts'] : array();
  $object->relationships = isset($handler->conf['relationships']) ? $handler->conf['relationships'] : array();

  return $object;
}

/**
 * Get the arguments from a task that are used to load contexts.
 */
function ctools_context_handler_get_task_arguments($task, $subtask_id) {
  if ($function = ctools_plugin_get_function($task, 'get arguments')) {
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
  if (!isset($form_state['handler']->conf['access'])) {
    $form_state['handler']->conf['access'] = array();
  }

  ctools_include('context');
  $form_state['module'] = 'delegator_task_handler';
  // Encode a bunch of info into the argument so we can get our cache later
  $form_state['callback argument'] = $form_state['task_name'] . '*' . $form_state['handler']->name;
  $form_state['access'] = $form_state['handler']->conf['access'];
  $form_state['no buttons'] = TRUE;
  $form_state['contexts'] = ctools_context_handler_get_all_contexts($form_state['task'], $form_state['subtask_id'], $form_state['handler']);

  ctools_include('context-access-admin');
  $form = array_merge($form, ctools_access_admin_form($form_state));
}

/**
 * Submit handler for criteria selection
 */
function ctools_context_handler_edit_criteria_submit(&$form, &$form_state) {
  $form_state['handler']->conf['access']['logic'] = $form_state['values']['logic'];
  if (!empty($form_state['handler']->conf['autogenerate_title'])) {
    $strings = ctools_context_handler_summary($form_state['task'], $form_state['subtask_id'], $form_state['handler']);
    if (!$strings) {
      $form_state['handler']->conf['title'] = t('Panel');
    }
    else {
      $form_state['handler']->conf['title'] = t('Panel: !criteria', array('!criteria' => implode(', ', $strings)));
    }
  }
}

/**
 * Edit contexts that go with this panel.
 */
function ctools_context_handler_edit_context(&$form, &$form_state) {
  ctools_include('context-admin');
  ctools_context_admin_includes();

  $handler = $form_state['handler'];
  // Check our context object cache.
  if (!empty($_POST)) {
    $cache = ctools_object_cache_get('context_object:delegator', $handler->name);
  }
  else {
    $cache = ctools_context_handler_get_task_object($form_state['task'], $form_state['subtask_id'], $form_state['handler']);
    ctools_object_cache_set('context_object:delegator', $handler->name, $cache);
  }

  $form['right'] = array(
    '#prefix' => '<div class="clear-block"><div class="right-container">',
    '#suffix' => '</div>',
  );

  $form['left'] = array(
    '#prefix' => '<div class="left-container">',
    '#suffix' => '</div></div>',
  );

  ctools_context_add_context_form('delegator', $form, $form_state, $form['right']['contexts_table'], $cache);
  ctools_context_add_relationship_form('delegator', $form, $form_state, $form['right']['relationships_table'], $cache);

  $form['left']['summary'] = array(
    '#prefix' => '<div class="delegator-contexts">',
    '#suffix' => '</div>',
    '#value' => theme('ctools_context_list', $cache, t('Summary of contexts')),
  );

  // @todo -- this CSS is actually dependent upon the plugins which means
  // the plugins need to be able to add it.
  drupal_add_css(panels_get_path('css/panels_admin.css'));
  $form_state['context_object'] = &$cache;
}

/**
 * Process submission of the context edit form.
 */
function ctools_context_handler_edit_context_submit(&$form, &$form_state) {
  $form_state['handler']->conf['contexts'] = $form_state['context_object']->contexts;
  $form_state['handler']->conf['relationships'] = $form_state['context_object']->relationships;
  ctools_object_cache_clear('context_object:delegator', $form_state['handler']->name);
}

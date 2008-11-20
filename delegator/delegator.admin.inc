<?php
// $Id$

/**
 * @file
 * Administrative pages for delegator module.
 */

/**
 * Bit flag on the 'changed' value to tell us if an item was moved.
 */
define('DGA_CHANGED_MOVED', 0x01);

/**
 * Bit flag on the 'changed' value to tell us if an item edited or added.
 */
define('DGA_CHANGED_CACHED', 0x02);

/**
 * Bit flag on the 'changed' value to tell us if an item edited or added.
 */
define('DGA_CHANGED_DELETED', 0x04);

/**
 * Page callback to administer a particular task.
 */
function delegator_administer_task($task_name) {
  // Determine if the task id came in the form of TASK-SUBTASK or just TASK
  if (strpos($task_name, '-') !== FALSE) {
    list($task_id, $subtask_id) = explode('-', $task_name, 2);
  }
  else {
    $task_id = $task_name;
    $subtask_id = NULL;
  }

  $task = delegator_get_task($task_id);
  if (!$task) {
    return drupal_not_found();
  }

  // @todo: check for lock.

  $task_handlers = delegator_load_task_handlers($task, $subtask_id);

  $form_state = array(
    'task' => $task,
    'subtask_id' => $subtask_id,
    'task_handlers' => $task_handlers,
    'cache' => delegator_admin_get_task_cache($task, $subtask_id, $task_handlers),
  );
  ctools_include('form');
  return ctools_build_form('delegator_admin_list_form', $form_state);
}

/**
 * Get the cached changes to a group of task handlers for
 * a given task.
 *
 * This cache stores the current weights for each task, and
 * it also stores a record of whether or not each task handler
 * has been changed so that we can update the display when
 * it is drawn.
 */
function delegator_admin_get_task_cache($task, $subtask_id, $task_handlers) {
  $key = $task['name'] . ':' . $subtask_id;
  ctools_include('object-cache');
  $cache = ctools_object_cache_get('handlers', $key);

  if (!$cache) {
    // If no cache found, create one. We don't write this, though,
    // because we only want to create this object when something
    // actually changes.
    $cache = array();
    foreach ($task_handlers as $id => $handler) {
      $cache[$id]['name'] = $id;
      $cache[$id]['weight'] = $handler->weight;
      $cache[$id]['changed'] = FALSE;
    }
  }

  // Sort the new cache.
  uasort($cache, '_delegator_admin_task_cache_sort');

  return $cache;
}

/**
 * Store information about task handlers in the object cache.
 *
 * This object should have already been retrieved or created by
 * delegator_admin_get_task_cache().
 */
function delegator_admin_set_task_cache($task, $subtask_id, $cache) {
  // First, sort the cache object.
  uasort($cache, '_delegator_admin_task_cache_sort');

  // Then write it.
  $key = $task['name'] . ':' . $subtask_id;
  ctools_include('object-cache');
  $cache = ctools_object_cache_set('handlers', $key, $cache);
}

/**
 * Reset information about the task handlers for a given task.
 */
function delegator_admin_clear_task_cache($task, $subtask_id) {
  ctools_include('object-cache');
  $key = $task['name'] . ':' . $subtask_id;
  $cache = ctools_object_cache_clear('handlers', $key);
}

/**
 * Get the cached changes to a given task handler.
 */
function delegator_admin_get_task_handler_cache($name) {
  ctools_include('object-cache');
  return ctools_object_cache_get('task_handler', $name);
}

/**
 * Store changes to a task handler in the object cache.
 */
function delegator_admin_set_task_handler_cache($handler) {
  ctools_include('object-cache');
  $cache = ctools_object_cache_set('task_handler', $handler->name, $handler);
}

/**
 * Remove an item from the object cache.
 */
function delegator_admin_clear_task_handler_cache($name) {
  ctools_include('object-cache');
  $cache = ctools_object_cache_clear('task_handler', $name);
}

/**
 * Used as a callback to uasort to sort the task cache by weight.
 *
 * The 'name' field is used as a backup when weights are the same, which
 * can happen when multiple modules put items out there at the same
 * weight.
 */
function _delegator_admin_task_cache_sort($a, $b) {
  if ($a['weight'] < $b['weight']) {
    return -1;
  }
  elseif ($a['weight'] > $b['weight']) {
    return 1;
  }
  elseif ($a['name'] < $b['name']) {
    return -1;
  }
  elseif ($a['name'] > $b['name']) {
    return 1;
  }

  return 0;
}

/**
 * Form to administer task handlers assigned to a task.
 */
function delegator_admin_list_form(&$form_state) {
  $task = &$form_state['task'];
  $task_handlers = &$form_state['task_handlers'];
  $cache = &$form_state['cache'];

  // Get a list of possible task handlers for this task.
  $task_handler_plugins = delegator_get_task_handler_plugins($task);

  $form['description'] = array(
    '#prefix' => '<div class="description">',
    '#value' => $task['description'],
    '#suffix' => '</div>',
  );

  $options = array('' => t('Choose'));
  foreach ($task_handler_plugins as $id => $plugin) {
    $options[$id] = $plugin['title'];
  }

  $form['handlers'] = array('#tree' => TRUE);
  $form['#changed'] = FALSE;

  // Create data for a table for all of the task handlers.
  foreach ($cache as $id => $info) {
    // Skip deleted items.
    if ($info['changed'] & DGA_CHANGED_DELETED) {
      $form['#changed'] = TRUE;
      continue;
    }

    if ($info['changed']) {
      $form['#changed'] = TRUE;
    }

    // Use the one from the database or an updated one in cache?
    if ($info['changed'] & DGA_CHANGED_CACHED) {
      $handler = delegator_admin_get_task_handler_cache($id);
    }
    else {
      $handler = $task_handlers[$id];
    }

    $plugin = $task_handler_plugins[$handler->handler];

    $title = delegator_get_handler_title($plugin, $handler, $task, $form_state['subtask_id']);

    $form['handlers'][$id]['title'] = array(
      '#value' => $title,
    );

    $form['handlers'][$id]['weight'] = array(
      '#type' => 'weight',
      '#default_value' => $info['weight'],
    );

    if (isset($plugin['edit forms'])) {
      $form['handlers'][$id]['config'] = array(
        '#value' => 'config button',
      );
    }

    $form['handlers'][$id]['changed'] = array(
      '#type' => 'value',
      '#value' => $info['changed'],
    );

    if (!empty($plugin['edit forms'])) {
      $form['handlers'][$id]['config'] = array(
        // '#value' must NOT be set on an image_button or it will always
        // appear clicked.
        '#type' => 'image_button',
        '#src' => drupal_get_path('module', 'delegator') . '/images/configure.png',
        '#handler' => $id, // so the submit handler can tell which one this is
        '#submit' => array('delegator_admin_list_form_config'),
      );
    }

    $form['handlers'][$id]['delete'] = array(
      // '#value' must NOT be set on an image_button or it will always
      // appear clicked.
      '#type' => 'image_button',
      '#src' => drupal_get_path('module', 'delegator') . '/images/delete.png',
      '#handler' => $id, // so the submit handler can tell which one this is
      '#submit' => array('delegator_admin_list_form_delete'),
    );

    $form['handlers'][$id]['class'] = array(
      '#value' => 'draggable',
    );

    // This sets the tabledrag last dragged class so that the most recently
    // touched row will show up yellow. This is a nice reminder after adding
    // or editing a row which one was touched.
    if (isset($info['last touched'])) {
      $form['handlers'][$id]['class']['#value'] .= ' delegator-changed';
    }
  }

  $form['handler'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => '',
  );
  $form['add_handler'] = array(
    '#type' => 'submit',
    '#value' => t('Add new handler'),
    '#validate' => array('delegator_admin_list_form_validate_add'),
    '#submit' => array('delegator_admin_list_form_add'),
  );

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#submit' => array('delegator_admin_list_form_submit'),
  );
  $form['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
    '#submit' => array('delegator_admin_list_form_cancel'),
  );

  return $form;
}

/**
 * Theme the form so it has a table.
 */
function theme_delegator_admin_list_form($form) {
  $output = '';

  $output .= drupal_render($form['description']);

  // Assemble the data for a table from everything in $form['handlers']
  foreach (element_children($form['handlers']) as $id) {
    // provide a reference shortcut.
    $element = &$form['handlers'][$id];
    if (isset($element['title'])) {
      $row = array();

      $title = drupal_render($element['title']);
      // Add a visible 'changed' flag to the title.
      if ($element['changed']['#value']) {
        $title .= '<span class="warning tabledrag-changed">*</span>';
      }

      $row[] = array(
        'data' => $title,
        'class' => 'delegator-handler',
      );

      $element['weight']['#attributes']['class'] = 'weight';
      $row[] = drupal_render($element['weight']);

      $operations = '';
      if (isset($element['config'])) {
        $operations .= drupal_render($element['config']);
      }
      $operations .= drupal_render($element['delete']);
      $row[] = array(
        'data' => $operations,
        'class' => 'delegator-operations',
      );

      $class = drupal_render($element['class']);

      $rows[] = array('data' => $row, 'class' => $class, 'id' => 'delegator-row-' . $id);
    }
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => t('No task handlers are defined for this task.'), 'colspan' => '3'));
  }

  $header = array(
    array('data' => t('Task handler'), 'class' => 'delegator-handler'),
    t('Weight'),
    array('data' => t('Operations'), 'class' => 'delegator-operations')
  );

  drupal_add_tabledrag('delegator-task-list-arrange', 'order', 'sibling', 'weight');

  $attributes = array('id' => 'delegator-task-list-arrange');
  if ($form['#changed']) {
    $attributes['class'] = 'changed';
  }

  $output .= theme('table', $header, $rows, $attributes);

  // Render the add button + select box as a table too.
  $output .= theme('table', array(), array(array(drupal_render($form['handler']), drupal_render($form['add_handler']))));

  $path = drupal_get_path('module', 'delegator');
  drupal_add_js("$path/js/task-handlers.js");
  drupal_add_css("$path/css/task-handlers.css");

  $output .= drupal_render($form);
  return $output;
}

/**
 * Don't let them submit the 'please choose' button.
 */
function delegator_admin_list_form_validate_add($form, &$form_state) {
  if (!$form_state['values']['handler']) {
    form_error($form['handler'], t('Please choose a task handler to add.'));
  }
}

/**
 * Update the weights from the form.
 *
 * Since we're looping, we determine the highest weight + 1 and return that.
 */
function delegator_admin_update_weights(&$form_state) {
  // Go through our cache and check weights.
  foreach ($form_state['cache'] as $id => $info) {
    // update weights from form.
    if (isset($form_state['values']['handlers'][$id]['weight']) && $form_state['values']['handlers'][$id]['weight'] != $info['weight']) {
      $form_state['cache'][$id]['weight'] = $info['weight'] = $form_state['values']['handlers'][$id]['weight'];
      $form_state['cache'][$id]['changed'] |= DGA_CHANGED_MOVED;
    }

    // Record the highest weight we've seen so we know what to set our addition.
    if (!isset($weight) || $info['weight'] >= $weight) {
      $weight = $info['weight'] + 1;
    }

    // Unset any 'last touched' flag and let whatever handler is updating the
    // weights do that if it wants to.
    if (isset($form_state['cache'][$id]['last touched'])) {
      unset($form_state['cache'][$id]['last touched']);
    }
  }

  // if weight stubbornly continues to not be set (meaning the cache was empty)
  // make it 0.
  if (!isset($weight)) {
    $weight = 0;
  }

  return $weight;
}

/**
 * Add a new task handler.
 *
 * This submit handler creates a new task handler and stores it in the
 * cache, then if there is a settings page, redirects to the proper
 * settings page. If there isn't one it simply redirects to the
 * back to itself.
 */
function delegator_admin_list_form_add($form, &$form_state) {
  $plugin = delegator_get_task_handler($form_state['values']['handler']);

  // Update the weights from the form.
  $weight = delegator_admin_update_weights($form_state);

  // Generate a unique name. Unlike most named objects, we don't let people choose
  // names for task handlers because they mostly don't make sense.
  $base = $form_state['task']['name'];
  if ($form_state['subtask_id']) {
    $base .= '_' . $form_state['subtask_id'];
  }
  $base .= '_' . $plugin['name'];

  // Once we have a base, check to see if it is used. If it is, start counting up.
  $name = $base;
  $count = 1;
  // If taken
  while (isset($form_state['cache'][$name])) {
    $name = $base . '_' . ++$count;
  }

  // Create a new, empty handler object.
  $handler = new stdClass;
  $handler->task = $form_state['task']['name'];
  $handler->subtask = $form_state['subtask_id'];
  $handler->name = $name;
  $handler->handler = $plugin['name'];
  $handler->weight = $weight;
  $handler->conf = array();
  if (isset($plugin['default conf'])) {
    if (is_array($plugin['default conf'])) {
      $handler->conf = $plugin['default conf'];
    }
    else if (function_exists($plugin['default conf'])) {
      $handler->conf = $plugin['default conf']($handler, $form_state['task'], $form_state['subtask_id']);
    }
  }

  // Store the new handler.
  delegator_admin_set_task_handler_cache($handler);

  $form_state['cache'][$handler->name] = array(
    'name' => $handler->name,
    'weight' => $handler->weight,
    'changed' => DGA_CHANGED_CACHED,
    'last touched' => TRUE,
  );

  // Store the changed task handler list.
  delegator_admin_set_task_cache($form_state['task'], $form_state['subtask_id'], $form_state['cache']);

  // If the task handler plugin specifies an add form, set a redirect.
  if (isset($plugin['add forms'])) {
    // Get the beginning of the array.
    reset($plugin['add forms']);
    list($id, $title) = each($plugin['add forms']);
    $form_state['redirect'] = "admin/build/delegator/" . $form_state['task']['name'] . "/add/$handler->name/$id";
  }
}

/**
 * Save all changes to the task handler list.
 */
function delegator_admin_list_form_submit($form, &$form_state) {
  // Update the weights from the form.
  delegator_admin_update_weights($form_state);

  $cache = &$form_state['cache'];
  // Go through each of the task handlers, check to see if it needs updating,
  // and update it if so.
  foreach ($cache as $id => $info) {
    // If it has been marked for deletion, delete it.
    if ($info['changed'] & DGA_CHANGED_DELETED) {
      if (isset($form_state['task_handlers'][$id])) {
        delegator_delete_task_handler($form_state['task_handlers'][$id]);
      }
    }
    // If it has been somehow edited (or added), write the cached version
    elseif ($info['changed'] & DGA_CHANGED_CACHED) {
      // load and write the cached version.
      $handler = delegator_admin_get_task_handler_cache($id);
      // Make sure we get updated weight from the form for this.
      $handler->weight = $info['weight'];
      delegator_save_task_handler($handler);

      // Now that we've written it, remove it from cache.
      delegator_admin_clear_task_handler_cache($id);
    }
    // Otherwise, check to see if it has moved and, if so, update the weight.
    elseif ($info['weight'] != $form_state['task_handlers'][$id]->weight) {
      // update the weight. Since it could be in the databsae, we'll just
      // write the task handler we have.
      $form_state['task_handlers'][$id]->weight = $info['weight'];
      delegator_save_task_handler($form_state['task_handlers'][$id]);
    }
  }

  drupal_set_message(t('All changes have been updated.'));

  // Clear the cache and set a redirect.
  delegator_admin_clear_task_cache($form_state['task'], $form_state['subtask_id']);
  $form_state['redirect'] = $_GET['q'];
}

/**
 * Cancel all changes to the task handler list.
 */
function delegator_admin_list_form_cancel($form, &$form_state) {
  drupal_set_message(t('All changes have been discarded.'));
  foreach ($form_state['cache'] as $id => $info) {
    if ($info['changed'] & DGA_CHANGED_CACHED) {
      // clear cached version.
      delegator_admin_clear_task_handler_cache($id);
    }
  }

  delegator_admin_clear_task_cache($form_state['task'], $form_state['subtask_id']);
}

/**
 * Submit handler for item deletion.
 *
 * This is attached to every delete button; it uses $form_state['clicked_value']
 * to know which delete button was pressed. In the form, we set #handler => $id
 * to that this information could be easily retrieved.
 */
function delegator_admin_list_form_delete($form, &$form_state) {
  // Update the weights from the form.
  delegator_admin_update_weights($form_state);

  $id = $form_state['clicked_button']['#handler'];
  // This overwrites 'moved' and 'cached' states.
  if ($form_state['cache'][$id]['changed'] & DGA_CHANGED_CACHED) {
    // clear cached version.
    delegator_admin_clear_task_handler_cache($id);
  }
  $form_state['cache'][$id]['changed'] = DGA_CHANGED_DELETED;

  // Store the changed task handler list.
  delegator_admin_set_task_cache($form_state['task'], $form_state['subtask_id'], $form_state['cache']);
  $form_state['redirect'] = $_GET['q'];
}

/**
 * Submit handler to configure an item.
 *
 * This is attached to every configure button; it uses $form_state['clicked_value']
 * to know which delete button was pressed. In the form, we set #handler => $id
 * to that this information could be easily retrieved.
 */
function delegator_admin_list_form_config($form, &$form_state) {
  // Update the weights from the form.
  delegator_admin_update_weights($form_state);

  // Store the changed task handler list.
  delegator_admin_set_task_cache($form_state['task'], $form_state['subtask_id'], $form_state['cache']);

  $id = $form_state['clicked_button']['#handler'];

  // Use the one from the database or an updated one in cache?
  if ($form_state['cache'][$id]['changed'] & DGA_CHANGED_CACHED) {
    $handler = delegator_admin_get_task_handler_cache($id);
  }
  else {
    $handler = $form_state['task_handlers'][$id];
  }

  $name = $form_state['task']['name'];
  // @todo: Allow an owner UI to control this URL.
  // @todo: subtask ID
  $form_state['redirect'] = "admin/build/delegator/$name/$handler->handler/$id";
}

/**
 * Entry point to edit a task handler.
 */
function delegator_administer_task_handler_edit($task_name, $handler_id, $name, $form_id) {
  // Determine if the task id came in the form of TASK-SUBTASK or just TASK
  if (strpos($task_name, '-') !== FALSE) {
    list($task_id, $subtask_id) = explode('-', $task_name, 2);
  }
  else {
    $task_id = $task_name;
    $subtask_id = NULL;
  }

  $handler = delegator_admin_get_task_handler_cache($name);
  if (!$handler) {
    $handler = delegator_load_task_handler($name);
  }

  if (!$handler) {
    return drupal_not_found();
  }

  $task = delegator_get_task($task_id);
  $plugin = delegator_get_task_handler($handler_id);

  // Prevent silliness of using some other handler type's tabs for this
  // particular handler, or of somehow having invalid tasks or task handlers.
  if ($handler_id != $handler->handler ||
      !$task ||
      !$plugin ||
      !isset($plugin['forms'][$form_id]) ||
      !isset($plugin['edit forms'][$form_id])) {
    return drupal_not_found();
  }

  $title = delegator_get_handler_title($plugin, $handler, $task, $subtask_id);
  drupal_set_title(t('Edit task handler "@title"', array('@title' => $title)));

  // Figure out what the next form is for the 'save and continue' button.
  // Loop through the forms because we need to know what our next form is
  reset($plugin['edit forms']);
  while (list($id, $title) = each($plugin['edit forms'])) {
    if ($form_id == $id) {
      break;
    }
  }

  // The 'break' made our next form current. Running each
  // again will advance by one.
  $next_info = each($plugin['edit forms']);

  $form_state = array(
    'task' => $task,
    'subtask_id' => $subtask_id,
    'plugin' => $plugin,
    'handler' => $handler,
    'plugin_form_id' => $form_id, // so it doesn't get confused with the form's form ID
    'forms' => $plugin['edit forms'],
    'type' => 'edit',
  );

  if (!empty($plugin['forms'][$form_id]['alternate next'])) {
    $form_state['next'] = "admin/build/delegator/$task_name/$handler_id/$name/" . $plugin['forms'][$form_id]['alternate next'];
  }
  elseif ($next_info) {
    $form_state['next'] = "admin/build/delegator/$task_name/$handler_id/$name/$next_info[key]";
  }

  ctools_include('form');
  $output = ctools_build_form('delegator_admin_edit_task_handler', $form_state);
  if ($output && !empty($plugin['forms'][$form_id]['no blocks'])) {
    print theme('page', $output, FALSE);
  }
  else {
    return $output;
  }
}

/**
 * Entry point to add a task handler.
 */
function delegator_administer_task_handler_add($task_id, $name, $form_id) {
  // Determine if the task id came in the form of TASK-SUBTASK or just TASK
  if (strpos($task_name, '-') !== FALSE) {
    list($task_id, $subtask_id) = explode('-', $task_name, 2);
  }
  else {
    $task_id = $task_name;
    $subtask_id = NULL;
  }

  $handler = delegator_admin_get_task_handler_cache($name);

  if (!$handler) {
    return drupal_not_found();
  }

  $task = delegator_get_task($task_id);
  $plugin = delegator_get_task_handler($handler->handler);

  // Prevent silliness of using some other handler type's tabs for this
  // particular handler, or of somehow having invalid tasks or task handlers.
  if (!$task ||
      !$plugin ||
      !isset($plugin['forms'][$form_id]) ||
      !isset($plugin['add forms'][$form_id])) {
    return drupal_not_found();
  }

  $title = delegator_get_handler_title($plugin, $handler, $task, $subtask_id);
  drupal_set_title(t('Add task handler "@title"', array('@title' => $title)));

  $form_state = array(
    'task' => $task,
    'subtask_id' => $subtask_id,
    'plugin' => $plugin,
    'handler' => $handler,
    'plugin_form_id' => $form_id, // so it doesn't get confused with the form's form ID
    'forms' => $plugin['add forms'],
    'type' => 'add',
  );

  $output = '';

  $crumbs = array();
  $previous = TRUE;
  foreach ($plugin['add forms'] as $id => $title) {
    if ($id == $form_id) {
      $previous = FALSE;
      $class = 'delegator-current';
    }
    elseif ($previous) {
      $not_first = TRUE;
      $class = 'delegator-previous';
    }
    else {
      $class = 'delegator-next';
      if (!isset($form_state['next'])) {
        $form_state['next'] = "admin/build/delegator/$task_name/add/$name/$id";
      }
    }
    $crumbs[] = '<span class="' . $class . '">' . $title . '</span>';
  }

  drupal_add_css(drupal_get_path('module', 'delegator') . '/css/task-handlers.css');

  if (empty($not_first)) {
    $output .= '<div class="delegator-explanation">' . t('Before proceeding, you must configure your new "@type"', array('@type' => $plugin['title'])) . '</div>';
  }

  $output .= theme('delegator_breadcrumb', $crumbs);
  ctools_include('form');
  $output .= ctools_build_form('delegator_admin_edit_task_handler', $form_state);
  return $output;
}

/**
 * Themable display of the 'breadcrumb' trail to show the process of
 * adding a new item.
 */
function theme_delegator_breadcrumb($breadcrumb) {
  if (!empty($breadcrumb)) {
    return '<div class="delegator-breadcrumb">' . implode(' » ', $breadcrumb) . '</div>';
  }
}

/**
 * Build an actual form to edit a task handler.
 *
 * Most of this form is handled by the task handler plugin, this is primarily
 * just providing a framework for the system to work within.
 */
function delegator_admin_edit_task_handler(&$form_state) {
  $plugin = $form_state['plugin'];
  $form_id = $form_state['plugin_form_id'];
  $task = $form_state['task'];
  $handler = $form_state['handler'];
  $forms = $form_state['forms']; // either edit or add forms depending upon entry

  $info = $plugin['forms'][$form_id];

  if (!empty($info['include'])) {
    if (is_array($info['include'])) {
      foreach ($info['include'] as $file) {
        require_once './' . $file;
      }
    }
    else {
      require_once './' . $info['include'];
    }
  }

  $form['conf']['#tree'] = TRUE;

  // Let it have validate and submit handlers in a way that's nicer to the
  // D6 system.
  $validate = array();
  if (!empty($info['validate']) && function_exists($info['validate'])) {
    $validate = array($info['validate']);
  }

  $submit = array();
  if (!empty($info['submit']) && function_exists($info['submit'])) {
    $submit = array($info['submit']);
  }
  $submit[] = 'delegator_admin_edit_task_handler_submit';

  // Ensure buttons stay on the bottom.
  $form['buttons'] = array(
    '#prefix' => '<div class="clear-block">',
    '#suffix' => '</div>',
    '#weight' => 1000
  );

  if (isset($form_state['next'])) {
    $form['buttons']['next'] = array(
      '#type' => 'submit',
      '#value' => t('Continue'),
      '#next' => $form_state['next'],
      '#validate' => $validate,
      '#submit' => $submit,
    );
  }

  if ($form_state['type'] == 'edit' && empty($info['no return'])) {
    $form['buttons']['return'] = array(
      '#type' => 'submit',
      '#value' => t('Update and return'),
      '#next' => "admin/build/delegator/$task[name]",
      '#validate' => $validate,
      '#submit' => $submit,
    );
  }
  else if (empty($form_state['next'])) {
    $form['buttons']['next'] = array(
      '#type' => 'submit',
      '#value' => t('Add handler'),
      '#next' => "admin/build/delegator/$task[name]",
      '#validate' => $validate,
      '#submit' => $submit,
    );
  }

  $form['buttons']['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
    '#next' => "admin/build/delegator/$task[name]",
    '#submit' => array('delegator_admin_edit_task_handler_cancel'),
  );

  // Allow the plugin to add its form items now.
  if (!empty($info['form']) && function_exists($info['form'])) {
    $info['form']($form, $form_state);
  }

  return $form;
}

/**
 * Submit handler for task handler edit form.
 *
 * Cache data and proceed to the next form as specified by the clicked button.
 */
function delegator_admin_edit_task_handler_submit($form, &$form_state) {
  $handler = &$form_state['handler'];

  // Update the task handler cache to let the system know this one has now
  // officially changed.
  $cache = delegator_admin_get_task_cache($form_state['task'], $form_state['subtask_id'], array());

  // Only bother updating the cache if we're going to change something, so if
  // our handler is not marked changed or is not the last touched handler, do so.
  if (!($cache[$handler->name]['changed'] & DGA_CHANGED_CACHED) || empty($cache[$handler->name]['last touched'])) {
    // Clear the last touched flag from all handlers
    foreach ($cache as $id => $info) {
      if (isset($info['last touched'])) {
        unset($cache[$id]['last touched']);
      }
    }

    // Set status of our handler
    $cache[$handler->name]['changed'] |= DGA_CHANGED_CACHED;
    $cache[$handler->name]['last touched'] = TRUE;
    delegator_admin_set_task_cache($form_state['task'], $form_state['subtask_id'], $cache);
  }

  if (isset($form_state['values']['conf']) && is_array($form_state['values']['conf'])) {
    // Merge whatever is in the form values with the existing configuration.
    $handler->conf = array_merge($handler->conf, $form_state['values']['conf']);
  }

  // Write to cache.
  delegator_admin_set_task_handler_cache($handler);
  $form_state['redirect'] = $form_state['clicked_button']['#next'];
}

/**
 * Submit handler for task handler edit form.
 *
 * This is really just a nice button to return to the top level without
 * caching changes from the form. It skips validation and submit.
 */
function delegator_admin_edit_task_handler_cancel($form, &$form_state) {
  if ($form_state['type'] == 'add') {
    // flush the newly added handler from the cache so that it won't show up.
    delegator_admin_clear_task_handler_cache($form_state['handler']->name);

    // Send an array() through as the list of $task_handlers -- because
    // if we're at this point there MUST be something in the cache.
    $cache = delegator_admin_get_task_cache($form_state['task'], $form_state['subtask_id'], array());
    if (isset($cache[$form_state['handler']->name])) {
      unset($cache[$form_state['handler']->name]);
    }

    delegator_admin_set_task_cache($form_state['task'], $form_state['subtask_id'], $cache);
  }
  $form_state['redirect'] = $form_state['clicked_button']['#next'];
}

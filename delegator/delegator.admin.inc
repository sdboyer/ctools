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
 * Bit flag on the 'changed' value to tell us if an item has had its disabled status changed.
 */
define('DGA_CHANGED_STATUS', 0x08);

/**
 * Reset the active menu trail to the trail specified by the task type.
 *
 * The task type can specify an 'admin path'. If it does,
 * 'admin/build/delegator' will be removed from the active trail and replaced
 * with whatever is in the admin path; that way task types can provide
 * their own administration.
 */
function delegator_set_trail($task) {
  $task_type = delegator_get_task_type($task['task type']);
  if (empty($task_type['admin path'])) {
    return;
  }

  ctools_include('menu');
  $trail = menu_get_active_trail();
  $remove = ctools_get_menu_trail('admin/build/delegator');
  foreach ($remove as $id => $info) {
    if (isset($trail[$id])) {
      unset($trail[$id]);
    }
  }

  $trail = array_merge(ctools_get_menu_trail($task_type['admin path']), $trail);
  menu_set_active_trail($trail);
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
function delegator_admin_get_task_cache($task, $subtask_id, $task_handlers = NULL) {
  $key = delegator_make_task_name($task['name'], $subtask_id);
  ctools_include('object-cache');
  $cache = ctools_object_cache_get('delegator_handlers', $key);

  if (!$cache) {
    // If no cache found, create one. We don't write this, though,
    // because we only want to create this object when something
    // actually changes.
    if (!isset($task_handlers)) {
      $task_handlers = delegator_load_task_handlers($task, $subtask_id);
    }

    $cache = new stdClass;
    $cache->handlers = array();
    foreach ($task_handlers as $id => $handler) {
      $cache->handlers[$id]['name'] = $id;
      $cache->handlers[$id]['weight'] = $handler->weight;
      $cache->handlers[$id]['changed'] = FALSE;
      $cache->handlers[$id]['disabled'] = !empty($handler->disabled);
    }
    $cache->locked = ctools_object_cache_test('delegator_handlers', $key);
  }

  // Sort the new cache.
  uasort($cache->handlers, '_delegator_admin_task_cache_sort');

  return $cache;
}

/**
 * Store information about task handlers in the object cache.
 *
 * This object should have already been retrieved or created by
 * delegator_admin_get_task_cache().
 */
function delegator_admin_set_task_cache($task, $subtask_id, $cache) {
  if ($cache->locked) {
    drupal_set_message(t('Unable to update task due to lock.'), 'error');
    return;
  }

  // We only bother if something has been marked changed. This keeps us from
  // locking when we should not.
  $changed = FALSE;
  if (!empty($cache->working)) {
    $changed = TRUE;
  }
  else {
    foreach ($cache->handlers as $handler) {
      if (!empty($handler['changed'])) {
        $changed = TRUE;
        break;
      }
    }
  }

  if (!$changed) {
    // We may have cancelled a working copy. We'll actually clear cache in this
    // instance.
    delegator_admin_clear_task_cache($task, $subtask_id);
    return;
  }

  // First, sort the cache object.
  uasort($cache->handlers, '_delegator_admin_task_cache_sort');

  // Then write it.
  $key = delegator_make_task_name($task['name'], $subtask_id);
  ctools_include('object-cache');
  $cache = ctools_object_cache_set('delegator_handlers', $key, $cache);
}

/**
 * Reset information about the task handlers for a given task.
 */
function delegator_admin_clear_task_cache($task, $subtask_id) {
  ctools_include('object-cache');
  $key = delegator_make_task_name($task['name'], $subtask_id);
  ctools_object_cache_clear('delegator_handlers', $key);
}

/**
 * Get the cached changes to a given task handler.
 */
function delegator_admin_get_task_handler_cache($name) {
  ctools_include('object-cache');
  return ctools_object_cache_get('delegator_task_handler', $name);
}

/**
 * Store changes to a task handler in the object cache.
 */
function delegator_admin_set_task_handler_cache($handler, $working = FALSE) {
  $name = $handler->name;

  if ($working) {
    $name = delegator_make_task_name($handler->task, $handler->subtask) . '-working';
  }

  ctools_include('object-cache');
  $cache = ctools_object_cache_set('delegator_task_handler', $name, $handler);
}

/**
 * Remove an item from the object cache.
 */
function delegator_admin_clear_task_handler_cache($name) {
  ctools_include('object-cache');
  ctools_object_cache_clear('delegator_task_handler', $name);
}

/**
 * Write a new handler to the database.
 */
function delegator_admin_new_task_handler($handler, $task_name, $task, $subtask_id, $cache, $plugin) {
  // Store the new handler.
  if (!$cache->locked) {
    delegator_admin_set_task_handler_cache($handler, TRUE);
  }

  $cache->handlers[$handler->name] = array(
    'name' => $handler->name,
    'weight' => $handler->weight,
    'changed' => DGA_CHANGED_CACHED,
    'disabled' => FALSE,
  );
  $cache->working = $handler->name;

  // Store the changed task handler list.
  delegator_admin_set_task_cache($task, $subtask_id, $cache);

  // If the task handler plugin specifies an add form, set a redirect.
  if (isset($plugin['add forms'])) {
    // Get the beginning of the array.
    reset($plugin['add forms']);
    list($id, $title) = each($plugin['add forms']);
    return "admin/build/delegator/$task_name/add/$handler->name/$id";
  }
  else {
    return "admin/build/delegator/$task_name";
  }
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
 * Find the right handler to use for an id during the edit process.
 *
 * When editing, a handler may be stored in cache. It may also be
 * reverted and unsaved, which can cause issues all their own. This
 * function can be used to find the right handler to use in these cases.
 */
function delegator_admin_find_handler($id, $cache, $task_name, $task_handlers = array()) {
  // Use the one from the database or an updated one in cache?
  if (isset($cache->working) && $cache->working == $id) {
    $handler = delegator_admin_get_task_handler_cache($task_name . '-working');
  }
  else if ($cache->handlers[$id]['changed'] & DGA_CHANGED_CACHED) {
    $handler = delegator_admin_get_task_handler_cache($id);
  }
  else {
    // Special case: Reverted handlers get their defaults back.
    if ($cache->handlers[$id]['changed'] & DGA_CHANGED_DELETED) {
      ctools_include('export');
      $handler = ctools_get_default_object('delegator_handlers', $id);
    }
    else if (!empty($task_handlers)) {
      $handler = $task_handlers[$id];
    }
    else {
      list($task_id, $subtask_id) = delegator_get_task_id($task_name);
      $task = delegator_get_task($task_id);
      $handler = delegator_load_task_handler($task, $subtask_id, $id);
    }
  }

  return $handler;
}

/**
 * Page callback to administer a particular task.
 */
function delegator_administer_task($task_name) {
  list($task_id, $subtask_id) = delegator_get_task_id($task_name);

  $task = delegator_get_task($task_id);
  if (!$task) {
    return drupal_not_found();
  }

  $subtask = delegator_get_task_subtask($task, $subtask_id);

  // Subtasks marked as single tasks won't see this page, so redirect
  // to wherever the subtask wants to be administered from.
  if (!empty($subtask['single task'])) {
    $task_type = delegator_get_task_type($task['task type']);
    return drupal_goto($task_type['admin path']);
  }

  $task_handlers = delegator_load_task_handlers($task, $subtask_id);
  delegator_set_trail($task);

  $form_state = array(
    'task_name' => $task_name,
    'task_id' => $task_id,
    'task' => $task,
    'subtask' => $subtask,
    'subtask_id' => $subtask_id,
    'task_handlers' => $task_handlers,
    'cache' => delegator_admin_get_task_cache($task, $subtask_id, $task_handlers),
  );
  ctools_include('form');
  return ctools_build_form('delegator_admin_list_form', $form_state);
}

/**
 * Form to administer task handlers assigned to a task.
 */
function delegator_admin_list_form(&$form_state) {
  $task = &$form_state['task'];
  $subtask = &$form_state['subtask'];
  $task_handlers = &$form_state['task_handlers'];
  $cache = &$form_state['cache'];

  // Get a list of possible task handlers for this task.
  $task_handler_plugins = delegator_get_task_handler_plugins($task);

  if (isset($task['description'])) {
    $form['description'] = array(
      '#prefix' => '<div class="description">',
      '#value' => $task['description'],
      '#suffix' => '</div>',
    );
  }

  if ($subtask && isset($subtask['description'])) {
    $form['subtask_description'] = array(
      '#prefix' => '<div class="description">',
      '#value' => $subtask['description'],
      '#suffix' => '</div>',
    );
  }

  $options = array('' => t('Choose'));
  foreach ($task_handler_plugins as $id => $plugin) {
    $options[$id] = $plugin['title'];
  }

  $form['handlers'] = array('#tree' => TRUE);
  $form['#changed'] = FALSE;

  // Create data for a table for all of the task handlers.
  foreach ($cache->handlers as $id => $info) {
    // Skip deleted items.
    $handler = delegator_admin_find_handler($id, $form_state['cache'], $form_state['task_name'], $task_handlers);

    if (!$handler || ($info['changed'] & DGA_CHANGED_DELETED && !($handler->export_type & EXPORT_IN_CODE))) {
      $form['#changed'] = TRUE;
      continue;
    }

    if ($info['changed']) {
      $form['#changed'] = TRUE;
    }

    if (isset($task_handler_plugins[$handler->handler])) {
      $plugin = $task_handler_plugins[$handler->handler];

      $title = delegator_get_handler_title($plugin, $handler, $task, $form_state['subtask_id']);
      $summary = delegator_get_handler_summary($plugin, $handler, $task, $form_state['subtask_id']);
    }
    else {
      $title = t('Broken/missing handler plugin "@plugin"', array('@plugin' => $handler->handler));
      $summary = '';
    }

    $form['handlers'][$id]['title'] = array(
      '#value' => $title,
    );

    $form['handlers'][$id]['summary'] = array(
      '#value' => $summary,
    );

    $form['handlers'][$id]['weight'] = array(
      '#type' => 'weight',
      '#default_value' => $info['weight'],
    );

    $form['handlers'][$id]['changed'] = array(
      '#type' => 'value',
      '#value' => $info['changed'],
    );

    // Make a list of possible actions.
    $actions = array(
      '' => t('Actions...'),
    );

    if ($info['disabled']) {
      $actions['enable'] = t('Enable');
    }
    // For enabled handlers.
    else {
      // Make all of the edit items under the Edit optgroup.
      if (!empty($plugin['edit forms'])) {
        foreach ($plugin['edit forms'] as $edit_id => $title) {
          if ($title) {
            $actions[t('Edit')]['edit-' . $edit_id] = $title;
          }
        }
      }

      if (isset($plugin)) {
        $actions['clone'] = t('Clone');
        $actions['export'] = t('Export');
      }

      if ($handler->export_type == (EXPORT_IN_CODE | EXPORT_IN_DATABASE)) {
        $actions['delete'] = t('Revert');
      }
      else if ($handler->export_type == EXPORT_IN_CODE) {
        $actions['disable'] = t('Disable');
      }
      else {
        $actions['delete'] = t('Delete');
      }
    }

    $form['handlers'][$id]['dropdown'] = array(
      '#value' => theme('ctools_dropdown', t('Operations'), delegator_admin_make_dropdown_links($actions), 'delegator-task-handler-operations')
    );

    $form['handlers'][$id]['action'] = array(
      '#type' => 'select',
      '#options' => $actions,
    );

    $form['handlers'][$id]['config'] = array(
      // image buttons must not have a #value or they will not be properly detected.
      '#type' => 'image_button',
      '#src' => drupal_get_path('module', 'delegator') . '/images/configure.png',
      '#handler' => $id, // so the submit handler can tell which one this is
      '#submit' => array('delegator_admin_list_form_action'),
    );

    $type = $handler->type;

    // Adjust type for this scenario: They have reverted a handler to the in code
    // version and have not modified it again.
    if ($type == t('Overridden') && $info['changed'] &= DGA_CHANGED_DELETED && !($info['changed'] &= DGA_CHANGED_CACHED)) {
      $type = t('Default');
    }

    $class = 'draggable';
    if ($type == t('Overridden')) {
      $class .= ' delegator-overridden';
    }
    else if ($type == t('Default')) {
      $class .= ' delegator-default';
      if ($info['disabled']) {
        $class .= ' delegator-disabled';
      }
    }

    $form['handlers'][$id]['class'] = array(
      '#value' => $class,
    );

    if ($info['disabled']) {
      $type .= ', ' . t('Disabled');
    }

    $form['handlers'][$id]['type'] = array(
      '#value' => $type,
    );

    // This sets the tabledrag last dragged class so that the most recently
    // touched row will show up yellow. This is a nice reminder after adding
    // or editing a row which one was touched.
    if (isset($cache->last_touched) && $cache->last_touched == $handler->name) {
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

  $form['#delegator-lock'] = $cache->locked;
  $form['#task-name'] = $form_state['task_name'];

  // Set up a list of callbacks for our actions. This method allows
  // clever form_alter uses to add more actions easily.

  // Bear in mind that any action will be split on a '-' so don't use it
  // in your name. This is how 'edit' can edit multiple forms, i.e,
  // edit-settings, edit-context, edit-foobarbaz.
  $form['#actions'] = array(
    'edit' => 'delegator_admin_list_form_action_edit',
    'delete' => 'delegator_admin_list_form_action_delete',
    'enable' => 'delegator_admin_list_form_action_enable',
    'disable' => 'delegator_admin_list_form_action_disable',
    'clone' => 'delegator_admin_list_form_action_clone',
    'export' => 'delegator_admin_list_form_action_export',
  );

  return $form;
}

/**
 * Make a set of links out of the actions array.
 *
 * because this can have embedded arrays, this is a function so it can
 * use recursion.
 */
function delegator_admin_make_dropdown_links($actions) {
  $links = array();
  // Take the actions and make a dropdown for those of us with javascript.
  foreach ($actions as $id => $text) {
    if (!$id) {
      continue;
    }

    if (is_array($text)) {
      $links[] =  array(
        'title' => '<span class="text">' . $id . '</span>' . theme('links', delegator_admin_make_dropdown_links($text)),
        'html' => TRUE,
      );
    }
    else {
      $links[] = array(
        'title' => $text,
        'href' => $id,
      );
    }
  }

  return $links;
}

/**
 * Theme the form so it has a table.
 */
function theme_delegator_admin_list_form($form) {
  $output = '';
  if (!empty($form['#delegator-lock'])) {
    $account = user_load($form['#delegator-lock']->uid);
    $name = theme('username', $account);
    $lock_age = format_interval(time() - $form['#delegator-lock']->updated);
    $break = url('admin/build/delegator/' . $form['#task-name'] . '/break-lock');

    $output .= '<div class="delegator-locked">';
    $output .= t('This task is being edited by user !user, and is therefore locked from editing by others. This lock is !age old. Click here to <a href="!break">break this lock</a>.', array('!user' => $name, '!age' => $lock_age, '!break' => $break));
    $output .= '</div>';
  }

  if (isset($form['description'])) {
    $output .= drupal_render($form['description']);
  }
  if (isset($form['subatsk_description'])) {
    $output .= drupal_render($form['subtask_description']);
  }

  // Assemble the data for a table from everything in $form['handlers']
  foreach (element_children($form['handlers']) as $id) {
    // provide a reference shortcut.
    $element = &$form['handlers'][$id];
    if (isset($element['title'])) {
      $row = array();

      $changed_text = '';
      // Add a visible 'changed' flag if necessary.
      if ($element['changed']['#value']) {
        $changed_text = '<span class="warning tabledrag-changed">*</span>';
      }

      $row[] = array(
        'data' => $changed_text,
        'class' => 'delegator-changed-col',
      );

      if ($summary = drupal_render($element['summary'])) {
        $title = theme('ctools_collapsible', drupal_render($element['title']), $summary, TRUE);
      }
      else {
        $title = drupal_render($element['title']);
      }

      $row[] = array(
        'data' => $title,
        'class' => 'delegator-handler',
      );

      $row[] = array(
        'data' => drupal_render($element['type']),
        'class' => 'delegator-type',
      );

      $element['weight']['#attributes']['class'] = 'weight';
      $row[] = drupal_render($element['weight']);

      $operations = '';
      $operations .= drupal_render($element['dropdown']);
      $operations .= drupal_render($element['action']);
      $operations .= drupal_render($element['config']);
      $row[] = array(
        'data' => $operations,
        'class' => 'delegator-operations',
      );

      $class = drupal_render($element['class']);

      $rows[] = array('data' => $row, 'class' => $class, 'id' => 'delegator-row-' . $id);
    }
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => t('No task handlers are defined for this task.'), 'colspan' => '5'));
  }

  $header = array(
    array('data' => '', 'class' => 'delegator-changed-col'),
    array('data' => t('Task handler'), 'class' => 'delegator-handler'),
    array('data' => t('Type'), 'class' => 'delegator-type'),
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
  $handlers = &$form_state['cache']->handlers;
  foreach ($handlers as $id => $info) {
    // update weights from form.
    if (isset($form_state['values']['handlers'][$id]['weight']) && $form_state['values']['handlers'][$id]['weight'] != $info['weight']) {
      $handlers[$id]['weight'] = $info['weight'] = $form_state['values']['handlers'][$id]['weight'];
      $handlers[$id]['changed'] |= DGA_CHANGED_MOVED;
    }

    // Record the highest weight we've seen so we know what to set our addition.
    if (!isset($weight) || $info['weight'] >= $weight) {
      $weight = $info['weight'] + 1;
    }

    // Unset any 'last touched' flag and let whatever handler is updating the
    // weights do that if it wants to.
    unset($form_state['cache']->last_touched);
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

  $handler = delegator_new_task_handler($form_state['task'], $form_state['subtask_id'], $plugin, $weight, $form_state['cache']);

  $form_state['redirect'] = delegator_admin_new_task_handler($handler, $form_state['task_name'], $form_state['task'], $form_state['subtask_id'], $form_state['cache'], $plugin);
}

/**
 * Save all changes to the task handler list.
 */
function delegator_admin_list_form_submit($form, &$form_state) {
  // Update the weights from the form.
  $form_state['redirect'] = $_GET['q'];

  // If there's an admin path, go there instead.
  $task_type = delegator_get_task_type($form_state['task']['task type']);
  if (isset($task_type['admin path'])) {
    $form_state['redirect'] = $task_type['admin path'];
  }

  delegator_admin_update_weights($form_state);

  $cache = &$form_state['cache'];
  if ($cache->locked) {
    drupal_set_message(t('Unable to update task due to lock.'), 'error');
    return;
  }

  // Go through each of the task handlers, check to see if it needs updating,
  // and update it if so.
  foreach ($cache->handlers as $id => $info) {
    // If it has been marked for deletion, delete it.
    if ($info['changed'] & DGA_CHANGED_DELETED) {
      if (isset($form_state['task_handlers'][$id])) {
        delegator_delete_task_handler($form_state['task_handlers'][$id]);
      }
    }
    // If it has been somehow edited (or added), write the cached version
    if ($info['changed'] & DGA_CHANGED_CACHED) {
      // load and write the cached version.
      $handler = delegator_admin_get_task_handler_cache($id);
      // Make sure we get updated weight from the form for this.
      $handler->weight = $info['weight'];
      delegator_save_task_handler($handler);

      // Now that we've written it, remove it from cache.
      delegator_admin_clear_task_handler_cache($id);

      // @todo -- do we need to clear the handler weight here?
    }
    // Otherwise, check to see if it has moved and, if so, update the weight.
    elseif ($info['weight'] != $form_state['task_handlers'][$id]->weight) {
      // Theoretically we could only do this for in code objects, but since our
      // load mechanism checks for all, this is less database work.
      delegator_update_task_handler_weight($form_state['task_handlers'][$id], $info['weight']);
    }

    // Set enable/disabled status.
    if ($info['changed'] & DGA_CHANGED_STATUS) {
      ctools_include('export');
      ctools_export_set_status('delegator_handlers', $id, $info['disabled']);
    }
  }

  drupal_set_message(t('All changes have been saved.'));

  // Clear the cache
  delegator_admin_clear_task_cache($form_state['task'], $form_state['subtask_id']);
  delegator_admin_clear_task_handler_cache($form_state['task_name'] . '-working');
}

/**
 * Cancel all changes to the task handler list.
 */
function delegator_admin_list_form_cancel($form, &$form_state) {
  drupal_set_message(t('All changes have been discarded.'));
  foreach ($form_state['cache']->handlers as $id => $info) {
    if ($info['changed'] & DGA_CHANGED_CACHED) {
      // clear cached version.
      delegator_admin_clear_task_handler_cache($id);
    }
  }

  delegator_admin_clear_task_cache($form_state['task'], $form_state['subtask_id']);
  delegator_admin_clear_task_handler_cache($form_state['task_name'] . '-working');

  // If there's an admin path, go there instead.
  $task_type = delegator_get_task_type($form_state['task']['task type']);
  if (isset($task_type['admin path'])) {
    $form_state['redirect'] = $task_type['admin path'];
  }
}

/**
 * Submit handler for item action.
 *
 * This is attached to every delete button; it uses $form_state['clicked_value']
 * to know which delete button was pressed. In the form, we set #handler => $id
 * to that this information could be easily retrieved.
 *
 * The actual action to call will be in the 'action' setting for the handler.
 */
function delegator_admin_list_form_action($form, &$form_state) {
  // Update the weights from the form.
  delegator_admin_update_weights($form_state);

  $id = $form_state['clicked_button']['#handler'];
  $action = $form_state['values']['handlers'][$id]['action'];

  // Set this now, that way handlers can override it to go elsewhere if they
  // want.
  $form_state['redirect'] = $_GET['q'];

  // Break up our
  if (strpos($action, '-') !== FALSE) {
    list($action, $argument) = explode('-', $action, 2);
  }
  else {
    $action = $action;
    $argument = NULL;
  }

  if (!empty($form['#actions'][$action]) && function_exists($form['#actions'][$action])) {
    $form['#actions'][$action]($form, $form_state, $id, $action, $argument);
  }

  delegator_admin_set_task_cache($form_state['task'], $form_state['subtask_id'], $form_state['cache']);
  return;
}


/**
 * Delegated submit handler to delete an item.
 */
function delegator_admin_list_form_action_delete($form, &$form_state, $id, $action, $argument) {
  // This overwrites 'moved' and 'cached' states.
  if ($form_state['cache']->handlers[$id]['changed'] & DGA_CHANGED_CACHED && !$form_state['cache']->locked) {
    // clear cached version.
    delegator_admin_clear_task_handler_cache($id);
  }
  $form_state['cache']->handlers[$id]['changed'] = DGA_CHANGED_DELETED;
}

/**
 * Delegated submit handler to edit an item.
 *
 * Which form to go to will be specified by $argument.
 */
function delegator_admin_list_form_action_edit($form, &$form_state, $id, $action, $argument) {
  // Use the one from the database or an updated one in cache?
  $handler = delegator_admin_find_handler($id, $form_state['cache'], $form_state['task_name'], $form_state['task_handlers']);

  $name = $form_state['task_name'];
  $form_state['redirect'] = "admin/build/delegator/$name/$handler->handler/$id/$argument";
}

/**
 * Clone an existing task handler into a new handler.
 */
function delegator_admin_list_form_action_clone($form, &$form_state, $id, $action, $argument) {
  // Use the one from the database or an updated one in cache?
  $handler = delegator_admin_find_handler($id, $form_state['cache'], $form_state['task_name'], $form_state['task_handlers']);

  // Get the next weight from the form
  $handler->weight = delegator_admin_update_weights($form_state);

  // Generate a unique name. Unlike most named objects, we don't let people choose
  // names for task handlers because they mostly don't make sense.
  $base = $form_state['task']['name'];
  if ($form_state['subtask_id']) {
    $base .= '_' . $form_state['subtask_id'];
  }
  $base .= '_' . $handler->handler;

  // Once we have a base, check to see if it is used. If it is, start counting up.
  $name = $base;
  $count = 1;
  // If taken
  while (isset($form_state['cache']->handlers[$name])) {
    $name = $base . '_' . ++$count;
  }

  $handler->name = $name;

  // Store the new handler.
  if (!$form_state['cache']->locked) {
    delegator_admin_set_task_handler_cache($handler);
  }

  $form_state['cache']->handlers[$handler->name] = array(
    'name' => $handler->name,
    'weight' => $handler->weight,
    'changed' => DGA_CHANGED_CACHED,
    'disabled' => FALSE,
  );
  $form_state['cache']->last_touched = $handler->name;
}

/**
 * Export a task handler.
 */
function delegator_admin_list_form_action_export($form, &$form_state, $id, $action, $argument) {
  // Redirect to the export page.
  $name = $form_state['task_name'];
  $form_state['redirect'] = "admin/build/delegator/$name/export/$id";
}

/**
 * Enable a task handler.
 */
function delegator_admin_list_form_action_enable($form, &$form_state, $id, $action, $argument) {
  $form_state['cache']->handlers[$id]['changed'] |= DGA_CHANGED_STATUS;
  $form_state['cache']->handlers[$id]['disabled'] = FALSE;
}

/**
 * Enable a task handler.
 */
function delegator_admin_list_form_action_disable($form, &$form_state, $id, $action, $argument) {
  $form_state['cache']->handlers[$id]['changed'] |= DGA_CHANGED_STATUS;
  $form_state['cache']->handlers[$id]['disabled'] = TRUE;
}

/**
 * Entry point to export a task handler.
 */
function delegator_administer_task_handler_export($task_name, $name) {
  list($task_id, $subtask_id) = delegator_get_task_id($task_name);
  $task = delegator_get_task($task_id);

  $handler = delegator_admin_get_task_handler_cache($name);
  if (!$handler) {
    $handler = delegator_load_task_handler($task, $subtask_id, $name);
  }

  if (!$handler) {
    return drupal_not_found();
  }

  $plugin = delegator_get_task_handler($handler->handler);

  $title = delegator_get_handler_title($plugin, $handler, $task, $subtask_id);
  drupal_set_title(t('Export task handler "@title"', array('@title' => $title)));

  ctools_include('export');
  delegator_set_trail($task);
  return drupal_get_form('ctools_export_form', delegator_export_task_handler($handler), $title);
}

/**
 * Basic form info for the wizard to multi-step edit a task handler.
 */
function delegator_admin_task_handler_form_info($task_name) {
  return array(
    'id' => 'delegator_task_handler',
    'show back' => TRUE,
    'show cancel' => TRUE,

    'return path' => "admin/build/delegator/$task_name",

    'next callback'   => 'delegator_admin_edit_task_handler_next',
    'finish callback' => 'delegator_admin_edit_task_handler_finish',
    'return callback' => 'delegator_admin_edit_task_handler_finish',
    'cancel callback' => 'delegator_admin_edit_task_handler_cancel',
  );
}

/**
 * Entry point to edit a task handler.
 */
function delegator_administer_task_handler_edit($task_name, $handler_id, $name, $step) {
  list($task_id, $subtask_id) = delegator_get_task_id($task_name);

  $task = delegator_get_task($task_id);
  $subtask = delegator_get_task_subtask($task, $subtask_id);
  $plugin = delegator_get_task_handler($handler_id);

  $cache = delegator_admin_get_task_cache($task, $subtask_id);

  $handler = delegator_admin_find_handler($name, $cache, $task_name);

  if (!$handler) {
    return drupal_not_found();
  }

  // Prevent silliness of using some other handler type's tabs for this
  // particular handler, or of somehow having invalid tasks or task handlers.
  if ($handler_id != $handler->handler ||
      !$task ||
      !$plugin ||
      !isset($plugin['forms'][$step]) ||
      !isset($plugin['edit forms'][$step])) {
    return drupal_not_found();
  }

  $form_info = delegator_admin_task_handler_form_info($task_name);
  if (!empty($subtask['single task'])) {
    $task_type = delegator_get_task_type($task['task type']);
    $form_info['return path'] = $task_type['admin path'];
  }
  $form_info['order'] = $plugin['edit forms'];
  $form_info['forms'] = $plugin['forms'];
  $form_info['path'] = "admin/build/delegator/$task_name/$handler_id/$name/%step";
  if (empty($plugin['forms'][$step]['no return'])) {
    $form_info['show return'] = TRUE;
  }

  $title = delegator_get_handler_title($plugin, $handler, $task, $subtask_id);
  drupal_set_title(t('Edit task handler "@title"', array('@title' => $title)));

  delegator_set_trail($task);
  $form_state = array(
    'step' => $step,
    'task_name' => $task_name,
    'task_id' => $task_id,
    'task' => $task,
    'subtask_id' => $subtask_id,
    'subtask' => $subtask,
    'plugin' => $plugin,
    'handler' => $handler,
    'type' => 'edit',
    'cache' => $cache,
  );

  ctools_include('wizard');
  $output = ctools_wizard_multistep_form($form_info, $step, $form_state);

  drupal_add_css(drupal_get_path('module', 'delegator') . '/css/task-handlers.css');
  if (!$output) {
    // redirect.
    drupal_redirect_form(array(), $form_state['redirect']);
  }

  if (!empty($plugin['forms'][$step]['no blocks'])) {
    print theme('page', $output, FALSE);
  }
  else {
    return $output;
  }
}

/**
 * Entry point to add a task handler.
 */
function delegator_administer_task_handler_add($task_name, $name, $step) {
  list($task_id, $subtask_id) = delegator_get_task_id($task_name);

  $handler = delegator_admin_get_task_handler_cache($task_name . '-working');

  if (!$handler) {
    return drupal_not_found();
  }

  $task = delegator_get_task($task_id);
  $subtask = delegator_get_task_subtask($task, $subtask_id);
  $plugin = delegator_get_task_handler($handler->handler);

  // Prevent silliness of using some other handler type's tabs for this
  // particular handler, or of somehow having invalid tasks or task handlers.
  if (!$task ||
      !$plugin ||
      !isset($plugin['forms'][$step]) ||
      !isset($plugin['add forms'][$step])) {
    return drupal_not_found();
  }

  $cache = delegator_admin_get_task_cache($task, $subtask_id);
  $form_info = delegator_admin_task_handler_form_info($task_name);
  // Single tasks will return to a different location, this will help that.
  if (!empty($subtask['single task'])) {
    $task_type = delegator_get_task_type($task['task type']);
    $form_info['return path'] = $task_type['admin path'];
  }

  $form_info['order'] = $plugin['add forms'];
  $form_info['forms'] = $plugin['forms'];
  $form_info['path'] = "admin/build/delegator/$task_name/add/$handler->name/%step";
  $form_info['show trail'] = TRUE;

  $title = isset($subtask['admin title']) ? $subtask['admin title'] : $task['admin title'];
  drupal_set_title(t('Add task handler for %task', array('%task' => $title)));

  delegator_set_trail($task);
  $form_state = array(
    'step' => $step,
    'task_name' => $task_name,
    'task_id' => $task_id,
    'task' => $task,
    'subtask_id' => $subtask_id,
    'subtask' => $subtask,
    'plugin' => $plugin,
    'handler' => $handler,
    'type' => 'add',
    'cache' => $cache,
  );

  ctools_include('wizard');
  $output = ctools_wizard_multistep_form($form_info, $step, $form_state);

  if (!$output) {
    // redirect.
    drupal_redirect_form(array(), $form_state['redirect']);
  }

  drupal_add_css(drupal_get_path('module', 'delegator') . '/css/task-handlers.css');

  if (!empty($plugin['forms'][$step]['no blocks'])) {
    print theme('page', $output, FALSE);
  }
  else {
    return $output;
  }

  /*
  if (empty($not_first)) {
    $output .= '<div class="delegator-explanation">' . t('Before proceeding, you must configure your new "@type"', array('@type' => $plugin['title'])) . '</div>';
  }
  */
}

/**
 * Form wizard 'next' handler -- update the cache.
 */
function delegator_admin_edit_task_handler_next(&$form_state) {
  if ($form_state['cache']->locked) {
    drupal_set_message(t('Unable to update task due to lock.'), 'error');
    return;
  }

  $handler = &$form_state['handler'];

  // This updates the working cache, since we're not finished.
  delegator_admin_set_task_handler_cache($handler, TRUE);

  // Make sure the cache is aware that we're editing this item:
  if (!isset($form_state['cache']->working) || $form_state['cache']->working != $handler->name) {
    $form_state['cache']->working = $handler->name;
    delegator_admin_set_task_cache($form_state['task'], $form_state['subtask_id'], $form_state['cache']);
  }
}


/**
 * Form wizard finish handler; called to update everything the wizard touched.
 *
 * This transfers the working cache to the task handler cache and
 * updates the cache to mark this item as having changed.
 */
function delegator_admin_edit_task_handler_finish(&$form_state) {
  $handler = &$form_state['handler'];
  $cache = &$form_state['cache'];

  if ($cache->locked) {
    drupal_set_message(t('Unable to update task due to lock.'), 'error');
    return;
  }


  // Set status of our handler
  $cache->handlers[$handler->name]['changed'] |= DGA_CHANGED_CACHED;
  $cache->last_touched = $handler->name;
  unset($cache->working);

  delegator_admin_set_task_cache($form_state['task'], $form_state['subtask_id'], $cache);

  if (isset($form_state['values']['conf']) && is_array($form_state['values']['conf'])) {
    // Merge whatever is in the form values with the existing configuration.
    $handler->conf = array_merge($handler->conf, $form_state['values']['conf']);
  }

  // Write to cache.
  delegator_admin_set_task_handler_cache($handler);

  // Remove working copy.
  delegator_admin_clear_task_handler_cache($form_state['task_name'] . '-working');

  // If the task or subtask is defined as containing a single handler, then just save
  // the whole cache here since they won't get a chance to later.
  if (!empty($form_state['subtask']['single task']) || !empty($form_state['task']['single task'])) {
    delegator_admin_list_form_submit(array(), $form_state);
  }
}

/**
 * Wizard cancel handler for the task handler edit. Clear the working
 * copy from cache.
 */
function delegator_admin_edit_task_handler_cancel(&$form_state) {
  // Remove working copy.
  delegator_admin_clear_task_handler_cache($form_state['task_name'] . '-working');
}

/**
 * Form to break a lock on a delegator task.
 */
function delegator_administer_break_lock(&$form_state, $task_name) {
  list($task_id, $subtask_id) = delegator_get_task_id($task_name);

  $form_state['task_name'] = $task_name;

  ctools_include('object-cache');
  $lock = ctools_object_cache_test('delegator_handlers', $form_state['task_name']);
  $form = array();

  // @todo put task title here, but also needs subtask support.
  if (empty($lock)) {
    return array('message' => array('#value' => t('There is no lock on this task to break.')));
  }

  $task = delegator_get_task($task_id);
  delegator_set_trail($task);

  $cancel = 'admin/build/delegator/' . $task_name;
  if (!empty($_REQUEST['cancel'])) {
    $cancel = $_REQUEST['cancel'];
  }

  $account = user_load($lock->uid);
  return confirm_form($form,
                  t('Are you sure you want to break this lock?'),
                  $cancel,
                  t('By breaking this lock, any unsaved changes made by !user will be lost!', array('!user' => theme('username', $account))),
                  t('Break lock'),
                  t('Cancel'));
}

/**
 * Submit handler to break_lock a view.
 */
function delegator_administer_break_lock_submit(&$form, &$form_state) {
  ctools_object_cache_clear_all('delegator_handlers', $form_state['task_name']);
  drupal_set_message(t('The lock has been broken and you may now edit this task.'));
  $form_state['redirect'] = 'admin/build/delegator/' . $form_state['task_name'];
}

/**
 * Import a task handler from cut & paste
 */
function delegator_admin_import_task_handler(&$form_state) {
  drupal_set_title(t('Import task handler'));
  $form['object'] = array(
    '#type' => 'textarea',
    '#title' => t('Paste task handler code here'),
    '#rows' => 15,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
  );
  return $form;
}

/**
 * Ensure we got a valid task handler.
 */
function delegator_admin_import_task_handler_validate($form, &$form_state) {
  ob_start();
  eval($form_state['values']['object']);
  ob_end_clean();

  if (!isset($handler) || !is_object($handler)) {
    form_error($form['object'], t('Unable to interpret task handler code.'));
  }

  // @todo support subtasks here
  $task = delegator_get_task($handler->task);
  if (!$task) {
    form_error($form['object'], t('The task for that handler does not seem to exist.'));
  }

  // See if the task is already locked.
  $cache = delegator_admin_get_task_cache($task, $handler->subtask);
  if ($cache->locked) {
    $account = user_load($cache->locked->uid);
    $name = theme('username', $account);
    $lock_age = format_interval(time() - $cache->locked->updated);
    $break = url('admin/build/delegator/' . $handler->task . '/break-lock', array('query' => array('destination' => $_GET['q'], 'cancel' => $_GET['q'])));

    form_error($form['object'], t('Unable to import task handler because the task is being edited by user !user, and is therefore locked from editing by others. This lock is !age old. Click here to <a href="!break">break this lock</a>.', array('!user' => $name, '!age' => $lock_age, '!break' => $break)));
  }

  $handler->type = t('Normal');

  ctools_include('export');
  $handler->export_type = EXPORT_IN_DATABASE;

  if (isset($cache->handlers[$handler->name])) {
    drupal_set_message(t('Warning: The handler you are importing already exists and this operation will overwrite an existing handler. If this is not what you intend, you may Cancel this. You should then modify the <code>$handler-&gt;name</code> field of your import to have a unique name.'), 'warning');

    $old_handler = delegator_admin_find_handler($handler->name, $cache, $form_state['task_name']);
    $handler->export_type = $old_handler->export_type | EXPORT_IN_DATABASE;
  }

  $form_state['task'] = $task;
  $form_state['cache'] = $cache;
  $form_state['handler'] = $handler;
}

/**
 * Submit handler to import an existing page.
 */
function delegator_admin_import_task_handler_submit($form, &$form_state) {
  // Use the one from the database or an updated one in cache?
  $handler = &$form_state['handler'];
  $cache = &$form_state['cache'];

  delegator_admin_set_task_handler_cache($handler);

  $cache->handlers[$handler->name] = array(
    'name' => $handler->name,
    'weight' => $handler->weight,
    'changed' => DGA_CHANGED_CACHED,
    'disabled' => FALSE,
  );
  $cache->last_touched = $handler->name;

  delegator_admin_set_task_cache($form_state['task'], $handler->subtask, $cache);
  $form_state['redirect'] = 'admin/build/delegator/' . $form_state['task_name'];
}

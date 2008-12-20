<?php
// $Id$

/**
 * @file
 * Administrative functions for the page task type system
 *
 * This system provides a unified administrative interface to handle page
 * tasks of all types, including a generic 'page' task that lets a user
 * create pages as well as specific page tasks for handling built-in pages.
 */
function delegator_page_type_list() {
  $tasks = delegator_get_tasks_by_type('page');

  $tables = array(
    'singles' => array(
      'title' => t('System pages'),
      'description' => t('Pages provided by the system that can be overridden to provide alternate content or layout.'),
      'rows' => array(),
    ),
    'page' => array(), // give this one special weighting,
  );

  delegator_page_type_sort_tasks($tasks, $tables, 'singles');

  $output = '';
  $header = array(
    array('data' => t('Title'), 'class' => 'delegator-page-title'),
    array('data' => t('Path'), 'class' => 'delegator-page-path'),
    array('data' => t('Operations'), 'class' => 'delegator-page-operations'),
  );

  foreach ($tables as $bucket => $info) {
    $output .= '<h3>' . check_plain($info['title']) . '</h3>';
    $output .= '<div class="description">' . check_plain($info['description']) . '</div>';
    if (isset($info['operations'])) {
      $info['rows'][] = array('data' => array(array('data' => theme('links', $info['operations']), 'colspan' => 3)), 'class' => 'delegator-page-operations');
    }
    $output .= theme('table', $header, $info['rows']);
  }

  drupal_add_css(drupal_get_path('module', 'delegator') . '/css/page-task.css');
  return $output;
}

/**
 * Sort tasks into buckets based upon whether or not they have subtasks.
 */
function delegator_page_type_sort_tasks($tasks, &$tables, $bucket, $task_id = NULL) {
  foreach ($tasks as $id => $task) {
    // If a type has subtasks, add its subtasks in its own table.
    if (!empty($task['subtasks'])) {
      $tables[$id]['title'] = $task['title'];
      $tables[$id]['description'] = $task['description'];
      if (isset($task['operations'])) {
        $tables[$id]['operations'] = $task['operations'];
      }

      delegator_page_type_sort_tasks(delegator_get_task_subtasks($task), $tables, $id, $task['name']);
      continue;
    }

    if (isset($task_id)) {
      $task_name = delegator_make_task_name($task_id, $task['name']);
    }
    else {
      $task_name = $task['name'];
    }

    $row = array('data' => array(), 'class' => 'page-task-' . $id);
    $row['data'][] = array('data' => $task['admin title'], 'class' => 'delegator-page-title');
    $row['data'][] = array('data' => $task['admin path'], 'class' => 'delegator-page-path');
    if (isset($task['operations'])) {
      $operations = $task['operations'];
    }
    else {
      $operations = array(
        array(
          'title' => t('Task handlers'),
          'href' => "admin/build/delegator/$task_name"
        ),
      );
    }
    $row['data'][] = array('data' => theme('ctools_dropdown', t('Operations'), $operations), 'class' => 'delegator-page-operations');

    $tables[$bucket]['rows'][] = $row;
  }
}


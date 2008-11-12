<?php
// $Id$

function delegator_test_simple_delegator_task_handlers() {
  return array(
    'test_simple' => array(
      'title' => t('Test simple type'),
      'task type' => 'node_view',
      'admin title' => 'delegator_test_simple_title',
    ),
  );
}

function delegator_test_simple_title($handler, $task, $subtask_id) {
  return 'Simple: ' . $handler->name;
}

<?php
// $Id$

function delegator_node_view_type_delegator_task_handlers() {
  return array(
    'node_view_type' => array(
      'title' => t('Test complex type'),
      'task type' => 'node_view',
      'admin title' => 'delegator_node_view_type_title',
      'add forms' => array('foo' => t('Foo form'), 'baz' => t('Baz form')),
      'edit forms' => array('foo' => t('Foo form'), 'bar' => t('Bar form')),
      'forms' => array(
         'foo' => array('form' => 'delegator_node_view_type_edit_foo'),
         'bar' => array('form' => 'delegator_node_view_type_edit_bar'),
         'baz' => array('form' => 'delegator_node_view_type_edit_baz'),
      ),
    ),
  );
}

function delegator_node_view_type_title($handler, $task, $subtask_id) {
  return 'Complex: ' . $handler->name;
}

function delegator_node_view_type_edit_foo(&$form_state) {
  $form['info'] = array(
    '#prefix' => '<div class="description">',
    '#value' => 'This is foo',
    '#suffix' => '</div>',
  );

  return $form;
}

function delegator_node_view_type_edit_bar(&$form_state) {
  $form['info'] = array(
    '#prefix' => '<div class="description">',
    '#value' => 'This is bar',
    '#suffix' => '</div>',
  );

  return $form;
}

function delegator_node_view_type_edit_baz(&$form_state) {
  $form['info'] = array(
    '#prefix' => '<div class="description">',
    '#value' => 'This is baz',
    '#suffix' => '</div>',
  );

  return $form;
}

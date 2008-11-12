<?php
// $Id$

function delegator_test_panel_delegator_task_handlers() {
  return array(
    'test_panel' => array(
      'title' => t('Panel'),
      'task type' => 'node_view',
      'admin title' => 'delegator_test_panel_title',
      'add forms' => array(
        'node_type' => t('Select node type'),
        'choose' => t('Choose layout'),
        'layout-settings' => t('Configure layout')
      ),
      'edit forms' => array(
        'node_type' => t('Node type'),
        'layout' => t('Change layout'),
        'layout-settings' => t('Layout settings'),
        'content' => t('Panel content')
      ),
      'forms' => array(
         'node_type' => array('form' => 'delegator_test_panel_edit_node_type'),
         'choose' => array('form' => 'delegator_test_panel_edit_choose'),
         'layout' => array('form' => 'delegator_test_panel_edit_change'),
         'layout-settings' => array('form' => 'delegator_test_panel_edit_layout_settings'),
         'content' => array('form' => 'delegator_test_panel_edit_content'),
      ),
      'default conf' => array('type' => array()),
    ),
  );
}

function delegator_test_panel_title($handler, $task, $subtask_id) {
  $types = node_get_types();

  $names = array();
  foreach (array_filter($handler->conf['type']) as $type) {
    $names[] = check_plain($types[$type]->name);
  }

  return format_plural(count($names), 'Panel for node type "@types"', 'Panel for node types "@types"', array('@types' => implode(', ', $names)));
}

function delegator_test_panel_edit_node_type(&$form_state) {
  $types = node_get_types();
  foreach ($types as $type => $info) {
    $options[$type] = check_plain($info->name);
  }

  $form['type'] = array(
    '#type' => 'checkboxes',
    '#options' => $options,
    '#description' => t('This task handler will only be active for the selected node types. If no node types are selected, it will be active for all node types.'),
    '#default_value' => $form_state['handler']->conf['type'],
  );

  return $form;
}

function delegator_test_panel_edit_choose(&$form_state) {
  $form['info'] = array(
    '#prefix' => '<div class="description">',
    '#value' => 'Please pretend you were just presented with a list of layout types, complete with icons and radio buttons, and asked to select one.',
    '#suffix' => '</div>',
  );

  return $form;
}

function delegator_test_panel_edit_change(&$form_state) {
  $form['info'] = array(
    '#prefix' => '<div class="description">',
    '#value' => 'Please pretend you were just presented with a list of layout types, complete with icons and radio buttons, and asked to select one. You are also then given an opportunity to move content from one to another.',
    '#suffix' => '</div>',
  );

  return $form;
}

function delegator_test_panel_edit_layout_settings(&$form_state) {
  $form['info'] = array(
    '#prefix' => '<div class="description">',
    '#value' => 'On this page you will be allowed to modify the settings for a given layout, which is where you get to do all the flexible panel stuff.',
    '#suffix' => '</div>',
  );

  return $form;
}

function delegator_test_panel_edit_content(&$form_state) {
  $form['info'] = array(
    '#prefix' => '<div class="description">',
    '#value' => 'And here you got to move stuff around. This one will probably be fun to implement.',
    '#suffix' => '</div>',
  );

  return $form;
}

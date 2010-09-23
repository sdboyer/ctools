<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide a node context. A node context is a node wrapped in a
 * context object that can be utilized by anything that accepts contexts.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t("Node"),
  'description' => t('A node object.'),
  'context' => 'ctools_context_create_node',
  'edit form' => 'ctools_context_node_settings_form',
  'defaults' => array('nid' => ''),
  'keyword' => 'node',
  'context name' => 'node',
  'convert list' => 'ctools_context_node_convert_list',
  'convert' => 'ctools_context_node_convert',
  'placeholder form' => array(
    '#type' => 'textfield',
    '#description' => t('Enter the node ID of a node for this context.'),
  ),
);

/**
 * It's important to remember that $conf is optional here, because contexts
 * are not always created from the UI.
 */
function ctools_context_create_node($empty, $data = NULL, $conf = FALSE) {
  $context = new ctools_context('node');
  $context->plugin = 'node';

  if ($empty) {
    return $context;
  }

  if ($conf) {
    $nid = is_array($data) && isset($data['nid']) ? $data['nid'] : (is_object($data) ? $data->nid : 0);

    if (module_exists('translation')) {
      if ($translation = module_invoke('translation', 'node_nid', $nid, $GLOBALS['language']->language)) {
        $nid = $translation;
        $reload = TRUE;
      }
    }

    if (is_array($data) || !empty($reload)) {
      $data = node_load($nid);
    }
  }

  if (!empty($data)) {
    $context->data     = $data;
    $context->title    = $data->title;
    $context->argument = $data->nid;

    $context->restrictions['type'] = array($data->type);
    return $context;
  }
}

function ctools_context_node_settings_form($form, &$form_state) {
  $conf = &$form_state['conf'];

  $form['node'] = array(
    '#prefix' => '<div class="no-float">',
    '#suffix' => '</div>',
    '#title' => t('Enter the title or NID of a node'),
    '#type' => 'textfield',
    '#maxlength' => 512,
    '#autocomplete_path' => 'ctools/autocomplete/node',
    '#weight' => -10,
  );

  if (!empty($conf['nid'])) {
    $info = db_query('SELECT * FROM {node} WHERE nid = :nid', array(':nid' => $conf['nid']))->fetchObject();
    if ($info) {
      $link = l(t("'%title' [node id %nid]", array('%title' => $info->title, '%nid' => $info->nid)), "node/$info->nid", array('attributes' => array('target' => '_blank', 'title' => t('Open in new window')), 'html' => TRUE));
      $form['node']['#description'] = t('Currently set to !link', array('!link' => $link));
    }
  }

  $form['nid'] = array(
    '#type' => 'value',
    '#value' => $conf['nid'],
  );

  $form['set_identifier'] = array(
    '#type' => 'checkbox',
    '#default_value' => FALSE,
    '#title' => t('Reset identifier to node title'),
    '#description' => t('If checked, the identifier will be reset to the node title of the selected node.'),
  );

  return $form;
}

/**
 * Validate a node.
 */
function ctools_context_node_settings_form_validate($form, &$form_state) {
  // Validate the autocomplete
  if (empty($form_state['values']['nid']) && empty($form_state['values']['node'])) {
    form_error($form['node'], t('You must select a node.'));
    return;
  }

  if (empty($form_state['values']['node'])) {
    return;
  }

  $nid          = $form_state['values']['node'];
  $preg_matches = array();
  $match        = preg_match('/\[nid: (\d+)\]/', $nid, $preg_matches);
  if (!$match) {
    $match = preg_match('/^nid: (\d+)/', $nid, $preg_matches);
  }

  if ($match) {
    $nid = $preg_matches[1];
  }
  if (is_numeric($nid)) {
    $node = db_query('SELECT nid FROM {node} WHERE nid = :nid', array(':nid' => $nid))->fetchObject();
  }
  else {
    $node = db_query('SELECT nid FROM {node} WHERE LOWER(title) = LOWER(:title)', array(':title' => $nid))->fetchObject();
  }

  // Do not allow unpublished nodes to be selected by unprivileged users
  if (!$node || (empty($node->status) && !(user_access('administer nodes')))) {
    form_error($form['node'], t('Invalid node selected.'));
  }
  else {
    form_set_value($form['nid'], $node->nid, $form_state);
    // $form_state['values']['nid'] = $node->nid;
  }
}

function ctools_context_node_settings_form_submit($form, &$form_state) {
  if ($form_state['values']['set_identifier']) {
    $node = node_load($form_state['values']['nid']);
    $form_state['values']['identifier'] = $node->title;
  }

  foreach (array_keys($form_state['plugin']['defaults']) as $key) {
    $form_state['conf'][$key] = $form_state['values'][$key];
  }
}

/**
 * Provide a list of ways that this context can be converted to a string.
 */
function ctools_context_node_convert_list() {
  $list = array(
    'nid' => t('Node ID'),
    'vid' => t('Node revision ID'),
    'title' => t('Node title'),
    'uid' => t('Author UID'),
    'type' => t('Node type'),
  );

  if (module_exists('token')) {
    $list += reset(token_get_list(array('node')));
  }

  return $list;
}

/**
 * Convert a context into a string.
 */
function ctools_context_node_convert($context, $type) {
  switch ($type) {
    case 'nid':
      return $context->data->nid;
    case 'vid':
      return $context->data->vid;
    case 'title':
      return $context->data->title;
    case 'uid':
      return $context->data->uid;
    case 'type':
      return $context->data->type;
  }
  if (module_exists('token')) {
    $values = token_get_values('node', $context->data);
    if ($key = array_search($type, $values->tokens)) {
      return $values->values[$key];
    }
  }
}

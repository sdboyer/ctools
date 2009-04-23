<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide a node_add_form context
 */

/**
 * Implementation of specially named hook_ctools_contexts().
 */
function ctools_node_add_form_ctools_contexts() {
  $args['node_add_form'] = array(
    'title' => t("Node add form"),
    'description' => t('A node add form.'),
    'context' => 'ctools_context_create_node_add_form',
    'settings form' => 'ctools_context_node_add_form_settings_form',
    'keyword' => 'node_add',
    'context name' => 'node_add_form',
    'convert list' => array('type' => t('Node type')),
    'convert' => 'ctools_context_node_add_form_convert',
    'placeholder form' => array(
      '#type' => 'textfield',
      '#description' => t('Enter the node type this context.'),
    ),
  );
  return $args;
}

/**
 * It's important to remember that $conf is optional here, because contexts
 * are not always created from the UI.
 */
function ctools_context_create_node_add_form($empty, $data = NULL, $conf = FALSE) {
  $context = new ctools_context(array('form', 'node_add', 'node_form'));
  $context->plugin = 'node_add_form';

  if ($empty) {
    return $context;
  }

  if ($conf) {
    $data = $data['type'];
  }

  if (!empty($data)) {
    $types = node_get_types();
    $type = str_replace('-', '_', $data);

    // Validate the node type exists.
    if (isset($types[$type]) && node_access('create', $type)) {
      // Initialize settings:
      global $user;
      $node = array('uid' => $user->uid, 'name' => $user->name, 'type' => $type);

      ctools_include('form');
      $form_id = $node->type . '_node_form';

      $form_state = array('want form' => TRUE, 'args' => array($node));
      $form = ctools_build_form($form_id, $form_state);
      // In a form, $data is the object being edited.
      $context->data     = $type;
      $context->title    = $types[$type]->name;
      $context->argument = $type;

      // These are specific pieces of data to this form.
      // All forms should place the form here.
      $context->form       = $form;
      $context->form_id    = $type . '_node_form';
      $context->form_title = t('Submit @name', array('@name' => $types[$type]->name));
      $context->node_type  = $type;
      $context->restrictions['type'] = array($type);
      return $context;
    }
  }
}

function ctools_context_node_add_form_settings_form($conf, $external = FALSE) {
  if ($external) {
    $options[0] = t('External source');
  }

  foreach (node_get_types() as $type => $info) {
    $options[$type] = $info->name;
  }

  $form['types'] = array(
    '#title' => t('Node type'),
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => $conf['types'],
    '#description' => t('Select the node type for this form.'),
  );

  if ($external) {
    $form['types']['#description'] .= ' ' . t('Select external to require this from an external source (such as a containing panel page).');
  }

  return $form;
}

/**
 * Convert a context into a string.
 */
function ctools_context_node_add_form_convert($context, $type) {
  switch ($type) {
    case 'type':
      return $context->data;
  }
}

<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide a node_edit_form context
 */

/**
 * Implementation of specially named hook_ctools_contexts().
 */
function ctools_node_edit_form_ctools_contexts() {
  $args['node_edit_form'] = array(
    'title' => t("Node edit form"),
    'description' => t('A node edit form.'),
    'context' => 'ctools_context_create_node_edit_form',
    'settings form' => 'ctools_context_node_edit_form_settings_form',
    'settings form validate' => 'ctools_context_node_edit_form_settings_form_validate',
    'keyword' => 'node_edit',
    'context name' => 'node_edit_form',
  );
  return $args;
}

/**
 * It's important to remember that $conf is optional here, because contexts
 * are not always created from the UI.
 */
function ctools_context_create_node_edit_form($empty, $node = NULL, $conf = FALSE) {
  $context = new ctools_context(array('form', 'node_edit', 'node_form', 'node'));
  $context->plugin = 'node_edit_form';

  if ($empty) {
    return $context;
  }

  if ($conf) {
    // In this case, $node is actually our $conf array.
    $node = node_load($node['nid']);
  }

  if (!empty($node)) {
    module_load_include('inc', 'node', 'node.pages');
    ctools_include('form');
    $form_id = $node->type . '_node_form';

    $form_state = array('want form' => TRUE, 'args' => array($node));
    $form = ctools_build_form($form_id, $form_state);

    // Fill in the 'node' portion of the context
    $context->data     = $node;
    $context->title    = $node->title;
    $context->argument = isset($node->nid) ? $node->nid : $node->type;

    $context->form       = $form;
    $context->form_state = &$form_state;
    $context->form_id    = $form_id;
    $context->form_title = $node->title;
    $context->node_type  = $node->type;
    return $context;
  }
}

function ctools_context_node_edit_form_settings_form($conf, $external = FALSE) {
  if ($external) {
    $form['external'] = array(
      '#type' => 'checkbox',
      '#default_value' => $conf['external'],
      '#title' => t('Require this context from an external source (such as a containing panel page).'),
      '#description' => t('If selected, node selection (below) will be ignored.'),
    );
  }

  $form['node'] = array(
    '#prefix' => '<div class="no-float">',
    '#suffix' => '</div>',
    '#title' => t('Enter the title or NID of a post'),
    '#type' => 'textfield',
    '#maxlength' => 512,
    '#autocomplete_path' => 'ctools/autocomplete/node',
    '#weight' => -10,
  );

  if (!empty($conf['nid'])) {
    $info = db_fetch_object(db_query("SELECT * FROM {node} WHERE nid = %d", $conf['nid']));
    if ($info) {
      $link = l(t("'%title' [node id %nid]", array('%title' => $info->title, '%nid' => $info->nid)), "node/$info->nid", array('target' => '_blank', 'title' => t('Open in new window')));
      $form['node']['#description'] = t('Currently set to !link', array('!link' => $link));
    }
  }

  $form['nid'] = array(
    '#type' => 'value',
    '#value' => $conf['nid'],
  );

  $form['external'] = array(
    '#type' => 'value',
    '#value' => $external,
  );

  return $form;
}

/**
 * Validate a node.
 */
function ctools_context_node_edit_form_settings_form_validate($form, &$form_values, &$form_state) {
  if (empty($form_values['external']) && empty($form_values['nid']) && empty($form_values['node'])) {
    form_error($form['node'], t('You must select a node.'));
    return;
  }

  if (empty($form_values['node'])) {
    return;
  }

  $nid = $form_values['node'];
  if (is_numeric($nid)) {
    $node = db_fetch_object(db_query(db_rewrite_sql("SELECT n.nid FROM {node} n WHERE n.nid = %d"), $nid));
  }
  else {
    $node = db_fetch_object(db_query(db_rewrite_sql("SELECT n.nid FROM {node} n WHERE LOWER(n.title) = LOWER('%s')"), $nid));
    if ($node) {
      form_set_value($form['nid'], $node->nid, $form_state);
    }
  }

  if (!$node) {
    form_error($form['node'], t('Invalid node selected.'));
  }
}


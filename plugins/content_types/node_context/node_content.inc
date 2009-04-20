<?php
// $Id$

/**
 * Callback function to supply a list of content types.
 */
function ctools_node_content_ctools_content_types() {
  return array(
    'single' => TRUE,
    'title' => t('Node content'),
    'icon' => 'icon_node.png',
    'description' => t('The content of the referenced node.'),
    'required context' => new ctools_context_required(t('Node'), 'node'),
    'category' => t('Node'),
    'defaults' => array(
      'links' => TRUE,
      'page' => TRUE,
      'no_extras' => TRUE,
      'override_title' => FALSE,
      'override_title_text' => '',
      'teaser' => TRUE,
      'identifier' => '',
      'link' => TRUE,
      'leave_node_title' => FALSE,
    ),
  );
}

/**
 * Render the node content.
 */
function ctools_node_content_content_type_render($subtype, $conf, $panel_args, $context) {
  if (!empty($context) && empty($context->data)) {
    return;
  }

  $node = isset($context->data) ? drupal_clone($context->data) : NULL;
  $block = new stdClass();
  $block->module = 'node';
  $block->delta  = $node->nid;

  if (empty($node)) {
    $block->delta   = 'placeholder';
    $block->title = t('Node title.');
    $block->content = t('Node content goes here.');
  }
  else {
    if (!empty($conf['identifier'])) {
      $node->panel_identifier = $conf['identifier'];
    }

    $block->title = $node->title;
    if (empty($conf['leave_node_title'])) {
      $node->title = NULL;
    }
    $block->content = ctools_node_content_render_node($node, $conf);
  }

  if (node_access('update', $node)) {
    $block->admin_links['update'] = array(
      'title' => t('Edit node'),
      'alt' => t("Edit this node"),
      'href' => "node/$node->nid/edit",
      'query' => drupal_get_destination(),
    );
  }

  if (!empty($conf['link']) && $node) {
    $block->title_link = "node/$node->nid";
  }

  return $block;
}

function ctools_node_content_render_node($node, $conf) {
  // The build mode identifies the target for which the node is built.
  if (!isset($node->build_mode)) {
    $node->build_mode = NODE_BUILD_NORMAL;
  }

  // Remove the delimiter (if any) that separates the teaser from the body.
  $node->body = str_replace('<!--break-->', '', $node->body);

  // The 'view' hook can be implemented to overwrite the default function
  // to display nodes.
  if (node_hook($node, 'view')) {
    $node = node_invoke($node, 'view', $conf['teaser'], $conf['page']);
  }
  else {
    $node = node_prepare($node, $conf['teaser']);
  }

  if (empty($conf['no_extras'])) {
    // Allow modules to make their own additions to the node.
    node_invoke_nodeapi($node, 'view', $conf['teaser'], $conf['page']);
  }

  if ($conf['links']) {
    $node->links = module_invoke_all('link', 'node', $node, $conf['teaser']);
    drupal_alter('link', $node->links, $node);
  }

  // Set the proper node part, then unset unused $node part so that a bad
  // theme can not open a security hole.
  $content = drupal_render($node->content);
  if ($conf['teaser']) {
    $node->teaser = $content;
    unset($node->body);
  }
  else {
    $node->body = $content;
    unset($node->teaser);
  }

  // Allow modules to modify the fully-built node.
  node_invoke_nodeapi($node, 'alter', $conf['teaser'], $conf['page']);

  return theme('node', $node, $conf['teaser'], $conf['page']);
}

/**
 * Returns an edit form for the custom type.
 */
function ctools_node_content_content_type_edit_form(&$form, &$form_state) {
  $conf = $form_state['conf'];

  $form['link'] = array(
    '#title' => t('Link title to node'),
    '#type' => 'checkbox',
    '#default_value' => $conf['link'],
    '#description' => t('Check here to make the title link to the node.'),
  );
  $form['teaser'] = array(
    '#title' => t('Teaser'),
    '#type' => 'checkbox',
    '#default_value' => $conf['teaser'],
    '#description' => t('Check here to show only the node teaser.'),
  );
  $form['page'] = array(
    '#type' => 'checkbox',
    '#default_value' => $conf['page'],
    '#title' => t('Node page'),
    '#description' => t('Check here if the node is being displayed on a page by itself.'),
  );
  $form['links'] = array(
    '#type' => 'checkbox',
    '#default_value' => $conf['links'],
    '#title' => t('Display links'),
    '#description' => t('Check here to display the links with the post.'),
  );

  $form['no_extras'] = array(
    '#type' => 'checkbox',
    '#default_value' => $conf['no_extras'],
    '#title' => t('No extras'),
    '#description' => t('Check here to disable additions that modules might make to the node, such as file attachments and CCK fields; this should just display the basic teaser or body.'),
  );

  $form['identifier'] = array(
    '#type' => 'textfield',
    '#default_value' => $conf['identifier'],
    '#title' => t('Identifier'),
    '#description' => t('Whatever is placed here will appear in $node->panel_identifier in the node template to make it easier to theme a node or part of a node as necessary. This identifier will automatically be added as a node template suggestion: node-panel-IDENTIFIER.tpl.php'),
  );

  $form['leave_node_title'] = array(
    '#type' => 'checkbox',
    '#default_value' => !empty($conf['leave_node_title']),
    '#title' => t('Leave node title'),
    '#description' => t('Advanced: if checked, do not touch the node title; this can cause the node title to appear twice unless your theme is aware of this.'),
  );

  return $form;
}

function ctools_node_content_content_type_edit_form_submit(&$form, &$form_state) {
  // Copy everything from our defaults.
  foreach (array_keys($form_state['plugin']['defaults']) as $key) {
    $form_state['conf'][$key] = $form_state['values'][$key];
  }
}

function ctools_node_content_content_type_admin_title($subtype, $conf, $context) {
  return t('"@s" content', array('@s' => $context->identifier));
}


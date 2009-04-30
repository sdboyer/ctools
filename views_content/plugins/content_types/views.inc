<?php
// $Id$

/**
 * @file
 * Content type plugin to expose all views as content.
 */

/**
 * Implementation of hook_ctools_content_types()
 */
function views_content_views_ctools_content_types() {
  if (!variable_get('ctools_content_all_views', TRUE)) {
    return;
  }

  return array(
    'title' => t('All views'),
    'defaults' => array(
      'nodes_per_page' => 10,
      'pager_id' => 1,
      'use_pager' => FALSE,
      'offset' => 0,
      'more_link' => FALSE,
      'feed_icons' => FALSE,
      'panel_args' => FALSE,
      'link_to_view' => FALSE,
      'args' => '',
      'url' => '',
    ),
    'add form' => array(
      'views_content_views_select_display' => t('Select display'),
      'views_content_views_content_type_edit_form' => array(
        'default' => TRUE, // put wrapper here, not on the previous form.
        'title' => t('Configure view'),
      ),
    ),
    'all contexts' => TRUE,
  );
}

/**
 * Return all content types available.
 */
function views_content_views_content_type_content_types($plugin) {
  $types = array();
  // It can be fairly intensive to calculate this, so let's cache this in the
  // cache_views table. The nice thing there is that if views ever change, that
  // table will always be cleared. Except for the occasional default view, so
  // we must use the Views caching functions in order to respect Views caching
  // settings.
  views_include('cache');
  $data = views_cache_get('views_content_all', TRUE);
  if (!empty($data->data)) {
    $types = $data->data;
  }

  if (empty($types)) {
    $views = views_get_all_views();

    foreach ($views as $view) {
      $types[$view->name] = _views_content_views_content_type($view);
    }

    views_cache_set('views_content_all', $types, TRUE);
  }

  return $types;
}

/**
 * Return a single content type.
 */
function views_content_views_content_type_content_type($subtype, $plugin) {
  $view = views_get_view($name);
  if (empty($view)) {
    return;
  }

  return _views_content_views_content_type($view);
}

/**
 * Create the content type info array to give back to ctools for a given display.
 */
function _views_content_views_content_type($view) {
  $title = $view->name;

  $icon = 'icon_views_page_legacy.png';

  return array(
    'view' => $view->name,
    'title' => $title,
    'icon' => $icon,
    'description' => filter_xss_admin($view->description),
//    'required context' => $contexts,
    'category' => t('Views'),
  );

}

/**
 * Output function for the 'views' content type.
 *
 * Outputs a view based on the module and delta supplied in the configuration.
 */
function views_content_views_content_type_render($subtype, $conf, $panel_args, $contexts) {
  _views_content_views_update_conf($conf, views_content_views_ctools_content_types());

  if (!is_array($contexts)) {
    $contexts = array($contexts);
  }

  if (strpos($subtype, '-')) {
    list($name, $display) = explode('-', $subtype);
    $view = views_get_view($name);
  }
  else {
    $view = views_get_view($subtype);
    $display = isset($conf['display']) ? $conf['display'] : 'default';
  }

  if (empty($view)) {
    return;
  }

  $view->set_display($display);
  if (!$view->display_handler->access($GLOBALS['user'])) {
    return;
  }

  $arguments = explode('/', $_GET['q']);
  $args = $conf['args'];

  foreach ($arguments as $id => $arg) {
    $args = str_replace("%$id", $arg, $args);
  }

  foreach ($panel_args as $id => $arg) {
    $args = str_replace("@$id", $arg, $args);
  }

  $args = preg_replace(',/?(%\d|@\d),', '', $args);
  $args = $args ? explode('/', $args) : array();

  if ($conf['panel_args'] && is_array($panel_args)) {
    $args = array_merge($panel_args, $args);
  }

  if (isset($conf['context']) && is_array($conf['context'])) {
    foreach ($conf['context'] as $count => $context_info) {
      if (!strpos($context_info, '.')) {
        // old skool: support pre-converter contexts as well.
        $cid = $context_info;
        $converter = '';
      }
      else {
        list($cid, $converter) = explode('.', $context_info, 2);
      }
      if (!empty($contexts[$cid])) {
        $arg = ctools_context_convert_context($contexts[$cid], $converter);
        array_splice($args, $count, 0, array($arg));
      }
    }
  }

  $view->set_arguments($args);

  if ($conf['url']) {
    $display_id = $view->display_handler->get_link_display();
    if ($display_id) {
      $view->display[$display_id]->handler->set_option('path', $conf['url']);
    }
    else {
      $view->display_handler->set_option('path', $conf['url']);
    }
  }

  $block = new stdClass();
  $block->module = 'views';
  $block->delta  = $view->name .'-'.  $display;

  if (!empty($conf['link_to_view'])) {
    $block->title_link = $view->get_url();
  }

  if (!empty($conf['more_link'])) {
    $block->more = array('href' => $view->get_url());
    $view->display_handler->set_option('use_more', FALSE);
  }
  $view->display_handler->set_option('use_pager', $conf['use_pager']);
  $view->display_handler->set_option('pager_element', $conf['pager_id']);
  $view->display_handler->set_option('items_per_page', $conf['nodes_per_page']);
  $view->display_handler->set_option('offset', $conf['offset']);

  $stored_feeds = drupal_add_feed();
  $block->content = $view->preview();
  $block->title = $view->get_title();

  if (empty($view->result) && !$view->display_handler->get_option('empty') && empty($view->style_plugin->definition['even empty'])) {
    return;
  }

  if (!empty($conf['feed_icons'])) {
    $new_feeds = drupal_add_feed();
    if ($diff = array_diff(array_keys($new_feeds), array_keys($stored_feeds))) {
      foreach ($diff as $url) {
        $block->feeds[$url] = $new_feeds[$url];
      }
    }
  }

  $view->destroy();
  return $block;
}

/**
 * Returns an edit form for a block.
 */
function views_content_views_select_display(&$form, &$form_state) {
  $view = views_get_view($form_state['subtype_name']);
  if (empty($view)) {
    return;
  }

  $displays = array();
  foreach ($view->display as $id => $display) {
    $displays[$id] = $display->display_title;
  }

  $form['display'] = array(
    '#type' => 'select',
    '#title' => t('Display'),
    '#options' => $displays,
    '#description' => t('Choose which display of this view you wish to use.')
  );
}

/**
 * Submit the basic view edit form.
 *
 * This just dumps everything into the $conf array.
 */
function views_content_views_select_display_submit(&$form, &$form_state) {
  $form_state['conf']['display'] = $form_state['values']['display'];
}

/**
 * Returns an edit form for a block.
 */
function views_content_views_content_type_edit_form(&$form, &$form_state) {
  $conf = $form_state['conf'];
  _views_content_views_update_conf($conf, views_content_views_ctools_content_types());
  // This allows older content to continue to work, where we used to embed
  // the display directly.
  if (strpos($form_state['subtype_name'], '-')) {
    list($name, $conf['display']) = explode('-', $form_state['subtype_name']);
    $view = views_get_view($name);
  }
  else {
    $view = views_get_view($form_state['subtype_name']);
  }

  if (empty($view)) {
    $form['markup'] = array('#value' => t('Broken/missing/deleted view.'));
    return;
  }

  $view->set_display($conf['display']);

  $form_state['title'] = t('Configure view @view (@display)', array('@view' => $view->name, '@display' => $view->display[$conf['display']]->display_title));

  // @todo
  // If using the older format, just a context is listed. We should go through
  // and check for that and forcibly set them to the right converter so that
  // it doesn't get changed to some whacky default. Oooor just let it get changed
  // to 'no context', I suppose.

  $required = array();
  if (isset($view->display_handler) && $arguments = $view->display_handler->get_handlers('argument')) {
    foreach ($arguments as $arg) {
      $required[] = new ctools_context_optional($arg->ui_name(), 'any');
    }
  }

  if ($required) {
    $form['context'] = ctools_context_converter_selector($form_state['contexts'], $required, isset($conf['context']) ? $conf['context'] : array());
  }

  $form['link_to_view'] = array(
    '#type' => 'checkbox',
    '#default_value' => $conf['link_to_view'],
    '#title' => t('Link title to view'),
    '#description' => t('If checked, the title will be a link to the view.'),
  );

  $form['more_link'] = array(
    '#type' => 'checkbox',
    '#default_value' => $conf['more_link'],
    '#title' => t('Provide a "more" link that links to the view'),
    '#description' => t('This is independent of any more link that may be provided by the view itself; if you see two more links, turn this one off. Views will only provide a more link if using the "block" type, however, so if using embed, use this one.'),
  );

  $form['feed_icons'] = array(
    '#type' => 'checkbox',
    '#default_value' => $conf['feed_icons'],
    '#title' => t('Display feed icons'),
    '#description' => t('If checked, any feed icons provided by this view will be displayed.'),
  );

  $form['pager_aligner_start'] = array(
    '#value' => '<div class="option-text-aligner">',
  );
  $form['use_pager'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use pager'),
    '#default_value' => $conf['use_pager'],
    '#id' => 'use-pager-checkbox',
  );
  $form['pager_id'] = array(
    '#type' => 'textfield',
    '#default_value' => $conf['pager_id'],
    '#title' => t('Pager ID'),
    '#size' => 4,
    '#id' => 'use-pager-textfield',
    '#process' => array('ctools_dependent_process'),
    '#dependency' => array('use-pager-checkbox' => array(1)),
  );
  $form['pager_aligner_stop'] = array(
    '#value' => '</div><div style="clear: both; padding: 0; margin: 0"></div>',
  );

  $form['nodes_per_page'] = array(
    '#type' => 'textfield',
    '#default_value' => $conf['nodes_per_page'],
    '#title' => t('Num posts'),
    '#size' => 4,
    '#description' => t('Select the number of posts to display, or 0 to display all results.'),
  );

  $form['offset'] = array(
    '#type' => 'textfield',
    '#default_value' => $conf['offset'],
    '#title' => t('Offset'),
    '#size' => 4,
    '#description' => t('Offset in the node list or 0 to start at 1st item.'),
  );

  $form['panel_args'] = array(
    '#type' => 'checkbox',
    '#title' => t('Send arguments'),
    '#default_value' => $conf['panel_args'],
    '#description' => t('Select this to send all arguments from the panel directly to the view. If checked, the panel arguments will come after any context arguments above and precede any additional arguments passed in through the Arguments field below.'),
  );

  $form['args'] = array(
    '#type' => 'textfield',
    '#default_value' => $conf['args'],
    '#title' => t('Arguments'),
    '#size' => 30,
    '#description' => t('Additional arguments to send to the view as if they were part of the URL in the form of arg1/arg2/arg3. You may use %0, %1, ..., %N to grab arguments from the URL. Or use @0, @1, @2, ..., @N to use arguments passed into the panel.'),
  );

  $form['url'] = array(
    '#type' => 'textfield',
    '#default_value' => $conf['url'],
    '#title' => t('Override URL'),
    '#size' => 30,
    '#description' => t('If this is set, override the View URL; this can sometimes be useful to set to the panel URL'),
  );

  $view->destroy();
  return $form;
}

/**
 * Store form values in $conf.
 */
function views_content_views_content_type_edit_form_submit(&$form, &$form_state) {
  // Copy everything from our defaults.
  foreach (array_keys($form_state['plugin']['defaults']) as $key) {
    $form_state['conf'][$key] = $form_state['values'][$key];
  }
}

/**
 * Returns the administrative title for a type.
 */
function views_content_views_content_type_admin_title($subtype, $conf) {
  _views_content_views_update_conf($conf, views_content_views_ctools_content_types());
  if (strpos($subtype, '-')) {
    list($name, $display) = explode('-', $subtype);
    $view = views_get_view($name);
  }
  else {
    $name = $subtype;
    $view = views_get_view($subtype);
  }

  if (empty($view)) {
    return t('Deleted/missing view @view', array('@view' => $name));
  }
  return t('View: @name', array('@name' => $view->name));
}

/**
 * Returns the administrative title for a type.
 */
function views_content_views_content_type_admin_info($subtype, $conf, $contexts) {
  _views_content_views_update_conf($conf, views_content_views_ctools_content_types());
  if (strpos($subtype, '-')) {
    list($name, $conf['display']) = explode('-', $subtype);
    $view = views_get_view($name);
  }
  else {
    $view = views_get_view($subtype);
    $name = $subtype;
  }

  if (empty($view)) {
    return t('Deleted/missing view @view', array('@view' => $name));
  }

  $view->set_display($conf['display']);
  $block->title = t('View information');

  $block->content = '<ul>';
  $block->content .= '<li>' . t('Using display @display.', array('@display' => $view->display[$conf['display']]->display_title)) . '</li>';

  if (!empty($conf['context']) && $arguments = $view->display_handler->get_handlers('argument')) {
    $argument = reset($arguments);
    foreach ($conf['context'] as $count => $context_info) {
      if (!$argument) {
        break;
      }

      if (!strpos($context_info, '.')) {
        // old skool: support pre-converter contexts as well.
        $cid = $context_info;
        $converter = '';
      }
      else {
        list($cid, $converter) = explode('.', $context_info, 2);
      }

      if (!empty($contexts[$cid])) {
        $converters = ctools_context_get_converters($cid . '.', $contexts[$cid]);
        $converter = !empty($converters[$context_info]) ? $converters[$context_info] : t('Default');
        $block->content .= '<li>' . t('Argument @arg using context @context converted into @converter', array(
          '@arg' => $argument->ui_name(), '@context' => $contexts[$cid]->get_identifier(),
          '@converter' => $converter)) . '</li>';
      }
      $argument = next($arguments);
    }
  }

  $block->content .= '<li>' . t('@count items displayed.', array('@count' => $conf['nodes_per_page'])) . '</li>';
  if ($conf['use_pager']) {
    $block->content .= '<li>' . t('With pager.') . '</li>';
  }
  else {
    $block->content .= '<li>' . t('Without pager.') . '</li>';
  }

  if ($conf['offset']) {
    $block->content .= '<li>' . t('Skipping first @count results', array('@count' => $conf['offset'])) . '</li>';
  }
  if ($conf['more_link']) {
    $block->content .= '<li>' . t('With more link.') . '</li>';
  }
  if ($conf['feed_icons']) {
    $block->content .= '<li>' . t('With feed icon.') . '</li>';
  }
  if ($conf['panel_args']) {
    $block->content .= '<li>' . t('Sending arguments.') . '</li>';
  }
  if ($conf['args']) {
    $block->content .= '<li>' . t('Using arguments: @args', array('@args' => $conf['args'])) . '</li>';
  }
  if ($conf['url']) {
    $block->content .= '<li>' . t('Using url: @url', array('@url' => $conf['url'])) . '</li>';
  }

  $view->destroy();
  return $block;
}

/**
 * Update the $conf to deal with updates from Drupal 5.
 */
function _views_content_views_update_conf(&$conf, $task) {
  if (isset($conf['display'])) {
    return;
  }
  $conf['display'] = 'default';
  foreach ($task['defaults'] as $key => $value) {
    if (!isset($conf[$key])) {
      $conf[$key] = $value;
    }
  }
}
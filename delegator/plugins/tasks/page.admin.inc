<?php
// $Id$
/**
 * @file
 * Administrative functions for the page subtasks.
 *
 * These are attached to the menu system in page.inc via the hook_menu
 * delegation. They are included here so that this code is loaded
 * only when needed.
 */

/**
 * Get the cached changes to a given task handler.
 */
function delegator_page_get_page_cache($name) {
  ctools_include('object-cache');
  $cache = ctools_object_cache_get('delegator_page', $name);
  if (!$cache) {
    // This ensures the task .inc file is loaded
    $task = delegator_get_task('page');
    $cache = delegator_page_load($name);
  }

  return $cache;
}

/**
 * Store changes to a task handler in the object cache.
 */
function delegator_page_set_page_cache($page) {
  $name = isset($page->new) || !isset($page->name) ? '::new' : $page->name;
  ctools_include('object-cache');
  $cache = ctools_object_cache_set('delegator_page', $name, $page);
}

/**
 * Remove an item from the object cache.
 */
function delegator_page_clear_page_cache($name) {
  ctools_include('object-cache');
  ctools_object_cache_clear('delegator_page', $name);
}

/**
 * Get a list of named arguments in a delegator page path.
 *
 * @param $path
 *   A normal Drupal path.
 *
 * @return
 *   An array of % marked variable arguments, keyed by the argument's name.
 *   The value will be the position of the argument so that it can easily
 *   be found. Items with a position of -1 have multiple positions.
 */
function delegator_page_get_arguments($path) {
  $arguments = array();
  $bits = explode('/', $path);
  foreach ($bits as $position => $bit) {
    if ($bit && $bit[0] == '%') {
      // special handling for duplicate path items and substr to remove the %
      $arguments[substr($bit, 1)] = isset($arguments[$bit]) ? -1 : $position;
    }
  }

  return $arguments;
}

/**
 * Page callback to add a subtask.
 */
function delegator_page_add_subtask($step = NULL) {
  // We load the task to make sure our .inc file is loaded.
  $task = delegator_get_task('page');

  $form_info = delegator_page_edit_form_info();
  $form_info += array(
    'path' => 'admin/build/pages/add/%step',
  );

  // If step is unset, we're creating a new one. Wipe out our values and start
  // over.
  if (!isset($step) || !$page = delegator_page_get_page_cache('::new')) {
    $step = 'basic';
    $page = delegator_page_new();
    $page->new = TRUE;
    delegator_page_set_page_cache($page);
  }

  ctools_include('wizard');
  $form_state = array(
    'cache name' => '::new',
    'page' => $page,
    'type' => 'add',
  );
  $output = ctools_wizard_multistep_form($form_info, $step, $form_state);

  if (!$output) {
    // redirect.
    drupal_redirect_form(array(), $form_state['redirect']);
  }

  return $output;
}

/**
 * Page callback to add a subtask.
 */
function delegator_page_edit_subtask($page_name, $step = NULL) {
  if (!$page = delegator_page_get_page_cache($page_name)) {
    return drupal_not_found();
  }

  // We load the task to make sure our .inc file is loaded.
  $task = delegator_get_task('page');

  $form_info = array(
    'path' => "admin/build/pages/edit/$page_name/%step",
    'show trail' => FALSE,
    'show return' => TRUE,
    'return path' => 'admin/build/pages',
  ) + delegator_page_edit_form_info();

  // If step is unset, go with the basic step.
  if (!isset($step)) {
    $step = 'basic';
  }

  ctools_include('wizard');
  $form_state = array(
    'cache name' => $page_name,
    'page' => $page,
    'type' => 'edit',
  );
  $output = ctools_wizard_multistep_form($form_info, $step, $form_state);

  if (!$output) {
    // redirect.
    drupal_redirect_form(array(), $form_state['redirect']);
  }

  return $output;
}

/**
 * Callback generated when the add page process is finished.
 */
function delegator_page_add_subtask_finish(&$form_state) {
  $page = &$form_state['page'];
  // Ensure $page->arguments contains only real arguments:
  $arguments = delegator_page_get_arguments($page->path);
  $args = array();
  foreach ($arguments as $keyword => $position) {
    if (isset($page->arguments[$keyword])) {
      $args[$keyword] = $page->arguments[$keyword];
    }
    else {
      $args[$keyword] = array(
        'id' => '',
        'identifier' => '',
        'argument' => '',
        'settings' => array()
      );
    }
  }
  $page->arguments = $args;
  // Create a real object from the cache
  delegator_page_save($page);

  // Clear the cache
  delegator_page_clear_page_cache($form_state['cache name']);

  // Force a menu rebuild to recognize our new subtask
  menu_rebuild();

  if ($form_state['type'] == 'add') {
    // Redirect to the new page's task handler editor.
    $form_state['redirect'] = 'admin/build/delegator/page-' . $page->name;
  }
  else {
    $form_state['redirect'] = 'admin/build/pages';
  }
}

/**
 * Callback generated when the 'next' button is clicked.
 *
 * All we do here is store the cache.
 */
function delegator_page_add_subtask_next(&$form_state) {
  // Update the cache with changes.
  delegator_page_set_page_cache($form_state['page']);
}

/**
 * Callback generated when the 'cancel' button is clicked.
 *
 * All we do here is clear the cache.
 */
function delegator_page_add_subtask_cancel(&$form_state) {
  // Update the cache with changes.
  delegator_page_clear_page_cache($form_state['cache name']);
}

/**
 * Basic settings form for a delegator page.
 */
function delegator_page_form_basic(&$form, &$form_state) {
  $page = &$form_state['page'];

  // @todo do not let the user change this name during the edit process.
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('The machine readable name of this page. It must be unique, and it must contain only alphanumeric characters and underscores.'),
    '#default_value' => $page->name,
  );

  $form['admin_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Administrative title'),
    '#description' => t('The name of this page. This will appear in the administrative interface to easily identify it.'),
    '#default_value' => $page->admin_title,
  );

  // path
  $form['path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path'),
    '#description' => t('The URL path to get to this page. You may create placeholders in the path by using %, or named placeholders by using %name. For example: "node/%node/foo", "forum/%forum" or "dashboard/%". Named placeholders can have more information attached to them on the arguments form.'),
    '#default_value' => $page->path,
  );
}

function delegator_page_form_basic_validate_filter($value) {
  return $value === -1;
}

/**
 * Validate the basic form.
 */
function delegator_page_form_basic_validate(&$form, &$form_state) {
  // Ensure name is properly formed.
  $args = delegator_page_get_arguments($form_state['values']['path']);
  if ($invalid_args = array_filter($args, 'delegator_page_form_basic_validate_filter')) {
    foreach ($invalid_args as $arg => $position) {
      form_error($form['path'], t('Duplicated argument %arg', array('%arg' => $arg)));
    }
  }

  if (isset($args['%'])) {
    form_error($form['path'], t('Invalid arg <em>%</em>. All arguments must be named with keywords.'));
  }

  if (empty($args)) {
    $form_state['clicked_button']['#next'] = 'access';
  }

  // Ensure name is unique.
  // @todo

  // Ensure path is unused.
  // @todo
}

/**
 * Store the values from the basic settings form.
 */
function delegator_page_form_basic_submit(&$form, &$form_state) {
  $form_state['page']->name = $form_state['values']['name'];
  $form_state['page']->admin_title = $form_state['values']['admin_title'];
  $form_state['page']->path = $form_state['values']['path'];
}

/**
 * Form to handle menu item controls.
 */
function delegator_page_form_menu(&$form, &$form_state) {
  ctools_include('dependent');
  $form['menu'] = array(
    '#prefix' => '<div class="clear-block">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
  );
  $menu = $form_state['page']->menu;
  if (empty($menu)) {
    $menu = array(
      'type' => 'none',
      'title' => '',
      'weight' => 0,
      'name' => 'navigation',
      'parent' => array(
        'type' => 'none',
        'title' => '',
        'weight' => 0,
        'name' => 'navigation',
      ),
    );
  }

  $form['menu']['type'] = array(
    '#title' => t('Type'),
    '#type' => 'radios',
    '#options' => array(
      'none' => t('No menu entry'),
      'normal' => t('Normal menu entry'),
      'tab' => t('Menu tab'),
      'default tab' => t('Default menu tab')
    ),
    '#default_value' => $menu['type'],
  );

  $form['menu']['title'] = array(
    '#title' => t('Title'),
    '#type' => 'textfield',
    '#default_value' => $menu['title'],
    '#description' => t('If set to normal or tab, enter the text to use for the menu item.'),
    '#process' => array('ctools_dependent_process'),
    '#dependency' => array('radio:menu[type]' => array('normal', 'tab', 'default tab')),
  );

  $form['menu']['name-warning'] = array(
    '#type' => 'markup',
    '#prefix' => '<div class="warning">',
    '#value' => t("Warning: Changing this item's menu will not work reliably in Drupal 6.4 or earlier. Please upgrade your copy of Drupal at !url.", array('!url' => l('drupal.org', 'http://drupal.org/project/Drupal+project'))),
    '#suffix' => '</div>',
    '#process' => array('ctools_dependent_process'),
    '#dependency' => array('radio:menu[type]' => array('normal')),
    '#access' => (VERSION < 6.5),
  );

  // Only display the menu selector if menu module is enabled.
  if (module_exists('menu')) {
    $form['menu']['name'] = array(
      '#title' => t('Menu'),
      '#type' => 'select',
      '#options' => menu_get_menus(),
      '#default_value' => $menu['name'],
      '#description' => t('Insert item into an available menu.'), //
      '#process' => array('ctools_dependent_process'),
      '#dependency' => array('radio:menu[type]' => array('normal')),
    );
  }
  else {
    $form['menu']['name'] = array(
      '#type' => 'value',
      '#value' => $menu['name'],
    );
    $form['menu']['markup'] = array(
      '#value' => t('Menu selection requires the activation of menu module.'),
    );
  }
  $form['menu']['weight'] = array(
    '#title' => t('Weight'),
    '#type' => 'textfield',
    '#default_value' => isset($menu['weight']) ? $menu['weight'] : 0,
    '#description' => t('The lower the weight the higher/further left it will appear.'),
    '#process' => array('ctools_dependent_process'),
    '#dependency' => array('radio:menu[type]' => array('normal', 'tab', 'default tab')),
  );

  $form['menu']['parent']['type'] = array(
    '#prefix' => '<div id="edit-menu-parent-type-wrapper">',
    '#suffix' => '</div>',
    '#title' => t('Parent menu item'),
    '#type' => 'radios',
    '#options' => array('none' => t('Already exists'), 'normal' => t('Normal menu item'), 'tab' => t('Menu tab')),
    '#default_value' => $menu['parent']['type'],
    '#description' => t('When providing a menu item as a default tab, Drupal needs to know what the parent menu item of that tab will be. Sometimes the parent will already exist, but other times you will need to have one created. The path of a parent item will always be the same path with the last part left off. i.e, if the path to this view is <em>foo/bar/baz</em>, the parent path would be <em>foo/bar</em>.'),
    '#process' => array('expand_radios', 'ctools_dependent_process'),
    '#dependency' => array('radio:menu[type]' => array('default tab')),
  );
  $form['menu']['parent']['title'] = array(
    '#title' => t('Parent item title'),
    '#type' => 'textfield',
    '#default_value' => $menu['parent']['title'],
    '#description' => t('If creating a parent menu item, enter the title of the item.'),
    '#process' => array('ctools_dependent_process'),
    '#dependency' => array('radio:menu[type]' => array('default tab'), 'radio:menu[parent][type]' => array('normal', 'tab')),
    '#dependency_count' => 2,
  );
  // Only display the menu selector if menu module is enabled.
  if (module_exists('menu')) {
    $form['menu']['parent']['name'] = array(
      '#title' => t('Parent item menu'),
      '#type' => 'select',
      '#options' => menu_get_menus(),
      '#default_value' => $menu['parent']['name'],
      '#description' => t('Insert item into an available menu.'),
      '#process' => array('ctools_dependent_process'),
      '#dependency' => array('radio:menu[type]' => array('default tab'), 'radio:menu[parent][type]' => array('normal')),
      '#dependency_count' => 2,
    );
  }
  else {
    $form['menu']['parent']['name'] = array(
      '#type' => 'value',
      '#value' => $menu['parent']['name'],
    );
  }
  $form['menu']['parent']['weight'] = array(
    '#title' => t('Tab weight'),
    '#type' => 'textfield',
    '#default_value' => $menu['parent']['weight'],
    '#size' => 5,
    '#description' => t('If the parent menu item is a tab, enter the weight of the tab. The lower the number, the more to the left it will be.'),
    '#process' => array('ctools_dependent_process'),
    '#dependency' => array('radio:menu[type]' => array('default tab'), 'radio:menu[parent][type]' => array('tab')),
    '#dependency_count' => 2,
  );
}

/**
 * Submit handler for the menu form for add/edit page task.
 */
function delegator_page_form_menu_submit(&$form, &$form_state) {
  $form_state['page']->menu = $form_state['values']['menu'];
}

/**
 * Form to handle menu item controls.
 */
function delegator_page_form_access(&$form, &$form_state) {
  ctools_include('context');
  $page = &$form_state['page'];
  if (!isset($page->access['type'])) {
    $page->access['type'] = '';
  }

  $contexts = array();
  // Load contexts based on argument data:
  if ($page->arguments) {
    $arguments = array();
    foreach ($page->arguments as $keyword => $argument) {
      if (isset($argument['name'])) {
        $argument['keyword'] = $keyword;
        $arguments[$argument['id']] = $argument;
      }
    }
    $contexts = ctools_context_get_placeholders_from_argument($arguments);
  }

  $plugins = ctools_get_access_plugins();
  $options = array();
  foreach ($plugins as $id => $plugin) {
    if (!empty($plugin['required context']) && !ctools_context_filter($contexts, $plugin['required context'])) {
      continue;
    }
    $options[$id] = $plugin['title'];
  }

  asort($options);
  $options = array('' => t('No access control')) + $options;

  $form['type'] = array(
    '#type' => 'radios',
    '#options' => $options,
    '#default_value' => $page->access['type'],
  );
}

/**
 * Submit handler to deal with access control changes.
 */
function delegator_page_form_access_submit(&$form, &$form_state) {
  // First, see if the new mechanism is different from the existing mechanism.
  $access = &$form_state['page']->access;
  $type = $form_state['values']['type'];

  ctools_include('context');
  $plugin = $type ? ctools_get_access_plugin($type) : array();
  if (!isset($access['type']) || $access['type'] != $type) {
    if (isset($plugin['default'])) {
      if (is_array($plugin['default'])) {
        $access['settings'] = $plugin['default'];
      }
      else if (function_exists($plugin['default'])) {
        $access['settings'] = $plugin['default']();
      }
      else {
        $access['settings'] = array();
      }
    }
  }

  $access['type'] = $type;

  // If there's no settings form, skip the settings form.
  if (!$plugin) {
    $form_state['clicked_button']['#next'] = 'menu';
  }
}

/**
 * Form to change access settings.
 */
function delegator_page_form_access_settings(&$form, &$form_state) {
  $access = &$form_state['page']->access;

  if (!isset($access['type'])) {
    // This should be impossible and thus never seen.
    $form['error'] = array('#value' => t('Error: missing argument.'));
    return;
  }

  ctools_include('context');
  $plugin = ctools_get_access_plugin($access['type']);

  $form['settings'] = array(
    '#tree' => TRUE,
  );

  if (!$plugin) {
    // This should be impossible and thus never seen.
    $form['error'] = array('#value' => t('Error: missing or invalid argument plugin %argument.', array('%argument', $argument)));
    return;
  }

  if ($function = ctools_plugin_get_function($plugin, 'settings form')) {
    $function($form, $form_state, $access['settings']);
  }

  $form_state['plugin'] = $plugin;
}

/**
 * Validate handler for argument settings.
 */
function delegator_page_form_access_settings_validate(&$form, &$form_state) {
  if ($function = ctools_plugin_get_function($form_state['plugin'], 'settings form validate')) {
    $function($form, $form_state);
  }
}

/**
 * Submit handler for argument settings.
 */
function delegator_page_form_access_settings_submit(&$form, &$form_state) {
  if ($function = ctools_plugin_get_function($form_state['plugin'], 'settings form submit')) {
    $function($form, $form_state);
  }

  $form_state['page']->access['settings'] = $form_state['values']['settings'];
}

/**
 * Form to handle assigning argument handlers to named arguments.
 */
function delegator_page_form_argument(&$form, &$form_state) {
  $path = $form_state['page']->path;
  $page = &$form_state['page'];

  $arguments = delegator_page_get_arguments($path);

  $form['table'] = array(
    '#theme' => 'delegator_page_form_argument_table',
    '#delegator-path' => $path,
  );

  $cache_name = $form_state['cache name'];
  foreach ($arguments as $keyword => $position) {
    $conf = array();

    if (isset($page->temporary_arguments[$keyword]) && !empty($form_state['allow temp'])) {
      $conf = $page->temporary_arguments[$keyword];
    }
    else if (isset($page->arguments[$keyword])) {
      $conf = $page->arguments[$keyword];
    }

    $context = t('No context assigned');

    $plugin = array();
    if ($conf && isset($conf['name'])) {
      ctools_include('context');
      $plugin = ctools_get_argument($conf['name']);

      if (isset($plugin['title'])) {
        $context = $plugin['title'];
      }
    }

    $form['table']['argument'][$keyword]['#keyword'] = $keyword;
    $form['table']['argument'][$keyword]['#position'] = $position;
    $form['table']['argument'][$keyword]['#context'] = $context;

    // The URL for this ajax button
    $form['table']['argument'][$keyword]['change-url'] = array(
      '#attributes' => array('class' => "delegator-context-$keyword-change-url"),
      '#type' => 'hidden',
      '#value' => url("admin/build/delegator/argument/change/$cache_name/$keyword", array('absolute' => TRUE)),
    );
    $form['table']['argument'][$keyword]['change'] = array(
      '#type' => 'submit',
      '#value' => t('Change'),
      '#attributes' => array('class' => 'ctools-use-modal'),
      '#id' => "delegator-context-$keyword-change",
    );

    $form['table']['argument'][$keyword]['settings'] = array();

    // Only show the button if this has a settings form available:
    if (!empty($plugin)) {
      // The URL for this ajax button
      $form['table']['argument'][$keyword]['settings-url'] = array(
        '#attributes' => array('class' => "delegator-context-$keyword-settings-url"),
        '#type' => 'hidden',
        '#value' => url("admin/build/delegator/argument/settings/$cache_name/$keyword", array('absolute' => TRUE)),
      );
      $form['table']['argument'][$keyword]['settings'] = array(
        '#type' => 'submit',
        '#value' => t('Settings'),
        '#attributes' => array('class' => 'ctools-use-modal'),
        '#id' => "delegator-context-$keyword-settings",
      );
    }
  }
}

/**
 * Theme the table for this form.
 */
function theme_delegator_page_form_argument_table($form) {
  $header = array(
    array('data' => t('Argument'), 'class' => 'delegator-argument'),
    array('data' => t('Position in path'), 'class' => 'delegator-position'),
    array('data' => t('Context assigned'), 'class' => 'delegator-context'),
    array('data' => t('Operations'), 'class' => 'delegator-operations'),
  );

  $rows = array();

  ctools_include('modal');
  ctools_modal_add_js();
  foreach (element_children($form['argument']) as $key) {
    $row = array();
    $row[] = '%' . check_plain($form['argument'][$key]['#keyword']);
    $row[] = check_plain($form['argument'][$key]['#position']);
    $row[] = $form['argument'][$key]['#context'] . ' &nbsp; ' . drupal_render($form['argument'][$key]['change']);;
    $row[] = drupal_render($form['argument'][$key]['settings']) . drupal_render($form['argument'][$key]);

    $rows[] = array('data' => $row);
  }

  if (!$rows) {
    $rows[] = array(array('data' => t('The path %path has no arguments to configure.', array('%path' => $form['#delegator-path'])), 'colspan' => 4));
  }

  $attributes = array(
    'id' => 'delegator-argument-table',
  );

  $output = theme('table', $header, $rows, $attributes);
  return $output;
}

/**
 * Ajax entry point to edit an item
 */
function delegator_page_subtask_argument_ajax($step = NULL, $cache_name = NULL, $keyword = NULL) {
  ctools_include('ajax');
  ctools_include('modal');
  ctools_include('context');
  ctools_include('wizard');

  if (!$step) {
    return ctools_ajax_render_error();
  }

  if (!$page = delegator_page_get_page_cache($cache_name)) {
    return ctools_ajax_render_error(t('Invalid object name.'));
  }

  $path = $page->path;
  $arguments = delegator_page_get_arguments($path);

  // Load stored object from cache.
  if (!isset($arguments[$keyword])) {
    return ctools_ajax_render_error(t('Invalid keyword.'));
  }

  // Set up wizard info
  $form_info = array(
    'id' => 'delegator_page_argument',
    'path' => "admin/build/delegator/argument/%step/$cache_name/$keyword",
    'show cancel' => TRUE,

    'next callback' => 'delegator_page_argument_next',
    'finish callback' => 'delegator_page_argument_finish',
    'cancel callback' => 'delegator_page_argument_cancel',

    'order' => array(
      'change' => t('Change context type'),
      'settings' => t('Argument settings'),
    ),
    'forms' => array(
      'change' => array(
        'title' => t('Change argument'),
        'form id' => 'delegator_page_argument_form_change'
      ),
      'settings' => array(
        'title' => t('Argument settings'),
        'form id' => 'delegator_page_argument_form_settings'
      ),
    ),
  );

  $form_state = array(
    'cache name' => $cache_name,
    'keyword' => $keyword,
    'page' => $page,
    'ajax' => TRUE,
    'modal' => TRUE,
    'commands' => array(),
  );

  // With 'modal' and 'ajax' true, rendering automatically happens here so
  // we do nothing with the result.
  ctools_wizard_multistep_form($form_info, $step, $form_state);
}

/**
 * Callback generated when the add page process is finished.
 */
function delegator_page_argument_finish(&$form_state) {
  // Check to see if there are changes.
  $page = &$form_state['page'];
  $keyword = &$form_state['keyword'];

  if (isset($page->temporary_arguments[$keyword])) {
    $page->arguments[$keyword] = $page->temporary_arguments[$keyword];
  }

  if (isset($page->temporary_arguments)) {
    unset($page->temporary_arguments);
  }

  // Update the cache with changes.
  delegator_page_set_page_cache($form_state['page']);

  // Rerender the table so we can ajax it back in.
  // Go directly to the form and retrieve it using a blank form and
  // a clone of our current form state. This is an abbreviated
  // drupal_get_form that is halted prior to render and is never
  // fully processed, but is guaranteed to produce the same form we
  // started with so we don't have to do crazy stuff to rerender
  // just part of it.

  // @todo should there be a tool to do this?

  $clone_state = $form_state;
  $clone_state['allow temp'] = TRUE;
  $form = array();
  delegator_page_form_argument($form, $clone_state);
  drupal_prepare_form('delegator_page_form_argument', $form, $clone_state);
  $form['#post'] = array();
  $form = form_builder('delegator_page_form_argument', $form, $clone_state);

  // Render just the table portion.
  $output = drupal_render($form['table']);
  $form_state['commands'][] = ctools_ajax_command_replace('#delegator-argument-table', $output);
}

/**
 * Callback generated when the 'next' button is clicked.
 *
 * All we do here is store the cache.
 */
function delegator_page_argument_next(&$form_state) {
  // Update the cache with changes.
  delegator_page_set_page_cache($form_state['page']);
}

/**
 * Callback generated when the 'cancel' button is clicked.
 *
 * We might have some temporary data lying around. We must remove it.
 */
function delegator_page_argument_cancel(&$form_state) {
  if (isset($form_state['page']->temporary_arguments)) {
    unset($form_state['page']->temporary_arguments);
    // Update the cache with changes.
    delegator_page_set_page_cache($form_state['page']);
  }
}

/**
 * Basic settings form for a delegator page.
 */
function delegator_page_argument_form_change(&$form, &$form_state) {
  $page = &$form_state['page'];
  $keyword = &$form_state['keyword'];

  ctools_include('context');
  $plugins = ctools_get_arguments();

  $options = array();
  foreach ($plugins as $id => $plugin) {
    $options[$id] = $plugin['title'];
  }

  asort($options);

  $options = array('' => t('No context selected')) + $options;

  $argument = '';
  if (isset($page->arguments[$keyword]) && isset($page->arguments[$keyword]['name'])) {
    $argument = $page->arguments[$keyword]['name'];
  }

  $form['argument'] = array(
    '#type' => 'radios',
    '#options' => $options,
    '#default_value' => $argument,
  );
}

/**
 * Submit handler to change an argument.
 */
function delegator_page_argument_form_change_submit(&$form, &$form_state) {
  $page = &$form_state['page'];
  $keyword = &$form_state['keyword'];
  $argument = $form_state['values']['argument'];

  // If the argument is not changing, we do not need to do anything.
  if (isset($page->arguments[$keyword]['name']) && $page->arguments[$keyword]['name'] == $argument) {
    // Set the task to cancel since no change means do nothing:
    $form_state['clicked_button']['#wizard type'] = 'cancel';
    return;
  }

  ctools_include('context');

  // If switching to the no context, just wipe out the old data.
  if (empty($argument)) {
    $form_state['clicked_button']['#wizard type'] = 'finish';
    $page->temporary_arguments[$keyword] = array(
      'settings' => array(),
      'identifier' => t('No context'),
    );
    return;
  }

  $plugin = ctools_get_argument($argument);

  // Acquire defaults.
  $settings = array();

  if (isset($plugin['default'])) {
    if (is_array($plugin['default'])) {
      $settings = $plugin['default'];
    }
    else if (function_exists($plugin['default'])) {
      $settings = $plugin['default']();
    }
  }

  $id = ctools_context_next_id($page->arguments, $argument);
  $title = isset($plugin['title']) ? $plugin['title'] : t('No context');

  // Set the new argument in a temporary location.
  $page->temporary_arguments[$keyword] = array(
    'id' => $id,
    'identifier' => $title . ($id > 1 ? ' ' . $id : ''),
    'name' => $argument,
    'settings' => $settings,
  );
}

/**
 * Basic settings form for a delegator page.
 */
function delegator_page_argument_form_settings(&$form, &$form_state) {
  $page = &$form_state['page'];
  $keyword = &$form_state['keyword'];

  if (isset($page->temporary_arguments[$keyword])) {
    $conf = $page->temporary_arguments[$keyword];
  }
  else if (isset($page->arguments[$keyword])) {
    $conf = $page->temporary_arguments[$keyword] = $page->arguments[$keyword];
  }

  if (!isset($conf)) {
    // This should be impossible and thus never seen.
    $form['error'] = array('#value' => t('Error: missing argument.'));
    return;
  }

  ctools_include('context');
  $plugin = ctools_get_argument($conf['name']);

  $form['settings'] = array(
    '#tree' => TRUE,
  );

  $form['identifier'] = array(
    '#type' => 'textfield',
    '#title' => t('Context identifier'),
    '#description' => t('This is the title of the context used to identify it later in the administrative process. This will never be shown to a user.'),
    '#default_value' => $conf['identifier'],
  );

  if (!$plugin) {
    // This should be impossible and thus never seen.
    $form['error'] = array('#value' => t('Error: missing or invalid argument plugin %argument.', array('%argument', $argument)));
    return;
  }

  if ($function = ctools_plugin_get_function($plugin, 'settings form')) {
    $function($form, $form_state, $conf['settings']);
  }

  $form_state['plugin'] = $plugin;
}

/**
 * Validate handler for argument settings.
 */
function delegator_page_argument_form_settings_validate(&$form, &$form_state) {
  if ($function = ctools_plugin_get_function($form_state['plugin'], 'settings form validate')) {
    $function($form, $form_state);
  }
}

/**
 * Submit handler for argument settings.
 */
function delegator_page_argument_form_settings_submit(&$form, &$form_state) {
  if ($function = ctools_plugin_get_function($form_state['plugin'], 'settings form submit')) {
    $function($form, $form_state);
  }

  $page = &$form_state['page'];
  $keyword = &$form_state['keyword'];

  // Copy the form to our temporary location which will get moved again when
  // finished. Yes, finished is always next but finish can happen from other
  // locations so we funnel through that path rather than duplicate.
  $page->temporary_arguments[$keyword]['identifier'] = $form_state['values']['identifier'];
  if (isset($form_state['values']['settings'])) {
    $page->temporary_arguments[$keyword]['settings'] = $form_state['values']['settings'];
  }
  else {
    $page->temporary_arguments[$keyword]['settings'] = array();
  }
}

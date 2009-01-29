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
 * Delegated implementation of hook_menu().
 */
function delegator_page_menu(&$items, $task) {
  // Set up access permissions.
  $access_callback = isset($task['admin access callback']) ? $task['admin access callback'] : 'user_access';
  $access_arguments = isset($task['admin access arguments']) ? $task['admin access arguments'] : array('administer delegator');

  $base = array(
    'access callback' => $access_callback,
    'access arguments' => $access_arguments,
    'file' => 'plugins/tasks/page.admin.inc',
  );

  $items['admin/build/pages/add'] = array(
    'title' => 'Add page',
    'description' => 'Add a delegator page subtask.',
    'page callback' => 'delegator_page_add_subtask',
    'type' => MENU_LOCAL_TASK,
  ) + $base;
  $items['admin/build/pages/import'] = array(
    'title' => 'Import page',
    'description' => 'Import a delegator page subtask.',
    'page callback' => 'drupal_get_form',
    'page arguments' => array('delegator_page_import_subtask'),
    'type' => MENU_LOCAL_TASK,
  ) + $base;

  $form_info = delegator_page_edit_form_info();
  $default_task = FALSE;
  $weight = 0;
  foreach ($form_info['order'] as $step => $form_title) {
    // The first edit form is the default for tabs, so it gets a bit
    // of special treatment here.
    if (!$default_task) {
      $default_task = TRUE;
      // Add the callback for the default tab.
      $items["admin/build/pages/edit/%"] = array(
        'title' => t('Edit'),
        'page callback' => 'delegator_page_edit_subtask',
        'page arguments' => array(4, $step),
        'type' => MENU_CALLBACK,
      ) + $base;

      // And make sure it's the default local task.
      $type = MENU_DEFAULT_LOCAL_TASK;
    }
    else {
      // This allows an empty form title to provide a hidden form
      // which is useful for doing more branch-like multi-step
      // functionality.
      $type = $form_title ? MENU_LOCAL_TASK : MENU_CALLBACK;
    }

    // Handler to edit delegator task handlers. May exist in its own UI.
    $items["admin/build/pages/edit/%/$step"] = array(
      'title' => $form_title,
      'page callback' => 'delegator_page_edit_subtask',
      'page arguments' => array(4, 5),
      'type' => $type,
      'weight' => $weight++,
    ) + $base;
  }

  $items["admin/build/pages/delete/%"] = array(
    'title' => t('Delete'),
    'page callback' => 'drupal_get_form',
    'page arguments' => array('delegator_page_delete_subtask', 4),
    'type' => MENU_CALLBACK,
  ) + $base;

  $items["admin/build/pages/break-lock/%"] = array(
    'title' => t('Break lock'),
    'page callback' => 'drupal_get_form',
    'page arguments' => array('delegator_page_break_lock_subtask', 4),
    'type' => MENU_CALLBACK,
  ) + $base;

  $items["admin/build/pages/clone/%"] = array(
    'title' => t('Clone'),
    'page callback' => 'drupal_get_form',
    'page arguments' => array('delegator_page_clone_subtask', 4),
    'type' => MENU_CALLBACK,
  ) + $base;

  $items["admin/build/pages/export/%"] = array(
    'title' => t('Clone'),
    'page callback' => 'delegator_page_export_subtask',
    'page arguments' => array(4),
    'type' => MENU_CALLBACK,
  ) + $base;

  $items["admin/build/pages/enable/%"] = array(
    'title' => t('Break lock'),
    'page callback' => 'delegator_page_enable_subtask',
    'page arguments' => array(4),
    'type' => MENU_CALLBACK,
  ) + $base;

  $items["admin/build/pages/disable/%"] = array(
    'title' => t('Disable'),
    'page callback' => 'delegator_page_disable_subtask',
    'page arguments' => array(4),
    'type' => MENU_CALLBACK,
  ) + $base;

  // AJAX callbacks for argument modal.
  $items['admin/build/delegator/argument'] = array(
    'page callback' => 'delegator_page_subtask_argument_ajax',
  ) + $base;

  // Add menu entries for each subtask
  foreach (delegator_page_load_all() as $subtask_id => $subtask) {
    if (!empty($subtask->disabled)) {
      continue;
    }

    if (!isset($subtask->access['type'])) {
      $subtask->access['type'] = 'none';
    }
    if (!isset($subtask->access['settings'])) {
      $subtask->access['settings'] = NULL;
    }

    $path = array();
    $page_arguments = array($subtask_id);
    $access_arguments = array($subtask->access);
    $load_arguments = array($subtask_id, '%index');

    // Replace named placeholders with our own placeholder to load contexts.
    foreach (explode('/', $subtask->path) as $position => $bit) {
      if ($bit[0] == '%' && $bit != '%') {
        // If an argument, swap it out with our argument loader and make sure
        // the argument gets passed through to the page callback.
        $path[] = '%dp_arg';
        $page_arguments[] = $position;
        $access_arguments[] = $position;
      }
      else {
        $path[] = $bit;
      }
    }

    $menu_path = implode('/', $path);

    $items[$menu_path] = delegator_page_menu_item($subtask->menu, $access_arguments, $page_arguments, $load_arguments);

    // Add a parent menu item if one is configured.
    if ($subtask->menu['type'] == 'default tab' && $subtask->menu['parent']['type'] != 'none') {
      array_pop($path);
      $parent_path = implode('/', $path);
      $items[$parent_path] = delegator_page_menu_item($subtask->menu['parent'], $access_arguments, $page_arguments, $load_arguments);
    }
  }
}

/**
 * Create a menu item for delegator pages.
 *
 * @param $menu
 *   The configuration to use. It will contain a type, and depending on the
 *   type may also contain weight, title and name. These are presumed to have
 *   been configured from the UI.
 * @param $access_arguments
 *   Arguments that go with ctools_access_menu; it should be loaded with
 *   the access plugin type, settings, and positions of any arguments that
 *   may produce contexts.
 * @param $page_arguments
 *   This should be seeded with the subtask name for easy loading and like
 *   the access arguments above should contain positions of arguments so
 *   that the menu system passes contexts through.
 * @param $load_arguments
 *   Arguments to send to the arg loader; should be the subtask id and '%index'.
 */
function delegator_page_menu_item($menu, $access_arguments, $page_arguments, $load_arguments) {
  $item = array(
    'access callback' => 'ctools_access_menu',
    'access arguments' => $access_arguments,
    'page callback' => 'delegator_page_execute',
    'page arguments' => $page_arguments,
    'load arguments' => $load_arguments,
    'file' => 'plugins/tasks/page.inc',
  );

  if (isset($menu['title'])) {
    $item['title'] = $menu['title'];
  }
  if (isset($menu['weight'])) {
    $item['weight'] = $menu['weight'];
  }

  switch ($menu['type']) {
    case 'none':
    default:
      $item['type'] = MENU_CALLBACK;
      break;
    case 'normal':
      $item['type'] = MENU_NORMAL_ITEM;
      // Insert item into the proper menu
      $item['menu_name'] = $menu['name'];
      break;
    case 'tab':
      $item['type'] = MENU_LOCAL_TASK;
      break;
    case 'default tab':
      $item['type'] = MENU_DEFAULT_LOCAL_TASK;
      break;
  }

  return $item;
}

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
function delegator_page_get_named_arguments($path) {
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
  $arguments = delegator_page_get_named_arguments($page->path);
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

  if (isset($form_state['create task handler'])) {
    require_once './' . drupal_get_path('module', 'delegator') . '/delegator.admin.inc';
    $task = $form_state['task'];
    $task_name = delegator_make_task_name($task['name'], $page->name);
    $cache = delegator_admin_get_task_cache($task, $page->name, $form_state['task_handlers']);
    $plugin = delegator_get_task_handler($form_state['create task handler']);

    // Create a new handler.
    $handler = delegator_new_task_handler($task, $page->name, $plugin, 0, $cache);
    $form_state['redirect'] = delegator_admin_new_task_handler($handler, $task_name, $task, $page->name, $cache, $plugin);
  }
  else if ($form_state['type'] == 'add') {
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

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('The machine readable name of this page. It must be unique, and it must contain only alphanumeric characters and underscores. Once created, you will not be able to change this value!'),
    '#default_value' => $page->name,
  );

  if (isset($page->pid) || !empty($page->import)) {
    $form['name']['#disabled'] = TRUE;
  }

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
    '#description' => t('The URL path to get to this page. You may create placeholders in the path by using %, or named placeholders by using %name. For example: "node/%node/foo", "forum/%forum" or "dashboard/%input". Named placeholders can be turned into contexts on the arguments form.'),
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
  // Ensure path is unused by other pages.
  $pages = delegator_page_load_all();
  $name = !empty($form_state['values']['name']) ? $form_state['values']['name'] : $form_state['page']->name;
  foreach ($pages as $page) {
    if ($page->name != $name && $page->path == $form_state['values']['path'] && empty($page->disabled)) {
      form_error($form['path'], t('That path is used by another page: @page', array('@page' => $page->admin_title)));
    }
  }

  // Ensure path is unused by things NOT pages. We do the double check because
  // we're checking against our page callback.

  // Replace named placeholders with our own placeholder to load contexts.
  $path = array();
  foreach (explode('/', $form_state['values']['path']) as $bit) {
    if ($bit[0] == '%') {
      $path[] = '%';
    }
    else {
      $path[] = $bit;
    }
  }

  $path = implode('/', $path);
  $result = db_query("SELECT * FROM menu_router WHERE path = '%s'", $path);
  while ($router = db_fetch_object($result)) {
    if ($router->page_callback != 'delegator_page_execute') {
      form_error($form['path'], t('That path is already in used. This system cannot override existing paths.'));
    }
  }

  // Ensure path is properly formed.
  $args = delegator_page_get_named_arguments($form_state['values']['path']);
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

  // Do name checking if it's something that can be changed.
  if (empty($form_state['page']->pid)) {
    // Ensure name fits the rules:
    if (preg_match('/[^a-zA-Z0-9_]/', $form_state['values']['name'])) {
      form_error($form['name'], t('Page name must be alphanumeric or underscores only.'));
    }

    // Ensure name is unique.
    if (delegator_page_load($form_state['values']['name'])) {
      form_error($form['name'], t('That name is already used; the name must be unique.'));
    }
  }
}

/**
 * Store the values from the basic settings form.
 */
function delegator_page_form_basic_submit(&$form, &$form_state) {
  if (!isset($form_state['page']->pid) && empty($form_state['page']->import)) {
    $form_state['page']->name = $form_state['values']['name'];
  }

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

  list($major, $minor) = explode('.', VERSION, 2);

  $form['menu']['name-warning'] = array(
    '#type' => 'markup',
    '#prefix' => '<div class="warning">',
    '#value' => t("Warning: Changing this item's menu will not work reliably in Drupal 6.4 or earlier. Please upgrade your copy of Drupal at !url.", array('!url' => l('drupal.org', 'http://drupal.org/project/Drupal+project'))),
    '#suffix' => '</div>',
    '#process' => array('ctools_dependent_process'),
    '#dependency' => array('radio:menu[type]' => array('normal')),
    '#access' => ($minor < 5),
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
  $form_state['module'] = 'delegator_page';
  $form_state['callback argument'] = $form_state['cache name'];
  $form_state['access'] = $form_state['page']->access;
  $form_state['no buttons'] = TRUE;
  $form_state['contexts'] = array();

  // Load contexts based on argument data:
  if ($arguments = _delegator_page_get_arguments($form_state['page'])) {
    $form_state['contexts'] = ctools_context_get_placeholders_from_argument($arguments);
  }

  ctools_include('context-access-admin');
  $form = array_merge($form, ctools_access_admin_form($form_state));
}

/**
 * Submit handler to deal with access control changes.
 */
function delegator_page_form_access_submit(&$form, &$form_state) {
  $form_state['page']->access['logic'] = $form_state['values']['logic'];
}


/**
 * Form to handle assigning argument handlers to named arguments.
 */
function delegator_page_form_argument(&$form, &$form_state) {
  $path = $form_state['page']->path;
  $page = &$form_state['page'];

  $arguments = delegator_page_get_named_arguments($path);

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
  $arguments = delegator_page_get_named_arguments($path);

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

/**
 * Form to configure a page to have a single task handler or multiple
 * task handlers.
 */
function delegator_page_argument_form_multiple(&$form, &$form_state) {
  $task = delegator_get_task('page');
  $task_handlers = delegator_load_task_handlers($task, $form_state['page']->name);

  $form['multiple'] = array(
    '#type' => 'radios',
    '#options' => array(
      1 => t('Allow multiple handlers for this page.'),
      0 => t('Create a single handler for this page.'),
    ),
    '#default_value' => $form_state['page']->multiple,
    '#description' => t('By allowing multiple handlers, the task handler UI will open up, allowing you to add and remove task handlers and adjust their priority.'),
  );

  // If they've selected multiple handlers and actually have more than one
  // don't let them switch back to single.
  if ($form_state['page']->multiple && count($task_handlers) > 1) {
    $form['multiple']['#disabled'] = TRUE;
    $form['multiple']['#description'] .= t('You may not modify this value while multiple task handlers exist. If you wish to change this to single only, you must reduce the number of task handlers attached to the page to zero or one.');
  }

  $form_state['task'] = $task;
  $form_state['task_handlers'] = $task_handlers;

  if ($form_state['type'] == 'add' || empty($task_handlers)) {
    // Get a list of possible task handlers for this task.
    $task_handler_plugins = delegator_get_task_handler_plugins($task);
    foreach ($task_handler_plugins as $id => $plugin) {
      $options[$id] = $plugin['title'];
    }

    ctools_include('dependent');
    $form['handler'] = array(
      '#title' => t('Select the handler for this page'),
      '#type' => 'select',
      '#options' => $options,
      '#process' => array('ctools_dependent_process'),
      '#dependency' => array('radio:multiple' => array(0)),
    );
  }
}

/**
 * Submit handler for the multiple form.
 */
function delegator_page_argument_form_multiple_submit(&$form, &$form_state) {
  $form_state['page']->multiple = $form_state['values']['multiple'];

  // The task handler will be created in the _finish hook.
  if (!$form_state['page']->multiple && empty($task_handlers)) {
    $form_state['create task handler'] = $form_state['values']['handler'];
  }
}

/**
 * Form to break a lock on a delegator page.
 */
function delegator_page_break_lock_subtask(&$form_state, $name) {
  // This ensures the task .inc file is loaded
  $task = delegator_get_task('page');
  $page = delegator_page_load($name);

  $form_state['name'] = $name;

  ctools_include('object-cache');
  $lock = ctools_object_cache_test('delegator_page', $name);

  $form = array();

  if (empty($lock)) {
    return array('message' => array('#value' => t('There is no lock on this page to break.')));
  }

  $task = delegator_get_task($page->task);
  delegator_set_trail($task);

  $cancel = 'admin/build/pages';
  if (!empty($_REQUEST['cancel'])) {
    $cancel = $_REQUEST['cancel'];
  }

  $account = user_load($lock->uid);
  return confirm_form($form,
                  t('Are you sure you want to break this lock?'),
                  $cancel,
                  t('By breaking this lock, any unsaved changes made by !user will be lost!', array('!user' => theme('username', $account))),
                  t('Break lock'),
                  t('Cancel'));
}

/**
 * Submit handler to break_lock a view.
 */
function delegator_page_break_lock_subtask_submit(&$form, &$form_state) {
  ctools_object_cache_clear_all('delegator_page', $form_state['name']);
  drupal_set_message(t('The lock has been broken and you may now edit this page.'));
  $form_state['redirect'] = 'admin/build/pages';
}

/**
 * Import a task handler from cut & paste
 */
function delegator_page_import_subtask(&$form_state) {
  drupal_set_title(t('Import page'));
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Page name'),
    '#description' => t('Enter the name to use for this page if it is different from the source page. Leave blank to use the name of the page.'),
  );

  $form['object'] = array(
    '#type' => 'textarea',
    '#title' => t('Paste page code here'),
    '#rows' => 15,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
  );
  return $form;
}

/**
 * Ensure we got a valid page.
 */
function delegator_page_import_subtask_validate($form, &$form_state) {
  ob_start();
  eval($form_state['values']['object']);
  ob_end_clean();

  if (!isset($page) || !is_object($page)) {
    form_error($form['object'], t('Unable to interpret page code.'));
  }

  $name = !empty($form_state['values']['name']) ? $form_state['values']['name'] : $page->name;

  // See if the task is already locked.
  $cache = delegator_page_get_page_cache($name);
  if (!empty($cache->locked)) {
    $account = user_load($cache->locked->uid);
    $username = theme('username', $account);
    $lock_age = format_interval(time() - $cache->locked->updated);
    $break = url('admin/build/pages/break-lock/' . $name, array('query' => array('destination' => $_GET['q'], 'cancel' => $_GET['q'])));

    form_error($form['object'], t('Unable to import page because it is being edited by user !user, and is therefore locked from editing by others. This lock is !age old. Click here to <a href="!break">break this lock</a>.', array('!user' => $username, '!age' => $lock_age, '!break' => $break)));
  }

  $page->type = t('Normal');

  ctools_include('export');
  $page->export_type = EXPORT_IN_DATABASE;

  if (isset($cache) && isset($cache->pid)) {
    drupal_set_message(t('Warning: The page you are importing already exists and this operation will overwrite an existing page. If this is not what you intend, you may Cancel this. You should then choose a different page name to ensure you use a unique name.'), 'warning');
    $page->pid = $cache->pid;
  }

  if (preg_match('/[^a-zA-Z0-9_]/', $name)) {
    form_error($form['name'], t('Page name must be alphanumeric or underscores only.'));
  }

  if ($page->name != $name) {
    $page->old_name = $page->name;
    $page->name = $name;
  }
  $page->import = TRUE; // set a flag so that we can get special import behavior.
  $form_state['page'] = $page;
}

/**
 * Submit the import page to create the new page and redirect.
 */
function delegator_page_import_subtask_submit($form, &$form_state) {
  // Use the one from the database or an updated one in cache?
  $page = &$form_state['page'];

  // If the name was changed, go through and rename any default handlers so
  // that they'll actually work.
  if (isset($page->old_name) && isset($page->default_handlers)) {
    foreach ($page->default_handlers as $name => $handler) {
      // replace the old name with the new name at the start of the name string.
      $page->default_handlers[$name]->name = preg_replace('/^page_' . $page->old_name . '/', 'page_' . $page->name, $handler->name);
    }
  }

  delegator_page_set_page_cache($page);
  $form_state['redirect'] = 'admin/build/pages/edit/' . $page->name;
}

/**
 * Entry point to export a page.
 */
function delegator_page_export_subtask($name, $handlers = FALSE) {
  // This ensures the task .inc file is loaded
  $task = delegator_get_task('page');
  $page = delegator_page_load($name);

  if (!$page) {
    return drupal_not_found();
  }

  $title = $page->admin_title;
  if ($handlers) {
    drupal_set_title(t('Export task page "@title" with handlers', array('@title' => $page->admin_title)));
    $export = delegator_page_export($page, TRUE);
  }
  else {
    drupal_set_title(t('Export task page "@title"', array('@title' => $page->admin_title)));
    $export = delegator_page_export($page);
  }

  return drupal_get_form('ctools_export_form', $export, $title);
}

/**
 * Entry point to clone a page.
 */
function delegator_page_clone_subtask(&$form_state, $name) {
  // This ensures the task .inc file is loaded
  $form_state['task'] = delegator_get_task('page');
  $page = delegator_page_load($name);

  if (!$page) {
    drupal_not_found();
    exit;
  }

  $form_state['page'] = $page;
  $form = array();

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Page name'),
    '#description' => t('Enter the name to the new page It must be unique and contain only alphanumeric characters and underscores.'),
  );

  $form['handlers'] = array(
    '#type' => 'checkbox',
    '#title' => t('Clone handlers'),
    '#description' => t('If checked all task handlers associated with the task will be cloned as well.'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Clone'),
  );

  return $form;
}

/**
 * Validate clone page form.
 */
function delegator_page_clone_subtask_validate(&$form, &$form_state) {
  // Ensure name fits the rules:
  if (preg_match('/[^a-zA-Z0-9_]/', $form_state['values']['name'])) {
    form_error($form['name'], t('Page name must be alphanumeric or underscores only.'));
  }

  // Ensure name is unique.
  if (delegator_page_load($form_state['values']['name'])) {
    form_error($form['name'], t('That name is already used; the name must be unique.'));
  }
}

/**
 * submit clone page form.
 *
 * Load the page, change the name(s) to protect the innocent, and if
 * requested, load all the task handlers so that they get saved properly too.
 */
function delegator_page_clone_subtask_submit(&$form, &$form_state) {
  $page = $form_state['page'];
  $task = $form_state['task'];
  $name = $form_state['values']['name'];

  if (!empty($form_state['values']['handlers'])) {
    $handlers = delegator_load_task_handlers($task, $page->name);
    $page->default_handlers = array();

    foreach ($handlers as $handler_name => $handler) {
      // replace the old name with the new name at the start of the name string.
      $handler->name = preg_replace('/^page_' . $page->name . '/', 'page_' . $name, $handler->name);
      $handler->subtask = $name;
      // Pretend these handlers are all default handlers so they get saved as new.
      $handler->type = t('Default');
      $handler->export_type = EXPORT_IN_CODE;
      $page->default_handlers[$handler_name] = $handler;
    }
  }

  $page->name = $name;
  delegator_page_set_page_cache($page);
  $form_state['redirect'] = 'admin/build/pages/edit/' . $page->name;
}
/**
 * Entry point to enable a page.
 */
function delegator_page_enable_subtask($name) {
  ctools_include('export');
  ctools_export_set_status('delegator_pages', $name, FALSE);
  drupal_goto('admin/build/pages');
}

/**
 * Entry point to disable a page.
 */
function delegator_page_disable_subtask($name) {
  ctools_include('export');
  ctools_export_set_status('delegator_pages', $name, TRUE);
  drupal_goto('admin/build/pages');
}

/**
 * Entry point to export a page.
 */
function delegator_page_delete_subtask($form_state, $name) {
  // This ensures the task .inc file is loaded
  $task = delegator_get_task('page');
  $page = delegator_page_load($name);

  if (!$page) {
    return drupal_not_found();
  }

  $form_state['page'] = $page;
  $form = array();

  $cancel = 'admin/build/pages';

  if ($page->type == t('Overridden')) {
    $title = t('Are you sure you want to revert the page %name?', array('%name' => $page->admin_title));
    $desc = t('Reverting the page will delete the page that is in the database, reverting it to the original default page. Any changes you have made will be lost and cannot be recovered.');
  }
  else {
    $title = t('Are you sure you want to delete the page %name?', array('%name' => $page->admin_title));
    $desc = t('Deleting a page cannot be undone.');
  }

  return confirm_form($form,
                  $title,
                  $cancel,
                  $desc,
                  t('Delete'),
                  t('Cancel'));
}

/**
 * Submit handler to delete a view.
 */
function delegator_page_delete_subtask_submit(&$form, &$form_state) {
  delegator_page_delete($form_state['page']);
  drupal_set_message(t('The page has been deleted.'));
  $form_state['redirect'] = 'admin/build/pages';
}

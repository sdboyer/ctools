<?php
// $Id$

/**
 * @file includes/common-context.inc
 * Provide API for adding contexts for modules that embed displays.
 *
 * Note that most of this code was directly copied from Panels 2, and as such
 * a lot of this code is crusty. It could probably stand to be rewritten,
 * and brought up to date, or at least better commented.
 */

/**
 * Provide a list of the ways contexts can be embedded.
 *
 * This provides a full list of context types that the tool understands
 * and can let modules utilize.
 */
function ctools_context_info($type = NULL) {
  static $info = NULL;

  // static doesn't work with functions like t().
  if (empty($info)) {
    $info = array(
      'argument' => array(
        'title' => t('Arguments'),
        'singular title' => t('argument'),
        'description' => '', // t("Arguments are parsed from the URL and translated into contexts that may be added to the display via the 'content' tab. These arguments are parsed in the order received, and you may use % in your URL to hold the place of an object; the rest of the arguments will come after the URL. For example, if the URL is node/%/panel and your user visits node/1/panel/foo, the first argument will be 1, and the second argument will be foo."),
        'add button' => t('Add argument'),
        'context function' => 'ctools_get_argument',
        'form id' => 'ctools_edit_argument_form',
        'key' => 'arguments', // the key that data will be stored on an object, eg $panel_page
        'sortable' => TRUE,
        'settings' => 'argument_settings',
      ),
      'relationship' => array(
        'title' => t('Relationships'),
        'singular title' => t('relationship'),
        'description' => '', // t('Relationships are contexts that are created from already existing contexts; the add relationship button will only appear once there is another context available. Relationships can load objects based upon how they are related to each other; for example, the author of a node, or a taxonomy term attached to a node, or the vocabulary of a taxonomy term.'),
        'add button' => t('Add relationship'),
        'context function' => 'ctools_get_relationship',
        'form id' => 'ctools_edit_relationship_form',
        'key' => 'relationships',
        'sortable' => FALSE,
        'settings' => 'relationship_settings',
      ),
      'context' => array(
        'title' => t('Contexts'),
        'singular title' => t('context'),
        'description' => '', // t('Contexts are embedded directly into the panel; you generally must select an object in the panel. For example, you could select node 5, or the term "animals" or the user "administrator"'),
        'add button' => t('Add context'),
        'context function' => 'ctools_get_context',
        'form id' => 'ctools_edit_context_form',
        'key' => 'contexts',
        'sortable' => FALSE,
        'settings' => 'context_settings',
      ),
      'requiredcontext' => array(
        'title' => t('Required contexts'),
        'singular title' => t('required context'),
        'description' => '', // t('Required contexts are passed in from some external source, such as a containing panel. If a mini panel has required contexts, it can only appear when that context is available, and therefore will not show up as a standard Drupal block.'),
        'add button' => t('Add required context'),
        'context function' => 'ctools_get_context',
        'form id' => 'ctools_edit_requiredcontext_form',
        'key' => 'requiredcontexts',
        'sortable' => FALSE,
      ),
    );
  }

  if ($type === NULL) {
    return $info;
  }

  return $info[$type];
}

/**
 * Get the cache for the context object.
 *
 * This can pass through to a module provided cache if it exists, or
 * the default cache will be used.
 */
function ctools_context_cache_get($fragment, $key) {
  // Separate the fragment into 'module' and 'argument'
  if (strpos($fragment, '-') === FALSE) {
    $module = $fragment;
    $argument = NULL;
  }
  else {
    list($module, $argument) = explode('-', $fragment, 2);
  }

  $function = $module . '_context_cache_get';
  if (function_exists($function)) {
    return $function($argument, $key);
  }
  else {
    return ctools_object_cache_get("context_object:$module", $key);
  }
}

/**
 * Get the cache for the context object.
 *
 * This can pass through to a module provided cache if it exists, or
 * the default cache will be used.
 */
function ctools_context_cache_set($fragment, $key, $object) {
  // Separate the fragment into 'module' and 'argument'
  if (strpos($fragment, '-') === FALSE) {
    $module = $fragment;
    $argument = NULL;
  }
  else {
    list($module, $argument) = explode('-', $fragment, 2);
  }

  $function = $module . '_context_cache_set';
  if (function_exists($function)) {
    return $function($argument, $key, $object);
  }
  else {
    return ctools_object_cache_set("context_object:$module", $key, $object);
  }
}

/**
 * Get the data belonging to a particular context.
 */
function ctools_context_data($type, $name) {
  $info = ctools_context_info($type);
  if (function_exists($info['context function'])) {
    return $info['context function']($name);
  }
}

/**
 * Add the argument table plus gadget plus javascript to the form.
 */
function ctools_context_add_argument_form($module, &$form, &$form_state, &$form_location, $object, $name = NULL) {
  if (empty($name)) {
    $name = $object->name;
  }

  $form_location = array(
    '#prefix' => '<div id="ctools-arguments-table">',
    '#suffix' => '</div>',
    '#theme' => 'ctools_context_item_form',
    '#object_name' => $name,
    '#ctools_context_type' => 'argument',
    '#ctools_context_module' => $module,
  );

  $args = ctools_get_arguments();
  $choices = array();
  foreach ($args as $name => $arg) {
    $choices[$name] = $arg['title'];
  }

  asort($choices);

  if (!empty($choices) || !empty($object->arguments)) {
    ctools_context_add_item_table('argument', $form_location, $choices, $object->arguments);
  }
}

function ctools_context_add_context_form($module, &$form, &$form_state, &$form_location, $object, $name = NULL) {
  if (empty($name)) {
    $name = $object->name;
  }

  $form_location = array(
    '#prefix' => '<div id="ctools-contexts-table">',
    '#suffix' => '</div>',
    '#theme' => 'ctools_context_item_form',
    '#object_name' => $name,
    '#ctools_context_type' => 'context',
    '#ctools_context_module' => $module,
  );

  // Store the order the choices are in so javascript can manipulate it.
  $form_location['markup'] = array(
    '#value' => '&nbsp;',
  );

  $choices = array();
  foreach (ctools_get_contexts() as $name => $arg) {
    if (empty($arg['no ui'])) {
      $choices[$name] = $arg['title'];
    }
  }

  asort($choices);

  if (!empty($choices) || !empty($object->contexts)) {
    ctools_context_add_item_table('context', $form_location, $choices, $object->contexts);
  }
}

function ctools_context_add_required_context_form($module, &$form, &$form_state, &$form_location, $object, $name = NULL) {
  if (empty($name)) {
    $name = $object->name;
  }

  $form_location = array(
    '#prefix' => '<div id="ctools-requiredcontexts-table">',
    '#suffix' => '</div>',
    '#theme' => 'ctools_context_item_form',
    '#object_name' => $name,
    '#ctools_context_type' => 'requiredcontext',
    '#ctools_context_module' => $module,
  );

  // Store the order the choices are in so javascript can manipulate it.
  $form_location['markup'] = array(
    '#value' => '&nbsp;',
  );

  $choices = array();
  foreach (ctools_get_contexts() as $name => $arg) {
    $choices[$name] = $arg['title'];
  }

  asort($choices);

  if (!empty($choices) || !empty($object->contexts)) {
    ctools_context_add_item_table('requiredcontext', $form_location, $choices, $object->requiredcontexts);
  }
}

function ctools_context_add_relationship_form($module, &$form, &$form_state, &$form_location, $object, $name = NULL) {
  if (empty($name)) {
    $name = $object->name;
  }

  $form_location = array(
    '#prefix' => '<div id="ctools-relationships-table">',
    '#suffix' => '</div>',
    '#theme' => 'ctools_context_item_form',
    '#object_name' => $name,
    '#ctools_context_type' => 'relationship',
    '#ctools_context_module' => $module,
  );

  // Store the order the choices are in so javascript can manipulate it.
  $form_location['markup'] = array(
    '#value' => '&nbsp;',
  );

  $base_contexts = isset($object->base_contexts) ? $object->base_contexts : array();
  $available_relationships = ctools_context_get_relevant_relationships(ctools_context_load_contexts($object, TRUE, $base_contexts));

  ctools_context_add_item_table('relationship', $form_location, $available_relationships, $object->relationships);
}

/**
 * Include all context administrative include files, css, javascript.
 */
function ctools_context_admin_includes() {
  ctools_include('context');
  ctools_include('modal');
  ctools_include('ajax');
  ctools_include('object-cache');
  ctools_modal_add_js();
  ctools_modal_add_plugin_js(ctools_get_contexts());
  ctools_modal_add_plugin_js(ctools_get_relationships());
}

/**
 * Add the context table to the page.
 */
function ctools_context_add_item_table($type, &$form, $available_contexts, $items) {
  $form[$type] = array(
    '#tree' => TRUE,
  );

  $module = $form['#ctools_context_module'];
  $name   = $form['#object_name'];

  if (isset($items) && is_array($items)) {
    foreach ($items as $position => $context) {
      ctools_context_add_item_to_form($module, $type, $name, $form[$type][$position], $position, $context);
    }
  }

  $type_info = ctools_context_info($type);
  $form['description'] = array(
    '#prefix' => '<div class="description">',
    '#suffix' => '</div>',
    '#value' => $type_info['description'],
  );

  ctools_context_add_item_table_buttons($type, $module, $form, $available_contexts);
}

function ctools_context_add_item_table_buttons($type, $module, &$form, $available_contexts) {
  drupal_add_js('misc/ajax.js');
  $form['buttons'] = array(
    '#tree' => TRUE,
  );

  if (!empty($available_contexts)) {
    $type_info = ctools_context_info($type);

    $module = $form['#ctools_context_module'];
    $name   = $form['#object_name'];

    // The URL for this ajax button
    $form['buttons'][$type]['add-url'] = array(
      '#attributes' => array('class' => array("ctools-$type-add-url")),
      '#type' => 'hidden',
      '#value' => url("ctools/context/ajax/add/$module/$type/$name", array('absolute' => TRUE)),
    );

    // This also will be in the URL.
    $form['buttons'][$type]['item'] = array(
      '#attributes' => array('class' => array("ctools-$type-add-url")),
      '#type' => 'select',
      '#options' => $available_contexts,
    );

    $form['buttons'][$type]['add'] = array(
      '#type' => 'submit',
      '#attributes' => array('class' => array('ctools-use-modal')),
      '#id' => "ctools-$type-add",
      '#value' => $type_info['add button'],
    );
  }
}

/**
 * Add a row to the form. Used both in the main form and by
 * the ajax to add an item.
 */
function ctools_context_add_item_to_form($module, $type, $name, &$form, $position, $item) {
  // This is the single function way to load any plugin by variable type.
  $info = ctools_context_data($type, $item['name']);
  $form['title'] = array(
    '#value' => check_plain($item['identifier']),
  );

  // Relationships not sortable.
  $type_info = ctools_context_info($type);

  if (!empty($type_info['sortable'])) {
    $form['position'] = array(
      '#type' => 'weight',
      '#default_value' => $position,
      '#attributes' => array('class' => array('drag-position')),
    );
  }

  $form['remove'] = array(
    '#value' => ctools_ajax_image_button(ctools_image_path('icon-delete.png'), "ctools/context/ajax/delete/$module/$type/$name/$position", t('Remove this item.')),
  );

  $form['settings'] = array(
    '#value' => ctools_modal_image_button(ctools_image_path('icon-configure.png'), "ctools/context/ajax/configure/$module/$type/$name/$position", t('Configure settings for this item.')),
  );
}


// ---------------------------------------------------------------------------
// AJAX forms and stuff.

/**
 * Ajax entry point to add an context
 */
function ctools_context_ajax_item_add($module = NULL, $type = NULL, $object_name = NULL, $name = NULL) {
  ctools_include('ajax');
  ctools_include('modal');
  ctools_include('context');
  ctools_include('object-cache');

  if (!$name) {
    return ctools_ajax_render_error();
  }

  // Load stored object from cache.
  if (!($object = ctools_context_cache_get($module, $object_name))) {
    ctools_ajax_render_error(t('Invalid object name.'));
  }

  // Get info about what we're adding.
  $info = ctools_context_data($type, $name);
  if (empty($info)) {
    return ctools_ajax_render_error();
  }
  $type_info = ctools_context_info($type);

  // Create a reference to the place our context lives. Since this is fairly
  // generic, this is the easiest way to get right to the place of the
  // object without knowing precisely what data we're poking at.
  $ref = &$object->{$type_info['key']};

  // Give this item an id, which is really just the nth version
  // of this particular context.
  $id = ctools_context_next_id($ref, $name);

  // Figure out the position for our new context.
  $position = empty($ref) ? 0 : max(array_keys($ref)) + 1;

  // Create the basis for our new context.
  $ref[$position] = array(
    'identifier' => $info['title'] . ($id > 1 ? ' ' . $id : ''),
    'keyword' => ctools_get_keyword($object, $info['keyword']),
    'id' => $id,
    'name' => $name,
  );

  if (isset($type_info['settings'])) {
    $ref[$position][$type_info['settings']] = isset($info['defaults']) ? $info['defaults'] : array();
  }

  $base_contexts = isset($object->base_contexts) ? $object->base_contexts : array();
  $contexts = ctools_context_load_contexts($object, TRUE, $base_contexts);

  $form_state = array(
    'object' => &$object,
    'module' => $module,
    'type' => $type,
    'name' => $name,
    'ajax' => TRUE,
    'info' => $info,
    'position' => $position,
    'contexts' => $contexts,
    'ref' => &$ref,
    'title' => t('Add @type "@context"', array('@type' => $type_info['singular title'], '@context' => $info['title'])),
  );

  $output = ctools_modal_form_wrapper('ctools_context_form', $form_state);

  if (empty($output)) {
    // successful submit
    ctools_context_cache_set($module, $object_name, $object);

    $arg_form_state = array();

    $arg_form = array(
      '#post' => array(),
      '#programmed' => FALSE,
      '#tree' => FALSE,
    );

    // Build a chunk of the form to merge into the displayed form
    $arg_form[$type] = array(
      '#tree' => TRUE,
    );
    $arg_form[$type][$position] = array(
      '#tree' => TRUE,
    );

    ctools_context_add_item_to_form($module, $type, $object_name, $arg_form[$type][$position], $position, $ref[$position]);
    $arg_form = form_builder('ctools_context_form', $arg_form, $arg_form_state);

    // Build the relationships table so we can ajax it in.
    // This is an additional thing that goes in here.
    $rel_form = array(
      '#theme' => 'ctools_context_item_form',
      '#object_name' => $object_name,
      '#ctools_context_type' => 'relationship',
      '#ctools_context_module' => $module,
      '#only_buttons' => TRUE,
      '#post' => array(),
      '#programmed' => FALSE,
      '#tree' => FALSE,
    );

    $rel_form['relationship'] = array(
      '#tree' => TRUE,
    );

    // Allow an object to set some 'base' contexts that come from elsewhere.
    $rel_contexts = isset($object->base_contexts) ? $object->base_contexts : array();
    $all_contexts = ctools_context_load_contexts($object, TRUE, $rel_contexts);
    $available_relationships = ctools_context_get_relevant_relationships($all_contexts);

    $output = array();
    if (!empty($available_relationships)) {
      ctools_context_add_item_table_buttons('relationship', $module, $rel_form, $available_relationships);
      $rel_form = form_builder('dummy_form_id', $rel_form, $arg_form_state);
      $output[] = ajax_command_replace('div#ctools-relationships-table div.buttons', drupal_render($rel_form));
    }

    $theme_vars = array();
    $theme_vars['type'] = $type;
    $theme_vars['form'] = $arg_form[$type][$position];
    $theme_vars['position'] = $position;
    $theme_vars['count'] = $position;
    $text = theme('ctools_context_item_row', $theme_vars);
    $output[] = ctools_ajax_command_append('#' . $type . '-table tbody', $text);
    $output[] = ctools_ajax_command_changed('#' . $type . '-row-' . $position, '.title');
    $output[] = ctools_modal_command_dismiss();
  }

  print ajax_render($output);
  exit;
}

/**
 * Ajax entry point to edit an item
 */
function ctools_context_ajax_item_edit($module = NULL, $type = NULL, $object_name = NULL, $position = NULL) {
  ctools_include('ajax');
  ctools_include('modal');
  ctools_include('context');
  ctools_include('object-cache');

  if (!isset($position)) {
    return ctools_ajax_render_error();
  }

  // Load stored object from cache.
  if (!($object = ctools_context_cache_get($module, $object_name))) {
    ctools_ajax_render_error(t('Invalid object name.'));
  }

  $type_info = ctools_context_info($type);

  // Create a reference to the place our context lives. Since this is fairly
  // generic, this is the easiest way to get right to the place of the
  // object without knowing precisely what data we're poking at.
  $ref = &$object->{$type_info['key']};

  $name = $ref[$position]['name'];
  if (empty($name)) {
    ctools_ajax_render_error();
  }

  // load the context
  $info = ctools_context_data($type, $name);
  if (empty($info)) {
    ctools_ajax_render_error(t('Invalid context type'));
  }

  $base_contexts = isset($object->base_contexts) ? $object->base_contexts : array();
  $contexts = ctools_context_load_contexts($object, TRUE, $base_contexts);

  // Remove this context, because we can't really allow circular contexts.
  unset($contexts[ctools_context_id($ref[$position])]);

  $form_state = array(
    'object' => &$object,
    'module' => $module,
    'type' => $type,
    'name' => $name,
    'ajax' => TRUE,
    'info' => $info,
    'position' => $position,
    'contexts' => $contexts,
    'ref' => &$ref,
    'title' => t('Edit @type "@context"', array('@type' => $type_info['singular title'], '@context' => $info['title'])),
  );

  $output = ctools_modal_form_wrapper('ctools_context_form', $form_state);

  if (empty($output)) {
    // successful submit
    ctools_context_cache_set($module, $object_name, $object);

    $output = array();
    $output[] = ctools_modal_command_dismiss();

    $arg_form = array(
      '#post' => array(),
      '#programmed' => FALSE,
      '#tree' => FALSE,
    );

    // Build a chunk of the form to merge into the displayed form
    $arg_form[$type] = array(
      '#tree' => TRUE,
    );
    $arg_form[$type][$position] = array(
      '#tree' => TRUE,
    );

    ctools_context_add_item_to_form($module, $type, $object_name, $arg_form[$type][$position], $position, $ref[$position]);
    $arg_form = form_builder('ctools_context_form', $arg_form, $arg_form_state);

    $theme_vars = array();
    $theme_vars['type'] = $type;
    $theme_vars['form'] = $arg_form[$type][$position];
    $theme_vars['position'] = $position;
    $theme_vars['count'] = $position;
    $output[] = ctools_ajax_command_replace('#' . $type . '-row-' . $position, theme('ctools_context_item_row', $theme_vars));
    $output[] = ctools_ajax_command_changed('#' . $type . '-row-' . $position, '.title');
  }
  print ajax_render($output);
  ajax_footer();
}

/**
 * Get the config form.
 *
 * The $form_info and $form_state need to be preconfigured with data you'll need
 * such as whether or not you're using ajax, or the modal. $form_info will need
 * your next/submit callbacks so that you can cache your data appropriately.
 *
 * @return
 *   If this function returns false, no form exists.
 */
function ctools_context_form($op, $form_info, &$form_state, $plugin, &$conf, $step = NULL) {
  $form_state += array(
    'plugin' => $plugin,
    'conf' => &$conf,
    'op' => $op,
  );

  $form_info += array(
    'id' => 'ctools_context_form',
    'show back' => TRUE,
  );

  // Turn the forms defined in the plugin into the format the wizard needs.
  if ($op == 'add') {
    if (!empty($plugin['add form'])) {
      _ctools_context_create_form_info($form_info, $plugin['add form'], $plugin, $op);
    }
  }

  if (empty($form_info['order'])) {
    // Use the edit form for the add form if add form was completely left off.
    if (!empty($plugin['edit form'])) {
      _ctools_context_create_form_info($form_info, $plugin['edit form'], $plugin, $op);
    }
  }

  if (empty($form_info['order'])) {
    return FALSE;
  }

  ctools_include('wizard');
  return ctools_wizard_multistep_form($form_info, $step, $form_state);

}

function _ctools_context_create_form_info(&$form_info, $info, $plugin_definition, $op) {
  if (is_string($info)) {
    if (empty($plugin_definition['title'])) {
      $title = t('Configure');
    }
    else if ($op == 'add') {
      $title = t('Configure new !plugin_title', array('!plugin_title' => $plugin_definition['title']));
    }
    else {
      $title = t('Configure !plugin_title', array('!plugin_title' => $plugin_definition['title']));
    }
    $form_info['order'] = array('form' => $title);
    $form_info['forms'] = array(
      'form' => array(
        'title' => $title,
        'form id' => $info,
        'wrapper' => 'ctools_edit_context_form_defaults',
      ),
    );
  }
  else if (is_array($info)) {
    $form_info['order'] = array();
    $form_info['forms'] = array();
    $count = 0;
    $base = 'step';
    $wrapper = NULL;
    foreach ($info as $form_id => $title) {
      // @todo -- docs say %title can be used to sub for the admin title.
      $step = $base . ++$count;
      if (empty($wrapper)) {
        $wrapper = $step;
      }

      if (is_array($title)) {
        if (!empty($title['default'])) {
          $wrapper = $step;
        }
        $title = $title['title'];
      }

      $form_info['order'][$step] = $title;
      $form_info['forms'][$step] = array(
        'title' => $title,
        'form id' => $form_id,
      );
    }
    if ($wrapper) {
      $form_info['forms'][$wrapper]['wrapper'] = 'ctools_edit_context_form_defaults';
    }
  }
}

function ctools_edit_context_form_defaults($form, &$form_state) {
  // Basic values required to orient ourselves
  $object            = $form_state['object'];
  $plugin_definition = $form_state['info'];
  $position          = $form_state['position'];
  $contexts          = $form_state['contexts'];

  $ctext = $object->{$plugin_definition['key']}[$position];

  if ($plugin_definition['key'] == 'arguments' && !isset($plugin_definition['default'])) {
    $plugin_definition['default'] = 'ignore';
    $plugin_definition['title'] = '';
  }

  $form['description'] = array(
    '#prefix' => '<div class="description">',
    '#suffix' => '</div>',
    '#value' => check_plain($plugin_definition['description']),
  );

  $form['name'] = array(
    '#type' => 'hidden',
    '#value' => $plugin_definition['name'],
  );

  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $ctext['id'],
  );
  if ($plugin_definition['key'] == 'relationships') {
    $form['context'] = ctools_context_selector($contexts, $plugin_definition['required context'], isset($ctext['context']) ? $ctext['context'] : '');
  }
  if ($plugin_definition['key'] == 'arguments') {
    $form['default'] = array(
      '#type' => 'select',
      '#title' => t('Default'),
      '#options' => array(
        'ignore' => t('Ignore it; content that requires this context will not be available.'),
        '404' => t('Display page not found or display nothing at all.'),
      ),
      '#default_value' => $ctext['default'],
      '#description' => t('If the argument is missing or is not valid, select how this should behave.'),
    );
  
    $form['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Title'),
      '#default_value' => $ctext['title'],
      '#description' => t('Enter a title to use when this argument is present. You may use %KEYWORD substitution, where the keyword is specified below.'),
    );
  }

  $form['identifier'] = array(
    '#type' => 'textfield',
    '#title' => t('Identifier'),
    '#description' => t('Enter a name to identify this !type on administrative screens.', array('!type' => t('context'))),
    '#default_value' => $ctext['identifier'],
  );

  $form['keyword'] = array(
    '#type' => 'textfield',
    '#title' => t('Keyword'),
    '#description' => t('Enter a keyword to use for substitution in titles.'),
    '#default_value' => $ctext['keyword'],
  );
  return $form;
}

/**
 * Ajax entry point to edit an item
 */
function ctools_context_ajax_item_delete($module = NULL, $type = NULL, $object_name = NULL, $position = NULL) {
  ctools_include('ajax');
  ctools_include('context');
  ctools_include('object-cache');

  if (!isset($position)) {
    return ctools_ajax_render_error();
  }

  // Load stored object from cache.
  if (!($object = ctools_context_cache_get($module, $object_name))) {
    ctools_ajax_render_error(t('Invalid object name.'));
  }

  $type_info = ctools_context_info($type);

  // Create a reference to the place our context lives. Since this is fairly
  // generic, this is the easiest way to get right to the place of the
  // object without knowing precisely what data we're poking at.
  $ref = &$object->{$type_info['key']};

  if (!array_key_exists($position, $ref)) {
    ctools_ajax_render_error(t('Unable to delete missing item!'));
  }

  unset($ref[$position]);
  ctools_context_cache_set($module, $object_name, $object);

  $output   = array();
  $output[] = ajax_command_replace('#' . $type . '-row-' . $position, '');
  $output[] = ctools_ajax_command_restripe("#$type-table");
  print ajax_render($output);
  exit;
}

// --- End of contexts

function ctools_save_context($type, &$ref, $form_values) {
  $type_info = ctools_context_info($type);

  // Organize arguments
  $new = array();
  $order = array();

  foreach ($ref as $id => $context) {
    $position = $form_values[$type][$id]['position'];
    $order[$position] = $id;
  }

  ksort($order);
  foreach ($order as $id) {
    $new[] = $ref[$id];
  }
  $ref = $new;
}

function ctools_get_keyword($page, $word) {
  // Create a complete set of keywords
  $keywords = array();
  foreach (array('arguments', 'relationships', 'contexts', 'requiredcontexts') as $type) {
    if (!empty($page->$type) && is_array($page->$type)) {
      foreach ($page->$type as $info) {
        $keywords[$info['keyword']] = TRUE;
      }
    }
  }

  $keyword = $word;
  $count = 0;
  while (!empty($keywords[$keyword])) {
    $keyword = $word . '_' . ++$count;
  }
  return $keyword;
}


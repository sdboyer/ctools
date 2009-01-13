<?php
// $Id$

/**
 * @file
 *
 * Contains code related to the ctools system of 'context'.
 *
 * Context, originally from Panels, is a method of packaging objects into
 * a more generic bundle and providing a plugin system so that a UI can
 * take advantage of them. The idea is that the context objects
 * represent 'the context' that a given operation (usually a page view)
 * is operating in or on.
 *
 * For example, when viewing a page, the 'context' is a node object. When
 * viewing a user, the 'context' is a user object. Contexs can also
 * have related contexts. For example, when viewing a 'node' you may need
 * to know something about the node author. Therefore, the node author
 * is a related context.
 */

/**
 * The context object is largely a wrapper around some other object, with
 * an interface to finding out what is contained and getting to both
 * the object and information about the object.
 *
 * Each context object has its own information, but some things are very
 * common, such as titles, data, keywords, etc. In particulare, the 'type'
 * of the context is important.
 */
class ctools_context {
  var $type = NULL;
  var $data = NULL;
  // The title of this object.
  var $title = '';
  // The title of the page if this object exists
  var $page_title = '';
  // The identifier (in the UI) of this object
  var $identifier = '';
  var $argument = NULL;
  var $keyword = '';
  var $original_argument = NULL;

  function ctools_context($type = 'none', $data = NULL) {
    $this->type  = $type;
    $this->data  = $data;
    $this->title = t('Unknown context');
  }

  function is_type($type) {
    if ($type == 'any' || $this->type == 'any') {
      return TRUE;
    }

    $a = is_array($type) ? $type : array($type);
    $b = is_array($this->type) ? $this->type : array($this->type);
    return (bool) array_intersect($a, $b);
  }

  function get_argument() {
    return $this->argument;
  }

  function get_original_argument() {
    if (!is_null($this->original_argument)) {
      return $this->original_argument;
    }
    return $this->argument;
  }

  function get_keyword() {
    return $this->keyword;
  }

  function get_identifier() {
    return $this->identifier;
  }

  function get_title() {
    return $this->title;
  }

  function get_page_title() {
    return $this->page_title;
  }
}

/**
 * Used to create a method of comparing if a list of contexts
 * match a required context type.
 */
class ctools_context_required {
  var $keywords = '';

  /**
   * If set, the title will be used in the selector to identify
   * the context. This is very useful when multiple contexts
   * are required to inform the user will be used for what.
   */
  var $title = NULL;

  /**
   * @param $title
   *   The first parameter should be the 'title' of the context for use
   *   in UYI selectors when multiple contexts qualify.
   * @param ...
   *   One or more keywords to use for matching which contexts are allowed.
   */
  function ctools_context_required($title) {
    $args = func_get_args();
    $this->title = array_shift($args);
    if (count($args) == 1) {
      $args = array_shift($args);
    }
    $this->keywords = $args;
  }

  function filter($contexts) {
    $result = array();

    // See which of these contexts are valid
    foreach ((array) $contexts as $cid => $context) {
      if ($context->is_type($this->keywords)) {
        $result[$cid] = $context;
      }
    }

    return $result;
  }

  function select($contexts, $context) {
    if (empty($context) || empty($contexts[$context])) {
      return FALSE;
    }
    return $contexts[$context];
  }
}

/**
 * Used to compare to see if a list of contexts match an optional context. This
 * can produce empty contexts to use as placeholders.
 */
class ctools_context_optional extends ctools_context_required {
  function ctools_context_optional() {
    $args = func_get_args();
    call_user_func_array(array($this, 'ctools_context_required'), $args);
  }

  /**
   * Add the 'empty' context which is possible for optional
   */
  function add_empty(&$contexts) {
    $context = new ctools_context('any');
    $context->title      = t('No context');
    $context->identifier = t('No context');
    $contexts = array_merge(array('empty' => $context), $contexts);
  }

  function filter($contexts) {
    $this->add_empty($contexts);
    return parent::filter($contexts);
  }

  function select($contexts, $context) {
    $this->add_empty($contexts);
    if (empty($context)) {
      return $contexts['empty'];
    }

    $result = parent::select($contexts, $context);

    // Don't flip out if it can't find the context; this is optional, put
    // in an empty.
    if ($result == FALSE) {
      $result = $contexts['empty'];
    }
    return $result;
  }
}

/**
 * Return a keyed array of context that match the given 'required context'
 * filters.
 *
 * Functions or systems that require contexts of a particular type provide a
 * ctools_context_required or ctools_context_optional object. This function
 * examines that object and an array of contexts to determine which contexts
 * match the filter.
 *
 * Since multiple contexts can be required, this function will accept either
 * an array of all required contexts, or just a single required context object.
 *
 * @param $contexts
 *   A keyed array of all available contexts.
 * @param $required
 *   A ctools_context_required or ctools_context_optional object, or an array
 *   of such objects.
 *
 * @return
 *   A keyed array of contexts that match the filter.
 */
function ctools_context_filter($contexts, $required) {
  if (is_array($required)) {
    $result = array();
    foreach ($required as $r) {
      $result = array_merge($result, _ctools_context_filter($contexts, $r));
    }
    return $result;
  }

  return _ctools_context_filter($contexts, $required);
}

function _ctools_context_filter($contexts, $required) {
  $result = array();

  if (is_object($required)) {
    $result = $required->filter($contexts);
  }

  return $result;
}

/**
 * Create a select box to choose possible contexts.
 *
 * This only creates a selector if there is actually a choice; if there
 * is only one possible context, that one is silently assigned.
 *
 * If an array of required contexts is provided, one selector will be
 * provided for each context.
 *
 * @param $contexts
 *   A keyed array of all available contexts.
 * @param $required
 *   The required context object or array of objects.
 *
 * @return
 *   A form element, or NULL if there are no contexts that satisfy the
 *   requirements.
 */
function ctools_context_selector($contexts, $required, $default) {
  if (is_array($required)) {
    $result = array('#tree' => TRUE);
    $count = 1;
    foreach ($required as $id => $r) {
      $result[] = _ctools_context_selector($contexts, $r, $default[$id], $count++);
    }
    return $result;
  }

  return _ctools_context_selector($contexts, $required, $default);
}

function _ctools_context_selector($contexts, $required, $default, $num = 0) {
  $filtered = ctools_context_filter($contexts, $required);
  $count = count($filtered);

  $form = array();

  if ($count == 1) {
    $keys = array_keys($filtered);
    return array(
      '#type' => 'value',
      '#value' => $keys[0],
    );
  }

  if ($count > 1) {
    // If there's more than one to choose from, create a select widget.
    foreach ($filtered as $cid => $context) {
      $options[$cid] = $context->get_identifier();
    }
    if (!empty($required->title)) {
      $title = $required->title;
    }
    else {
      $title = $num ? t('Context %count', array('%count' => $num)) : t('Context');
    }

    return array(
      '#type' => 'select',
      '#options' => $options,
      '#title' => $title,
      '#description' => t('Multiple contexts are valid for this; one must be chosen.'),
      '#default_value' => $default,
    );
  }
}

/**
 * Choose a context or contexts based upon the selection made via
 * ctools_context_filter.
 *
 * @param $contexts
 *   A keyed array of all available contexts
 * @param $required
 *   The required context object provided by the plugin
 * @param $context
 *   The selection made using ctools_context_selector
 */
function ctools_context_select($contexts, $required, $context) {
  if (is_array($required)) {
    $result = array();
    foreach ($required as $id => $r) {
      if (($result[] = _ctools_context_select($contexts, $r, $context[$id])) == FALSE) {
        return FALSE;
      }
    }
    return $result;
  }

  return _ctools_context_select($contexts, $required, $context);
}

function _ctools_context_select($contexts, $required, $context) {
  if (!is_object($required)) {
    return FALSE;
  }

  return $required->select($contexts, $context);
}

/**
 * Create a new context object.
 *
 * @param $type
 *   The type of context to create; this loads a plugin.
 * @param $data
 *   The data to put into the context.
 * @param $empty
 *   Whether or not this context is specifically empty.
 * @param $conf
 *   A configuration structure if this context was created via UI.
 *
 * @return
 *   A $context or NULL if one could not be created.
 */
function ctools_context_create($type, $data = NULL, $conf = FALSE) {
  if ($function = ctools_plugin_load_function('ctools', 'contexts', $type, 'context')) {
    return $function(FALSE, $data, $conf);
  }
}

/**
 * Create an empty context object.
 *
 * Empty context objects are primarily used as placeholders in the UI where
 * the actual contents of a context object may not be known. It may have
 * additional text embedded to give the user clues as to how the context
 * is used.
 *
 * @param $type
 *   The type of context to create; this loads a plugin.
 *
 * @return
 *   A $context or NULL if one could not be created.
 */
function ctools_context_create_empty($type) {
  if ($function = ctools_plugin_load_function('ctools', 'contexts', $type, 'context')) {
    return $function(TRUE);
  }
}

/**
 * Fetch keywords for use in string substitutions.
 *
 * @param $contexts
 *   An array of contexts.
 *
 * @return
 *   An array of keyword substitutions suitable for @code{strtr()}
 */
function ctools_context_get_keywords($contexts) {
  $keywords = array();
  if (!empty($contexts)) {
    foreach ($contexts as $id => $context) {
      if ($keyword = $context->get_keyword()) {
        $keywords["%$keyword"] = $context->get_title();
      }
    }
  }
  return $keywords;
}

/**
 * Determine a unique context ID for a context
 *
 * Often contexts of many different types will be placed into a list. This
 * ensures that even though contexts of multiple types may share IDs, they
 * are unique in the final list.
 */
function ctools_context_id($context, $type = 'context') {
  return $type . '_' . $context['name'] . '_' . $context['id'];
}

/**
 * Get the next id available given a list of already existing objects.
 *
 * This finds the next id available for the named object.
 *
 * @param $objects
 *   A list of context descriptor objects, i.e, arguments, relationships, contexts, etc.
 * @param $name
 *   The name being used.
 */
function ctools_context_next_id($objects, $name) {
  // Figure out which instance of this argument we're creating
  $id = 0;
  if (!$objects) {
    return $id;
  }

  foreach ($objects as $object) {
    if (isset($object['name']) && $object['name'] == $name) {
      if ($object['id'] > $id) {
        $id = $object['id'];
      }
    }
  }
  return $id;
}


// ---------------------------------------------------------------------------
// Functions related to contexts from arguments.

/**
 * Fetch metadata on a specific argument plugin.
 *
 * @param $argument
 *   Name of an argument plugin.
 *
 * @return
 *   An array with information about the requested argument plugin.
 */
function ctools_get_argument($argument) {
  ctools_include('plugins');
  return ctools_get_plugins('ctools', 'arguments', $argument);
}

/**
 * Fetch metadata for all argument plugins.
 *
 * @return
 *   An array of arrays with information about all available argument plugins.
 */
function ctools_get_arguments() {
  ctools_include('plugins');
  return ctools_get_plugins('ctools', 'arguments');
}

/**
 * Get a context from an argument.
 *
 * @param $argument
 *   The configuration of an argument. It must contain the following data:
 *   - name: The name of the argument plugin being used.
 *   - argument_settings: The configuration based upon the plugin forms.
 *   - identifier: The human readable identifier for this argument, usually
 *     defined by the UI.
 *   - keyword: The keyword used for this argument for substitutions.
 *
 * @param $arg
 *   The actual argument received. This is expected to be a string from a URL but
 *   this does not have to be the only source of arguments.
 * @param $empty
 *   If true, the $arg will not be used to load the context. Instead, an empty
 *   placeholder context will be loaded.
 *
 * @return
 *   A context object if one can be loaded.
 */
function ctools_context_get_context_from_argument($argument, $arg, $empty = FALSE) {
  ctools_include('plugins');
  if ($function = ctools_plugin_load_function('ctools', 'arguments', $argument['name'], 'context')) {
    if (!isset($argument['settings'])) {
      $argument['settings'] = array();
    }

    $context = $function($arg, $argument['settings'], $empty);

    if (is_object($context)) {
      $context->identifier = $argument['identifier'];
      $context->page_title = isset($argument['title']) ? $argument['title'] : '';
      $context->keyword    = $argument['keyword'];
      $context->id         = ctools_context_id($argument, 'argument');
      $context->original_argument = $arg;
    }
    return $context;
  }
}

/**
 * Retrieve a list of empty contexts for all arguments.
 */
function ctools_context_get_placeholders_from_argument($arguments) {
  $contexts = array();
  foreach ($arguments as $argument) {
    $context = ctools_context_get_context_from_argument($argument, NULL, TRUE);
    if ($context) {
      $contexts[ctools_context_id($argument, 'argument')] = $context;
    }
  }
  return $contexts;
}

/**
 * Load the contexts for a given list of arguments.
 *
 * @param $arguments
 *   The array of argument definitions.
 * @param &$contexts
 *   The array of existing contexts. New contexts will be added to this array.
 * @param $args
 *   The arguments to load.
 *
 * @return
 *   FALSE if an argument wants to 404.
 */
function ctools_context_get_context_from_arguments($arguments, &$contexts, $args) {
  foreach ($arguments as $argument) {
    // pull the argument off the list.
    $arg = array_shift($args);
    $id = ctools_context_id($argument, 'argument');

    // For % arguments embedded in the URL, our context is already loaded.
    // There is no need to go and load it again.
    if (empty($contexts[$id])) {
      if ($context = ctools_context_get_context_from_argument($argument, $arg)) {
        $contexts[$id] = $context;
      }
    }
    else {
      $context = $contexts[$id];
    }

    if ((empty($context) || empty($context->data)) && $argument['default'] == '404') {
      return FALSE;
    }
  }
  return TRUE;
}

// ---------------------------------------------------------------------------
// Functions related to contexts from relationships.

/**
 * Fetch metadata on a specific relationship plugin.
 *
 * @param $content type
 *   Name of a panel content type.
 *
 * @return
 *   An array with information about the requested relationship.
 */
function ctools_get_relationship($relationship) {
  ctools_include('plugins');
  return ctools_get_plugins('ctools', 'relationships', $relationship);
}

/**
 * Fetch metadata for all relationship plugins.
 *
 * @return
 *   An array of arrays with information about all available relationships.
 */
function ctools_get_relationships() {
  ctools_include('plugins');
  return ctools_get_plugins('ctools', 'relationships');
}

/**
 * @param $relationship
 *   The configuration of a relationship. It must contain the following data:
 *   - name: The name of the relationship plugin being used.
 *   - relationship_settings: The configuration based upon the plugin forms.
 *   - identifier: The human readable identifier for this relationship, usually
 *     defined by the UI.
 *   - keyword: The keyword used for this relationship for substitutions.
 * @param $source_context
 *   The context this relationship is based upon.
 *
 * @return
 *   A context object if one can be loaded.
 */
function ctools_context_get_context_from_relationship($relationship, $source_context) {
  ctools_include('plugins');
  if ($function = ctools_plugin_load_function('ctools', 'relationships', $relationship['name'], 'context')) {
    if (!isset($relationship['relationship_settings'])) {
      $relationship['relationship_settings'] = array();
    }

    $context = $function($source_context, $relationship['relationship_settings']);
    if ($context) {
      $context->identifier = $relationship['identifier'];
      $context->page_title = isset($relationship['title']) ? $relationship['title'] : '';
      $context->keyword    = $relationship['keyword'];
      return $context;
    }
  }
}

/**
 * Fetch all relevant relationships.
 *
 * Relevant relationships are any relationship that can be created based upon
 * the list of existing contexts. For example, the 'node author' relationship
 * is relevant if there is a 'node' context, but makes no sense if there is
 * not one.
 *
 * @param $contexts
 *   An array of contexts used to figure out which relationships are relevant.
 *
 * @return
 *   An array of relationship keys that are relevant for the given set of
 *   contexts.
 */
function ctools_context_get_relevant_relationships($contexts) {
  $relevant = array();
  $relationships = ctools_get_relationships();

  // Go through each relationship
  foreach ($relationships as $rid => $relationship) {
    // For each relationship, see if there is a context that satisfies it.
    if (ctools_context_filter($contexts, $relationship['required context'])) {
      $relevant[$rid] = $relationship['title'];
    }
  }

  return $relevant;
}

/**
 * Fetch all active relationships
 *
 * @param $relationships
 *   An keyed array of relationship data including:
 *   - name: name of relationship
 *   - context: context id relationship belongs to. This will be used to
 *     identify which context in the $contexts array to use to create the
 *     relationship context.
 * @param $contexts
 *   A keyed array of contexts used to figure out which relationships
 *   are relevant. New contexts will be added to this.
 */
function ctools_context_get_context_from_relationships($relationships, &$contexts) {
  $return = array();

  foreach ($relationships as $rdata) {
    if (!isset($rdata['context'])) {
      continue;
    }

    if (empty($contexts[$rdata['context']])) {
      continue;
    }
    /*
    $relationship = ctools_context_get_context_from_relationship($rdata['name']);
    // If the relationship can't be found or its context can't be found,
    // ignore.
    if (!$relationship) {
      continue;
    }
    */

    $cid = ctools_context_id($rdata, 'relationship');
    if ($context = ctools_context_get_context_from_relationship($rdata, $contexts[$rdata['context']])) {
      $contexts[$cid] = $context;
    }
  }
}

// ---------------------------------------------------------------------------
// Functions related to loading contexts from simple context definitions.

/**
 * Fetch metadata on a specific context plugin.
 *
 * @param $context
 *   Name of a context.
 *
 * @return
 *   An array with information about the requested panel context.
 */
function ctools_get_context($context) {
  ctools_include('plugins');
  return ctools_get_plugins('ctools', 'contexts', $context);
}

/**
 * Fetch metadata for all context plugins.
 *
 * @return
 *   An array of arrays with information about all available panel contexts.
 */
function ctools_get_contexts() {
  ctools_include('plugins');
  return ctools_get_plugins('ctools', 'contexts');
}

/**
 * @param $context
 *   The configuration of a context. It must contain the following data:
 *   - name: The name of the context plugin being used.
 *   - context_settings: The configuration based upon the plugin forms.
 *   - identifier: The human readable identifier for this context, usually
 *     defined by the UI.
 *   - keyword: The keyword used for this context for substitutions.
 * @param $type
 *   This is either 'context' which indicates the context will be loaded
 *   from data in the settings, or 'required_context' which means the
 *   context must be acquired from an external source. This is the method
 *   used to pass pure contexts from one system to another.
 *
 * @return
 *   A context object if one can be loaded.
 */
function ctools_context_get_context_from_context($context, $type = 'context') {
  ctools_include('plugins');
  if ($function = ctools_plugin_load_function('ctools', 'contexts', $context['name'], 'context')) {
    if (!isset($context['context_settings'])) {
      $context['context_settings'] = array();
    }

    $return = $function($type == 'requiredcontext', $context['context_settings'], TRUE);
    if ($return) {
      $return->identifier = $context['identifier'];
      $return->page_title = isset($context['title']) ? $context['title'] : '';
      $return->keyword    = $context['keyword'];
      return $return;
    }
  }
}

/**
 * Retrieve a list of base contexts based upon a simple 'contexts' definition.
 *
 * For required contexts this will always retrieve placeholders.
 *
 * @param $contexts
 *   The list of contexts defined in the UI.
 * @param $type
 *   Either 'context' or 'requiredcontext', which indicates whether the contexts
 *   are loaded from internal data or copied from an external source.
 */
function ctools_context_get_context_from_contexts($contexts, $type = 'context') {
  $return = array();
  foreach ($contexts as $context) {
    $ctext = ctools_context_get_context_from_context($context, $type);
    if ($ctext) {
      $return[ctools_context_id($context, $type)] = $ctext;
    }
  }
  return $return;
}

/**
 * Match up external contexts to our required contexts.
 *
 * This function is used to create a list of contexts with proper
 * IDs based upon a list of required contexts.
 *
 * These contexts passed in should match the numeric positions of the
 * required contexts. The caller must ensure this has already happened
 * correctly as this function will not detect errors here.
 *
 * @param $required
 *   A list of required contexts as defined by the UI.
 * @param $contexts
 *   A list of matching contexts as passed in from the calling system.
 */
function ctools_context_match_required_contexts($required, $contexts) {
  $return = array();
  if (!is_array($required)) {
    return $return;
  }

  foreach ($required as $r) {
    $return[ctools_context_id($r, 'requiredcontext')] = array_shift($contexts);
  }

  return $return;
}

/**
 * Load a full array of contexts for an object.
 *
 * Not all of the types need to be supported by this object.
 *
 * This function is not used to load contexts from external data, but may
 * be used to load internal contexts and relationships. Otherwise it can also
 * be used to generate a full set of placeholders for UI purposes.
 *
 * @param $object
 *   An object that contains some or all of the following variables:
 *
 * - requiredcontexts: A list of UI configured contexts that are required
 *   from an external source. Since these require external data, they will
 *   only be added if $placeholders is set to TRUE, and empty contexts will
 *   be created.
 * - arguments: A list of UI configured arguments that will create contexts.
 *   Since these require external data, they will only be added if $placeholders
 *   is set to TRUE.
 * - contexts: A list of UI configured contexts that have no external source,
 *   and are essentially hardcoded. For example, these might configure a
 *   particular node or a particular taxonomy term.
 * - relationships: A list of UI configured contexts to be derived from other
 *   contexts that already exist from other sources. For example, these might
 *   be used to get a user object from a node via the node author relationship.
 * @param $placeholders
 *   If TRUE, this will generate placeholder objects for types this function
 *   cannot load.
 * @param $contexts
 *   An array of pre-existing contexts that will be part of the return value.
 */
function ctools_context_load_contexts($object, $placeholders = TRUE, $contexts = array()) {
  if ($placeholders) {
    // This will load empty contexts as placeholders for arguments that come
    // from external sources. If this isn't set, it's assumed these context
    // will already have been matched up and loaded.
    if (!empty($object->requiredcontexts) && is_array($object->requiredcontexts)) {
      $contexts += ctools_context_get_context_from_contexts($object->requiredcontexts, 'requiredcontext');
    }

    if (!empty($object->arguments) && is_array($object->arguments)) {
      $contexts += ctools_context_get_placeholders_from_argument($object->arguments);
    }
  }

  if (!empty($object->contexts) && is_array($object->contexts)) {
    $contexts += ctools_context_get_context_from_contexts($object->contexts);
  }

  // add contexts from relationships
  if (!empty($object->relationships) && is_array($object->relationships)) {
    ctools_context_get_context_from_relationships($object->relationships, $contexts);
  }

  return $contexts;
}

/**
 * Return the first context with a form id from a list of contexts.
 *
 * This function is used to figure out which contexts represents 'the form'
 * from a list of contexts. Only one contexts can actually be 'the form' for
 * a given page, since the @code{<form>} tag can not be embedded within
 * itself.
 */
function ctools_context_get_form($contexts) {
  if (!empty($contexts)) {
    foreach ($contexts as $context) {
      if (!empty($context->form_id)) {
        return $context;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Functions related to loading access control plugins

/**
 * Fetch metadata on a specific access control plugin.
 *
 * @param $name
 *   Name of a plugin.
 *
 * @return
 *   An array with information about the requested access control plugin.
 */
function ctools_get_access_plugin($name) {
  ctools_include('plugins');
  return ctools_get_plugins('ctools', 'access', $name);
}

/**
 * Fetch metadata for all access control plugins.
 *
 * @return
 *   An array of arrays with information about all available access control plugins.
 */
function ctools_get_access_plugins() {
  ctools_include('plugins');
  return ctools_get_plugins('ctools', 'access');
}

/**
 * Fetch a list of access plugins that are available for a given list of
 * contexts.
 *
 * if 'logged-in-user' is not in the list of contexts, it will be added as
 * this is required.
 */
function ctools_get_relevant_access_plugins($contexts) {
  if (!isset($contexts['logged-in-user'])) {
    $contexts['logged-in-user'] = ctools_access_get_loggedin_context();
  }

  $all_plugins = ctools_get_access_plugins();
  $plugins = array();
  foreach ($all_plugins as $id => $plugin) {
    if (!empty($plugin['required context']) && !ctools_context_filter($contexts, $plugin['required context'])) {
      continue;
    }
    $plugins[$id] = $plugin;
  }

  return $plugins;
}

/**
 * Create a context for the logged in user.
 */
function ctools_access_get_loggedin_context() {
  global $user;
  $context = ctools_context_create('user', $user);
  $context->identifier = t('Logged in user');
  $context->keyword = 'viewer';
  $context->id = 0;

  return $context;
}

/**
 * Get a summary of an access plugin's settings.
 */
function ctools_access_summary($plugin, $contexts, $test) {
  if (!isset($contexts['logged-in-user'])) {
    $contexts['logged-in-user'] = ctools_access_get_loggedin_context();
  }

  $description = '';
  if ($function = ctools_plugin_get_function($plugin, 'summary')) {
    $required_context = isset($plugin['required context']) ? $plugin['required context'] : array();
    $context = isset($test['context']) ? $test['context'] : array();
    $description = $function($test['settings'], ctools_context_select($contexts, $required_context, $context));
  }
  return $description;
}

/**
 * Determine if the current user has access via  plugin.
 *
 * @param $plugin_name
 *   The access plugin to use. If empty or 'none' then no access control
 *   is being used and this function returns TRUE. If the plugin can't be
 *   found otherwise, this function automatically returns FALSE.
 * @param $settings
 *   An array of settings theoretically set by the user.
 * @param $contexts
 *   An array of zero or more contexts that may be used to determine if
 *   the user has access.
 * @param $account
 *   The account to test against. If NULL the logged in user will be tested.
 *
 * @return
 *   TRUE if access is granted, false if otherwise.
 */
function ctools_access($settings, $contexts = array()) {
  if (empty($settings['plugins'])) {
    return TRUE;
  }

  if (!isset($contexts['logged-in-user'])) {
    $contexts['logged-in-user'] = ctools_access_get_loggedin_context();
  }

  foreach ($settings['plugins'] as $test) {
    $pass = FALSE;
    $plugin = ctools_get_access_plugin($test['name']);
    if ($plugin && $function = ctools_plugin_get_function($plugin, 'callback')) {
    $required_context = isset($plugin['required context']) ? $plugin['required context'] : array();
    $context = isset($test['context']) ? $test['context'] : array();
      $pass = $function($test['settings'], ctools_context_select($contexts, $required_context, $context));
    }

    if ($pass && $settings['logic'] == 'or') {
      return TRUE;
    }
    else if (!$pass && $settings['logic'] == 'and') {
      return FALSE;
    }
  }

  return TRUE;
}

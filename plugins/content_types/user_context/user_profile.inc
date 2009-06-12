<?php
// $Id$

/**
 * Callback function to supply a list of content types.
 */
function ctools_user_profile_ctools_content_types() {
  return array(
    'single' => TRUE,
    'title' => t('User profile'),
    'icon' => 'icon_user.png',
    'description' => t('The profile of a user.'),
    'required context' => new ctools_context_required(t('User'), 'user'),
    'category' => t('User'),
  );
}

function ctools_user_profile_content_type_render($subtype, $conf, $panel_args, $context) {
  $account = isset($context->data) ? drupal_clone($context->data) : NULL;
  $block = new stdClass();
  $block->module = 'user-profile';

  if (!$account || ($account->access == 0 && !user_access('administer users'))) {
    return NULL;
  }
  // Retrieve and merge all profile fields:
  $fields = array();
  foreach (module_list() as $module) {
    if ($data = module_invoke($module, 'user', 'view', '', $account)) {
      foreach ($data as $category => $items) {
        if (is_array($items)) {
          foreach ($items as $key => $item) {
            $item['class'] = "$module-". $item['class'];
            $fields[$category][$key] = $item;
          }
        }
      }
    }
  }

  drupal_alter('profile', $account);

  $block->title = check_plain($account->name);
  $block->content = theme('user_profile', $account, $fields);

  return $block;
}


/**
 * Display the administrative title for a panel pane in the drag & drop UI
 */
function ctools_user_profile_content_type_admin_title($subtype, $conf, $context) {
  return t('"@s" user profile', array('@s' => $context->identifier));
}

function ctools_user_profile_content_type_edit_form(&$form, &$form_state) {
  // provide a blank form so we have a place to have context setting.
}


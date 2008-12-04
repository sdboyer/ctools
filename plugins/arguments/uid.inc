<?php
// $Id$

/**
 * @file
 *
 * Plugin to provide an argument handler for a user id
 */

/**
 * Implementation of specially named hook_ctools_arguments().
 */
function ctools_uid_ctools_arguments() {
  $args['uid'] = array(
    'title' => t("User ID"),
    // keyword to use for %substitution
    'keyword' => 'user',
    'description' => t('Creates a user object from the argument.'),
    'context' => 'ctools_uid_context',
  );
  return $args;
}

/**
 * Discover if this argument gives us the user we crave.
 */
function ctools_uid_context($arg = NULL, $conf = NULL, $empty = FALSE) {
  // If unset it wants a generic, unfilled context.
  if ($empty) {
    return ctools_context_create_empty('user');
  }

  if (!is_numeric($arg)) {
    return NULL;
  }

  $account = user_load(array('uid' => $arg));
  if (!$account) {
    return NULL;
  }

  return ctools_context_create('user', $account);
}

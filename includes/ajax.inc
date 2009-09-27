<?php
// $Id$

/**
 * @file
 * Utilize the CTools AJAX responder.
 *
 */

/**
 * Render an image as a button link. This will automatically apply an AJAX class
 * to the link and add the appropriate javascript to make this happen.
 *
 * @param $image
 *   The path to an image to use that will be sent to theme('image') for rendering.
 * @param $dest
 *   The destination of the link.
 * @param $alt
 *   The alt text of the link.
 * @param $class
 *   Any class to apply to the link. @todo this should be a options array.
 */
function ctools_ajax_image_button($image, $dest, $alt, $class = '') {
  return ctools_ajax_text_button(theme('image', $image), $dest, $alt, $class);
}

/**
 * Render text as a link. This will automatically apply an AJAX class
 * to the link and add the appropriate javascript to make this happen.
 *
 * Note: 'html' => true so be sure any text is vetted! Chances are these kinds of buttons will
 * not use user input so this is a very minor concern.
 *
 * @param $image
 *   The path to an image to use that will be sent to theme('image') for rendering.
 * @param $dest
 *   The destination of the link.
 * @param $alt
 *   The alt text of the link.
 * @param $class
 *   Any class to apply to the link. @todo this should be a options array.
 * @param $type
 *   A type to use, in case a different behavior should be attached. Defaults
 *   to use-ajax.
 */
function ctools_ajax_text_button($text, $dest, $alt, $class = '', $type = 'use-ajax') {
  return l($text, $dest, array('html' => TRUE, 'attributes' => array('class' => "$type $class", 'title' => $alt)));
}

/**
 * Set a single property to a value, on all matched elements.
 *
 * @param $selector
 *   The CSS selector. This can be any selector jquery uses in $().
 * @param $name
 *   The name or key: of the data attached to this selector.
 * @param $value
 *  The value of the data.
 */
function ctools_ajax_command_attr($selector, $name, $value) {
  return array(
    'command' => 'attr',
    'selector' => $selector,
    'name' => $name,
    'value' => $value,
  );
}

/**
 * Force a client-side redirect.
 *
 * @param $url
 *   The url to be redirected to. This can be an absolute URL or a
 *   Drupal path.
 */
function ctools_ajax_command_redirect($url) {
  return array(
    'command' => 'redirect',
    'url' => url($url),
  );
}

/**
 * Force a reload of the current page.
 */
function ctools_ajax_command_reload() {
  return array(
    'command' => 'reload',
  );
}
/**
 * Associate a URL to a form element with a hidden form.
 *
 * This is a helper function to easily associate a URL with a form element
 * which can be used for different ajax functionality.
 *
 * You would call this function on a form element in the form function like this:
 *
 * @code
 *   $form['example'] = array(
 *     '#title' => t('Example'),
 *     '#type' => 'select',
 *     '#options' => array(1 => 'One', 2 => 'Two', 3 => 'Three'),
 *     '#default_value' => 1,
 *   );
 *   ctools_ajax_associate_url_to_element($form, $form['example'], 'example/ajax/urlpath');
 * @endcode
 *
 * The AJAX request will POST the value of the form element in the "ctools_changed" parameter.
 *
 * @param &$form
 *   Reference to the form element. This is required to have the #id and #attribute
 *   elements populated and to create the hidden form element for each select.
 * @param &$form_element
 *   The form element we are going to take action on.
 * @param $dest
 *   The URL to associate the form element to.
 * @param $type
 *   A type to use, in case a different behavior should be attached. Defaults
 *   to ctools-use-ajax.
 */
function ctools_ajax_associate_url_to_element(&$form, &$form_element, $dest, $type = 'ctools-use-ajax') {
  drupal_add_js('misc/jquery.form.js', 'core');
  if (!isset($form_element['#id'])) {
    //Create a unique ID to associate $form_element and hidden elements since we dont have an ID
    $form_element['#id'] = uniqid('ctools-ajax-url-');

    $form_element['#attributes']['class'] = $type;
  }

  //Add hidden form element to hold base URL
  $form[$form_element['#id'] . '-url'] = array(
    '#type' => 'hidden',
    '#value' => $dest,
    '#attributes' => array('class' =>  $form_element['#id'] . '-url'),
  );
}

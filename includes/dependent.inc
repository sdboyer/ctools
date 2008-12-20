<?php
// $Id$

/**
 * @file
 * Provide dependent checkboxes that can be easily used in forms.
 *
 * This system will ensure that form items are invisible if the dependency is
 * not met. What this means is that you set the #dependency of an item to a
 * list of form ids that must be set, and the list of values that qualify.
 *
 * For a simple use, setting an item to be dependent upon a select box, if
 * any of the listed values are selected, the item will be visible. Otherwise,
 * the item will be visible.
 *
 * If dependent upon multiple items, use #dependency_count = X to set the
 * number of items that must be set in order to make this item visible. This
 * defaults to 1. If set to 2, then at least 2 form items in the list must
 * have their items set for the item to become visible.
 *
 * Checkboxes don't have their own id, so you need to add one in a div
 * around the checkboxes via #prefix and #suffix. You actually need to add TWO
 * divs because it's the parent that gets hidden. Also be sure to retain the
 * 'expand_checkboxes' in the #process array, because the views process will
 * override it.
 *
 * For radios, because they are selected a little bit differently, instead of
 * using the CSS id, use: radio:NAME where NAME is the #name of the property.
 * This can be quickly found by looking at the HTML of the generated form, but
 * it is usually derived from the array which contains the item. For example,
 * $form['menu']['type'] would have a name of menu[type]. This name is the same
 * field that is used to determine where in $form_state['values'] you will find
 * the value of the form.
 *
 * Usage:
 *
 * First, ensure this tool is loaded:
 * @code { ctools_include('dependent'); }
 *
 * On any form item, add
 * - @code '#process' => array('ctools_dependent_process'), @endcode
 * - @code '#dependency' => array('id-of-form-without-the-#' => array(list, of, values, that, make, this, gadget, visible)), @endcode
 */

/**
 * Process callback to add dependency to form items.
 */
function ctools_dependent_process($element, $edit, &$form_state, &$form) {
  if (isset($element['#dependency'])) {
    if (!isset($element['#dependency_count'])) {
      $element['#dependency_count'] = 1;
    }
    if (!empty($form_state['ajax'])) {
      $form_state['js settings']['CTools']['dependent'][$element['#id']] = array('num' => $element['#dependency_count'], 'values' => $element['#dependency']);
    }
    else {
      ctools_add_js('dependent');
      $options['CTools']['dependent'][$element['#id']] = array('num' => $element['#dependency_count'], 'values' => $element['#dependency']);
      drupal_add_js($options, 'setting');
    }
  }

  return $element;
}

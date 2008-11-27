// $Id$

// All CTools tools begin with this:
if (!Drupal.CTools) {
  Drupal.CTools = {};
}

/**
 * Creating a collapsible div with this doesn't take too much. There are 
 * three classes necessary:
 *
 * - ctools-collapsible-container: This is the overall container that will be
 *   collapsible. This must be a div.
 * - ctools-collapsible-handle: This is the title area, and is what will be
 *   visible when it is collapsed. This can be any block element, such as div 
 *   or h2.
 * - ctools-collapsible-content: This is the ocntent area and will only be
 *   visible when expanded. This must be a div.
 *
 * The div will be 'open' unless the container class has 'ctools-collapsed' as
 * a class, which will cause the container to draw collapsed.
 */

// Set up an array for callbacks.
Drupal.CTools.CollapsibleCallbacks = [];
Drupal.CTools.CollapsibleCallbacksAfterToggle = [];

/**
 * Bind collapsible behavior to a given container.
 */
Drupal.CTools.bindCollapsible = function() {
  var $container = $(this);

  var handle = $container.children('.ctools-collapsible-handle');
  var content = $container.children('div.ctools-collapsible-content');
  if (content.length) {
    // Create the toggle item and place it in front of the toggle.
    var toggle = $('<span class="ctools-toggle"></span>');
    handle.before(toggle);

    // If we should start collapsed, do so:
    if ($container.hasClass('ctools-collapsed')) {
      toggle.toggleClass('ctools-toggle-collapsed');
      content.hide();
    }

    var afterToggle = function() {
      if (Drupal.CTools.CollapsibleCallbacksAfterToggle) {
        for (i in Drupal.CTools.CollapsibleCallbacksAfterToggle) {
          Drupal.CTools.CollapsibleCallbacksAfterToggle[i]($container, handle, content, toggle);
        }
      }
    }
    
    var clickMe = function() {
      if (Drupal.CTools.CollapsibleCallbacks) {
        for (i in Drupal.CTools.CollapsibleCallbacks) {
          Drupal.CTools.CollapsibleCallbacks[i]($container, handle, content, toggle);
        }
      }
      content.slideToggle(100, afterToggle);
      toggle.toggleClass('ctools-toggle-collapsed');
    }

    // Let both the toggle and the handle be clickable.
    toggle.click(clickMe);
    handle.click(clickMe);
  }
};

/**
 * Support Drupal's 'behaviors' system for binding.
 */
Drupal.behaviors.CToolsCollapsible = function(context) {
  $('div.ctools-collapsible-container:not(.ctools-collapsible-processed)', context)
    .each(Drupal.CTools.bindCollapsible)
    .addClass('ctools-collapsible-processed');
}
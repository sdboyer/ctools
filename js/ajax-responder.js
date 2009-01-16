// $Id$
/**
 * @file
 *
 * CTools flexible AJAX responder object.
 */

Drupal.CTools = Drupal.CTools || {};
Drupal.CTools.AJAX = Drupal.CTools.AJAX || {};

/**
 * Success callback for an ajax request.
 *
 * This function expects to receive a packet of data from a JSON object
 * which is essentially a list of commands. Each commands must have a
 * 'command' setting and this setting must resolve to a function in the
 * Drupal.CTools.AJAX.commands space.
 */
Drupal.CTools.AJAX.respond = function(data) {
  for (i in data) {
    if (data[i]['command'] && Drupal.CTools.AJAX.commands[data[i]['command']]) {
      Drupal.CTools.AJAX.commands[data[i]['command']](data[i]);
    }
  }
};

/**
 * Generic replacement click handler to open the modal with the destination
 * specified by the href of the link.
 */
Drupal.CTools.AJAX.clickAJAXLink = function() {
  var url = $(this).attr('href');
  url.replace('/nojs/', '/ajax/');
  $.ajax({
    type: "POST",
    url: url,
    data: '',
    global: true,
    success: Drupal.CTools.AJAX.respond,
    error: function() { 
      alert("An error occurred while attempting to process " + url); 
    },
    dataType: 'json'
  });
  return false;
};

/**
 * Generic replacement click handler to open the modal with the destination
 * specified by the href of the link.
 */
Drupal.CTools.AJAX.clickAJAXButton = function() {
  // @todo -- if no url we should take the form action and submit the
  // form.
  var url = Drupal.CTools.AJAX.findURL(this);
  if (url) {
    url.replace('/nojs/', '/ajax/');
    $.ajax({
      type: "POST",
      url: url,
      data: '',
      global: true,
      success: Drupal.CTools.AJAX.respond,
      error: function() { 
        alert("An error occurred while attempting to process " + url); 
      },
      dataType: 'json'
    });
  }
  else {
    var form = $(this).parents('form');
    url = $(form).attr('action');
    url.replace('/nojs/', '/ajax/');
    $(form).ajaxSubmit({
      type: "POST",
      url: url,
      data: '',
      global: true,
      success: Drupal.CTools.AJAX.respond,
      error: function() { 
        alert("An error occurred while attempting to process " + url); 
      },
      dataType: 'json'
    });
  }
  return false;
};

/**
 * Find a URL for an AJAX button.
 *
 * The URL for this gadget will be composed of the values of items by
 * taking the ID of this item and adding -url and looking for that
 * class. They need to be in the form in order since we will
 * concat them all together using '/'.
 */
Drupal.CTools.AJAX.findURL = function(item) {
  var url = '';
  var url_class = '.' + $(item).attr('id') + '-url';
  $(url_class).each(
    function() { 
      if (url && $(this).val()) { 
        url += '/'; 
      }
      url += $(this).val(); 
    });
  return url;
};

Drupal.CTools.AJAX.commands = {
  append: function(data) {
    $(data.selector).append(data.data);
    Drupal.attachBehaviors($(data.selector));
  },

  replace: function(data) {
    $(data.selector).replaceWith(data.data);
    Drupal.attachBehaviors($(data.selector));
  }, 

  changed: function(data) {
    if (!$(data.selector).hasClass('changed')) {
      $(data.selector).addClass('changed');
      if (data.star) {
        $(data.selector).find(data.star).append(' <span class="star">*</span> ');
      }
    }
  },

  alert: function(data) {
    alert(data.text, data.title);
  }, 

  restripe: function(data) {
    // :even and :odd are reversed because jquery counts from 0 and
    // we count from 1, so we're out of sync.
    $('tbody tr:not(:hidden)', $(data.selector))
      .removeClass('even')
      .removeClass('odd')
      .filter(':even')
        .addClass('odd')
      .end()
      .filter(':odd')
        .addClass('even');
  }
};

/**
 * Bind links that will open modals to the appropriate function.
 */
Drupal.behaviors.CToolsAJAX = function(context) {
  // Bind links
  $('a.ctools-use-ajax:not(.ctools-use-ajax-processed)', context)
    .addClass('ctools-use-ajax-processed')
    .click(Drupal.CTools.AJAX.clickAJAXLink);

  // Bind buttons
  $('input.ctools-use-ajax:not(.ctools-use-ajax-processed)', context)
    .addClass('ctools-use-ajax-processed')
    .click(Drupal.CTools.AJAX.clickAJAXButton);
};

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
  if ($(this).hasClass('ctools-ajaxing')) {
    return false;
  }

  var url = $(this).attr('href');
  var object = $(this);
  $(this).addClass('ctools-ajaxing');
  try {
    url = url.replace('/nojs/', '/ajax/');
    $.ajax({
      type: "POST",
      url: url,
      data: '',
      global: true,
      success: Drupal.CTools.AJAX.respond,
      error: function() { 
        alert("An error occurred while attempting to process " + url); 
      },
      complete: function() {
        object.removeClass('ctools-ajaxing');
      },
      dataType: 'json'
    });
  }
  catch (err) {
    alert("An error occurred while attempting to process " + url); 
    $(this).removeClass('ctools-ajaxing');
    return false;
  }

  return false;
};

/**
 * Generic replacement click handler to open the modal with the destination
 * specified by the href of the link.
 */
Drupal.CTools.AJAX.clickAJAXButton = function() {
  if ($(this).hasClass('ctools-ajaxing')) {
    return false;
  }

  // Put our button in.
  this.form.clk = this;

  var url = Drupal.CTools.AJAX.findURL(this);
  $(this).addClass('ctools-ajaxing');
  var object = $(this);
  try {
    if (url) {
      url = url.replace('/nojs/', '/ajax/');
      $.ajax({
        type: "POST",
        url: url,
        data: '',
        global: true,
        success: Drupal.CTools.AJAX.respond,
        error: function() { 
          alert("An error occurred while attempting to process " + url); 
        },
        complete: function() {
          object.removeClass('ctools-ajaxing');
        },
        dataType: 'json'
      });
    }
    else {
      var form = this.form;
      url = $(form).attr('action');
      url = url.replace('/nojs/', '/ajax/');
      $(form).ajaxSubmit({
        type: "POST",
        url: url,
        data: '',
        global: true,
        success: Drupal.CTools.AJAX.respond,
        error: function() { 
          alert("An error occurred while attempting to process " + url); 
        },
        complete: function() {
          object.removeClass('ctools-ajaxing');
        },
        dataType: 'json'
      });
    }
  }
  catch (err) {
    alert("An error occurred while attempting to process " + url);
    $(this).removeClass('ctools-ajaxing');
    return false;
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
  prepend: function(data) {
    $(data.selector).prepend(data.data);
    Drupal.attachBehaviors($(data.selector));
  },

  append: function(data) {
    $(data.selector).append(data.data);
    Drupal.attachBehaviors($(data.selector));
  },

  replace: function(data) {
    $(data.selector).replaceWith(data.data);
    Drupal.attachBehaviors($(data.selector));
  }, 

  after: function(data) {
    var object = $(data.data);
    $(data.selector).after(object);
    Drupal.attachBehaviors(object);
  },

  before: function(data) {
    var html = $(data.selector).before(data.data);
    Drupal.attachBehaviors(html);
  },

  html: function(data) {
    $(data.selector).html(data.data);
    Drupal.attachBehaviors($(data.selector));
  }, 

  remove: function(data) {
    $(data.selector).remove();
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

  css: function(data) {
/*
    if (data.selector && data.selector.contains('* html ')) {
      // This indicates an IE hack and we should only do it if we are IE.
      if (!jQuery.browser.msie) {
        return;
      }
      data.selector = data.selector.replace('* html ', '');
    }
*/
    $(data.selector).css(data.argument);
  }, 

  settings: function(data) {
    $.extend(Drupal.settings, data.argument);
  },

  data: function(data) {
    $(data.selector).data(data.name, data.value);
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

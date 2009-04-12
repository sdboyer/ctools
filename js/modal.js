// $Id$
/**
 * @file 
 *
 * Implement a modal form.
 *
 * @see modal.inc for documentation.
 *
 * This javascript relies on the CTools ajax responder.
 */

// Make sure our objects are defined.
Drupal.CTools = Drupal.CTools || {};
Drupal.CTools.Modal = Drupal.CTools.Modal || {};

/**
 * Display the modal
 */
Drupal.CTools.Modal.show = function() {
  var resize = function(e) {
    // For reasons I do not understand, when creating the modal the context must be
    // Drupal.CTools.Modal.modal but otherwise the context must be more than that.
    var context = e ? document : Drupal.CTools.Modal.modal;
    $('div.ctools-modal-content', context).css({
      'width': $(window).width() * .8 + 'px', 
      'height': $(window).height() * .8 + 'px'
    });
    $('div.ctools-modal-content .modal-content', context).css({
      'width': ($(window).width() * .8 - 25) + 'px', 
      'height': ($(window).height() * .8 - 22) + 'px'
    });
  }

  if (!Drupal.CTools.Modal.modal) {
    Drupal.CTools.Modal.modal = $(Drupal.theme('CToolsModalDialog'));
    $(window).bind('resize', resize);
  }

  resize();
  $('span.modal-title', Drupal.CTools.Modal.modal).html(Drupal.t('Loading...'));
  Drupal.CTools.Modal.modal.modalContent({
    // @todo this should be elsewhere.
    opacity: '.40', 
    background: '#fff'
  });
  $('#modalContent .modal-content').html(Drupal.theme('CToolsModalThrobber'));
};

/**
 * Hide the modal
 */
Drupal.CTools.Modal.dismiss = function() {
  if (Drupal.CTools.Modal.modal) {
    Drupal.CTools.Modal.modal.unmodalContent();
  }
};

/**
 * Provide the HTML to create the modal dialog.
 */
Drupal.theme.prototype.CToolsModalDialog = function () {
  var html = ''
  html += '  <div id="ctools-modal">'
  html += '    <div class="ctools-modal-content">' // panels-modal-content
  html += '      <div class="modal-header">';
  html += '        <a class="close" href="#">';
  html +=            Drupal.settings.CToolsModal.closeText + Drupal.settings.CToolsModal.closeImage;
  html += '        </a>';
  html += '        <span id="modal-title" class="modal-title">&nbsp;</span>';
  html += '      </div>';
  html += '      <div id="modal-content" class="modal-content">';
  html += '      </div>';
  html += '    </div>';
  html += '  </div>';

  return html;
}

/**
 * Provide the HTML to create the throbber.
 */
Drupal.theme.prototype.CToolsModalThrobber = function () {
  var html = '';
  html += '  <div id="modal-throbber">';
  html += '    <div class="modal-throbber-wrapper">';
  html +=        Drupal.settings.CToolsModal.throbber;
  html += '    </div>';
  html += '  </div>';

  return html;
};

/**
 * Generic replacement click handler to open the modal with the destination
 * specified by the href of the link.
 */
Drupal.CTools.Modal.clickAjaxLink = function() {
  // show the empty dialog right away.
  Drupal.CTools.Modal.show();
  return Drupal.CTools.AJAX.clickAJAXLink.apply(this);
};

/**
 * Generic replacement click handler to open the modal with the destination
 * specified by the href of the link.
 */
Drupal.CTools.Modal.clickAjaxButton = function() {
  Drupal.CTools.Modal.show();
  return Drupal.CTools.AJAX.clickAJAXButton.apply(this);
};

/**
 * Submit responder to do an AJAX submit on all modal forms.
 */
Drupal.CTools.Modal.submitAjaxForm = function() {
  url = $(this).attr('action');
  url.replace('/nojs/', '/ajax/');
  $(this).ajaxSubmit({
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
}

/**
 * Bind links that will open modals to the appropriate function.
 */
Drupal.behaviors.CToolsModal = function(context) {
  // Bind links
  $('a.ctools-use-modal:not(.ctools-use-modal-processed)', context)
    .addClass('ctools-use-modal-processed')
    .click(Drupal.CTools.Modal.clickAjaxLink);

  // Bind buttons
  $('input.ctools-use-modal:not(.ctools-use-modal-processed)', context)
    .addClass('ctools-use-modal-processed')
    .click(Drupal.CTools.Modal.clickAjaxButton);

  if ($(context).attr('id') == 'modal-content') {
    // Bind submit links in the modal form.
    $('form:not(.ctools-use-modal-processed)', context)
      .addClass('ctools-use-modal-processed')
      .submit(Drupal.CTools.Modal.submitAjaxForm)
      .append('<input type="hidden" name="op" value="">');
    // add click handlers so that we can tell which button was clicked,
    // because the AJAX submit does not set the values properly.
    $('input[type="submit"]:not(.ctools-use-modal-processed)', context)
      .addClass('ctools-use-modal-processed')
      .click(function() {
        var name = $(this).attr('name');
        $('input[name="' + name + '"]', context).val($(this).val());
      });
  }
};

// The following are implementations of AJAX responder commands.

/**
 * AJAX responder command to place HTML within the modal.
 */
Drupal.CTools.AJAX.commands.modal_display = function(command) {
  $('#modal-title').html(command.title);
  $('#modal-content').html(command.output);
  Drupal.attachBehaviors($('#modal-content'));
}

/**
 * AJAX responder command to dismiss the modal.
 */
Drupal.CTools.AJAX.commands.modal_dismiss = function(command) {
  Drupal.CTools.Modal.dismiss();
}

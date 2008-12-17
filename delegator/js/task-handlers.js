// $Id$

Drupal.behaviors.zzGoLastDelegatorTaskList = function(context) {
  var id = 'delegator-task-list-arrange';

  /**
   * There's no way from PHP to tell the tabledrag code to turn on the
   * 'table has changed' code. So what we do is look for a particular
   * class and if so, invoke it. We also look to see if we should
   * hilite a particular row as the 'previous' drag.
   */
  if ($('#' + id, context).hasClass('changed') && Drupal.tableDrag[id]) {
    Drupal.tableDrag[id].changed = true;
    $(Drupal.theme('tableDragChangedWarning')).insertAfter(Drupal.tableDrag[id].table);
    if ($('#' + id + ' .delegator-changed', context).size() != 0) {
      var $row = $('#' + id + ' .delegator-changed', context);
      $row.removeClass('delegator-changed').addClass('drag-previous');
      Drupal.tableDrag[id].oldRowElement = $row;
    }
  }

  $('.delegator-operations select:not(.delegator-processed)', context).each(function() {
    var $next = $(this).parent().next('input');
    $next.hide();
    $(this).change(function() {
      var val = $(this).val();
      // ignore empty
      if (!val) {
        return;
      }

      // force confirm on delete
      if ($(this).val() == 'delete' && !confirm(Drupal.t('Remove this task?'))) {
        $(this).val('');
        return;
      }

      $next.trigger('click');
    });
  });
}

Drupal.Delegator = {};

Drupal.Delegator.CollapsibleCallback = function($container, handle, content, toggle) {
  var $parent = $container.parents('tr.draggable');
  var id = $parent.attr('id') + '-collapse';
  if (toggle.hasClass('ctools-toggle-collapsed')) {
    // Force any other item to close, like an accordion:
    $('#delegator-task-list-arrange .ctools-toggle:not(.ctools-toggle-collapsed)').trigger('click');
    // Closed, about to be opened.
    var tr = '<tr class="delegator-collapsible" id="' + id + '"><td colspan=4></td></tr>';
    $parent.after(tr);
    $('#' + id + ' td').append(content);
    $('#' + id).addClass($parent.attr('class'));
  }
};

Drupal.Delegator.CollapsibleCallbackAfterToggle = function($container, handle, content, toggle) {
  var $parent = $container.parents('tr.draggable');
  var id = $parent.attr('id') + '-collapse';
  if (toggle.hasClass('ctools-toggle-collapsed')) {
    // Was just closed.
    content.hide();
    handle.after(content);
    $('#' + id).remove();
  }
};

$(document).ready(function() {
  if (Drupal.CTools && Drupal.CTools.CollapsibleCallbacks) {
    Drupal.CTools.CollapsibleCallbacks.push(Drupal.Delegator.CollapsibleCallback);
    Drupal.CTools.CollapsibleCallbacksAfterToggle.push(Drupal.Delegator.CollapsibleCallbackAfterToggle);

    // Force all our accordions to close when tabledragging to prevent ugliness:
    $('#delegator-task-list-arrange .tabledrag-handle').mousedown(function() {
      $('#delegator-task-list-arrange .ctools-toggle:not(.ctools-toggle-collapsed)').trigger('click');
    });
  }
});
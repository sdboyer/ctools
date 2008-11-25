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
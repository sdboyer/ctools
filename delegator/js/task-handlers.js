// $Id $

Drupal.behaviors.zzGoLastDelegatorTaskList = function(context) {
  var id = 'delegator-task-list-arrange';

  if ($('#' + id, context).hasClass('changed') && Drupal.tableDrag[id]) {
    Drupal.tableDrag[id].changed = true;
    $(Drupal.theme('tableDragChangedWarning')).insertAfter(Drupal.tableDrag[id].table);
    if ($('#' + id + ' .delegator-changed', context).size() != 0) {
      var $row = $('#' + id + ' .delegator-changed', context);
      $row.removeClass('delegator-changed').addClass('drag-previous');
      Drupal.tableDrag[id].oldRowElement = $row;
    }
  }
}
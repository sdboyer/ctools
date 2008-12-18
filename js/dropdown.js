// $Id$

Drupal.behaviors.CToolsDropdown = function() {
  $('div.ctools-dropdown:not(.ctools-dropdown-processed)')
    .removeClass('ctools-dropdown-no-js')
    .addClass('ctools-dropdown-processed')
    .each(function() {
      var $dropdown = $(this);
      var open = false;
      var hovering = false;
      var timerID = 0;

      var toggle = function(close) {
        // if it's open or we're told to close it, close it.
        if (open || close) {
          // If we're just toggling it, close it immediately.
          if (!close) {
            open = false;
            $("div.ctools-dropdown-container", $dropdown).slideUp(100);
          }
          else {
            // If we were told to close it, wait half a second to make
            // sure that's what the user wanted.
            // Clear any previous timer we were using.
            if (timerID) {
              clearTimeout(timerID);
            }
            timerID = setTimeout(function() {
              if (!hovering) {
                open = false;
                $("div.ctools-dropdown-container", $dropdown).slideUp(100);
              }}, 500); 
          }
        }
        else {
          // open it.
          open = true;
          $("div.ctools-dropdown-container", $dropdown).animate({height: "show", opacity: "show"}, 100); 
        }
      }
      $("a.ctools-dropdown-link", $dropdown).click(function() {
          toggle();
          return false;
        });

      $dropdown.hover(
        function() {
          hovering = true;
        }, // hover in
        function() { // hover out
          hovering = false;
          toggle(true);
          return false;
        });
        // @todo -- just use CSS for this noise.
      $("div.ctools-dropdown-container a").hover(
        function() { $(this).addClass('ctools-dropdown-hover'); },
        function() { $(this).removeClass('ctools-dropdown-hover'); }
        );
    });
};


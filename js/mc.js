/**
 * modalContent jQuery Plugin
 *
 * @version   0.11
 * @since     2006-11-28
 * @copyright Copyright (c) 2006 Glyphix Studio, Inc. http://www.glyphix.com
 * @author    Gavin M. Roy <gmr@glyphix.com>
 * @license   MIT http://www.opensource.org/licenses/mit-license.php
 * @requires  >= jQuery 1.0.3 http://jquery.com/
 * @requires  dimensions.js http://jquery.com/dev/svn/trunk/plugins/dimensions/dimensions.js?format=raw
 *
 * History:
 *  0.11:
 *   2006-12-19 patch from Tim Saker <tjsaker@yahoo.com>
 *    1) Keyboard events are now only permitted on visible elements belonging to the modal layer (child elements). Attempting to place focus on any other page element will cause focus to be transferred back to the first (ordinal) visible child element of the modal layer.
 *    2) The modal overlay shading now covers the entire height of the document except for a small band at the bottom, which is the height of a scrollbar (a separate thread to be opened on this problem with dimension.js).
 *    3) I removed the code that disables and reenables the scrollbars.  This is just a suggestion really, realizing it could be one of those little things that causes fellow developers to become unnecessary foes ;=).  Personally, I found it an annoying behaviour to remove a visible scrollbar causing the page elements to shift right upon modal popup, then back after closure. If the intent was to prevent scrolling it doesn't anyway since you can still page down with the keyboard. Maybe it should be a boolean option passed in the function signature?
 *   2007-01-03 gmr
 *    1) Updated to set the top of the background div to 0
 *    2) Add 50px to the background div (ugly hack until dimensions.js returns the proper height
 *    3) Removed the .focus from the $('#modalContent .focus') selector since that required something with a class of focus.
 *    4) Created a function for reaize and bound and unbound that so it doesnt clobber other resize functions on unbind
 *    5) Created a function for binding the .close class and bound/unbound click using it.  Close now will work on any clickable element including a map area.
 *    6) Renamed animation commands to match jQuery's.
 *
 * Call modalContent() on a DOM object and it will make a centered modal box over a div overlay preventing access to the page.
 * Create an element (anchor/img/etc) with the class "close" in your content to close the modal box on click.
 */

/**
 * modalContent
 * @param content string to display in the content box
 * @param css obj of css attributes
 * @param animation (fadeIn, slideDown, show)
 * @param speed (valid animation speeds slow, medium, fast or # in ms)
 */
jQuery.modalContent = function(content, css, animation, speed) {

  // if we already ahve a modalContent, remove it
  if ( $('#modalBackdrop') ) $('#modalBackdrop').remove();
  if ( $('#modalContent') ) $('#modalContent').remove();

  // position code lifted from http://www.quirksmode.org/viewport/compatibility.html
  if (self.pageYOffset) { // all except Explorer
  var wt = self.pageYOffset;
  } else if (document.documentElement && document.documentElement.scrollTop) { // Explorer 6 Strict
    var wt = document.documentElement.scrollTop;
  } else if (document.body) { // all other Explorers
    var wt = document.body.scrollTop;
  }

  // Get our dimensions

  // Get the docHeight and (ugly hack) add 50 pixels to make sure we dont have a *visible* border below our div
  var docHeight = $(document).outerHeight() + 50;
  var docWidth = $(document).innerWidth();
  var winHeight = $(window).height();
  var winWidth = $(window).innerWidth();
  if( docHeight < winHeight ) docHeight = winHeight;

  // Create our divs
  $('body').append('<div id="modalBackdrop" style="z-index: 1000; display: none;"></div><div id="modalContent" style="z-index: 1001; position: absolute;">' + $(content).html() + '</div>');

  // Keyboard and focus event handler ensures focus stays on modal elements only
  modalEventHandler = function( event ) {
    target = null;
    if ( event ) { //Mozilla
      target = event.target;
    } else { //IE
      event = window.event;
      target = event.srcElement;
    }
    if( $(target).filter('*:visible').parents('#modalContent').size() ) {
      // allow the event only if target is a visible child node of #modalContent
      return true;
    }
    if ( $('#modalContent') ) $('#modalContent').get(0).focus();
    return false;
  };
  $('body').bind( 'focus', modalEventHandler );
  $('body').bind( 'keypress', modalEventHandler );

  // Create our content div, get the dimensions, and hide it
  var modalContent = $('#modalContent').css('top','-1000px');
  var mdcTop = wt + ( winHeight / 2 ) - (  modalContent.outerHeight() / 2);
  var mdcLeft = ( winWidth / 2 ) - ( modalContent.outerWidth() / 2);
  $('#modalBackdrop').css(css).css('top', 0).css('height', docHeight + 'px').css('width', docWidth + 'px').show();
  modalContent.css({top: mdcTop + 'px', left: mdcLeft + 'px'}).hide()[animation](speed);

  // Bind a click for closing the modalContent
  modalContentClose = function(){close(); return false;};
  $('.close').bind('click', modalContentClose);

  // Close the open modal content and backdrop
  function close() {
    // Unbind the events
    $(window).unbind('resize',  modalContentResize);
    $('body').unbind( 'focus', modalEventHandler);
    $('body').unbind( 'keypress', modalEventHandler );
    $('.close').unbind('click', modalContentClose);

    // Set our animation parameters and use them
    if ( animation == 'fadeIn' ) animation = 'fadeOut';
    if ( animation == 'slideDown' ) animation = 'slideUp';
    if ( animation == 'show' ) animation = 'hide';

    // Close the content
    modalContent.hide()[animation](speed);

    // Remove the content
    $('#modalContent').remove();
    $('#modalBackdrop').remove();
  };

  // Move and resize the modalBackdrop and modalContent on resize of the window
   modalContentResize = function(){
    // Get our heights
    var docHeight = $(document).outerHeight();
    var docWidth = $(document).innerWidth();
    var winHeight = $(window).height();
    var winWidth = $(window).width();
    if( docHeight < winHeight ) docHeight = winHeight;

    // Get where we should move content to
    var modalContent = $('#modalContent');
    var mdcTop = ( winHeight / 2 ) - (  modalContent.outerHeight() / 2);
    var mdcLeft = ( winWidth / 2 ) - ( modalContent.outerWidth() / 2);

    // Apply the changes
    $('#modalBackdrop').css('height', docHeight + 'px').css('width', docWidth + 'px').show();
    modalContent.css('top', mdcTop + 'px').css('left', mdcLeft + 'px').show();
  };
  $(window).bind('resize', modalContentResize);

  $('#modalContent').focus();
};

/**
 * jQuery function init
 */
jQuery.fn.modalContent = function(css, animation, speed)
{
  // If our animation isn't set, make it just show/pop
  if (!animation) { var animation = 'show'; } else {
    // If our animation isn't "fadeIn" or "slideDown" then it always is show
    if ( ( animation != 'fadeIn' ) && ( animation != 'slideDown') ) animation = 'show';
  }

  if ( !speed ) var speed = 'fast';

  // Build our base attributes and allow them to be overriden
  css = jQuery.extend({
    position: 'absolute',
    left: '0px',
    margin: '0px',
    background: '#000',
    opacity: '.55'
  }, css);

  // jQuery mojo
  this.each(function(){
    $(this).hide();
    new jQuery.modalContent($(this), css, animation, speed);
  });

  // return this object
  return this;
};

/**
 * unmodalContent
 * @param animation (fadeOut, slideUp, show)
 * @param speed (valid animation speeds slow, medium, fast or # in ms)
 */
jQuery.fn.unmodalContent = function(animation, speed)
{
  // If our animation isn't set, make it just show/pop
  if (!animation) { var animation = 'show'; } else {
    // If our animation isn't "fade" then it always is show
    if ( ( animation != 'fadeOut' ) && ( animation != 'slideUp') ) animation = 'show';
  }
  // Set a speed if we dont have one
  if ( !speed ) var speed = 'fast';

  // Unbind the events we bound
  $(window).unbind('resize', modalContentResize);
  $('body').unbind( 'focus', modalEventHandler);
  $('body').unbind( 'keypress', modalEventHandler);
  $('.close').unbind('click', modalContentClose);

  // jQuery magic loop through the instances and run the animations or removal.
  this.each(function(){
    if ( animation == 'fade' ) {
      $('#modalContent').fadeOut(speed,function(){$('#modalBackdrop').fadeOut(speed, function(){$(this).remove();});$(this).remove();});
    } else {
      if ( animation == 'slide' ) {
        $('#modalContent').slideUp(speed,function(){$('#modalBackdrop').slideUp(speed, function(){$(this).remove();});$(this).remove();});
      } else {
        $('#modalContent').remove();$('#modalBackdrop').remove();
      }
    }
  });
};

$(document).ready(function(){

  updateOutboundStatus();

   
});

var outboundTemplates = [
'<p class="pull-right outbound-status"><i class="icn-sending"></i> Sending...</p>',
  '<p class="pull-right outbound-status"><i class="icn-sent"></i> Sent</p>',
  '<p class="pull-right outbound-status"><i class="icon-ok-sign"></i> Delivered</p>'
];
var currentOutboundTemplateIndex = 2;

function updateOutboundStatus() {
  currentOutboundTemplateIndex = ( currentOutboundTemplateIndex + 1 ) % 3;
  $('.outbound-status-test').html(outboundTemplates[currentOutboundTemplateIndex]);

  setTimeout(updateOutboundStatus, 3000);
}




// DECRYPTION LOADER TEST ILLUSTRATION
$(document).ready(function(){
  $('[href=#group-chat]').click(function(){

    $('#group-chat .chat-area').noiseLoader();


    setTimeout(function(){
      $('#group-chat .chat-area').noiseLoader('setPercent',.1);
    }, 100);
    setTimeout(function(){
      $('#group-chat .chat-area').noiseLoader('setPercent',.2);
    }, 210);
    setTimeout(function(){
      $('#group-chat .chat-area').noiseLoader('setPercent',.3);
    }, 320);
    setTimeout(function(){
      $('#group-chat .chat-area').noiseLoader('setPercent',.4);
    }, 430);
    setTimeout(function(){
      $('#group-chat .chat-area').noiseLoader('setPercent',.5);
    }, 540);
    setTimeout(function(){
      $('#group-chat .chat-area').noiseLoader('setPercent',.6);
    }, 650);
    setTimeout(function(){
      $('#group-chat .chat-area').noiseLoader('setPercent',.7);
    }, 760);
    setTimeout(function(){
      $('#group-chat .chat-area').noiseLoader('setPercent',.8);
    }, 870);
    setTimeout(function(){
      $('#group-chat .chat-area').noiseLoader('setPercent',.9);
    }, 980);
    setTimeout(function(){
      $('#group-chat .chat-area').noiseLoader('setPercent',1);
    }, 1090);
  })
});





/* Changed to use backbone for it
 * 
 * // Initialize Multi Selects
$(document).ready(function() {

  $('.multi-select a').each(function(){

    var multiSelect = $(this).parents('.multi-select');
    var sourceColumnUL = multiSelect.find('.source-column ul');
    var destColumnUL = multiSelect.find('.destination-column ul');

    $(this).click(function(e){
      e.preventDefault();
      $this = $(this);
      $li = $(this).parents('li');


      // Copy append and show
      $clone = $li.clone().hide();
      destColumnUL.prepend($clone);
      $clone.show();

      // Hide source
      $li.addClass('selected');

      // Update count
      multiSelect.find('.count').text(destColumnUL.find('li').length);

      // Setup removal
      $clone.data('owner', $li);
      $clone.find('a').click(function(e){
        e.preventDefault();
        $this = $(this);
        $li = $(this).parents('li');

        $owner = $li.data('owner');
        // Hide source
        $owner.removeClass('selected');

        $li.hide().detach();

        // Update Count
        multiSelect.find('.count').text(destColumnUL.find('li').length);
      });
    });
  });
});
*/


// User Modal Illustration
$('.info-link').click(function(e){
  e.preventDefault();

  $('#profile').modal('show');
})


// Press enter to send toggle
$('.press-enter-to-send').change(function(e){
  if ($(this).is(':checked')) {
    $('.send').attr('disabled','disabled');
  } else {
    $('.send').removeAttr('disabled');
  }
});


//// Simple Faked Toggle Encryption.
//// Designed for illustration only, could be used or scrapped
//$('.toggle-encrypted').click(function(e){
//  var key = hexToByteArray(genkey());
//  var mode = 'ECB'; // ECB or CBC
//
//  e.preventDefault();
//  if ($(this).parents('.tab-pane').hasClass('show-encrypted')) {
//    $(this).parents('.tab-pane').removeClass('show-encrypted');
//    $(this).find('span').text('Show Encrypted');
//
//    $('.chat-list li .message p').each(function(){
//      if ($(this).data('plaintext')) {
//        $(this).html($(this).data('plaintext'));
//      }
//    });
//  } else {
//    $(this).parents('.tab-pane').addClass('show-encrypted');
//    $(this).find('span').text('Show Plain Text');
//   
//    $('.chat-list li .message p').each(function(){
//      var plaintext = $(this).html();
//      $(this).data('plaintext', plaintext);
//
//      if (!$(this).data('ciphertext')) {
//        var ciphertext = byteArrayToHex(rijndaelEncrypt(plaintext,key, mode));
//        $(this).data('ciphertext', ciphertext);
//        $(this).html(ciphertext);
//      } else {
//        $(this).html($(this).data('ciphertext'));
//      }
//    });
//  }
//});

$('.draggable').draggable({ cancel: "a, .no-drag" });



//Add members popover for AV Chats
$('.avchat-member-popover').click(function(e){
var popover = $(this).siblings('.popover');
popover.toggleClass('in');

// Click anywhere to close the popover
if (popover.hasClass('in')) {
 setTimeout(function(){
   popover.on('click.edit-users', function(e){
     e.stopPropagation();
   });
   $('html').on('click.edit-users', function(e){
     popover.removeClass('in');
     $('html').off('click.edit-users');
     popover.off('click.edit-users');
   });
 }, 1);
}
});
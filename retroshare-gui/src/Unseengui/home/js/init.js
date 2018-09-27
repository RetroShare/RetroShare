
jQuery(document).ready(function(){
	/*jQuery('#topnav').localScroll(3000);
	jQuery('#top-section').localScroll(6000);*/
    jQuery.localScroll( {offset:-100, duration: 500});

});


//----Menu---//
jQuery('.navbar .nav > li > a').click(function(){
		jQuery('.navbar-collapse.navbar-ex1-collapse.in').removeClass('in').addClass('collapse').css('height', '0');
		});


//----show hide sign up forms---//

$(document).ready(function() {

    $( "a#forgotten-link").on( "click", function() {

        $("a#forgotten-link").hide("fast");
        $("form#password-reset").slideToggle("fast");

    });


});

$(document).ready(function() {

  $(".forgot-password-link").click(function() {
    $(".register-content").hide();
    $(".forgot-password-content").show();
  });
  
  $(".login-link").click(function() {
    $(".register-content").show();
    $(".forgot-password-content").hide();
  });

  $(".conference").click(function() {
    if ($(this).children(".hidden_conference").is(":visible")) {
        $(".hidden_conference").hide();
    } else {
        $(".hidden_conference").hide();
        $(this).children(".hidden_conference").show();
    };
  });

});
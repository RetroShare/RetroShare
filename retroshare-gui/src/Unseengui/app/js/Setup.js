/**
 * Setup
 * 
 * @author Unseen App team
 */

var unsene = unsene || {};
var app = unsene.app || {};

(function ($, unsene, app) {
    'use strict';

    app.setupUnseenLocal = function () {
        unsene.BASE_URL = "http://192.168.1.217";
        unsene.DEFAULT_PROFILE_IMAGE_URL = 'asset/chat/img/avatar_new.png';
        unsene.BOSH_URL = 'http://192.168.1.218:5280/http-bind';
        unsene.SERVER_NAME = 'local.unseen.is';
        unsene.XMPP_RESOURCE = (store.get("is_client") ? "clientnw" : 'web') + Math.random().toString(36).substring(12);
        unsene.SSIG_DOMAIN = "qasip.unseen.is";
        unsene.XMPP_DOMAIN = '@' + unsene.SERVER_NAME;
        unsene.XMPP_FULL_JID = unsene.XMPP_DOMAIN + '/' + unsene.XMPP_RESOURCE;
        unsene.MUC_SERVICE = 'muc.' + unsene.SERVER_NAME;
        unsene.NS_UNSENE = "jabber:x:unsene";
        unsene.CALL_SERVER_LIST = ["https://conf01.unseen.is/"];
        unsene.GROUP_AUDIO_CALL_SERVER_LIST = ["https://conf02.unseen.is/"];
        unsene.GROUP_VIDEO_CALL_SERVER_LIST = ["https://conf03.unseen.is/"];
        unsene.PUBLIC_MESSAGE_LOCATION = "/msg/pm/";
        unsene.LOGOUT_URL = store.get("is_client") ? "index.html" : "/";
        unsene.PUBSUB_SERVICE = 'pubsub.' + unsene.SERVER_NAME;
        unsene.WS_SERVICE_URL = 'ws://192.168.1.218:5280/ws-xmpp';
        unsene.WEBMAIL_URL = 'https://qamail.unseen.is/interweb/';
        unsene.WEBMAIL_PREMIUM_URL = 'https://qamail.unseen.is/interweb/';
        unsene.DONT_USE_WS = false;
        unsene.DEFAULT_HEADER_TITLE = "";
        unsene.debug = true;
        unsene.isUnseenApp = true;
        
        app.environment = "local";
        app.log('>>> Connect to Server Local (192.168.1.217)');
            
        var hiddenInputs = '';
        hiddenInputs += '<input type="hidden" id="widget" value="">';
        hiddenInputs += '<input type="hidden" id="payment_status" value="">';
        hiddenInputs += '<input type="hidden" id="token_id" value="">';
        hiddenInputs += '<input type="hidden" id="reset_pass_id" value="">';
        hiddenInputs += '<input type="hidden" id="reset_username" value="">';
        hiddenInputs += '<input type="hidden" id="reset_acctype" value="">';
        hiddenInputs += '<input type="hidden" id="unseen_ntru_param" value="P100_200_S342b">';
        hiddenInputs += '<input type="hidden" id="unseen_public_key" value="AAAA3wAACACkf9QLNhPxceV9QAfOvCZhJqB+7ZLKnjIhZp8jOowRuqOeN4jFbGnS8dfPo2tkpj+x4OQdyoI00sc0HwE9mwbxMIgHbhtu48P3kLP2be/xbgdF763zVSePPAkbpBs2MysOwPgZVURhGrp7tV+NQ3Ks+cQF8okaAwVDTLb4HMBFKjstkURyvjrsrnMgqXWvKc1ZiE634ecdB8MA5GHdDutyP9m1NFEpMuj5k+T+b2CJ1L9B6kTzW7raLWBVY+a87JjT97+ry7tDkskl3yl1OwyMCpnqF+8nyKiFDKiX1QYrM1MwfNRsiLbd6NgfpOIZ9B2seKcWFadSVvg+gpjA7Z5frxgMJPOvBbw1O7Y91wXnUrhToXfG+PD/ITZUI1BjRqy+aNceeNvsbcCsEIjxffxNO7we">';
        hiddenInputs += '<input type="hidden" id="public_key" value="AAABNwAACAAt9LpwS7iJSo2kCNjhSBWdJmZIlVYDixKlx0I6L4vIuVuRTFxrIwxCYleqhJkfL6grBZjUWoZZIPUbWM6CYv0HzJX9USbI79fN3vxplvHtdSmsbvjZYS42NM8DpX1RKUMuIcqBrgiAKFb8uIxpFHHERFQ84MI1OZZrrV/INNH6DCv+nCIGswqDCn0oDn1+zvYD+u98yducdxkS4SF4corijK+f6TpnaV91gRLto2aEYKdURPqGLRnK+qxoEKzeUy0d1Hb2OhQwXfe3oL19OWhFJzDoGL4sCT6cGW78lg3rXG2mlEmZ9l1tUJXf1qLVpCCghwJ3ZlzQ/NSRFhH+xt0bil8xQyprKYRLshQHNnw9mzLH1mIuJFjbZmIgPdGRgiC96bhqXh+kA89ZlwNbwoPg1MEsPhTaD/G1F349LJEfaIKZoYcB8UaGWnK9IBs9Xt5fTCLaLF97dBtgFnguRmKkCoDeofBFBA0kIb5d3P/0OhewEY0fL1WfZTTTZ0iXAQD31CLbanBIh5iVcD0EFTR2CX1OuVP7iCEZA3NoZQOTmRxXwga32ZQ0P/aFAg==">';
        hiddenInputs += '<input type="hidden" id="sso_login_url" value="http://cas.unseen.local:3000/cas/login?service=http://unseen.local/">';
        $('body').append(hiddenInputs);
    };
    
    app.setupUnseenDev = function () {
        unsene.BASE_URL = "https://dev.unseen.is/";
        unsene.DEFAULT_PROFILE_IMAGE_URL = 'asset/chat/img/avatar_new.png';
        unsene.BOSH_URL = '/http-bind/';
        unsene.SERVER_NAME = 'dev.unseen.is';
        unsene.XMPP_RESOURCE = (store.get("is_client") ? "clientnw" : 'web') + Math.random().toString(36).substring(12);
        unsene.SSIG_DOMAIN = "dev.unseen.is:8443";
        unsene.XMPP_DOMAIN = '@' + unsene.SERVER_NAME;
        unsene.XMPP_FULL_JID = unsene.XMPP_DOMAIN + '/' + unsene.XMPP_RESOURCE;
        unsene.MUC_SERVICE = 'muc.' + unsene.SERVER_NAME;
        unsene.NS_UNSENE = "jabber:x:unsene";
        unsene.CALL_SERVER_LIST = ["https://conf01.unseen.is/"];
        unsene.GROUP_AUDIO_CALL_SERVER_LIST = ["https://conf02.unseen.is/"];
        unsene.GROUP_VIDEO_CALL_SERVER_LIST = ["https://conf03.unseen.is/"];
        unsene.PUBLIC_MESSAGE_LOCATION = "/msg/pm/";
        unsene.LOGOUT_URL = store.get("is_client") ? "index.html" : "/";
        unsene.PUBSUB_SERVICE = 'pubsub.' + unsene.SERVER_NAME;
        unsene.WS_SERVICE_URL = 'wss://dev.unseen.is:5443/ws-xmpp';
        unsene.WEBMAIL_URL = 'https://maildev.unseen.is/interweb/';
        unsene.WEBMAIL_PREMIUM_URL = 'https://maildev.unseen.is/interweb/';
        unsene.DONT_USE_WS = false;
        unsene.DEFAULT_HEADER_TITLE = "";
        unsene.DEFAULT_KEYSIZE = 512;
        unsene.debug = true;
        unsene.isUnseenApp = true;

        app.environment = "local";
        app.log('>>> Connect to Server Dev (https://dev.unseen.is)');

        var hiddenInputs = '';
        hiddenInputs += '<input type="hidden" id="widget" value="">';
        hiddenInputs += '<input type="hidden" id="payment_status" value="">';
        hiddenInputs += '<input type="hidden" id="token_id" value="">';
        hiddenInputs += '<input type="hidden" id="reset_pass_id" value="">';
        hiddenInputs += '<input type="hidden" id="reset_username" value="">';
        hiddenInputs += '<input type="hidden" id="reset_acctype" value="">';
        hiddenInputs += '<input type="hidden" id="unseen_ntru_param" value="P100_200_S342b">';
        hiddenInputs += '<input type="hidden" id="unseen_public_key" value="AAAA3wAACACkf9QLNhPxceV9QAfOvCZhJqB+7ZLKnjIhZp8jOowRuqOeN4jFbGnS8dfPo2tkpj+x4OQdyoI00sc0HwE9mwbxMIgHbhtu48P3kLP2be/xbgdF763zVSePPAkbpBs2MysOwPgZVURhGrp7tV+NQ3Ks+cQF8okaAwVDTLb4HMBFKjstkURyvjrsrnMgqXWvKc1ZiE634ecdB8MA5GHdDutyP9m1NFEpMuj5k+T+b2CJ1L9B6kTzW7raLWBVY+a87JjT97+ry7tDkskl3yl1OwyMCpnqF+8nyKiFDKiX1QYrM1MwfNRsiLbd6NgfpOIZ9B2seKcWFadSVvg+gpjA7Z5frxgMJPOvBbw1O7Y91wXnUrhToXfG+PD/ITZUI1BjRqy+aNceeNvsbcCsEIjxffxNO7we">';
        hiddenInputs += '<input type="hidden" id="public_key" value="AAABNwAACADPmmBVu3d6pfUsjgq2CIJwYIrlVCclugTiGSEsQop+clzlNp9Vs74Md9w0hZgXtyXzs64ltsIpyWHNiRLw07GIcJSaD9xxFPNc6ii9SfFMkzqFnfoZZiLwLL7sNcQXfQ3W/fSE3iys/04MnJND1NiVbG8+srhRfC3NJFx7jNDWhRTHPrZEYqV+cy/SaPwiGHw1ylKX8dr+RwY0JaCVFQVa36L1a7ABfU5z+vKIeyJ3NkIAA9gUrxb8GSQPRggn1BjpnijyPcBNyAkM03Y4gol/qniIVqTb8735ZTFQ75UOi42CKeVM4u7/tNEpjL+BjakTdiORQ8k07iiGoVbSInm2h1a52wo5+X7Bx09tX+Xe4Mpj3PK/aybQtNQnE4MeWNvc1Vqw0RXitMi1P7FKcLE3VIyjRSzsfdJ4SkDMb78sydRm3nDlP8WhwwFbRhyDUxHiHbLKrHSZsbjBtHgnwqlvXpo6RKTjJSUzxzwKzEgUvvGaO+muupxT9GI7B3GqLz+T2WyB3dUWpHLQIBJ9cXy68LufP6sPTN5aLzCatVVwORHdWD2YXFxe6UOAAA==">';
        hiddenInputs += '<input type="hidden" id="sso_login_url" value="https://qacas.unseen.is/cas/login?service=https://qa.unseen.is/">';
        $('body').append(hiddenInputs);
    };

    app.setupUnseenQA = function () {
        unsene.BASE_URL = "https://qa.unseen.is";
        unsene.DEFAULT_PROFILE_IMAGE_URL = 'asset/chat/img/avatar_new.png';
        unsene.BOSH_URL = 'https://qaimbosh.unseen.is:443/http-bind';
        unsene.SERVER_NAME = 'im.unseen.is';
        unsene.XMPP_RESOURCE = (store.get("is_client") ? "clientnw" : 'web') + Math.random().toString(36).substring(12);
        unsene.SSIG_DOMAIN = "qasip.unseen.is";
        unsene.XMPP_DOMAIN = '@' + unsene.SERVER_NAME;
        unsene.XMPP_FULL_JID = unsene.XMPP_DOMAIN + '/' + unsene.XMPP_RESOURCE;
        unsene.MUC_SERVICE = 'muc.' + unsene.SERVER_NAME;
        unsene.NS_UNSENE = "jabber:x:unsene";
        unsene.CALL_SERVER_LIST = ["https://conf01.unseen.is/"];
        unsene.GROUP_AUDIO_CALL_SERVER_LIST = ["https://conf02.unseen.is/"];
        unsene.GROUP_VIDEO_CALL_SERVER_LIST = ["https://conf03.unseen.is/"];
        unsene.PUBLIC_MESSAGE_LOCATION = "/msg/pm/";
        unsene.LOGOUT_URL = store.get("is_client") ? "index.html" : "/";
        unsene.PUBSUB_SERVICE = 'pubsub.' + unsene.SERVER_NAME;
        unsene.WS_SERVICE_URL = 'wss://im.unseen.is:443/ws-xmpp';
        unsene.WEBMAIL_URL = 'https://qamail.unseen.is/interweb/';
        unsene.WEBMAIL_PREMIUM_URL = 'https://qamail.unseen.is/interweb/';
        unsene.DONT_USE_WS = false;
        unsene.DEFAULT_HEADER_TITLE = "";
        unsene.debug = true;
        unsene.DEFAULT_KEYSIZE = 512;
        unsene.isUnseenApp = true;
        
        app.environment = "qa";
        app.log('>>> Connect to Server QA (qa.unseen.is)');
            
        var hiddenInputs = '';
        hiddenInputs += '<input type="hidden" id="widget" value="">';
        hiddenInputs += '<input type="hidden" id="payment_status" value="">';
        hiddenInputs += '<input type="hidden" id="token_id" value="">';
        hiddenInputs += '<input type="hidden" id="reset_pass_id" value="">';
        hiddenInputs += '<input type="hidden" id="reset_username" value="">';
        hiddenInputs += '<input type="hidden" id="reset_acctype" value="">';
        hiddenInputs += '<input type="hidden" id="unseen_ntru_param" value="P100_200_S342b">';
        hiddenInputs += '<input type="hidden" id="unseen_public_key" value="AAAA3wAACACkf9QLNhPxceV9QAfOvCZhJqB+7ZLKnjIhZp8jOowRuqOeN4jFbGnS8dfPo2tkpj+x4OQdyoI00sc0HwE9mwbxMIgHbhtu48P3kLP2be/xbgdF763zVSePPAkbpBs2MysOwPgZVURhGrp7tV+NQ3Ks+cQF8okaAwVDTLb4HMBFKjstkURyvjrsrnMgqXWvKc1ZiE634ecdB8MA5GHdDutyP9m1NFEpMuj5k+T+b2CJ1L9B6kTzW7raLWBVY+a87JjT97+ry7tDkskl3yl1OwyMCpnqF+8nyKiFDKiX1QYrM1MwfNRsiLbd6NgfpOIZ9B2seKcWFadSVvg+gpjA7Z5frxgMJPOvBbw1O7Y91wXnUrhToXfG+PD/ITZUI1BjRqy+aNceeNvsbcCsEIjxffxNO7we">';
        hiddenInputs += '<input type="hidden" id="public_key" value="AAABNwAACABMD8THElFfhjqdVPEOmWPGaY54EomZTznosJ7QYrX47hmXC/Erh7AEJxHt9KkZLrjGQOeG/2/WbrgvNFZbcddiPqamOBmNjgVqk8CZ5Orj6AkaMqHy7tdNuWXBHZCOpmNf79P/Zf0Q1Iszn91z51hGuvc7ib+LiFRV0QX6F29UK02yA9DvMIrPkc/Xf6QkGVBlOdnsLAYnU7fE1KQ8IvMkVOneDFh6tXhfjzOTBYeD8N8gGtNwyM/cRrL5lBAiEblzRNSBRtVx/str8DmxC5a8tZ1XfB7ddKCv7mvvyjbK/AsMAvBVDbXPJLZ055GgUDYWEwbXABF6mK9/u8tU18xH4TBsUhCHOzSlIq3wRyx6vLPeigueVECJ34IgLlF3lKuP/ApxioXxD6Jik8ly815nf/b5GNod+cFvxlhQs5GB8XehmoaHnAe5LTlK9YMjaKGHDTvKvQczl/BX9r5JubrHjGgxbgTSZYoJkX+VIDtAfciNkmKPVeoX0bN+tOR7cFyuvHaEDEwoD2OHX1OsYkragfDyKdd6Avay3RLq/PpyqkZEIQScXf+/f1PIEg==">';
        hiddenInputs += '<input type="hidden" id="sso_login_url" value="https://qacas.unseen.is/cas/login?service=https://qa.unseen.is/">';
        $('body').append(hiddenInputs);
    };

    app.setupUnseenProd = function () {
        unsene.BASE_URL = "https://unseen.is";
        unsene.DEFAULT_PROFILE_IMAGE_URL = 'asset/chat/img/avatar_new.png';
        unsene.BOSH_URL = 'https://imis01bosh.unseen.is:443/http-bind';
        unsene.SERVER_NAME = 'unseen.is';
        unsene.XMPP_RESOURCE = (store.get("is_client") ? "clientnw" : 'web') + Math.random().toString(36).substring(12);
        unsene.SSIG_DOMAIN = "ssig.unseen.is";
        unsene.XMPP_DOMAIN = '@' + unsene.SERVER_NAME;
        unsene.XMPP_FULL_JID = unsene.XMPP_DOMAIN + '/' + unsene.XMPP_RESOURCE;
        unsene.MUC_SERVICE = 'muc.' + unsene.SERVER_NAME;
        unsene.NS_UNSENE = "jabber:x:unsene";
        unsene.CALL_SERVER_LIST = ["https://conf01.unseen.is/"];
        unsene.GROUP_AUDIO_CALL_SERVER_LIST = ["https://conf02.unseen.is/"];
        unsene.GROUP_VIDEO_CALL_SERVER_LIST = ["https://conf03.unseen.is/"];
        unsene.PUBLIC_MESSAGE_LOCATION = "/msg/pm/";
        unsene.LOGOUT_URL = store.get("is_client") ? "index.html" : "/";
        unsene.PUBSUB_SERVICE = 'pubsub.' + unsene.SERVER_NAME;
        unsene.WS_SERVICE_URL = 'wss://imis01.unseen.is:443/ws-xmpp';
        unsene.WEBMAIL_URL = 'https://mail.unseen.is/interweb/';
        unsene.WEBMAIL_PREMIUM_URL = 'https://premiummail.unseen.is/interweb/';
        unsene.DEFAULT_HEADER_TITLE = "";
        unsene.debug = true;
        unsene.DEFAULT_KEYSIZE = 512;
        unsene.isUnseenApp = true;
        
        app.environment = "production";
        app.log('>>> Connect to Server Production (unseen.is)');

        var hiddenInputs = '';
        hiddenInputs += '<input type="hidden" id="widget" value="">';
        hiddenInputs += '<input type="hidden" id="payment_status" value="">';
        hiddenInputs += '<input type="hidden" id="token_id" value="">';
        hiddenInputs += '<input type="hidden" id="reset_pass_id" value="">';
        hiddenInputs += '<input type="hidden" id="reset_username" value="">';
        hiddenInputs += '<input type="hidden" id="reset_acctype" value="">';
        hiddenInputs += '<input type="hidden" id="unseen_ntru_param" value="P100_200_S342b">';
        hiddenInputs += '<input type="hidden" id="unseen_public_key" value="AAAA3wAACAB3ydpFXDINIr0Y/CW6h8JlRund3dL5tfbSPuU/3GCuaO0xHUjeoqcHxHyompUDxyWzVWakiiY2fMlDMUC/X/X0C7ITq42OKdarmSv+7kxYeJ0pQlLgeF4xO1K+oVwspMVMTTNTQuazGP0QOORJjwl8PL0+yr0TCKtP+jwGvMSK5HF1J+fa2TSawife3uw68wdJMHWt93GSg3Iievt63e/PwDqGIUG8RY7waQzlux2qKZAicpHjFyc44RJKeB4690Hkl2OvbFUSb+aQyS2XFIO88X68/GWpqjMcKmrefIKch3fhhZG4KuB+4R8Khe3GnPUbT0UUW3hIxP7FGa7MCLeft01qMeKTmVIBNbO7QVpitZ9a2yN6DMakdB39WhMMZRtjYE4kepi+8M77EDKYUG3YtK4P">';
        hiddenInputs += '<input type="hidden" id="public_key" value="AAABNwAACAAvPtyHRhrBpywHvILPf/ZTZnt0gLi1Sf6xqA5hEUfq3+cyW2a/lRCPPA9UDGnrApTgEMV1mDCpYo/98e9AXegL4O9lczysQa9W7bum1gIW8jEKLYHG6I72hG9bgRItBzwxWwRzhOQIoTthxsBLgkfVBjTcNSTFXtjfj/fGUwrVblYh1MIlvhvq5n78E95rb5IJlu2BrDFilBxnGtsJyJzdxC+mU2uRkV2I6Nzp6HB2uAAMCBE/THEUk506bSy4RU92u59cf8ePhicOLMW2lq76EvaikqEA2fZuLY49AM3pbn7lUiexAOt92gfWKYl3vMeScyNUVCq3R2n5Ef1A5aWvaDBpb+gUIZCkTOgB0hrYxoLZV/e6oghHQzWquaoCReunm8WvG/BUNfHsazP5waSZYBUWOk3OmWrWNVncxLoQa/ctU0B9Ttti7aFQsXwuFHXiDN4hcM0bGQnMpP0ZMsqlzi3xEunMwWLq0LLFDYOwlHXxYYRM/1180ntdOYwuohi84TgG0cTG9yHWn17CW1LR46kT7iVpA+/wQLgusWdUr1RvapIMSQTVaPqEBQ==">';
        hiddenInputs += '<input type="hidden" id="sso_login_url" value="https://cas.unseen.is/cas/login?service=https://unseen.is/">';
        $('body').append(hiddenInputs);
    };

    app.setupUnseenTW = function () {
        unsene.BASE_URL = "https://unseen.tw";
        unsene.DEFAULT_PROFILE_IMAGE_URL = 'asset/chat/img/avatar_new.png';
        unsene.BOSH_URL = 'https://imbosh.unseen.tw:443/http-bind';
        unsene.SERVER_NAME = 'unseen.tw';
        unsene.XMPP_RESOURCE = (store.get("is_client") ? "clientnw" : 'web') + Math.random().toString(36).substring(12);
        unsene.SSIG_DOMAIN = "ssig.unseen.is";
        unsene.XMPP_DOMAIN = '@' + unsene.SERVER_NAME;
        unsene.XMPP_FULL_JID = unsene.XMPP_DOMAIN + '/' + unsene.XMPP_RESOURCE;
        unsene.MUC_SERVICE = 'muc.' + unsene.SERVER_NAME;
        unsene.NS_UNSENE = "jabber:x:unsene";
        unsene.CALL_SERVER_LIST = ["https://conf01.unseen.is/"];
        unsene.GROUP_AUDIO_CALL_SERVER_LIST = ["https://conf02.unseen.is/"];
        unsene.GROUP_VIDEO_CALL_SERVER_LIST = ["https://conf03.unseen.is/"];
        unsene.PUBLIC_MESSAGE_LOCATION = "/msg/pm/";
        unsene.LOGOUT_URL = store.get("is_client") ? "index.html" : "/";
        unsene.PUBSUB_SERVICE = 'pubsub.' + unsene.SERVER_NAME;
        unsene.WS_SERVICE_URL = 'wss://im.unseen.tw:443/ws-xmpp';
        unsene.WEBMAIL_URL = 'https://mail.unseen.tw/interweb/';
        unsene.WEBMAIL_PREMIUM_URL = 'https://mail.unseen.tw/interweb/';
        unsene.DEFAULT_HEADER_TITLE = "";
        unsene.debug = true;
        unsene.DEFAULT_KEYSIZE = 512;
        unsene.isUnseenApp = true;
        
        app.environment = "tw";
        app.log('>>> Connect to Server Taiwan (unseen.tw)');

        var hiddenInputs = '';
        hiddenInputs += '<input type="hidden" id="widget" value="">';
        hiddenInputs += '<input type="hidden" id="payment_status" value="">';
        hiddenInputs += '<input type="hidden" id="token_id" value="">';
        hiddenInputs += '<input type="hidden" id="reset_pass_id" value="">';
        hiddenInputs += '<input type="hidden" id="reset_username" value="">';
        hiddenInputs += '<input type="hidden" id="reset_acctype" value="">';
        hiddenInputs += '<input type="hidden" id="unseen_ntru_param" value="P100_200_S342b">';
        hiddenInputs += '<input type="hidden" id="unseen_public_key" value="AAAA3wAACACkf9QLNhPxceV9QAfOvCZhJqB+7ZLKnjIhZp8jOowRuqOeN4jFbGnS8dfPo2tkpj+x4OQdyoI00sc0HwE9mwbxMIgHbhtu48P3kLP2be/xbgdF763zVSePPAkbpBs2MysOwPgZVURhGrp7tV+NQ3Ks+cQF8okaAwVDTLb4HMBFKjstkURyvjrsrnMgqXWvKc1ZiE634ecdB8MA5GHdDutyP9m1NFEpMuj5k+T+b2CJ1L9B6kTzW7raLWBVY+a87JjT97+ry7tDkskl3yl1OwyMCpnqF+8nyKiFDKiX1QYrM1MwfNRsiLbd6NgfpOIZ9B2seKcWFadSVvg+gpjA7Z5frxgMJPOvBbw1O7Y91wXnUrhToXfG+PD/ITZUI1BjRqy+aNceeNvsbcCsEIjxffxNO7we">';
        hiddenInputs += '<input type="hidden" id="public_key" value="AAABNwAACADS/ptliwiPPdMH+vebPKOR62k6dGH6v2NeVVfNaMmjtlrYgSdETE8LGq3XiwJzhy5LRuq4ILTqljffQPLiBCRWdStJWf/1MzmZmYnIZH89nxWucS0PxHcUDx+AVtKPHDtLNlwp5Tqc4V2E/I3uLtH1EQL1HV1K1DrK+bm/Er2mGgPcN40TBOHLvFvdIrC0WXb0BEQ6a5TPRQKguguPnJviYzw1M1pw2zay3AkRspJ7P/KArB42UlqwFjboTlzFBZw+d1AoLDh2StoW8E0Y2wVsj85cPoqzb7QiOMVr9WB4uSzJ2czZxqeMI7lAvYVL+Zi/MsDxdsJn5ZJxXL7t6/yN53H3B+zPWtQIVHOwyXNlNjeYYwrcKx6K7NAxEpBiKHwYtzbNRUlwBK+6SvG6RpY8rZr5jY41WeFgs5YdfAbK7AOiDpyQiOmGJahO3RYaePaeH1+eFSE8HCeZCZm1YaxpSj6VtHTUIj1XimhKb6ge6HuOGOSoYJH9BSMhHXN53GsNDZBnP6s5/2BA5R0jUAoKu9YWVrbj4uByfC+aG79ifJNpIM+x/hLp59I4BQ==">';
        hiddenInputs += '<input type="hidden" id="sso_login_url" value="https://cas.unseen.tw/cas/login?service=http://unseen.tw/">';
        $('body').append(hiddenInputs);
        
        //append for Unseen Taiwan
        $('#logo img').attr('src', 'asset/app/images/unseen-tw-logo.png');
        $('#logo img').attr('alt', 'Unseen.tw');
        $('#logo img').attr('title', 'Unseen.tw');
        $('#main-nav .languages').hide();
        $('#user-menu .menuTab[title="seen.is"]').hide();
    };
    
    /**
     * Get system OS Info
     */
    function getSysOSInfo() {
        var sysOS = require("os");
        var path = require('path');
        var execPath = process.execPath;
        var sysOSArch = sysOS.arch();
        app.sysOSPlatform = sysOS.platform();
        app.sysOSVersion = sysOS.release();

        if (sysOSArch.indexOf("64") > -1)
            app.sysOSBit = "64";
        else
            app.sysOSBit = "32";

        if (app.sysOSPlatform == "darwin") {
            app.sysOSPlatformName = "mac";
            app.appFolder = path.normalize(execPath + "/../../../../../../..");
            app.appName = path.normalize(execPath + "/../../../../../..").replace(app.appFolder + "/", "");
            app.appFullPath = app.appFolder + "/" + app.appName;
            app.sysOSTmpDir = process.env["TMPDIR"];
        } else if (app.sysOSPlatform == "linux") {
            app.sysOSPlatformName = "linux";
            app.appFolder = path.dirname(execPath);
            app.appName = path.basename(execPath);
            app.appFullPath = execPath;
            app.sysOSTmpDir = "/tmp";
        } else if (app.sysOSPlatform == "win32" || app.sysOSPlatform == "win64") {
            app.sysOSPlatformName = "win";
            app.appFolder = path.dirname(execPath);
            app.appName = path.basename(execPath);
            app.appFullPath = process.execPath;
            app.sysOSTmpDir = process.env["TMP"];
        }

    }
    

    $(document).ready(function () {
//        console.log = function (msg) {
//            try {
//                var fs = require('fs');
//                fs.appendFile(app.appFolder + "/logs.txt", msg + '\r\n');
//            } catch (e) {}
//        };
        
        //get System OS info
        getSysOSInfo();
        
        //setup for Unseen environment
        if (app.appName.indexOf('@local.unseen.is') > -1) {
            app.setupUnseenLocal();
        } else if (app.appName.indexOf('@dev.unseen.is') > -1) {
            app.setupUnseenDev();
        } else if (app.appName.indexOf('@qa.unseen.is') > -1) {
            app.setupUnseenQA();
        } else if (app.appName.indexOf('@unseen.is') > -1) {
            app.setupUnseenProd();
        } else if (app.appName.indexOf('@unseen.tw') > -1) {
            app.setupUnseenTW();
        } else {
            switch (app.environment) {
                case 'local':
                    app.setupUnseenLocal();
                    break;
                case 'dev':
                    app.setupUnseenDev();
                    break;
                case 'qa':
                    app.setupUnseenQA();
                    break;
                case 'production':
                    app.setupUnseenProd();
                    break;
                case 'tw':
                    app.setupUnseenTW();
                    break;
                default:
                    app.setupUnseenProd();
            }
        }

        //init Unseen App
        app.init();
        
    });

})(jQuery, unsene, app);

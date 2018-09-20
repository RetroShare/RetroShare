/**
 * Handle the desktop App process.
 * 
 * @author Unseen App team
 */

var unsene = unsene || {};
var store = store || {};
var app = unsene.app || {};
var winState = winState || {};
var win = require('nw.gui').Window.get();

(function (window, $, require, process, unsene) {
    'use strict';

    app.node = {};
    app.node.gui = require('nw.gui');
    app.envLogin = app.node.gui.App.manifest["unseen-login"];
    app.envChat = app.node.gui.App.manifest["unseen-main"];
    app.environment = app.node.gui.App.manifest["environment"];
    app.defaultLang = app.node.gui.App.manifest["defaultLang"];
    app.request2Update = false;
    app.isClearCache = false;
    app.hidden = false;
    app.iconAvailable = "asset/app/images/statusicons/online.png";
    app.iconAway = "asset/app/images/statusicons/away.png";
    app.iconUnavailable = "asset/app/images/statusicons/dnd.png";
    app.iconInvisible = "asset/app/images/statusicons/invisible.png";
    app.iconOffline = "asset/app/images/statusicons/offline.png";

    $.i18n2 = new (require('i18n-2'))({
        locales: ['en', 'zh_CN', 'zh_TW']
    });

    var envApp;
    var blinkTimout;
    var numberNotifier = 0;
    var notificationParam;
    var layoutTimer, elapsedSec = 0;
    var CHECK_LAYOUT_TIMEOUT = 1000;

    /**
     * Init Unseen App
     */
    app.init = function () {
        if (app.isLoggedIn()) {
            app.log('>>> init Unseen Chat ...');
            envApp = app.envChat;
        } else {
            app.log('>>> init Unseen Login ...');
            envApp = app.envLogin;
        }

        //init and complete loading
        initLoading(envApp);

        app.updateLayout();
        completedLoading(envApp);
    };

    function initLoading(envApp) {
        //store lang
        var href = window.location.href;
        if (app.defaultLang === 'en' || app.defaultLang === null) {
            app.defaultLang = 'en_US';
        }
        store.set('lang', app.defaultLang);
        if (href.indexOf("en_US-") > -1) {
            store.set('lang', "en_US");
        } else if (href.indexOf("zh_TW-") > -1) {
            store.set('lang', "zh_TW");
        } else if (href.indexOf("zh_CN-") > -1) {
            store.set('lang', "zh_CN");
        }
        app.log("lang = " + store.get('lang'));

        //load dynamic js language
        var dynLang = app.getLanguage().replace('en_US', 'en');
        $.i18n2.setLocale(dynLang);
        loadJsDynamic('js/unsene/lang/' + dynLang + '.js');

        //load winState
        try {
            if (store.get('winState')) {
                winState = JSON.parse(store.get('winState'));
            } else {
                winState = JSON.parse(JSON.stringify({x: 308, y: 84, width: 1110, height: 770, isNotificationEnable: true, isCheckAutoUpdate: true}));
            }
            app.log("winState: " + winState);
        } catch (e) {
        }

        //init TrayMenu and MainMenu when loading Chat window
        if (app.isLoggedIn()) {
            app.createTrayMenu(app.envLogin);
            app.createMainMenu(app.envLogin);
        }

        //hide loading
        app.showLoading(false);

        //check to Update for new version
        if (app.isAutoUpdate(envApp)) {
            app.autoUpdate();
        }

        //check to Show Dev tools to debug
        if (!app.isProduction() && app.isShowDevTools()) {
            win.showDevTools();
        }
    }


    /**
     * Waiting for App loading to completed
     * 
     * @param {string} envApp
     */
    function completedLoading(envApp) {
        if (layoutTimer) {
            global.clearTimeout(layoutTimer);
        }
        app.log("UnseenApp loading ... " + elapsedSec + "s");

        if (checkTargetPageReady(envApp)) {
            app.createTrayMenu(envApp);
            app.createMainMenu(envApp);
            app.updateScript(envApp);
            app.setLayout(envApp);
            app.applyNewScript();

            //check and do login
            if (!app.isLoggedIn()) {
                unsene.event.bind("login:success", app.onLoginSuccess, app);
                app.autoLogin();
            }

            return;
        }

        //check the limited timeout to stop:
        elapsedSec += 1;
        if (elapsedSec > CHECK_LAYOUT_TIMEOUT) {
            elapsedSec = 0;
            return;
        }

        layoutTimer = global.setTimeout(function () {
            completedLoading(envApp);
        }, 1000);
    }

    /**
     * Open External link
     */
    app.openExternal = function (lnkUrl) {
        app.node.gui.Shell.openExternal(lnkUrl);
    };

    app.isShowDevTools = function () {
        if (app.node.gui.App.manifest["showDevTools"]) {
            return true;
        } else {
            return false;
        }
    };

    app.isAutoUpdate = function (envApp) {
        if (envApp['title'] == "Unseen Login") {
            try {
                if (typeof winState.isCheckAutoUpdate == 'undefined' || winState.isCheckAutoUpdate == true) {
                    return true;
                } else {
                    return false;
                }
            } catch (e) {
                return true;
            }
        } else {
            return false;
        }
    };

    /**
     * Check for App environment
     */
    app.isProduction = function () {
        if (app.environment === "production") {
            return true;
        } else {
            return false;
        }
    };


    /**
     * Show loading mask
     */
    app.showLoading = function (isShow) {
        if (isShow) {
            document.getElementById("is-bg").className = "isbackground";
            document.getElementById("ld-mask").className = "loadingmask";
            $('#mainApp').hide();
        } else {
            document.getElementById("is-bg").className = "";
            document.getElementById("ld-mask").className = "";
            $('#mainApp').show();
        }
    };

    app.showUpdating = function (isShow) {
        if (isShow) {
            document.getElementById("update-is-bg").className = "loaderBackground";
            $('#mainApp').hide();
            $('#updateProgress').show();
        } else {
            document.getElementById("update-is-bg").className = "";
            $('#mainApp').show();
            $('#updateProgress').hide();
        }
    };

    app.setLayout = function (env) {
        if (app.sysOSPlatform !== "linux") {
            win.setMinimumSize(env["min_width"], env["min_height"]);
        }

        if (app.sysOSPlatform === "Windows_NT" || app.sysOSPlatform === "win32" || app.sysOSPlatform === "win64") {
            if (env['title'] === "Unseen App")
                win.setMaximumSize(0, 0); // remove maximumSize
            else {
                win.setMaximumSize(app.envLogin["width"] - 16, app.envLogin["height"] - 39);
                win.setMinimumSize(app.envLogin["width"] - 16, app.envLogin["height"] - 39);
            }
        }

        //set window state
        setWindowState(env);

        //set resizable
        win.setResizable(env["resizable"]);

        win.setShowInTaskbar(true);
        win.show();
        win.focus();
    };

    app.queryString = (function (a) {
        if (a == "")
            return {};
        var b = {};
        for (var i = 0; i < a.length; ++i) {
            var p = a[i].split('=');
            if (p.length != 2)
                continue;
            b[p[0]] = decodeURIComponent(p[1].replace(/\+/g, " "));
        }
        return b;
    })(window.location.search.substr(1).split('&'));

    app.getQueryString = function (query) {
        var uri = app.queryString[query];
        if (uri == null) {
            uri = "en";
        }
        return uri;
    };

    app.compareVersions = function (n, t) {
        for (var e = ("" + n).split("."), r = ("" + t).split("."), o = 0; o < e.length; ++o) {
            if (r.length == o)
                return !0;
            if (e[o] != r[o])
                return e[o] > r[o] ? !0 : !1;
        }
        return e.length != r.length ? !1 : !1;
    };

    app.log = function (msg) {
        console.log(msg);
    };

    app.getLanguage = function () {
        return store.get('lang');
    };

    app.onlineStateIcon = function (onlState) {
        if (typeof onlState == 'undefined' || onlState == null || onlState == "") {
            onlState = getCurrentOnlineState();
        }

        switch (onlState) {
            case "online":
                return app.iconAvailable;
            case "away":
                return app.iconAway;
            case "dnd":
                return app.iconUnavailable;
            case "invisible":
                return app.iconInvisible;
            case "offline":
                return app.iconOffline;
        }

        return app.iconOffline;
    };


    /**
     * Show confirm dialog
     * @param {string} title
     * @param {string} message
     * @param {int} buttonNo
     * @param {function} buttonOkClick
     */
    app.showMessage = function (title, message, buttonNo, buttonOkClick) {
        var dialogButtons = {};
        if (buttonNo > 0)
            dialogButtons["OK"] = function () {
                $(this).dialog("close");
                buttonOkClick();
            };
        if (buttonNo > 1)
            dialogButtons["Cancel"] = function () {
                $(this).dialog("close");
            };
        win.show();
        win.focus();
        $("#dialog").attr("title", title);
        $("#dialog-message").html(message);
        $("#dialog").dialog({
            modal: true,
            resizable: false,
            width: 300,
            height: "auto",
            buttons: dialogButtons
        });
    };

    app.addEvent = function (el, eType, fn, uC) {
        try {
            if (el.addEventListener) {
                el.addEventListener(eType, fn, uC);
                return true;
            } else if (el.attachEvent) {
                return el.attachEvent('on' + eType, fn);
            }
        } catch (e) {
            app.log("app.addEvent() error: " + e.message);
        }
    };

    app.updateScript = function (env) {
        if (env["title"] === "Unseen App") {
            //listen to notify event
            app.addEvent(window, 'handleTitlebarNotifier', handleTitlebarNotifier, false);
            app.addEvent(window, 'notificationCallback', handleNotificationEvent, false);
            app.addEvent(window, 'handleLogoutWebApp', handleLogoutWebApp, false);

            app.tray.icon = app.onlineStateIcon();
            app.addEvent(document.body, 'mouseover', function () {
                app.tray.icon = app.onlineStateIcon();
            }, false);
            app.addEvent(document.body, 'keyup', function () {
                app.tray.icon = app.onlineStateIcon();
            }, false);
        }
    };

    app.showAboutInfo = function () {
        var msg, updateMessage, updateStatus;
        if (app.request2Update) {
            updateMessage = $.i18n2.__("New version of Unseen App is ready to update. Please restart your application to complete the update process.");
            updateStatus = "<p>" + $.i18n2.__("Status:") + "<font style='color:#8AC007;'>" + updateMessage + "</font>";
            updateStatus += "<br>";
            updateStatus += "<a href='javascript:app.doRestart();' style='color:#0084b4;'>" + $.i18n2.__("Restart now!") + "</a>";
            updateStatus += "</p>";
        } else {
            updateMessage = $.i18n2.__("Your application is up to date.");
            updateStatus = "<p>" + $.i18n2.__("Status:") + "<font style='color:#8AC007;'>" + updateMessage + "</font>";
            updateStatus += "</p>";
        }
        msg = "<div style='text-align:center;'><img style='height:100px;' src='asset/app/images/icon.png'></div>";
        msg += "<div style='text-align:center;'><h2>" + $.i18n2.__("Unseen App") + "</h2></div>";
        msg += "<p>" + $.i18n2.__("Version: ") + app.node.gui.App.manifest["version"] + "</p>";
        msg += updateStatus;
        msg += "<p>Website: <a id='dlgUsnLnk' href='" + unsene.BASE_URL + "' target='_blank' style='color:#0084b4;'>" + unsene.BASE_URL + "</a></p>";
        app.showMessage($.i18n2.__("Unseen App"), msg, 0);
    };

    app.doRestart = function () {
        var exec = require('child_process').exec;
        try {
            app.request2Update = false;
            if (app.sysOSPlatformName === "mac") {
                exec('open "' + app.appFullPath + '"', function (err) {
                    if (err)
                        alert('Restart process error: ' + err);
                });
                exec('kill -9 ' + process.pid, function (err) {
                    if (err)
                        alert('Kill/Restart process error: ' + err);
                });
            } else if (app.sysOSPlatformName === "linux") {
                win.hide();
                exec('cd "' + app.appFolder + '" && ./' + app.appName, function (err) {
                    if (err)
                        alert('Restart process error: ' + err);
                });
                exec('kill -9 ' + process.pid, function (err) {
                    if (err)
                        alert('Kill process error: ' + err);
                });
            } else if (app.sysOSPlatformName === "win") {
                win.hide();
                exec('"' + app.appFullPath + '"', function (err) {
                    if (err)
                        alert('Restart process error: ' + err);
                });
                exec('taskkill /F /PID ' + process.pid, function (err) {
                    if (err)
                        alert('Kill process error: ' + err);
                });
            }
        } catch (e) {
        }
    };


    app.dispatchEvent = function (evName, evDetail, evBubbles, evCancelable) {
        var customEvent = new CustomEvent(evName, {detail: evDetail, bubbles: evBubbles, cancelable: evCancelable});
        window.dispatchEvent(customEvent);
        app.log("customEvent", customEvent);
    };

    app.getLogoutUrl = function () {
        if (app.getLanguage()) {
            return app.getLanguage() + '-index.html';
        } else {
            return 'index.html';
        }
    };

    app.doLogout = function () {
        var continueLogout = function () {
            if (app.tray && !app.isLoggedIn()) {
                app.tray.remove();
                app.tray = null;
            }

            //redirect to Login Url
            window.location.replace(app.getLogoutUrl());

            //set Layout for Login window
            app.setLayout(app.envLogin);

            //disable resizable
            win.setResizable(false);
            setTimeout('win.setResizable(false);', 1000);

            //setBadgeLabel is for windows and MacOS only
            if (app.sysOSPlatform !== "linux") {
                win.setBadgeLabel("");
            }

            //remove autoLogin status
            store.remove('username');
            store.remove('password');
        };

        app.showMessage($.i18n2.__("Unseen App"), $.i18n2.__("Are you sure you want to Logout?"), 2, continueLogout);
    };

    app.doQuit = function () {
        var continueQuit = function () {
            if (app.tray) {
                app.tray.remove();
                app.tray = null;
            }

            if (win) {
                win.close(true);
                win = null;
            }

            app.node.gui.App.quit(true);
            process.exit(1);
        };

        if (app.isLoggedIn()) {
            app.showMessage($.i18n2.__("Unseen App"), $.i18n2.__("Are you sure you want to quit Unseen App?"), 2, continueQuit);
        } else {
            continueQuit();
        }
    };

    win.on('blur', windowBlurred);
    win.on('focus', windowFocused);
//    win.on('maximize', windowFocused);
//    win.on('minimize', windowBlurred);
//    win.on('restore', windowFocused);

    window.addEventListener('focus', function (e) {
        windowFocused();
    });

    window.addEventListener('blur', function (e) {
        windowBlurred();
    });

    $(window).on('unload', function () {
        if (app.tray) {
            app.tray.remove();
            app.tray = null;
        }
    });

    win.on('resize', function () {
        saveWindowState();
    });

    win.on('move', function () {
        saveWindowState();
    });

    win.on('close', function (event) {
        if (event == 'quit' || app.hidden == true) {
            app.doQuit();
        } else {
            win.hide();
            app.hidden = true;
        }
    });

    app.node.gui.App.on('reopen', function () {
        app.hidden = false;
        win.show();
    });

    function windowFocused() {
        if (blinkTimout) {
            global.clearInterval(blinkTimout);
            blinkTimout = null;
            app.tray.icon = app.onlineStateIcon();
        }

        if (app.isLoggedIn()) {
            try {
                unsene.isWindowFocus = true;
            } catch (e) {
                app.log('windowFocused error!');
            }
        }
    }

    function windowBlurred() {
        if (app.isLoggedIn()) {
            try {
                unsene.isWindowFocus = false;
            } catch (e) {
                app.log('windowBlurred error!');
            }
        }
    }

    function saveWindowState() {
        if (app.isClearCache) {
            return;
        }

        dumpWindowState();
        if (typeof winState === 'object' && winState !== null) {
            try {
                store.set('winState', JSON.stringify(winState));
            } catch (e) {
            }
        }

    }

    function handleLogoutWebApp(event) {
        app.doLogout();
    }

    function handleTitlebarNotifier(event) {
        app.log('numberNotifier = ', numberNotifier);
        if (app.sysOSPlatform != "linux") {
            if (numberNotifier < event.detail) {
                setTimeout('win.requestAttention(3)', 600);
            }

            app.log("numberNotifier", numberNotifier);
            app.log("event.detail", event.detail);
            if (event.detail) {
                win.setBadgeLabel(event.detail.toString());
            } else {
                win.setBadgeLabel("");
            }
        } else {
            if (numberNotifier < event.detail) {
                win.requestAttention(0);
                win.requestAttention(3);
            }
        }

        numberNotifier = event.detail;
    }

    function handleNotificationEvent(event) {
        if (!winState.isNotificationEnable)
            return;

        if (app.sysOSPlatform != "linux") {
            if (typeof unsene.isMinimize != 'undefined' && unsene.isMinimize) {
                setTimeout('win.requestAttention(3)', 600);
            } else if (typeof unsene.isWindowFocus != 'undefined' && !unsene.isWindowFocus) {
                setTimeout('win.requestAttention(1)', 600);
            }
        } else {
            if (typeof unsene.isMinimize != 'undefined' && unsene.isMinimize) {
                win.requestAttention(0);
                win.requestAttention(3);
            } else if (typeof unsene.isWindowFocus != 'undefined' && !unsene.isWindowFocus) {
                win.requestAttention(0);
                win.requestAttention(1);
            }
        }

        event.on('click', handleClickOnNotification);
    }

    function handleClickOnNotification() {
        onNotificationClick();
        app.hidden = false;
        win.show();
        win.focus();

        // must clearInterval before return to the last status icon 
        if (blinkTimout) {
            global.clearInterval(blinkTimout);
            blinkTimout = null;
            app.tray.icon = app.onlineStateIcon();
        }
    }

    function setWindowState(env) {
        if (env["title"] === "Unseen Login") {
            resizeWindow(env["width"], env["height"], true);
        } else if (env["title"] === "Unseen App") {
            //load winState
            if (store.get('winState')) {
                winState = JSON.parse(store.get('winState'));
                restoreWindowState();
            } else {
                resizeWindow(env["width"], env["height"], true);
                dumpWindowState();
            }
        }
    }

    function dumpWindowState() {
        if (!app.isLoggedIn())
            return;

        if (((win.width + 150) < screen.width || (win.height + 150) < screen.height) &&
                (win.x > 0 && win.x < screen.width && win.y > 0 && win.y < screen.height) &&
                win.width > app.envChat["min_width"] && win.height > app.envChat["min_height"]) {

            if (!winState) {
                winState = {x: 308, y: 84, width: 1110, height: 770, isNotificationEnable: true, isCheckAutoUpdate: true};
            }

            winState.x = win.x;
            winState.y = win.y;
            winState.width = win.width;
            winState.height = win.height;
        }
    }

    function restoreWindowState() {
        if (winState.x) {
            win.moveTo(winState.x, winState.y);
            resizeWindow(winState.width, winState.height, false);
        }
    }

    function onNotificationClick() {
        try {
            if (notificationParam.isOneToOneChat) {
                var contactView = unsene.contactsView.contactViews[notificationParam.contact];
                if (contactView) {
                    contactView.showChatArea();
                }
            } else if (notificationParam.isGroupChat) {
                var groupView = unsene.groupsView.groupViews[notificationParam.groupId];
                if (groupView) {
                    groupView.showChatArea();
                }
            } else if (notificationParam.isCommunity) {
                var communityView = unsene.communitiesView.groupViews[notificationParam.communityId];
                if (communityView) {
                    communityView.showChatArea();
                }
            }
            window.focus();
        } catch (e) {
            app.log("onNotificationClick() error: " + e.message);
        }
    }

    function getCurrentOnlineState() {
        try {
            if (unsene.unseneView) {
                return unsene.unseneView.me.get("status");
            }
        } catch (e) {
            app.log("getCurrentOnlineState() error: " + e.message);
        }

        return "";
    }


    /**
     * Must check page loaded ready before addEventListener() to element
     * 
     * @param {string} env
     * @returns {Boolean}
     */
    function checkTargetPageReady(env) {
        if (env["title"] === "Unseen Login") {
            var eleInputEmail = document.getElementById('inputEmail');
            if (typeof eleInputEmail != "undefined" && eleInputEmail != null && unsene.loginView) {
                return true;
            }
        } else if (env["title"] === "Unseen App") {
            try {
                if (unsene.recentsView.isLoaded) {
                    return true;
                }
            } catch (e) {
            }
        }
        return false;
    }

    function resizeWindow(width, height, iscenter) {
        app.log("Window resize to: " + width + " x " + height);
        win.resizeTo(width, height);
        if (!iscenter)
            return;

        // for window, and the other OSs - define the position so it can be showed at the center of the screen
        // window.screenX - left corner of the window
        // window.screenY - top corner of the window
        // screen.heigth - max height of the solution of monitor screen (for ex. 1600)(1600x900)
        // screen.width - max width of the solution of monitor screen ( for ex. 900) (1600x900)
        var x, y;
        if (window.screenX + width > screen.width || screen.width > width)
            x = Math.round((screen.width - width) / 2);
        else
            x = window.screenX;
        if (window.screenY + height + 100 > screen.height || screen.height > height)
            y = Math.round((screen.height - height) / 2);
        else
            y = window.screenY;
        win.moveTo(x, y);
    }

    unsene.app = app;

})(window, jQuery, require, process, unsene);

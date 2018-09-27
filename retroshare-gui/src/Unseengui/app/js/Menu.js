/**
 * App Menu
 * 
 * @author Unseen App team
 */

var unsene = unsene || {};
var app = unsene.app || {};
var win = win || {};
var winState = winState || {};

(function ($, app) {
    'use strict';

    var globalEle;
    var fs1, fs2;
    var mnuMain;

    //load winState
    try {
        if (store.get('winState')) {
            winState = JSON.parse(store.get('winState'));
        } else {
            winState = JSON.parse(JSON.stringify({x: 308, y: 84, width: 1110, height: 770, isNotificationEnable: true, isCheckAutoUpdate: true}));
        }
    } catch (e) {
    }

    app.createMainMenu = function (env) {
        var mnuApp, mnuAppSubTools, mnuAppSubOnlStatus, mnuOnlineStatus, mnuAppSubFile, mnuAppSubEdit, mnuAppSubAbout;

        if (env["title"] != "Unseen App") {
            mnuOnlineStatus = null;
            mnuMain = null;

            /**
             * Remove menubar on Ubuntu because have error on nwjs v0.12.3
             * //(app.sysOSPlatform === "linux" && process.env["DESKTOP_SESSION"].indexOf("ubuntu") > -1)
             */
            if (app.sysOSPlatform === "darwin") {
                mnuAppSubAbout = new app.node.gui.Menu();
                mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("Getting Started"), click: onMenuClick}));
                mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("Support"), click: onMenuClick}));
                mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("FAQ"), click: onMenuClick}));
                mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("Privacy Policy"), click: onMenuClick}));
                mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("Terms of Service"), click: onMenuClick}));
                mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("About Unseen App"), click: onMenuClick}));

                mnuApp = new app.node.gui.Menu({type: 'menubar'});
                if (app.sysOSPlatform === "darwin") {
                    mnuApp.createMacBuiltin("Unseen");
                }
                mnuApp.append(new app.node.gui.MenuItem({label: $.i18n2.__("Help"), submenu: mnuAppSubAbout}));

                win.menu = mnuApp;
            }
            return;
        }

        mnuOnlineStatus = new app.node.gui.Menu();
        mnuOnlineStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Available"), click: onMenuClick, icon: app.iconAvailable, alticon: app.iconAvailable, iconIsTemplate: false}));
        mnuOnlineStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Away"), click: onMenuClick, icon: app.iconAway, alticon: app.iconAway, iconIsTemplate: false}));
        mnuOnlineStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Unavailable"), click: onMenuClick, icon: app.iconUnavailable, alticon: app.iconUnavailable, iconIsTemplate: false}));
        mnuOnlineStatus.append(new app.node.gui.MenuItem({type: $.i18n2.__("separator")}));
        mnuOnlineStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Invisible"), click: onMenuClick, icon: app.iconInvisible, alticon: app.iconInvisible, iconIsTemplate: false}));

        mnuMain = new app.node.gui.Menu();
        mnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Add Contact"), click: onMenuClick}));
        mnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Add Group"), click: onMenuClick}));

        mnuMain.append(new app.node.gui.MenuItem({type: "separator"}));
        mnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Online Status"), submenu: mnuOnlineStatus}));
        mnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("View Profile"), click: onMenuClick}));
        mnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Invite Friends"), click: onMenuClick}));

        mnuMain.append(new app.node.gui.MenuItem({type: "separator"}));
        fs1 = new app.node.gui.MenuItem({type: "checkbox", label: $.i18n2.__("Full Screen Mode"), click: onMenuClick});
        mnuMain.append(fs1);

//        mnuMain.append(new app.node.gui.MenuItem({type: "separator"}));
//        mnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Copy"), click: onMenuClick}));
//        mnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Cut"), click: onMenuClick}));
//        mnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Paste"), click: onMenuClick}));

        document.body.addEventListener('contextmenu', popupMenu, false);

        /**
         * Only create main app menu for MacOSX:
         * Remove menubar on Ubuntu because have error on nwjs v0.12.3
         * //(app.sysOSPlatform === "linux" && process.env["DESKTOP_SESSION"].indexOf("ubuntu") > -1)
         */
        if (app.sysOSPlatform === "darwin") {
            mnuApp = new app.node.gui.Menu({type: 'menubar'});
            if (app.sysOSPlatform === "darwin") {
                mnuApp.createMacBuiltin("Unseen");
            } else {
                mnuAppSubEdit = new app.node.gui.Menu();
                mnuAppSubEdit.append(new app.node.gui.MenuItem({label: $.i18n2.__("Copy"), click: onMenuClick}));
                mnuAppSubEdit.append(new app.node.gui.MenuItem({label: $.i18n2.__("Cut"), click: onMenuClick}));
                mnuAppSubEdit.append(new app.node.gui.MenuItem({label: $.i18n2.__("Paste"), click: onMenuClick}));
                mnuApp.append(new app.node.gui.MenuItem({label: $.i18n2.__("Edit"), submenu: mnuAppSubEdit}));
            }

            mnuAppSubOnlStatus = new app.node.gui.Menu();
            mnuAppSubOnlStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Available"), click: onMenuClick, icon: app.iconAvailable, alticon: app.iconAvailable, iconIsTemplate: false}));
            mnuAppSubOnlStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Away"), click: onMenuClick, icon: app.iconAway, alticon: app.iconAway, iconIsTemplate: false}));
            mnuAppSubOnlStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Unavailable"), click: onMenuClick, icon: app.iconUnavailable, alticon: app.iconUnavailable, iconIsTemplate: false}));
            mnuAppSubOnlStatus.append(new app.node.gui.MenuItem({type: "separator"}));
            mnuAppSubOnlStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Invisible"), click: onMenuClick, icon: app.iconInvisible, alticon: app.iconInvisible, iconIsTemplate: false}));

            mnuAppSubTools = new app.node.gui.Menu();
            mnuAppSubTools.append(new app.node.gui.MenuItem({label: $.i18n2.__("Online Status"), submenu: mnuAppSubOnlStatus}));
            mnuAppSubTools.append(new app.node.gui.MenuItem({label: $.i18n2.__("View Profile"), click: onMenuClick}));
            mnuAppSubTools.append(new app.node.gui.MenuItem({label: $.i18n2.__("Invite Friends"), click: onMenuClick}));
            mnuAppSubTools.append(new app.node.gui.MenuItem({type: "separator"}));
            fs2 = new app.node.gui.MenuItem({type: "checkbox", label: $.i18n2.__("Full Screen Mode"), click: onMenuClick});
            mnuAppSubTools.append(fs2);
            if (app.isShowDevTools()) {
                mnuAppSubTools.append(new app.node.gui.MenuItem({label: $.i18n2.__("Developer Tools"), click: onMenuClick}));
            }

            mnuAppSubTools.append(new app.node.gui.MenuItem({type: "checkbox", checked: winState.isNotificationEnable, label: $.i18n2.__("Enable Notification"), click: onMenuClick}));
            mnuAppSubTools.append(new app.node.gui.MenuItem({type: "checkbox", checked: winState.isCheckAutoUpdate, label: $.i18n2.__("Check for Updates..."), click: onMenuClick}));
            mnuAppSubTools.append(new app.node.gui.MenuItem({label: $.i18n2.__("Clear Cache"), click: onMenuClick}));


            mnuAppSubFile = new app.node.gui.Menu();
            mnuAppSubFile.append(new app.node.gui.MenuItem({label: $.i18n2.__("Add Contact"), click: onMenuClick}));
            mnuAppSubFile.append(new app.node.gui.MenuItem({label: $.i18n2.__("Add Group"), click: onMenuClick}));
            mnuAppSubFile.append(new app.node.gui.MenuItem({type: "separator"}));
            mnuAppSubFile.append(new app.node.gui.MenuItem({label: $.i18n2.__("Logout"), click: onMenuClick}));
            if (app.sysOSPlatform != "darwin") {
                mnuAppSubFile.append(new app.node.gui.MenuItem({label: $.i18n2.__("Quit Unseen"), click: onMenuClick}));
            }

            //mnuAppSubEdit = new app.node.gui.Menu();
            //mnuAppSubEdit.append(new app.node.gui.MenuItem({ label: $.i18n2.__("Copy"), click: onMenuClick }));
            //mnuAppSubEdit.append(new app.node.gui.MenuItem({ label: $.i18n2.__("Paste"), click: onMenuClick }));

            mnuAppSubAbout = new app.node.gui.Menu();
            mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("Getting Started"), click: onMenuClick}));
            mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("Support"), click: onMenuClick}));
            mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("FAQ"), click: onMenuClick}));
            mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("Privacy Policy"), click: onMenuClick}));
            mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("Terms of Service"), click: onMenuClick}));
            mnuAppSubAbout.append(new app.node.gui.MenuItem({label: $.i18n2.__("About Unseen App"), click: onMenuClick}));

            mnuApp.append(new app.node.gui.MenuItem({label: $.i18n2.__("Account"), submenu: mnuAppSubFile}));
            //mnuApp.append(new app.node.gui.MenuItem({ label: $.i18n2.__("Edit"), submenu: mnuAppSubEdit }));
            mnuApp.append(new app.node.gui.MenuItem({label: $.i18n2.__("Tools"), submenu: mnuAppSubTools}));
            mnuApp.append(new app.node.gui.MenuItem({label: $.i18n2.__("Help"), submenu: mnuAppSubAbout}));

            win.menu = mnuApp;
        }

    };

    app.createTrayMenu = function (env) {
        var trayMnuMain, trayMnuOnlineStatus, trayMnuHelp;

        if (!winState) {
            winState = {x: 308, y: 84, width: 1110, height: 770, isNotificationEnable: true, isCheckAutoUpdate: true};
        }

        if (typeof app.tray == 'undefined' || app.tray == null) {
            if (app.sysOSPlatformName === "mac") {
                app.tray = new app.node.gui.Tray({title: '', icon: app.iconOffline, alticon: app.iconOffline, iconsAreTemplates: false});
            } else {
                app.tray = new app.node.gui.Tray({title: 'Unseen', icon: app.iconOffline});
            }
            
            app.tray.icon = app.onlineStateIcon();
        }

        trayMnuHelp = new app.node.gui.Menu();
        trayMnuHelp.append(new app.node.gui.MenuItem({label: $.i18n2.__("Getting Started"), click: onMenuClick}));
        trayMnuHelp.append(new app.node.gui.MenuItem({label: $.i18n2.__("Support"), click: onMenuClick}));
        trayMnuHelp.append(new app.node.gui.MenuItem({label: $.i18n2.__("FAQ"), click: onMenuClick}));
        trayMnuHelp.append(new app.node.gui.MenuItem({label: $.i18n2.__("Privacy Policy"), click: onMenuClick}));
        trayMnuHelp.append(new app.node.gui.MenuItem({label: $.i18n2.__("Terms of Service"), click: onMenuClick}));
        trayMnuHelp.append(new app.node.gui.MenuItem({label: $.i18n2.__("About Unseen App"), click: onMenuClick}));

        trayMnuMain = new app.node.gui.Menu();
        trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Show Unseen App"), click: onMenuClick}));

        if (env["title"] === "Unseen App") {
            trayMnuOnlineStatus = new app.node.gui.Menu();
            trayMnuOnlineStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Available"), click: onMenuClick, icon: app.iconAvailable, alticon: app.iconAvailable, iconIsTemplate: false}));
            trayMnuOnlineStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Away"), click: onMenuClick, icon: app.iconAway, alticon: app.iconAway, iconIsTemplate: false}));
            trayMnuOnlineStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Unavailable"), click: onMenuClick, icon: app.iconUnavailable, alticon: app.iconUnavailable, iconIsTemplate: false}));
            trayMnuOnlineStatus.append(new app.node.gui.MenuItem({type: "separator"}));
            trayMnuOnlineStatus.append(new app.node.gui.MenuItem({label: $.i18n2.__("Invisible"), click: onMenuClick, icon: app.iconInvisible, alticon: app.iconInvisible, iconIsTemplate: false}));

            trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Add Contact"), click: onMenuClick}));
            trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Add Group"), click: onMenuClick}));
            trayMnuMain.append(new app.node.gui.MenuItem({type: "separator"}));
            trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Online Status"), submenu: trayMnuOnlineStatus}));
            trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("View Profile"), click: onMenuClick}));
            trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Invite Friends"), click: onMenuClick}));
            trayMnuMain.append(new app.node.gui.MenuItem({type: "separator"}));
            trayMnuMain.append(new app.node.gui.MenuItem({type: "checkbox", label: $.i18n2.__("Full Screen Mode"), click: onMenuClick}));
            if (app.isShowDevTools()) {
                trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Developer Tools"), click: onMenuClick}));
            }

            trayMnuMain.append(new app.node.gui.MenuItem({type: "checkbox", checked: winState.isNotificationEnable, label: $.i18n2.__("Enable Notification"), click: onMenuClick}));
            trayMnuMain.append(new app.node.gui.MenuItem({type: "checkbox", checked: winState.isCheckAutoUpdate, label: $.i18n2.__("Check for Updates..."), click: onMenuClick}));
            trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Clear Cache"), click: onMenuClick}));
            trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Help"), submenu: trayMnuHelp}));
            trayMnuMain.append(new app.node.gui.MenuItem({type: "separator"}));
            trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Logout"), click: onMenuClick}));
        } else {
            trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Help"), submenu: trayMnuHelp}));
            trayMnuMain.append(new app.node.gui.MenuItem({type: "separator"}));
            if (app.isShowDevTools()) {
                trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Developer Tools"), click: onMenuClick}));
            }
            trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Clear Cache"), click: onMenuClick}));
        }

        trayMnuMain.append(new app.node.gui.MenuItem({label: $.i18n2.__("Quit Unseen"), click: onMenuClick}));

        app.tray.menu = trayMnuMain;

        // Show window and click tray icon
        app.tray.on('click', function () {
            app.hidden = false;
            win.show();
        });
    };


    function onMenuClick() {
        app.hidden = false;
        win.show();

        //this would cause Copy function not work
        if (this.label != $.i18n2.__("Copy") && this.label != $.i18n2.__("Paste") && this.label != $.i18n2.__("Paste As Text"))
            win.focus();

        switch (this.label) {
            case $.i18n2.__("Available"):
                var ele = document.getElementsByClassName('status-available');
                if (ele.length > 1) {
                    ele[1].click();
                    $('.popoverWrapper').toggle(false);
                    app.tray.icon = app.iconAvailable;
                }
                break;
            case $.i18n2.__("Away"):
                var ele = document.getElementsByClassName('status-away');
                if (ele.length > 1) {
                    ele[1].click();
                    $('.popoverWrapper').toggle(false);
                    app.tray.icon = app.iconAway;
                }
                break;
            case $.i18n2.__("Unavailable"):
                var ele = document.getElementsByClassName('status-unavailable');
                if (ele.length > 1) {
                    ele[1].click();
                    $('.popoverWrapper').toggle(false);
                    app.tray.icon = app.iconUnavailable;
                }
                break;
            case $.i18n2.__("Invisible"):
                var ele = document.getElementsByClassName('status-invisible');
                if (ele.length > 1) {
                    ele[1].click();
                    $('.popoverWrapper').toggle(false);
                    app.tray.icon = app.iconInvisible;
                }
                break;
            case $.i18n2.__("Add Contact"):
                var ele = document.getElementsByClassName('btn-add-contact');
                if (ele.length > 0)
                    ele[0].click();
                break;
            case $.i18n2.__("Add Group"):
                var ele = document.getElementsByClassName('add-group');
                if (ele.length > 0)
                    ele[0].click();
                break;
            case $.i18n2.__("Create Community"):
                var ele = document.getElementsByClassName('add-community');
                if (ele.length > 0)
                    ele[0].click();
                break;
            case $.i18n2.__("View Profile"):
                var ele = document.getElementsByClassName('profile');
                if (ele.length > 0)
                    ele[0].click();
                break;
            case $.i18n2.__("Email"):
                var ele = document.getElementById('secure-email-header-link');
                if (typeof ele != "undefined" && ele != null)
                    ele.click();
                break;
            case $.i18n2.__("Invite Friends"):
                var ele = document.getElementsByClassName('invite-btn');
                if (ele.length > 0)
                    ele[0].click();
                break;
            case $.i18n2.__("Full Screen Mode"):
                doToggleFullscreen();
                if (win.isFullscreen) {
                    this.checked = true;
                    !!fs1 && (fs1.checked = true);
                    !!fs2 && (fs2.checked = true);
                }
                else {
                    this.checked = false;
                    !!fs1 && (fs1.checked = false);
                    !!fs2 && (fs2.checked = false);
                }
                break;
            case $.i18n2.__("Developer Tools"):
                win.showDevTools('', false);
                break;
            case $.i18n2.__("Show Unseen App"):
                break;
            case $.i18n2.__("Logout"):
                app.doLogout();
                break;
            case $.i18n2.__("Quit Unseen"):
                app.doQuit();
                break;
            case $.i18n2.__("Clear Cache"):
                app.node.gui.App.clearCache();
                app.isClearCache = true;
                app.showMessage($.i18n2.__("Unseen App"), $.i18n2.__("Clear cache is done!<br>Do you want to re login?"), 2, app.doLogout);
                break;
            case $.i18n2.__("Enable Notification"):
                winState.isNotificationEnable = this.checked;
                try {
                    store.set('winState', JSON.stringify(winState));
                } catch (e) {
                }
                break;
            case $.i18n2.__("Check for Updates..."):
                winState.isCheckAutoUpdate = this.checked;
                try {
                    store.set('winState', JSON.stringify(winState));
                } catch (e) {
                }
                break;
            case $.i18n2.__("About Unseen App"):
                app.showAboutInfo();
                break;
            case $.i18n2.__("Getting Started"):
                app.node.gui.Shell.openExternal(unsene.BASE_URL + "/gettingstarted.html");
                break;
            case $.i18n2.__("Support"):
                app.node.gui.Shell.openExternal("http://support.unseen.is/");
                break;
            case $.i18n2.__("FAQ"):
                app.node.gui.Shell.openExternal(unsene.BASE_URL + "/faq.html");
                break;
            case $.i18n2.__("Privacy Policy"):
                app.node.gui.Shell.openExternal(unsene.BASE_URL + "/privacy.html");
                break;
            case $.i18n2.__("Terms of Service"):
                app.node.gui.Shell.openExternal(unsene.BASE_URL + "/term.html");
                break;
            case $.i18n2.__("Copy"):
                document.execCommand("copy");
                break;
            case $.i18n2.__("Cut"):
                document.execCommand("cut");
                break;
            case $.i18n2.__("Paste"):
                document.execCommand("paste");
                break;
            case $.i18n2.__("Paste As Text"):
                doPasteAsText();
                break;
            case $.i18n2.__("Zoom In"):
                win.zoomLevel = win.zoomLevel + 1;
                break;
            case $.i18n2.__("Zoom Out"):
                win.zoomLevel = win.zoomLevel - 1;
                break;
            case $.i18n2.__("Zoom Reset"):
                win.zoomLevel = 0;
                break;
        }
    }


    function popupMenu(ev) {
        globalEle = document.elementFromPoint(ev.x, ev.y);
        ev.preventDefault();
        mnuMain.popup(ev.x, ev.y);
        return false;
    }

    function doPasteAsText() {
        var clipboard = app.node.gui.Clipboard.get();
        var text = clipboard.get('text');
        var getResultStr = function (existedStr, selected, addedText) {
            first_occurance = existedStr.indexOf(selected);
            string_a_length = selected.toString().length;
            if (first_occurance == 0) {
                new_string = addedText + existedStr.substring(string_a_length);
            } else if (first_occurance > 0) {
                var stringFirst = existedStr.substring(0, first_occurance);
                var stringSecond = existedStr.substring(first_occurance + string_a_length);
                new_string = stringFirst + addedText + stringSecond;
            }
            return new_string;
        };
        if (globalEle != null && typeof globalEle != 'undefined') {
            app.log("className: " + globalEle.className.toString());
            app.log("globalEle.tagName: " + globalEle.tagName);
            if (globalEle.className.toString().indexOf("nicEdit-main") > -1) {
                var selected = "";
                selected = document.getSelection();
                if (selected != "") {
                    globalEle.innerHTML = getResultStr(globalEle.innerHTML, selected, text.toString());
                } else {
                    var pos = document.getSelection().getRangeAt(0).startOffset;
                    if (globalEle.innerHTML != "") {
                        var existedStr = globalEle.innerHTML;
                        globalEle.innerHTML = existedStr.substring(0, pos) + text.toString() + existedStr.substring(pos);
                    } else {
                        globalEle.innerHTML = text.toString();
                    }
                }
            } else if (globalEle.tagName.toString().indexOf("INPUT") > -1 || globalEle.tagName.toString().indexOf("TEXTAREA") > -1) {
                var selected = "";
                selected = document.getSelection();
                if (selected != "") {
                    globalEle.value = getResultStr(globalEle.value, selected, text.toString());
                } else {
                    var pos = getCaret(globalEle);
                    if (globalEle.value != "") {
                        var existedStr = globalEle.value;
                        globalEle.value = existedStr.substring(0, pos) + text.toString() + existedStr.substring(pos);
                    } else {
                        globalEle.value = text.toString();
                    }
                }
            }
        }
    }

    function getCaret(el) {
        if (el.selectionStart) {
            return el.selectionStart;
        } else if (document.selection) {
            el.focus();

            var r = document.selection.createRange();
            if (r == null) {
                return 0;
            }

            var re = el.createTextRange();
            var rc = re.duplicate();
            re.moveToBookmark(r.getBookmark());
            rc.setEndPoint('EndToStart', re);

            return rc.text.length;
        }
        return 0;
    }

    function doToggleFullscreen() {
        win.toggleFullscreen();

        var ele = document.getElementById('fullScreenModeIcon');
        if (typeof ele == 'undefined' || ele == null)
            return;
        if (win.isFullscreen)
            ele.style.backgroundPosition = "-384px -96px";
        else
            ele.style.backgroundPosition = "-360px -96px";
    }


})(jQuery, app);
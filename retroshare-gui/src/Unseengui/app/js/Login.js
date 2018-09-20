/**
 * Login
 * 
 * @author Unseen App team
 */

var unsene = unsene || {};
var app = unsene.app || {};

(function ($, app) {
    'use strict';
    
    /**
     * Check is logged in
     */
    app.isLoggedIn = function () {
        var href = window.location.href;

        if (href.indexOf("index.html") > -1) {
            return false;
        } else if (href.indexOf("chat.html") > -1) {
            return true;
        }

        return false;
    };

    app.onLoginSuccess = function () {
        var keychain = false;

        //store username and password to auto-login
        if (this.sysOSPlatform === "darwin" && this.compareVersions(app.sysOSVersion, '10.6')) {
            keychain = require('keychain');
        }

        var auto = document.getElementById("automaticlogin");
        if (auto && auto.checked) {
            store.set('username', unsene.loginView.model.get('username'));

            if (keychain) {
                try {
                    keychain.setPassword({
                        account: unsene.loginView.model.get('username'),
                        service: 'UnseenApp',
                        password: unsene.loginView.model.get('password')
                    }, function (err) {
                        app.log(err);
                    });
                } catch (e) {
                    app.log('#ERROR: keychain setPassword error ', e);
                }
            } else {
                store.set('password', unsene.encrypt(unsene.sha1($("#unseen_public_key").val() + app.sysOSVersion + app.appFullPath), unsene.loginView.model.get('password')));
            }
        }

        if (store.get("lang")) {
            app.log('>>> loading ' + store.get("lang") + "-chat.html?t=" + (new Date()).getTime());
            window.location.assign(store.get("lang") + "-chat.html?t=" + (new Date()).getTime());
        } else {
            window.location.assign("en_US-chat.html?t=" + (new Date()).getTime());
        }
    };

    app.autoLogin = function () {
        if (!store.get('username')) {
            return;
        }
        
        var keychain = false;
        if (app.sysOSPlatform === "darwin" && app.compareVersions(app.sysOSVersion, '10.6')) {
            keychain = require('keychain');
        }

        if (keychain) {
            app.log('>>> Auto Login by keychain with account: ' + store.get('username'));
            try {
                keychain.getPassword({
                    account: store.get('username'),
                    service: 'UnseenApp'
                }, function (err, password) {
                    if (password) {
                        document.getElementById("inputEmail").value = store.get('username');
                        document.getElementById("inputPassword").value = password;
                        document.getElementById("automaticlogin").checked = true;
                        document.getElementById("loginButton").click();
                    } else {
                        app.log(err);
                    }
                });
            } catch (e) {
                app.log('#ERROR: keychain getPassword error ', e);
                store.remove('username');
            }
        } else {
            app.log('>>> Auto Login by store with account: ' + store.get('username'));
            try {
                if (store.get('password')) {
                    document.getElementById("inputEmail").value = store.get('username');
                    document.getElementById("inputPassword").value = unsene.decrypt(unsene.sha1($("#unseen_public_key").val() + app.sysOSVersion + app.appFullPath), store.get('password'));
                    document.getElementById("automaticlogin").checked = true;
                    document.getElementById("loginButton").click();
                }
            } catch (e) {
                app.log('#ERROR: decrypt password error ', e);
                store.remove('username');
            }
            
        }

    };

})(jQuery, app);
/**
 * Auto Update.
 * 
 * @author Unseen App team
 */

var unsene = unsene || {};
var app = unsene.app || {};

(function ($, require, app) {
    'use strict';
    
    var $topLoader;
    var downloaded = 0;
    var $downloadUrl;
    
    app.autoUpdate = function () {
        //check the latest Release Notes to download and update:
        if (app.environment === "production") {
            $downloadUrl = "http://download.unseen.is/unseen-app-release-notes.json";
        } else if (app.environment === "tw") {
            $downloadUrl = "http://download.unseen.is/unseen-tw-release-notes.json";
        } else {
            $downloadUrl = "http://64.62.227.4/unseen-app-release-notes.json";
        }
        
        try {
            $.ajax({
                url: $downloadUrl,
                contentType: "application/json",
                dataType: 'json',
                cache: false,
                type: 'GET'
            }).fail(function (e) {
                app.log("Download unseen-app-release-notes.json fail ", e.toString());
            }).done(function (data) {
                app.log('>>> UnseenApp check to update ...');
                app.log($downloadUrl);
                
                if (!data) {
                    return false;
                }

                try {
                    var Updater = require('auto-updater');
                    var updater = new Updater(app.node.gui.App.manifest["version"], app.sysOSBit, app.sysOSPlatformName, app.sysOSTmpDir, app.appFolder, app.appFullPath, app.appName, data, app.environment);

                    app.log('Cur version: ' + updater.config.curVersion);
                    app.log('New version: ' + updater.config.newVersion);

                    if (updater.checkLatestPackage()) {
                        app.log("Updater config: ", updater.config);

                        var doAutoUpdate = function () {
                            app.showUpdating(true);

                            $topLoader = $("#topLoader").percentageLoader({
                                width: 180, height: 180, controllable: false, progress: 0.5, onProgressUpdate: function (val) {
                                    this.setValue(Math.round(val * 100.0) + 'KB');
                                }
                            });

                            updater.download(updater.config.source, updater.config.pkgFullPath, updater.config.progress, app.updatePercentageLoader, function (err) {
                                var pkgLink = "http://" + updater.config.source.host + "/" + updater.config.updPkgName + "/" + updater.config.fullPkgName;

                                if (err) {
                                    autoUpdateFinished(err, updater.config.newVersion, pkgLink);
                                } else {
                                    updater.update(function (msgFeedback) {
                                        var fs = require('fs');
                                        if (fs.existsSync(msgFeedback)) {
                                            app.log("New App:" + msgFeedback);
                                            app.appFullPath = msgFeedback;
                                            msgFeedback = "";
                                        }
                                        autoUpdateFinished(msgFeedback, updater.config.newVersion, pkgLink);
                                    });
                                }
                            });
                        };

                        var msgNewVersion = data[app.sysOSPlatformName + "-version"];
                        var msgUpdPkgName = data[app.sysOSPlatformName + "-pkg-ia" + app.sysOSBit];
                        var msgFullPkgName = data[app.sysOSPlatformName + "-full-ia" + app.sysOSBit];
                        var msgNewAppPath = "http://" + updater.config.source.host + "/" + msgUpdPkgName + "/" + msgFullPkgName;
                        msgNewAppPath.replace(/.zip/g, '.cap');
                        msgNewAppPath = "<a href='" + msgNewAppPath + "' target='_blank' style='color:#0084b4;'>" + $.i18n2.__("click here") + "</a> ";

                        var msg;
                        msg = "<p>" + $.i18n2.__("New update with version") + " v" + msgNewVersion + " " + $.i18n2.__("is available") + "!</h2>";
                        msg += "<p>" + $.i18n2.__("You can") + " " + msgNewAppPath + $.i18n2.__("to download and update manually.") + "</p>";
                        msg += "<p>" + $.i18n2.__("Click OK button to auto update to this version.");
                        app.showMessage($.i18n2.__("Unseen App"), msg, 2, doAutoUpdate);

                    }
                } catch (e) {
                    app.log("Can't download or update to the latest version!", e);
                }
            });
        } catch (e) {
            app.log("#ERROR:  Unseen App udpate error!", e);
        }

    };

    function autoUpdateFinished(message, newVersion, pkgLink) {
        app.showUpdating(false);
        pkgLink.replace(/.cap/g, '.zip');
        
        var msg;
        msg = "<div style='text-align:center;'><img style='height:100px;' src='asset/app/images/icon.png'></div>";
        msg += "<div style='text-align:center;'><h2>" + $.i18n2.__("Unseen App") + "</h2></div>";
        if (message) {
            msg += "<p>" + $.i18n2.__("Can't update to the latest version of Unseen App") + ": " + newVersion + "</p>";
            msg += "<p>" + $.i18n2.__("Please update manually") + ": <a id='dlgUsnUpdLnk' href='" + pkgLink + "' target='_blank' style='color:#0084b4;'>" + $.i18n2.__("Download") + "</a></p>";
            
            app.log("Auto Update error: \n" + msg);
            app.showMessage($.i18n2.__("Unseen App"), msg, 0);
        } else {
            msg += "<p>" + $.i18n2.__("Unseen App has been updated!<br>Current version is") + ": " + newVersion + "</p>";
            msg += "<p>" + $.i18n2.__("Please restart the app.") + "</p>";
            
            app.log("Auto Update done: \n" + msg);
            app.request2Update = true;
            app.showMessage($.i18n2.__("Unseen App"), msg, 2, app.doRestart);
        }
    }
    
    app.updatePercentageLoader = function (kb, totalKb) {
        $topLoader.percentageLoader({progress: kb / totalKb});

        var fSExt = new Array('Bytes', 'KB', 'MB', 'GB');
        var fSize = kb;
        var i = 0;
        while (fSize > 900) {
            fSize /= 1024;
            i++;
        }
        downloaded = (Math.round(fSize * 100) / 100) + ' ' + fSExt[i];

        $topLoader.percentageLoader({value: downloaded.toString()});
    };


})(jQuery, require, app);
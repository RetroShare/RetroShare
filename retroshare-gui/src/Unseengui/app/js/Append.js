/**
 * Open External
 * 
 * @author Unseen App team
 */

var unsene = unsene || {};
var app = unsene.app || {};

(function ($, unsene, app) {
    'use strict';

    app.applyNewScript = function () {
        if (app.isLoggedIn()) {
            // Pay with Safe Cash Modal
            $('#safecash-payment').on('shown.bs.modal', function () {
                $('#safecash-payment iframe').attr('src', unsene.BASE_URL + '/safecash.html');
            });

            // Update Online State on tray icon when user change status
            try {
                var ele = document.getElementsByClassName('status-available');
                if (ele.length > 1)
                    app.addEvent(ele[1], 'click', function () {
                        app.tray.icon = app.onlineStateIcon("online");
                    }, false);
                ele = document.getElementsByClassName('status-away');
                if (ele.length > 1)
                    app.addEvent(ele[1], 'click', function () {
                        app.tray.icon = app.onlineStateIcon("away");
                    }, false);
                ele = document.getElementsByClassName('status-unavailable');
                if (ele.length > 1)
                    app.addEvent(ele[1], 'click', function () {
                        app.tray.icon = app.onlineStateIcon("dnd");
                    }, false);
                ele = document.getElementsByClassName('status-invisible');
                if (ele.length > 1)
                    app.addEvent(ele[1], 'click', function () {
                        app.tray.icon = app.onlineStateIcon("invisible");
                    }, false);
            } catch (e) {
                app.log("#ERROR: Update online state error: " + e.message);
            }


            // Overwrite Notification Objects
            try {
                var setTitlebarNotifier = function () {
                    var titleCount = this.getTotalUnreadCount();
                    app.log("setTitlebarNotifier: titleCount = ", titleCount);
                    app.dispatchEvent("handleTitlebarNotifier", titleCount, true, true);
                    
                    if (unsene.unseneView) {
                        document.title = 'Unseen App - ' + unsene.unseneView.me.get("jid");
                    }
                };
                unsene.mainView.setTitlebarNotifier = setTitlebarNotifier.bind(unsene.mainView);
            } catch (e) {
                app.log('#ERROR: setTitlebarNotifier error ', e.message);
            }
            
            
            // Filter a href to openExternal
            $('#group-chat').delegate('a[target]', 'click', function (e) {
                e.preventDefault();
                e.stopPropagation();
                app.openExternal(e.target.href);
            });
            
            $('#unseen-tab-content').delegate('a[target]', 'click', function (e) {
                e.preventDefault();
                e.stopPropagation();
                app.openExternal(e.target.href);
            });
            
            $('#modal-open-external-link').delegate('a[target]', 'click', function (e) {
                e.preventDefault();
                e.stopPropagation();
                app.openExternal(e.target.href);
            });

        }

    };


    /**
     * Set Open External Link
     */
    app.updateLayout = function () {
        if (app.isLoggedIn()) {
            // Remove Email Menu
            try {
                // Load default for unseen_chat_recent_tab
                $('#user-menu .unseen_chat_recent_tab').tab('show');
                $("#mainApp").removeClass('mailbox');
                
                if (unsene.unseneView) {
                    unsene.unseneView.credential.set("isActiveEmail",false);
                    $(".unseen_email_link").parent().remove();
                }
            } catch (e) {
                app.log("#ERROR: Remove Email Menu error: " + e.message);
            }

            // Append Setting Menu
            var eleSetting = $(".logout").parent();
            eleSetting.append('<li id="logoutApp" onclick="app.doLogout()"><a href="#"><i class="fa fa-sign-out"></i> ' + $.i18n2.__("Logout") + '</a></li>');
            eleSetting.append('<li id="quitApp" onclick="app.doQuit()"><a href="#"><i class="fa fa-power-off"></i> ' + $.i18n2.__("Quit Unseen") + '</a></li>');
            $(".logout").remove();
            
            
            // Append About Menu
            try {
                var eleHelpMenuList = document.getElementById("ul-menu").childNodes[11].childNodes[3];
                var eleHelpAbout = document.getElementById('usnAppAbout');

                if (typeof eleHelpMenuList != 'undefined' && eleHelpMenuList != null && (typeof eleHelpAbout == 'undefined' || eleHelpAbout == null)) {
                    eleHelpAbout = document.createElement('li');
                    eleHelpAbout.setAttribute('id', "usnAppAbout");
                    eleHelpAbout.innerHTML = '<a href="#"><i class="fa fa-info-circle" style="margin-right:10px"></i>\n' + $.i18n2.__("About Unseen App") + '</a>';
                    app.addEvent(eleHelpAbout, 'click', app.showAboutInfo, false);
                    eleHelpMenuList.appendChild(eleHelpAbout);
                }
            } catch (e) {
                app.log("#ERROR: Add About menu error: " + e.message);
            }
            
            //hide Safecash button upgrade
            $('.upgrade-with-safecash').hide();
            
            //append links to open external
            $('a[href^="http"]').on("click", function (ev) {
                ev.preventDefault();
                app.openExternal($(this).attr('href'));
            });

            $('a[href="affiliate.html"]').parent().remove();
            $('a[href="affiliate.html"]').on("click", function (ev) {
                ev.preventDefault();
                app.openExternal(unsene.BASE_URL + '/affiliate.html');
            });

            $('a[href="gettingstarted.html"]').on("click", function (ev) {
                ev.preventDefault();
                app.openExternal(unsene.BASE_URL + '/gettingstarted.html');
            });

            $('a[href="faq.html"]').on("click", function (ev) {
                ev.preventDefault();
                app.openExternal(unsene.BASE_URL + '/faq.html');
            });

            $('a[href="privacy.html"]').on("click", function (ev) {
                ev.preventDefault();
                app.openExternal(unsene.BASE_URL + '/privacy.html');
            });

            $('a[href="term.html"]').on("click", function (ev) {
                ev.preventDefault();
                app.openExternal(unsene.BASE_URL + '/term.html');
            });
            
            $('a[href="admin.html"]').on("click", function (ev) {
                ev.preventDefault();
                app.openExternal(unsene.BASE_URL + '/admin.html');
            });
        } else {
            $('a[href="/"]').on("click", function (ev) {
                ev.preventDefault();
                app.openExternal(unsene.BASE_URL);
            });

            $('a[href="/payment.html"]').on("click", function (ev) {
                ev.preventDefault();
                app.openExternal(unsene.BASE_URL + $(this).attr('href'));
            });

            $('a[href="/plans.html"]').on("click", function (ev) {
                ev.preventDefault();
                app.openExternal(unsene.BASE_URL + $(this).attr('href'));
            });

            $('a[href="/term.html"]').on("click", function (ev) {
                ev.preventDefault();
                app.openExternal(unsene.BASE_URL + $(this).attr('href'));
            });

            // Change Language
            $('a[href="/?lang=zh_CN"]').on("click", function (ev) {
                ev.preventDefault();
                window.location.replace('zh_CN-index.html');
            });

            $('a[href="/?lang=zh_TW"]').on("click", function (ev) {
                ev.preventDefault();
                window.location.replace('zh_TW-index.html');
            });

            $('a[href="/?lang=en"]').on("click", function (ev) {
                ev.preventDefault();
                window.location.replace('en_US-index.html');
            });
            
            //add Auto Login
            var ele = $("#inputPassword").parent();
            ele.append('<input type="checkbox" id="automaticlogin"/><label for="automaticlogin" style="padding:5px 0 0 5px;margin-top: -10px;color:#008484;font-weight:normal;cursor:pointer;"> ' + $.i18n2.__("Automatic login") + '</label>');

        }
        
        // Open external for link on dialog-message
        $('#dialog-message').delegate('a[target]', 'click', function (e) {
            e.preventDefault();
            e.stopPropagation();
            app.openExternal(e.target.href);
        });
        
        // Overwrite Unseen onExit
        unsene.onExit = function () {
            app.log("onExit UnseenApp");

            window.onbeforeunload = null;
            $(window).bind('unload', function () {
                //only if user login already, then logout this user
                if (unsene.unseneView.me.get("username")) {
                    unsene.logoutWebApp(true);
                }
            });
        };
        
        // Overwrite Unseen download function
        var fs = require("fs");
        unsene._downloadFile = function (file_url, filePath, divFile) {
            if (app.sysOSPlatform == "darwin") {
                var fileName = divFile['context']['attributes'][3]['value'];
                if (fileName !=  null || typeof fileName != 'undefined') {
                    var fileExtension = fileName.split('.').pop();
                    if (fileExtension.length > 0) {
                        filePath = filePath + '.' + fileExtension;
                    }
                }
                app.log('fileName', fileName);
            }
            
            var fBasename = filePath.replace(/\\/g,'/').replace( /.*\//, '' );
            var fDirname = filePath.replace(/\\/g,'/').replace(/\/[^\/]*$/, '');
            store.set('nwsaveas_path', fDirname + '/');

            app.log('Download file: ' + file_url + ' -TO-  ' + filePath);
            app.log('divFile: ', divFile);
            
            var updateProgress = function (evt) {
                if (evt.lengthComputable) {  //evt.loaded the bytes browser receive

                    //evt.total the total bytes seted by the header
                    var percentComplete = (evt.loaded / evt.total) * 100;
                    percentComplete = Math.floor(percentComplete);

                    var progress_bar_blue = divFile.find('.progress-blue');
                    if (progress_bar_blue.length == 0) {
                        divFile.append($('<div class="progress-bar progress-blue"><div class="progress-value"></div></div>'));
                    }

                    if (divFile.find('.progress-blue .progress-value').find('.file-meta').length == 0) {
                        divFile.find('.progress-blue .progress-value').append(divFile.find('.file-meta'));
                    }

                    divFile.find('.btn-download').hide();
                    divFile.find('.btn-download-c').hide();

                    divFile.find('.file-label').html('Downloading: ');
                    divFile.find('.progress-blue').show();
                    divFile.find('.progress-blue').removeClass('hidden');
                    divFile.find('.progress-blue').addClass('progress-blue');
                    divFile.find('.progress-blue .progress-value').css('width', percentComplete + '%');
                    if (percentComplete >= 100) {
                        divFile.find('.progress-blue').remove();
                        divFile.find('.file-label').html('Send a file:');
                    }
                }
            };

            var xmlhttp = new XMLHttpRequest();
            xmlhttp.open("GET", file_url, true);
            xmlhttp.responseType = "arraybuffer";
            xmlhttp.onprogress = updateProgress;

            xmlhttp.onload = function (oEvent) {
                var arrayBuffer = xmlhttp.response; // Note: not xmlhttp.responseText
                if (arrayBuffer) {
                    var byteArray = new Uint8Array(arrayBuffer);
                    app.log('file lenght: ' + byteArray.byteLength);
                    var buffer = new Buffer(byteArray.length);

                    // save byte array to buffer
                    for (var i = 0; i < byteArray.length; i++) {
                        buffer.writeUInt8(byteArray[i], i);
                    }

                    // save to file
                    try {
                        fs.writeFileSync(filePath, buffer);
                    } catch (e) {
                        app.log('#ERROR: Save file error', e.message);
                    }

                    // show button open file
                    divFile.find('.file-info').append($('<a class="btn btn-success btn-small btn-Open"><i class="icon-inbox icon-white"></i>Open</a>'));
                    divFile.find('.btn-Open').click(function () {
                        var exec = require('child_process').exec, child;
                        
                        try {
                            if (app.sysOSPlatform === "darwin") {
                                child = exec('open "' + filePath + '"');
                            } else if (app.sysOSPlatform == "win32" || app.sysOSPlatform == "win64") {
                                child = exec('"' + filePath + '"');
                            } else if (app.sysOSPlatform == "linux") {
                                var cmdOpenFile = 'cd "' + fDirname + '" &&  xdg-open ./"' + fBasename + '"';
                                app.log("cmd open file: " + cmdOpenFile);
                                child = exec(cmdOpenFile);
                            }
                        } catch (e) {
                            app.log('#ERROR: Open file error', e.message);
                        }
                    });
                }
            };

            xmlhttp.send(null);
        };
        
        
        

    };

})(jQuery, unsene, app);


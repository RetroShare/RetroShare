libresapi: resource_api and new webinterface
============================================

* ./api contains a C++ backend to control retroshare from webinterfaces or scripting
* ./webfiles contains compiled files for the webinterface
* ./webui contains HTML/CSS/JavaScript source files for the webinterface (OLD)
* ./webui-src contains HTML/CSS/JavaScript source files for the webinterface (NEW, webinterface made with mithril.js)

Quickinfo for builders and packagers
====================================

* copy the files in ./webfiles to
* ./webui (Windows)
* /usr/share/RetroShare06/webui (Linux)
* other OS: see RsAccountsDetail::PathDataDirectory()
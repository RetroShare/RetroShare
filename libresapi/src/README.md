libresapi: resource_api and new webinterface
============================================

* ./api contains a C++ backend to control retroshare from webinterfaces or scripting
* ./webfiles contains compiled files for the webinterface
* ./webui contains HTML/CSS/JavaScript source files for the webinterface

Quickinfo for builders and packagers
====================================

* copy the files in ./webfiles to
* ./webui (Windows)
* /usr/share/RetroShare0.6/webui (Linux)
* other OS: see RsAccountsDetail::PathDataDirectory()
REACT_VERSION = 0.13.1

DISTDIR = ../webfiles
JSEXTLIBS = $(DISTDIR)/react.js $(DISTDIR)/JSXTransformer.js 
JSLIBS = RsXHRConnection.js RsApi.js
HTML = index.html
JSGUI = gui.jsx
CSS = green-black.css

all: $(DISTDIR) $(JSEXTLIBS) $(addprefix $(DISTDIR)/, $(JSLIBS)) $(addprefix $(DISTDIR)/, $(HTML)) $(addprefix $(DISTDIR)/, $(JSGUI)) $(addprefix $(DISTDIR)/, $(CSS))
.PHONY: all

$(DISTDIR)/livereload: $(DISTDIR) $(JSEXTLIBS) $(addprefix $(DISTDIR)/, $(JSLIBS)) $(addprefix $(DISTDIR)/, $(HTML)) $(addprefix $(DISTDIR)/, $(JSGUI)) $(addprefix $(DISTDIR)/, $(CSS))
	wget -qO- http://localhost:9090/api/v2/livereload/trigger
	touch $(DISTDIR)/livereload

$(DISTDIR)/react.js:
	cd $(DISTDIR) && wget --no-check-certificate --output-document react.js http://fb.me/react-$(REACT_VERSION).js

$(DISTDIR)/JSXTransformer.js:
	cd $(DISTDIR) && wget --no-check-certificate --output-document JSXTransformer.js http://fb.me/JSXTransformer-$(REACT_VERSION).js

$(addprefix $(DISTDIR)/, $(JSLIBS)): $(DISTDIR)/%: %
	cp $< $@

$(addprefix $(DISTDIR)/, $(HTML)): $(DISTDIR)/%: %
	cp $< $@

$(addprefix $(DISTDIR)/, $(JSGUI)): $(DISTDIR)/%: %
	cp $< $@

$(addprefix $(DISTDIR)/, $(CSS)): $(DISTDIR)/%: %
	cp $< $@

$(DISTDIR):
	mkdir $(DISTDIR)

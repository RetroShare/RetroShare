REACT_VERSION = 0.13.1

JSEXTLIBS = dist/react.js dist/JSXTransformer.js 
JSLIBS = RsXHRConnection.js RsApi.js
HTML = index.html
JSGUI = gui.jsx
CSS = green-black.css

all: dist $(JSEXTLIBS) $(addprefix dist/, $(JSLIBS)) $(addprefix dist/, $(HTML)) $(addprefix dist/, $(JSGUI)) $(addprefix dist/, $(CSS))
.PHONY: all

dist/livereload: dist $(JSEXTLIBS) $(addprefix dist/, $(JSLIBS)) $(addprefix dist/, $(HTML)) $(addprefix dist/, $(JSGUI)) $(addprefix dist/, $(CSS))
	wget -qO- http://localhost:9090/api/v2/livereload/trigger
	touch dist/livereload

dist/react.js:
	cd dist && wget --no-check-certificate --output-document react.js http://fb.me/react-$(REACT_VERSION).js

dist/JSXTransformer.js:
	cd dist && wget --no-check-certificate --output-document JSXTransformer.js http://fb.me/JSXTransformer-$(REACT_VERSION).js

$(addprefix dist/, $(JSLIBS)): dist/%: %
	cp $< $@

$(addprefix dist/, $(HTML)): dist/%: %
	cp $< $@

$(addprefix dist/, $(JSGUI)): dist/%: %
	cp $< $@

$(addprefix dist/, $(CSS)): dist/%: %
	cp $< $@

dist:
	mkdir dist

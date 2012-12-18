TEMPLATE = subdirs

SUBDIRS += \
		LinksCloud \
		VOIP

# disabled until fixed.
win32 {
	SUBDIRS += FeedReader
}

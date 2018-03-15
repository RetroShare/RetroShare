#define RS_MAJOR_VERSION     0
#define RS_MINOR_VERSION     6
#define RS_BUILD_NUMBER      4
#define RS_BUILD_NUMBER_ADD  "" // <-- do we need this?
// The revision number should be the 4 first bytes of the git revision hash, which is obtained using:
//    git log --pretty="%H" | head -1 | cut -c1-8
//
// Do not forget the 0x, since the RS_REVISION_NUMBER should be an integer.
//
#define RS_REVISION_STRING   "01234567"
#define RS_REVISION_NUMBER   0x01234567

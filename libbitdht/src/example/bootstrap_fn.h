
#include <string>
#include <inttypes.h>


/* NOTE. At the moment only the bootstrapfile is actually used.
 * peerId is ignored (a random peerId is searched for). ip & port are not filled in either.
 *
 * This is mainly to finish testing.
 *
 * Once the best form of the return functions is decided (ipv4 structure, or strings).
 * this can be finished off.
 *
 */

bool bdSingleShotFindPeer(const std::string bootstrapfile, const std::string peerId, std::string &ip, uint16_t &port);


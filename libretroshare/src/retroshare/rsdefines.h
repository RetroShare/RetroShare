/**************************************************************************************************************************************************
 *
 * V07_NON_BACKWARD_COMPATIBLE_CHANGE_001:
 * 
 *    What: Computes the node id by performing a sha256 hash of the certificate's PGP signature, instead of simply picking up the last 20 bytes of it. 
 *    
 *    Why: There is no real risk in forging a certificate with the same ID as the authentication is performed over the PGP signature of the certificate
 *          which hashes the full SSL certificate (i.e. the full serialized CERT_INFO structure). However the possibility to 
 *          create two certificates with the same IDs is a problem, as it can be used to cause disturbance in the software.
 * 
 *    Backward compat: makes connexions impossible with non patched peers, probably because the SSL id that is computed is not the same on both side, 
 *                   and in particular unpatched peers see a cerficate with ID different (because computed with the old method) than the ID that was 
 *                   submitted when making friends.
 * 
 *    Note: the advantage of basing the ID on the signature rather than the public key is not very clear, given that the signature is based on a hash 
 *          of the public key (and the rest of the certificate info).
 * 
 * V07_NON_BACKWARD_COMPATIBLE_CHANGE_002:
 * 
 *    What: Use RSA+SHA256 instead of RSA+SHA1 for PGP certificate signatures
 * 
 *    Why:  Sha1 is likely to be prone to primary collisions anytime soon, so it is urgent to turn to a more secure solution.
 * 
 *    Backward compat: unpatched peers are able to verify signatures since openpgp-sdk already handle it.
 * 
 * V07_NON_BACKWARD_COMPATIBLE_CHANGE_003:
 * 
 *     What: Do not hash PGP certificate twice when signing
 * 
 * 	 Why: hasing twice is not per se a security issue, but it makes it harder to change the settings for hashing.
 * 
 * 	 Backward compat: patched peers cannot connect to non patched peers.
 ***************************************************************************************************************************************************/

#define  V07_NON_BACKWARD_COMPATIBLE_CHANGE_001 False
#define  V07_NON_BACKWARD_COMPATIBLE_CHANGE_002 False
#define  V07_NON_BACKWARD_COMPATIBLE_CHANGE_003 False

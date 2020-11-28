#include "i2pcommon.h"

#include "util/rsbase64.h"
#include "util/rsdebug.h"

namespace i2p {

std::string keyToBase32Addr(const std::string &key)
{
	std::string copy(key);

	// replace I2P specific chars
	std::replace(copy.begin(), copy.end(), '~', '/');
	// replacing the - with a + is not necessary, as RsBase64 can handle base64url encoding, too
	// std::replace(copy.begin(), copy.end(), '-', '+');

	// decode
	std::vector<uint8_t> bin;
	RsBase64::decode(copy, bin);

	// hash
	std::vector<uint8_t> sha256 = RsUtil::BinToSha256(bin);
	// encode
	std::string out = Radix32::encode(sha256);

	// i2p uses lowercase
	std::transform(out.begin(), out.end(), out.begin(), ::tolower);
	out.append(".b32.i2p");

	return out;
}

const std::string makeOption(const std::string &lhs, const int8_t &rhs) {
	return lhs + "=" + std::to_string(rhs);
}

uint16_t readTwoBytesBE(std::vector<uint8_t>::const_iterator &p)
{
	uint16_t val = 0;
	val += *p++;
	val <<= 8;
	val += *p++;
	return val;
}

std::string publicKeyFromPrivate(std::string const &priv)
{
	/*
	 * https://geti2p.net/spec/common-structures#destination
	 * https://geti2p.net/spec/common-structures#keysandcert
	 * https://geti2p.net/spec/common-structures#certificate
	 */
	if (priv.length() < privKeyMinLenth_b64) {
		RS_WARN("key to short!");
		return std::string();
	}

	// creat a copy to work on, need to convert it to standard base64
	auto priv_copy(priv);
	std::replace(priv_copy.begin(), priv_copy.end(), '~', '/');
	// replacing the - with a + is not necessary, as RsBase64 can handle base64url encoding, too
	// std::replace(copy.begin(), copy.end(), '-', '+');

	// get raw data
	std::vector<uint8_t> dataPriv;
	RsBase64::decode(priv_copy, dataPriv);

	auto p = dataPriv.cbegin();
	RS_DBG("dataPriv.size ", dataPriv.size());

	size_t publicKeyLen = 256 + 128; // default length (bytes)
	uint8_t certType = 0;
	uint16_t len = 0;
	uint16_t signingKeyType = 0;
	uint16_t cryptKeyType = 0;

	// only used for easy break
	do {
		try {
			// jump to certificate
			p += publicKeyLen;

			// try to read type and length
			certType = *p++;
			len = readTwoBytesBE(p);

			// only 0 and 5 are used / valid at this point
			// check for == 0
			if (certType == static_cast<typename std::underlying_type<CertType>::type>(CertType::Null)) {
				/*
				 * CertType.Null
				 * type null is followed by 0x00 0x00 <END>
				 * so len has to be 0!
				 */
				RS_DBG("cert is CertType.Null");
				publicKeyLen += 3; // add 0x00 0x00 0x00

				if (len != 0)
					// weird
					RS_DBG("cert is CertType.Null but len != 0");

				break;
			}

			// check for != 5
			if (certType != static_cast<typename std::underlying_type<CertType>::type>(CertType::Key)) {
				// unsupported
				RS_DBG("cert type ", certType, " is unsupported");
				return std::string();
			}

			RS_DBG("cert is CertType.Key");
			publicKeyLen += 7; // <type 1B> <len 2B> <keyType1 2B> <keyType2 2B> = 1 + 2 + 2 + 2 = 7 bytes

			/*
			 * "Key certificates were introduced in release 0.9.12. Prior to that release, all PublicKeys were 256-byte ElGamal keys, and all SigningPublicKeys were 128-byte DSA-SHA1 keys."
			 * --> there is space for 256+128 bytes, longer keys are splitted and appended to the certificate
			 * We don't need to bother with the splitting here as only the lenght is important!
			 */

			// Signing Public Key
			// likely 7
			signingKeyType = readTwoBytesBE(p);

			RS_DBG("signing pubkey type ", signingKeyType);
			if (signingKeyType >= 3 && signingKeyType <= 6) {
				RS_DBG("signing pubkey type ", certType, " has oversize");
				// calculate oversize

				if (signingKeyType >= signingKeyLengths.size()) {
					// just in case
					RS_DBG("signing pubkey type ", certType, " cannot be found in size data!");
					return std::string();
				}

				auto values = signingKeyLengths[signingKeyType];
				if (values.first <= 128) {
					// just in case, it's supposed to be larger!
					RS_DBG("signing pubkey type ", certType, " is oversize but size calculation would underflow!");
					return std::string();
				}

				publicKeyLen += values.first - 128; // 128 = default DSA key length = the space that can be used before the key must be splitted
			}

			// Crypto Public Key
			// likely 0
			cryptKeyType = readTwoBytesBE(p);
			RS_DBG("crypto pubkey type ", cryptKeyType);
			// info: these are all smaller than the default 256 bytes, so no oversize calculation is needed

			break;
		}  catch (const std::out_of_range &e) {
			RS_DBG("hit an exception! ", e.what());
			return std::string();
		}
	} while(false);

	std::string pub;
	auto data2 = std::vector<uint8_t>(dataPriv.cbegin(), dataPriv.cbegin() + publicKeyLen);
	RsBase64::encode(data2.data(), data2.size(), pub, false, false);

	return pub;
}

bool getKeyTypes(const std::string &key, std::string &signingKey, std::string &cryptoKey)
{
	if (key.length() < pubKeyMinLenth_b64) {
		RS_WARN("key to short!");
		return false;
	}

	// creat a copy to work on, need to convert it to standard base64
	auto key_copy(key);
	std::replace(key_copy.begin(), key_copy.end(), '~', '/');
	// replacing the - with a + is not necessary, as RsBase64 can handle base64url encoding, too
	// std::replace(copy.begin(), copy.end(), '-', '+');

	// get raw data
	std::vector<uint8_t> data;
	RsBase64::decode(key_copy, data);

	auto p = data.cbegin();

	constexpr size_t publicKeyLen = 256 + 128; // default length (bytes)
	uint8_t certType = 0;
	uint16_t signingKeyType = 0;
	uint16_t cryptKeyType = 0;

	// try to read types
	try {
		// jump to certificate
		p += publicKeyLen;

		// try to read type and skip length
		certType = *p++;
		p += 2;

		// only 0 and 5 are used / valid at this point
		// check for == 0
		if (certType == static_cast<typename std::underlying_type<CertType>::type>(CertType::Null)) {
			RS_DBG("cert is CertType.Null");

			signingKey = "DSA_SHA1";
			cryptoKey = "ElGamal";
			return true;
		}

		// check for != 5
		if (certType != static_cast<typename std::underlying_type<CertType>::type>(CertType::Key)) {
			// unsupported
			RS_DBG("cert type ", certType, " is unsupported");
			return false;
		}

		RS_DBG("cert is CertType.Key");

		// Signing Public Key
		// likely 7
		signingKeyType = readTwoBytesBE(p);
		RS_DBG("signing pubkey type ", signingKeyType);

		// Crypto Public Key
		// likely 0
		cryptKeyType = readTwoBytesBE(p);
		RS_DBG("crypto pubkey type ", cryptKeyType);
	}  catch (const std::out_of_range &e) {
		RS_DBG("hit an exception! ", e.what());
		return false;
	}

	// now convert to string (this would be easier with c++17)
#define HELPER(a, b, c) \
	case static_cast<typename std::underlying_type<a>::type>(a::c): \
	    b = #c; \
	    break;

	switch (signingKeyType) {
	HELPER(SigningKeyType, signingKey, DSA_SHA1)
	HELPER(SigningKeyType, signingKey, ECDSA_SHA256_P256)
	HELPER(SigningKeyType, signingKey, ECDSA_SHA384_P384)
	HELPER(SigningKeyType, signingKey, ECDSA_SHA512_P521)
	HELPER(SigningKeyType, signingKey, RSA_SHA256_2048)
	HELPER(SigningKeyType, signingKey, RSA_SHA384_3072)
	HELPER(SigningKeyType, signingKey, RSA_SHA512_4096)
	HELPER(SigningKeyType, signingKey, EdDSA_SHA512_Ed25519)
	HELPER(SigningKeyType, signingKey, EdDSA_SHA512_Ed25519ph)
	HELPER(SigningKeyType, signingKey, RedDSA_SHA512_Ed25519)
	default:
	    RsWarn("unkown signing key type:", signingKeyType);
	    return false;
	}

	switch (cryptKeyType) {
	HELPER(CryptoKeyType, cryptoKey, ElGamal)
	HELPER(CryptoKeyType, cryptoKey, P256)
	HELPER(CryptoKeyType, cryptoKey, P384)
	HELPER(CryptoKeyType, cryptoKey, P521)
	HELPER(CryptoKeyType, cryptoKey, X25519)
	default:
	    RsWarn("unkown crypto key type:", cryptKeyType);
	    return false;
	}
#undef HELPER

	return true;
}

} // namespace i2p

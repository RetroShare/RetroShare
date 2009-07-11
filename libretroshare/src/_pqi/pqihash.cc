#include "pqihash.h"


pqihash::pqihash()
{

    sha_hash = new uint8_t[SHA_DIGEST_LENGTH];
    sha_ctx = new SHA_CTX;
    SHA1_Init(sha_ctx);
    doHash = true;
}

pqihash::~pqihash()
{
    delete[] sha_hash;
    delete sha_ctx;
}


void pqihash::addData(void *data, uint32_t len)
{
    if (doHash)
    {
        SHA1_Update(sha_ctx, data, len);
    }
}

void pqihash::Complete(std::string &hash)
{
    if (!doHash)
    {
        hash = endHash;
        return;
    }

    SHA1_Final(sha_hash, sha_ctx);

    std::ostringstream out;
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        out << std::setw(2) << std::setfill('0') << std::hex;
        out << (unsigned int) (sha_hash[i]);
    }
    endHash = out.str();
    hash = endHash;
    doHash = false;
}


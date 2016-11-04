#include <gtest/gtest.h>

// from libretroshare

#include "crypto/chacha20.h"

TEST(libretroshare_crypto, ChaCha20)
{
    std::cerr << "Testing Chacha20" << std::endl;

    EXPECT_TRUE(librs::crypto::perform_tests()) ;
}

/*

    RC4


    Created on 2025.1.6 at Jiangchuan, Minhang, Shanghai

    gongty  [at] tongji [dot] edu [dot] cn
    feng.yt [at]  sjtu  [dot] edu [dot] cn

    
*/


#pragma once

#include <adl/sys/types.h>

namespace adl { class TString; }

namespace monkey::crypto {
    void rc4(unsigned char* data, adl::size_t dataLen, const adl::TString& key);
}


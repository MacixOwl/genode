/*

    RC4


    Created on 2025.1.6 at Jiangchuan, Minhang, Shanghai

    gongty  [at] tongji [dot] edu [dot] cn
    feng.yt [at]  sjtu  [dot] edu [dot] cn

  
    Reference:
        https://blog.csdn.net/weixin_45582916/article/details/121429688
    
*/

#include <adl/collections/ArrayList.hpp>


static void rc4Init(unsigned char* s, const adl::ByteArray& key) {
    int j = 0;
    char k[256] = { 0 };
    unsigned char tmp = 0;
    for (unsigned int i = 0; i < 256; i++) {
        s[i] = (unsigned char) i;
        k[i] = (char) key[i % key.size()];
    }
    for (unsigned int i = 0; i < 256; i++) {
        j = (j + s[i] + k[i]) % 256;
        tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
    }
}


namespace monkey::crypto {

void rc4(unsigned char* data, adl::size_t dataLen, const adl::ByteArray& key) {
    unsigned char s[256];
    rc4Init(s, key);
    int i = 0, j = 0, t = 0;
    unsigned long k = 0;
    unsigned char tmp;
    for (k = 0; k < dataLen; k++) {
        i = (i + 1) % 256;
        j = (j + s[i]) % 256;
        tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
        t = (s[i] + s[j]) % 256;
        data[k] = data[k] ^ s[t];
    }
}



}

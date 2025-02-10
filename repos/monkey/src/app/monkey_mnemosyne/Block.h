/*

    Memory Block

    Created on 2025.2.7 at Xiangzhou, Zhuhai, Guangdong

    gongty [at] tongji [dot] edu [dot] cn

*/


#pragma once

#include <adl/sys/types.h>

struct Block {
    adl::size_t size = 0;
    char* data = nullptr;
};


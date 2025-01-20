/*
 * IPv4 Addr
 *
 * Created on 2025.1.20 at Zhuhai, Guangdong
 * 
 * 
 * gongty  [at] tongji [dot] edu [dot] cn
 * 
 */

#include <monkey/net/IP4Addr.h>
#include <adl/TString.h>

using namespace adl;

namespace monkey::net {

TString IP4Addr::toString() const {
    adl::TString ipStr;
    
    ipStr += adl::TString::to_string(ui8arr[0]);    
    ipStr += '.';
    ipStr += adl::TString::to_string(ui8arr[1]);
    ipStr += '.';
    ipStr += adl::TString::to_string(ui8arr[2]);
    ipStr += '.';
    ipStr += adl::TString::to_string(ui8arr[3]);
    return ipStr;
}


}

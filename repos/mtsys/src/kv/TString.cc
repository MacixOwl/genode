/* 2051565 GTY Tongji CS */
// Ported from TJ OOP HW. Modified for Amkos.

#define _CRT_SECURE_NO_WARNINGS

#include <util/string.h>
#include <base/exception.h>
#include <base/allocator.h>

#include <kv/TString/TString.h>
#include <kv/TString/TStringAdv.h>
using namespace std;


namespace MtsysKv {

Genode::Allocator* tstring_alloc = nullptr;


TString::TString()
{
    content = nullptr;
    len = 0;
}

TString::TString(const char* str)
{
    if (tstring_alloc == nullptr) {
        Genode::error("ops... allocator not availabel!\n");
    }

    if (str == nullptr || str[0] == '\0') {
        content = nullptr;
        len = 0;
    }
    else {
        len = Genode::strlen(str);

        content = new(tstring_alloc) char[len + 1];

        if (content == nullptr) {
            throw Genode::Exception{};
        }


        Genode::strcpy(content, str);
    }
}

#if defined(__GNUC__)

#else
TString::TString(const int x)
{
    if (x == 0) { // x == NULL
        len = 0;
        content = nullptr;
    }
    else {
        len = 0;
        content = nullptr;
    }
}
#endif

TString::TString(const TString& str)
{
    if (str.length() == 0) {
        content = nullptr;
        len = 0;
    }
    else {
        len = str.length();
        content = new(tstring_alloc) char[len + 1];
        if (content == nullptr) {
            throw Genode::Exception{};
        }

        Genode::strcpy(content, str.c_str());
    }
}

TString::~TString()
{
    freeUp();
}

inline void TString::freeUp()
{
    if (content != nullptr) {
        freeUpContent();
        len = 0;
    }
}

inline void TString::freeUpContent()
{
    if (content != nullptr) {
        delete[] (content, tstring_alloc);
        content = nullptr;
    }
}

int TString::length() const
{
    return len;
}

const char* TString::c_str() const
{
    return content;
}

TString& TString::operator=(const TString& str)
{
    if (&str == this) {
        return *this;
    }

    freeUp();

    if (str.length() == 0) {
        return *this;
    }
    else {
        content = new(tstring_alloc) char[str.length() + 1];
        if (content == nullptr) {
            throw Genode::Exception{};
        }

        Genode::strcpy(content, str.c_str());
        len = str.length();
        return *this;
    }
}

TString& TString::operator=(const char* str)
{
    freeUp();

    if (str == nullptr || str[0] == '\0') {
        return *this;
    }
    else {
        len = Genode::strlen(str);
        content = new(tstring_alloc) char[len + 1];
        if (content == nullptr) {
            throw Genode::Exception{};
        }

        Genode::strcpy(content, str);
        return *this;
    }
}

const TString TString::operator+(const TString& str) const
{
    TString ret;

    ret.len = this->len + str.len;

    if (ret.len == 0) {
        return ret;
    }

    ret.content = new(tstring_alloc) char[ret.len + 1];
    if (ret.content == nullptr) {
        throw Genode::Exception{};
    }


    if (this->content != nullptr) {
        Genode::memcpy(ret.content, this->content, this->len * sizeof(char));
    }
    if (str.c_str() != nullptr) {
        Genode::memcpy(ret.content + this->len, str.c_str(), str.length() * sizeof(char));
    }
    ret.content[ret.len] = '\0';

    return ret;
}

const TString TString::operator+(const char* str) const
{
    TString ret;

    int lengthOfStr = (str == nullptr ? 0 : Genode::strlen(str));

    ret.len = this->len + lengthOfStr;

    if (ret.len == 0) {
        return ret;
    }

    ret.content = new(tstring_alloc) char[ret.len + 1];
    if (ret.content == nullptr) {
        throw Genode::Exception{};
    }

    if (this->content != nullptr) {
        Genode::memcpy(ret.content, this->content, this->len * sizeof(char));
    }

    Genode::memcpy(ret.content + this->len, str, lengthOfStr * sizeof(char));
    ret.content[ret.len] = '\0';

    return ret;
}

const TString operator+(const char* strA, const TString& strB)
{
    TString ret;

    int lengthOfStrA = (strA == nullptr ? 0 : Genode::strlen(strA));
    ret.len = lengthOfStrA + strB.length();

    if (ret.len == 0) {
        return ret;
    }

    ret.content = new(tstring_alloc) char[ret.len + 1];
    if (ret.content == nullptr) {
        throw Genode::Exception{};
    }

    Genode::memcpy(ret.content, strA, lengthOfStrA * sizeof(char));
    Genode::memcpy(ret.content + lengthOfStrA, strB.c_str(), strB.length() * sizeof(char));
    ret.content[ret.len] = '\0';

    return ret;
}

const TString TString::operator+(const char c) const
{
    char tmp[2] = { c, '\0' };
    return *this + tmp;
}

const TString TString::operator!() const
{
    TString ret = *this;
    if (ret.length() >= 2) {
        char c;
        for (int i = 0; i < ret.length() / 2; i++) {
            c = ret[i];
            ret[i] = ret[ret.length() - 1 - i];
            ret[ret.length() - 1 - i] = c;
        }
    }

    return ret;

}

const TString TString::operator-(const TString& str) const
{
    if (str.len == 0 || this->len == 0) {
        return *this;
    }
    else {
        char* occurenceLocation = Genode::strstr(content, str.content);
        if (occurenceLocation == nullptr) {
            return *this;
        }
        else if (len == str.length()) {
            return TString();
        }
        else {
            char* p = new(tstring_alloc) char[len - str.length() + 1];
            if (p == nullptr) {
                throw Genode::Exception{};
            }
            TString ret;
            ret.len = len - str.length();
            ret.content = p;

            Genode::memcpy(p, content, (occurenceLocation - content) * sizeof(char));
            Genode::memcpy(
                p + (occurenceLocation - content),
                occurenceLocation + str.length(),
                (len - (occurenceLocation - content) - str.length()) * sizeof(char)
            );
            p[ret.len] = '\0';

            return ret;
        }
    }
}

const TString TString::operator-(const char* str) const
{
    if (str == nullptr || str[0] == '\0' || this->len == 0) {
        return *this;
    }
    else {
        int lengthOfStr = Genode::strlen(str);

        char* occurenceLocation = Genode::strstr(content, str);
        if (occurenceLocation == nullptr) {
            return *this;
        }
        else if (len == lengthOfStr) {
            return TString();
        }
        else {
            char* p = new(tstring_alloc) char[len - lengthOfStr + 1];
            if (p == nullptr) {
                throw Genode::Exception{};
            }
            TString ret;
            ret.len = len - lengthOfStr;
            ret.content = p;

            Genode::memcpy(p, content, (occurenceLocation - content) * sizeof(char));
            Genode::memcpy(
                p + (occurenceLocation - content),
                occurenceLocation + lengthOfStr,
                (len - (occurenceLocation - content) - lengthOfStr) * sizeof(char)
            );
            p[ret.len] = '\0';

            return ret;
        }
    }
}

const TString TString::operator - (const char c) const
{
    if (c == '\0' || len == 0) {
        return *this;
    }
    else {
        for (int i = 0; i < len; i++) {
            if (c == content[i]) {
                TString ret;
                ret.len = len - 1;
                ret.content = new(tstring_alloc) char[len];
                if (ret.content == nullptr) {
                    throw Genode::Exception{};
                }

                Genode::memcpy(ret.content, content, i * sizeof(char));
                Genode::memcpy(ret.content + i, content + i + 1, (len - i - 1) * sizeof(char));
                ret[ret.len] = '\0';
                return ret;
            }
        }

        return *this;
    }
}

TString& TString::operator+=(const TString& str)
{
    if (str.len == 0) {
        return *this;
    }
    else {
        char* p;
        int resLen = len + str.len;

        p = new(tstring_alloc) char[resLen + 1];
        if (p == nullptr) {
            throw Genode::Exception{};
        }
        if (content != nullptr) {
            Genode::memcpy(p, content, len * sizeof(char));

            this->freeUpContent();
        }

        Genode::memcpy(p + len, str.content, str.len * sizeof(char));
        p[resLen] = '\0';

        this->len = resLen;

        this->content = p;
        return *this;
    }
}
TString& TString::operator+=(const char* str)
{
    if (str == nullptr || str[0] == '\0') {
        return *this;
    }
    else {
        int lengthOfStr = Genode::strlen(str);
        int resLen = len + lengthOfStr;

        char* p;

        p = new(tstring_alloc) char[resLen + 1];
        if (p == nullptr) {
            throw Genode::Exception{};
        }
        if (content != nullptr) {
            Genode::memcpy(p, content, len * sizeof(char));
            this->freeUpContent();
        }

        Genode::memcpy(p + len, str, lengthOfStr * sizeof(char));
        p[resLen] = '\0';

        this->len = resLen;

        this->content = p;
        return *this;
    }
}
TString& TString::operator+=(const char c)
{
    if (c == '\0') {
        return *this;
    }
    else {
        int resLen = len + 1;
        char* p;

        p = new(tstring_alloc) char[resLen + 1];
        if (p == nullptr) {
            throw Genode::Exception{};
        }

        if (this->content != nullptr) {
            Genode::memcpy(p, content, len * sizeof(char));
            this->freeUpContent();
        }

        p[resLen - 1] = c;
        p[resLen] = '\0';

        this->len = resLen;
        this->content = p;
        return *this;
    }
}

TString& TString::operator-=(const TString& str)
{
    return *this = *this - str;
}
TString& TString::operator-=(const char* str)
{
    return *this = *this - str;
}
TString& TString::operator-=(const char c)
{
    return *this = *this - c;
}

char& TString::operator[](int i)
{
    return content[i];
}

const char& TString::operator[](int i) const
{
    return content[i];
}


const TString TString::operator*(int x) const
{
    TString ret;

    if (this->len == 0 || x <= 0) {
        return ret;
    }
    else {
        // x >= 1
        char* p = new(tstring_alloc) char[len * x + 1];
        if (p == nullptr) {
            throw Genode::Exception{};
        }

        for (int i = 0; i < x; i++) {
            Genode::memcpy(p + i * len, content, len * sizeof(char));
        }

        ret.content = p;
        ret.len = this->len * x;
        ret.content[ret.len] = '\0';

        return ret;
    }
}

const TString operator*(const int x, const TString& str)
{
    return str * x;
}

TString& TString::operator*=(int x)
{
    return *this = *this * x;
}

const TString operator+(const char c, const TString& str)
{
    char tmp[2] = { c, '\0' };
    return tmp + str;
}

bool TString::operator==(const TString& str) const
{
    if (this->content == str.content) {
        return true;
    }
    else if (this->len != str.len) {
        return false;
    }
    else if (this->content == nullptr || str.content == nullptr) {
        return false;
    }
    else {
        for (int i = 0; i < len; i++) {
            if (this->content[i] != str[i]) {
                return false;
            }
        }
        return true;
    }
}

bool TString::operator==(const char* str) const
{
    if (str == content) {
        return true;
    }
    else if (str == nullptr || str[0] == '\0') {
        return len == 0;
    }
    else if (len == 0) {
        return false;
    }

    int lenOfStr = Genode::strlen(str);
    if (lenOfStr != len) {
        return false;
    }
    for (int i = 0; i < lenOfStr; i++) {
        if (str[i] != content[i]) {
            return false;
        }
    }
    return true;
}

bool TString::operator!=(const char* str) const
{
    return !(*this == str);
}

bool TString::operator!=(const TString& str) const
{
    return !(*this == str);
}

bool TString::operator>(const TString& str) const
{
    if (this->len + str.len == 0 || this->len == 0) {
        return false;
    }
    else if (str.len == 0) {
        return true;
    }
    else {
        int minLen = len < str.len ? len : str.len;
        for (int i = 0; i < minLen; i++) {
            if (content[i] > str[i]) {
                return true;
            }
            else if (content[i] < str[i]) {
                return false;
            }
        }

        return len > str.len;
    }
}

bool TString::operator>(const char* str) const
{
    int lengthOfStr = (str == nullptr ? 0 : Genode::strlen(str));
    if (this->len + lengthOfStr == 0 || this->len == 0) {
        return false;
    }
    else if (lengthOfStr == 0) {
        return true;
    }
    else {
        int minLen = len < lengthOfStr ? len : lengthOfStr;
        for (int i = 0; i < minLen; i++) {
            if (content[i] > str[i]) {
                return true;
            }
            else if (content[i] < str[i]) {
                return false;
            }
        }

        return len > lengthOfStr;
    }
}







bool TString::operator<(const TString& str) const
{
    if (this->len + str.len == 0 || str.len == 0) {
        return false;
    }
    else if (len == 0) {
        return true;
    }
    else {
        int minLen = len < str.len ? len : str.len;
        for (int i = 0; i < minLen; i++) {
            if (content[i] < str[i]) {
                return true;
            }
            else if (content[i] > str[i]) {
                return false;
            }
        }

        return len < str.len;
    }
}

bool TString::operator<(const char* str) const
{
    int lengthOfStr = (str == nullptr ? 0 : Genode::strlen(str));
    if (this->len + lengthOfStr == 0 || lengthOfStr == 0) {
        return false;
    }
    else if (len == 0) {
        return true;
    }
    else {
        int minLen = len < lengthOfStr ? len : lengthOfStr;
        for (int i = 0; i < minLen; i++) {
            if (content[i] < str[i]) {
                return true;
            }
            else if (content[i] > str[i]) {
                return false;
            }
        }

        return len < lengthOfStr;
    }
}

bool TString::operator>=(const TString& str) const
{
    return !(*this < str);
}
bool TString::operator>=(const char* str) const
{
    return !(*this < str);
}

bool TString::operator<=(const TString& str) const
{
    return !(*this > str);
}
bool TString::operator<=(const char* str) const
{
    return !(*this > str);
}


bool operator>(const char* strA, const TString& strB)
{
    return strB < strA;
}
bool operator<(const char* strA, const TString& strB)
{
    return strB > strA;
}
bool operator>=(const char* strA, const TString& strB)
{
    return strB <= strA;
}
bool operator<=(const char* strA, const TString& strB)
{
    return strB >= strA;
}


bool operator==(const char* strA, const TString& strB)
{
    return strB == strA;
}

bool operator!=(const char* strA, const TString& strB)
{
    return strB != strA;
}


}  // namespace MtsysKv

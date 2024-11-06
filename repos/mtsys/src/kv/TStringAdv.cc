/* 2051565 GTY Tongji CS */
// Ported from TJ OOP HW. Modified for Amkos.

#include <util/string.h>
#include <base/exception.h>
#include <base/allocator.h>

#include <kv/TString/TStringAdv.h>

namespace MtsysKv {


TStringAdv::TStringAdv() : TString()
{

}

TStringAdv::TStringAdv(const char* str) : TString(str)
{
}

TStringAdv::TStringAdv(const TStringAdv& str) : TString(str)
{
}

TStringAdv::TStringAdv(const TString& str) : TString(str)
{
}

#if defined(__GNUC__)

#else
TStringAdv::TStringAdv(const int x) : TString(x)
{
}
#endif

TStringAdv& TStringAdv::operator=(const TStringAdv& str)
{
    TString& tsThis = *this;
    const TString& tsStr = str;
    tsThis = tsStr;
    return *this;
}

TStringAdv& TStringAdv::operator=(const char* str)
{
    TString& tsThis = *this;

    tsThis = str;
    return *this;
}

const TStringAdv TStringAdv::operator+(const TStringAdv& str) const
{
    return (*this) + str.c_str();
}

const TStringAdv TStringAdv::operator+(const char* str) const
{
    TString ret = *this;
    ret += str;
    return TStringAdv(ret);
}

const TStringAdv TStringAdv::operator+(const char c) const
{
    char s[] = { c, '\0' };
    return (*this) + s;
}

const TStringAdv TStringAdv::operator-(const TStringAdv& str) const
{
    return (*this) - str.c_str();
}

const TStringAdv TStringAdv::operator-(const char* str) const
{
    TString ret = *this;
    ret -= str;
    return TStringAdv(ret);
}

const TStringAdv TStringAdv::operator-(const char c) const
{
    char s[] = { c, '\0' };
    return (*this) - s;
}

const TStringAdv TStringAdv::operator*(const int x) const
{
    TString ts = *this;
    return TStringAdv(ts * x);
}

const TStringAdv TStringAdv::operator!() const
{
    TString ts = *this;
    return TStringAdv(!ts);
}

TStringAdv& TStringAdv::operator*=(int x)
{
    TString& ts = *this;
    ts *= x;
    return *this;
}

TStringAdv& TStringAdv::operator+=(const TStringAdv& str)
{
    return (*this) += str.c_str();
}

TStringAdv& TStringAdv::operator+=(const char* str)
{
    TString& ts = *this;
    ts += str;
    return *this;
}

TStringAdv& TStringAdv::operator+=(const char c)
{
    char s[2] = { c, '\0' };
    return (*this) += s;
}

TStringAdv& TStringAdv::operator-=(const TStringAdv& str)
{
    return (*this) -= str.c_str();
}

TStringAdv& TStringAdv::operator-=(const char* str)
{
    TString& ts = *this;
    ts -= str;
    return *this;
}

TStringAdv& TStringAdv::operator-=(const char c)
{
    char s[2] = { c, '\0' };
    return (*this) -= s;
}

TStringAdv& TStringAdv::assign(const TStringAdv& ts2)
{
    if (this == &ts2) {
        return *this;
    }

    this->freeUp();
    this->len = ts2.len;

    if (this->len > 0) {
        this->content = new(tstring_alloc) char[this->len + 1];
        if (this->content == nullptr) {
            throw Genode::Exception{};
        }

        Genode::memcpy(this->content, ts2.c_str(), this->len + 1);
    }

    return *this;
}

TStringAdv& TStringAdv::assign(const char* s)
{
    this->freeUp();
    this->len = (s == nullptr ? 0 : Genode::strlen(s));

    if (this->len > 0) {
        this->content = new(tstring_alloc) char[this->len + 1];
        
        if (this->content == nullptr) {
            throw Genode::Exception{};
        }

        Genode::memcpy(this->content, s, this->len + 1);
    }

    return *this;
}

TStringAdv& TStringAdv::assign(const char& c)
{
    this->freeUp();

    if (c != '\0') {
        this->len = 1;
        this->content = new(tstring_alloc) char[2];
        if (this->content == nullptr) {
            throw Genode::Exception{};
        }
        this->content[0] = c;
        this->content[1] = '\0';
    }

    return *this;
}

TStringAdv& TStringAdv::append(const TStringAdv& ts2)
{
    TString& tsTs = *this;
    const TString& tsTs2 = ts2;

    tsTs += tsTs2;

    return *this;
}

TStringAdv& TStringAdv::append(const char* s)
{
    TString& str = *this;

    str += s;

    return *this;
}

TStringAdv& TStringAdv::append(const char& c)
{
    char s[2] = { c, '\0' };
    this->append(s);

    return *this;
}

TStringAdv& TStringAdv::insert(const TStringAdv& ts2, int pos)
{
    return this->insert(ts2.c_str(), pos);
}

TStringAdv& TStringAdv::insert(const char* s, int pos)
{
    int targetIdx = pos - 1;
    if (s == nullptr || s[0] == '\0' || targetIdx < 0 || targetIdx > len) {
        return *this;
    }
    
    int lengthOfS = Genode::strlen(s);
    char* p = new(tstring_alloc) char[this->len + lengthOfS + 1];
    if (p == nullptr) {
        throw Genode::Exception {};
    }

    Genode::memcpy(p, this->content, targetIdx);
    Genode::memcpy(p + targetIdx, s, lengthOfS);
    Genode::memcpy(p + targetIdx + lengthOfS, this->content + targetIdx, len - targetIdx);
    
    this->len = this->len + lengthOfS;
    p[this->len] = '\0';

    delete[] (this->content, tstring_alloc);
    
    this->content = p;

    return *this;
}

TStringAdv& TStringAdv::insert(const char& c, int pos)
{
    char s[2] = { c, '\0' };
    return this->insert(s, pos);
}

TStringAdv& TStringAdv::erase(const TStringAdv& ts2)
{
    TString& pTsStr = *this;
    const TString& pTsTs2 = ts2;
    pTsStr -= pTsTs2;

    return *this;
}

TStringAdv& TStringAdv::erase(const char* s)
{
    TString& pTsStr = *this;

    pTsStr -= s;

    return *this;
}

TStringAdv& TStringAdv::erase(const char& c)
{
    TString& pTsStr = *this;

    pTsStr -= c;

    return *this;
}

TStringAdv TStringAdv::substr(const int pos, const int len) const
{
    TStringAdv ret;

    int targetIdx = pos - 1;
    int targetLength = (len > this->len - targetIdx) ? this->len - targetIdx : len;

    if (targetIdx < 0 || targetIdx >= this->len || len <= 0)
    {
        return ret;
    }

    ret.len = targetLength;
    ret.content = new(tstring_alloc) char[targetLength + 1];
    if (ret.content == nullptr) {    
        throw Genode::Exception{};
    }

    Genode::memcpy(ret.content, this->content + targetIdx, targetLength);
    ret.content[targetLength] = '\0';

    return ret;
}

char& TStringAdv::at(const int n)
{
    return content[n];
}

const char& TStringAdv::at(const int n) const
{
    return content[n];
}

const TStringAdv operator+(const char* strA, const TStringAdv& strB)
{
    const TString& tsStrB = strB;
    return strA + tsStrB;
}

const TStringAdv operator+(const char c, const TStringAdv& str)
{
    const TString& tsStr = str;
    return c + tsStr;
}

const TStringAdv operator*(const int x, const TStringAdv& str)
{
    const TString& tsStr = str;
    return x * tsStr;
}

bool operator==(const char* strA, const TStringAdv& strB)
{
    const TString& tsStrB = strB;
    return strA == tsStrB;
}

bool operator!=(const char* strA, const TStringAdv& strB)
{
    const TString& tsStrB = strB;
    return strA != tsStrB;
}

bool operator>(const char* strA, const TStringAdv& strB)
{
    const TString& tsStrB = strB;
    return strA > tsStrB;
}

bool operator<(const char* strA, const TStringAdv& strB)
{
    const TString& tsStrB = strB;
    return strA < tsStrB;
}

bool operator>=(const char* strA, const TStringAdv& strB)
{
    const TString& tsStrB = strB;
    return strA >= tsStrB;
}

bool operator<=(const char* strA, const TStringAdv& strB)
{
    const TString& tsStrB = strB;
    return strA <= tsStrB;
}

int TStringAdvLen(const TStringAdv& str)
{
    return str.length();
}


}  // namespace MtsysKv

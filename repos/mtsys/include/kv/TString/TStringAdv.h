/* 2051565 GTY Tongji CS */
// Ported from TJ OOP HW. Modified for Amkos.
#pragma once

#include "./TString.h"

namespace MtsysKv {

#ifndef INT_MAX
    #define INT_MAX ((int) (~0U>>1))
#endif

class TStringAdv : public TString
{
public:
    TStringAdv();
    TStringAdv(const char* str);
    TStringAdv(const TStringAdv& str);
    TStringAdv(const TString& str);

#if defined(__GNUC__)

#else
    TStringAdv(const int x);
#endif

    TStringAdv& operator = (const TStringAdv& str);
    TStringAdv& operator = (const char* str);

    const TStringAdv operator + (const TStringAdv& str) const;
    const TStringAdv operator + (const char* str) const;
    const TStringAdv operator + (const char c) const;

    const TStringAdv operator - (const TStringAdv& str) const;
    const TStringAdv operator - (const char* str) const;
    const TStringAdv operator - (const char c) const;

    const TStringAdv operator * (const int x) const;

    const TStringAdv operator ! () const;

    TStringAdv& operator *= (int x);
    TStringAdv& operator += (const TStringAdv& str);
    TStringAdv& operator += (const char* str);
    TStringAdv& operator += (const char c);

    TStringAdv& operator -= (const TStringAdv& str);
    TStringAdv& operator -= (const char* str);
    TStringAdv& operator -= (const char c);

    TStringAdv& assign(const TStringAdv& ts2);
    TStringAdv& assign(const char* s);
    TStringAdv& assign(const char& c);

    TStringAdv& append(const TStringAdv& ts2);
    TStringAdv& append(const char* s);
    TStringAdv& append(const char& c);

    TStringAdv& insert(const TStringAdv& ts2, int pos);
    TStringAdv& insert(const char* s, int pos);
    TStringAdv& insert(const char& c, int pos);

    TStringAdv& erase(const TStringAdv& ts2);
    TStringAdv& erase(const char* s);
    TStringAdv& erase(const char& c);

    TStringAdv substr(const int pos, const int len = INT_MAX) const;

    char& at(const int n);
    const char& at(const int n) const;

    friend const TStringAdv operator + (const char* strA, const TStringAdv& strB);

    friend const TStringAdv operator + (const char c, const TStringAdv& str);
    friend const TStringAdv operator * (const int x, const TStringAdv& str);

    friend bool operator == (const char* strA, const TStringAdv& strB);
    friend bool operator != (const char* strA, const TStringAdv& strB);
    friend bool operator > (const char* strA, const TStringAdv& strB);
    friend bool operator < (const char* strA, const TStringAdv& strB);
    friend bool operator >= (const char* strA, const TStringAdv& strB);
    friend bool operator <= (const char* strA, const TStringAdv& strB);
};


}  // namespace MtsysKv


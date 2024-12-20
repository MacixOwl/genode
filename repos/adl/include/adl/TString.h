/* 2051565 GTY Tongji CS */
// Ported from TJ OOP HW. Modified for Amkos.

#pragma once

#include "./Allocator.h"

namespace adl {


class TString {
protected:
    char* content;
    size_t   len;

    inline void freeUpContent();


public:
    static const size_t npos = -1;

    TString();
    TString(const char* str);
    TString(const TString& str);

#if defined(__GNUC__)

#else
    TString(const int x);
#endif

    virtual ~TString();


    void freeUp();

    size_t length() const;
    const char* c_str() const;
    inline char* data() { return this->content; }
    inline const char* data() const { return this->content; }

    TString& operator = (const TString& str);
    TString& operator = (const char* str);

    const TString operator + (const TString& str) const;
    const TString operator + (const char* str) const;
    const TString operator + (const char c) const;

    const TString operator - (const TString& str) const;
    const TString operator - (const char* str) const;
    const TString operator - (const char c) const;

    const TString operator * (const int x) const;

    const TString operator ! () const;

    TString& operator *= (int x);
    TString& operator += (const TString& str);
    TString& operator += (const char* str);
    TString& operator += (const char c);

    TString& operator -= (const TString& str);
    TString& operator -= (const char* str);
    TString& operator -= (const char c);

    bool operator == (const TString& str) const;
    bool operator == (const char* str) const;
    bool operator != (const TString& str) const;
    bool operator != (const char* str) const;
    bool operator > (const TString& str) const;
    bool operator > (const char* str) const;
    bool operator < (const TString& str) const;
    bool operator < (const char* str) const;
    bool operator >= (const TString& str) const;
    bool operator >= (const char* str) const;
    bool operator <= (const TString& str) const;
    bool operator <= (const char* str) const;

    TString substr(const size_t pos, const size_t len = npos) const;
    char& at(const int n);
    const char& at(const int n) const;

    char& operator[] (size_t i);
    const char& operator[] (size_t i) const;

    friend const TString operator + (const char* strA, const TString& strB);

    friend const TString operator + (const char c, const TString& str);
    friend const TString operator * (const int x, const TString& str);

    friend bool operator == (const char* strA, const TString& strB);
    friend bool operator != (const char* strA, const TString& strB);
    friend bool operator > (const char* strA, const TString& strB);
    friend bool operator < (const char* strA, const TString& strB);
    friend bool operator >= (const char* strA, const TString& strB);
    friend bool operator <= (const char* strA, const TString& strB);
};



}  // namespace MtsysKv


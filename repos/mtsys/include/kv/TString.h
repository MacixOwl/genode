/* 2051565 GTY Tongji CS */
// Ported from TJ OOP HW. Modified for Amkos.

#pragma once

namespace Genode {
    class Allocator;
}


namespace MtsysKv {

extern Genode::Allocator* tstring_alloc;


class TString {
protected:
    char* content;
    int   len;

    inline void freeUpContent();


public:

    TString();
    TString(const char* str);
    TString(const TString& str);

#if defined(__GNUC__)

#else
    TString(const int x);
#endif

    virtual ~TString();


    void freeUp();

    int length() const;
    const char* c_str() const;

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



    char& operator[] (int i);
    const char& operator[] (int i) const;

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


/*
 *  PTime.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PTIME_HEADER
#define PTIME_HEADER

namespace pika {

class PIKA_API CTime : public Object {
    PIKA_DECL(CTime, Object)
public:
    CTime(Engine* eng, Type* t);

    virtual ~CTime();
    void LocalTime();
    void GmTime();
    
    virtual String* ToString();
    
    void SetSec(pint_t X);
    void SetMin(pint_t x);
    void SetHour(pint_t x);
    void SetMDay(pint_t x);
    void SetMon(pint_t x);
    void SetYear(pint_t x);
    void SetWDay(pint_t x);
    void SetYDay(pint_t x);
    void SetIsDst(bool x);
        
    pint_t GetSec() { return the_time.tm_sec; }
    pint_t GetMin() { return the_time.tm_min; }
    pint_t GetHour() { return the_time.tm_hour; }
    pint_t GetMDay() { return the_time.tm_mday; }
    pint_t GetMon() { return the_time.tm_mon+1; }
    pint_t GetYear() { return the_time.tm_year + 1900; }
    pint_t GetWDay() { return the_time.tm_wday; }
    pint_t GetYDay() { return the_time.tm_yday; }
    pint_t GetGmtOff() { return the_time.tm_gmtoff; }
    pint_t GetMicroseconds() { return microseconds; }
    bool   GetIsDst() { return the_time.tm_isdst != 0; }
    String* GetTimezone();
    
    time_t MkTime();
    preal_t Diff(CTime* rhs);
    static void Constructor(Engine* eng, Type* type, Value& res);
    static void StaticInitType(Package* pkg, Engine* eng);
protected:
    void SetMicroseconds();
    pint_t microseconds;
    tm the_time;
};

DECLARE_BINDING(CTime);
}// pika

#endif

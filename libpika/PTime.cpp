/*
 *  PTime.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PTime.h"
#include "PPlatform.h"
namespace pika {
namespace {

void RangeCheck(pint_t x, pint_t const lo, pint_t const hi, const char* val)
{
    if (x < lo || x > hi)
    {
        RaiseException(Exception::ERROR_runtime, "Attempt to set CTime's %s field to " PINT_FMT " must be between [" PINT_FMT " - " PINT_FMT "].", val, x, lo, hi);
    }
}

}// namespace

PIKA_IMPL(CTime)

CTime::CTime(Engine* eng, Type* typ) : ThisSuper(eng, typ), microseconds(0) {
    LocalTime();
}

CTime::~CTime() {}

String* CTime::ToString()
{
    return engine->GetString( asctime( &the_time ) );
}

void CTime::LocalTime()
{
    time_t t;
    t = time(NULL);
    tm* lt = localtime(&t);	
    Pika_memcpy(&the_time, lt, sizeof(tm));
    SetMicroseconds();
}

void CTime::GmTime()
{
    time_t t;
    t = time(NULL);
    tm* lt = gmtime(&t);	
    Pika_memcpy(&the_time, lt, sizeof(tm));
    SetMicroseconds();
}

void CTime::SetSec(pint_t x)
{
    RangeCheck(x, 0, 60, "sec");
    the_time.tm_sec = x;
}

void CTime::SetMin(pint_t x)
{
    RangeCheck(x, 0, 59, "min");
    the_time.tm_min = x;
}

void CTime::SetHour(pint_t x)
{
    RangeCheck(x, 0, 23, "hour");
    the_time.tm_hour = x;
}

void CTime::SetMDay(pint_t x)
{
    RangeCheck(x, 0, 31, "mday");
    the_time.tm_mday = x;
}

void CTime::SetMon(pint_t x)
{
    RangeCheck(x, 1, 12, "mon");
    the_time.tm_mon = x-1;
}

void CTime::SetYear(pint_t x)
{
    if (x < 1900)
    {
        RaiseException(Exception::ERROR_runtime, "Attempt to set CTime's year field to " PINT_FMT " must be greater than 1900.", x);
    }
    the_time.tm_year = x - 1900;
}

void CTime::SetWDay(pint_t x)
{
    RangeCheck(x, 0, 6, "wday");
    the_time.tm_wday = x;
}

void CTime::SetYDay(pint_t x)
{
    RangeCheck(x, 0, 365, "yday");
    the_time.tm_yday = x;
}

void CTime::SetIsDst(bool x)
{
    the_time.tm_isdst = x;
}

void CTime::Constructor(Engine* eng, Type* type, Value& res)
{
    CTime* d = 0;
    GCNEW(eng, CTime, d, (eng, type));
    res.Set(d);
}

time_t CTime::MkTime()
{
    time_t res = mktime(&the_time);
    if (res == -1)
    {
        RaiseException("Attempt to call CTime.mktime failed.");
    }
    return res;
}

preal_t CTime::Diff(CTime* rhs)
{   
    time_t tl = this->MkTime();
    time_t tr = rhs->MkTime();
    preal_t res = difftime(tl, tr);
    return res;
}

String* CTime::GetTimezone()
{
    if (the_time.tm_zone) {
        return engine->GetString(the_time.tm_zone);
    }
    return engine->emptyString;
}

void CTime::SetMicroseconds()
{
    Pika_timeval tv;
    Pika_gettimeofday(&tv, 0);
    this->microseconds = tv.tv_usec;
}

namespace {
    int CTime_mktime(Context* ctx, Value& res)
    {
        CTime* date = static_cast<CTime*>(res.val.object);
        time_t t = date->MkTime();
        pint_t i = static_cast<pint_t>(t); // TODO: What if time_t is larger than pint_t?
        ctx->Push((pint_t)i);
        return 1;
    }
}

int ctime_gettimeofday(Context* ctx, Value&)
{
    Pika_timeval tv;
    Pika_timezone tz;
    Pika_gettimeofday(&tv, &tz);
    ctx->Push((pint_t)tv.tv_sec);
    ctx->Push((pint_t)tv.tv_usec);
    ctx->Push((pint_t)tz.tz_minuteswest);
    ctx->Push((pint_t)tz.tz_dsttime);
    return 4;
}

void CTime::StaticInitType(Package* pkg, Engine* eng)
{
    GCPAUSE_NORUN(eng);
    String* CTime_String = eng->AllocString("CTime");
    Type* CTime_Type = Type::Create(eng, CTime_String, eng->Object_Type, CTime::Constructor, pkg);
    pkg->SetSlot(CTime_String, CTime_Type);
    SlotBinder<CTime>(eng, CTime_Type)
    .PropertyRW("sec",
            &CTime::GetSec,      0,
            &CTime::SetSec,      0)
    .PropertyRW("min",
            &CTime::GetMin,      0,
            &CTime::SetMin,      0)
    .PropertyRW("hour",
            &CTime::GetHour,     0,
            &CTime::SetHour,     0)
    .PropertyRW("mday",
            &CTime::GetMDay,     0,
            &CTime::SetMDay,     0)
    .PropertyRW("mon",
            &CTime::GetMon,      0,
            &CTime::GetMon,      0)
    .PropertyRW("year",
            &CTime::GetYear,     0,
            &CTime::SetYear,     0)
    .PropertyRW("wday",
            &CTime::GetWDay,     0,
            &CTime::SetWDay,     0)
    .PropertyRW("yday",
            &CTime::GetYDay,     0,
            &CTime::SetYDay,     0)
    .PropertyRW("isdst",
            &CTime::GetIsDst,    0,
            &CTime::SetIsDst,    0)
    .PropertyR("gmtoff",
            &CTime::GetGmtOff,   0)
    .PropertyR("microseconds",
            &CTime::GetMicroseconds, 0)
    .RegisterMethod(CTime_mktime, "mktime", 0, false, true)
    .Method(&CTime::LocalTime,   "localtime")
    .Method(&CTime::GmTime,      "gmtime")
    .Method(&CTime::GetTimezone, "getTimezone")
    .Method(&CTime::Diff,        "opSub")
    .Alias("diff",              "opSub")
    ;
            
    static RegisterFunction ctime_Functions[] = {
        { "gettimeofday", ctime_gettimeofday, 0, DEF_STRICT, 0 },
    };
    
    pkg->EnterFunctions(ctime_Functions, countof(ctime_Functions));
}

}// pika

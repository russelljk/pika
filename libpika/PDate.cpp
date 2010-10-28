/*
 *  PDate.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PDate.h"

namespace pika {
namespace {

void RangeCheck(pint_t x, pint_t const lo, pint_t const hi, const char* val)
{
    if (x < lo || x > hi)
    {
        RaiseException(Exception::ERROR_runtime, "attempt to set Date's %s field to "PINT_FMT" must be between ["PINT_FMT" - "PINT_FMT"].", val, x, lo, hi);
    }
}

}// namespace

PIKA_IMPL(Date)

Date::Date(Engine* eng, Type* typ) : ThisSuper(eng, typ) {
    LocalTime();
}

Date::~Date() {}

String* Date::ToString()
{
    return engine->AllocStringNC( asctime( &the_time ) );
}

void Date::LocalTime()
{
    time_t t;
    t = time(NULL);
    tm* lt = localtime(&t);	
    Pika_memcpy(&the_time, lt, sizeof(tm));
}

void Date::GmTime()
{
    time_t t;
    t = time(NULL);
    tm* lt = gmtime(&t);	
    Pika_memcpy(&the_time, lt, sizeof(tm));
}

void Date::SetSec(pint_t x)
{
    RangeCheck(x, 0, 60, "sec");
    the_time.tm_sec = x;
}

void Date::SetMin(pint_t x)
{
    RangeCheck(x, 0, 59, "min");
    the_time.tm_min = x;
}

void Date::SetHour(pint_t x)
{
    RangeCheck(x, 0, 23, "hour");
    the_time.tm_hour = x;
}

void Date::SetMDay(pint_t x)
{
    RangeCheck(x, 0, 31, "mday");
    the_time.tm_mday = x;
}

void Date::SetMon(pint_t x)
{
    RangeCheck(x, 1, 12, "mon");
    the_time.tm_mon = x-1;
}

void Date::SetYear(pint_t x)
{
    if (x < 1900)
    {
        RaiseException(Exception::ERROR_runtime, "attempt to set Date's year field to "PINT_FMT" must be greater than 1900.", x);
    }
    the_time.tm_year = x - 1900;
}

void Date::SetWDay(pint_t x)
{
    RangeCheck(x, 0, 6, "wday");
    the_time.tm_wday = x;
}

void Date::SetYDay(pint_t x)
{
    RangeCheck(x, 0, 365, "yday");
    the_time.tm_yday = x;
}

void Date::SetIsDst(bool x)
{
    the_time.tm_isdst = x;
}

void Date::SetGmtOff(pint_t x)
{
    the_time.tm_gmtoff = x;
}

void Date::Constructor(Engine* eng, Type* type, Value& res)
{
    Date* d = 0;
    GCNEW(eng, Date, d, (eng, type));
    res.Set(d);
}

time_t Date::MkTime()
{
    time_t res = mktime(&the_time);
    if (res == -1)
    {
        RaiseException("attempt to call Date.mktime failed.");
    }
    return res;
}

pint_t Date::Diff(Date* rhs)
{   
    time_t tl = this->MkTime();
    time_t tr = rhs->MkTime();
    time_t res = difftime(tl, tr);
    return res;
}

namespace {
    int Date_mktime(Context* ctx, Value& res)
    {
        Date* date = static_cast<Date*>(res.val.object);
        time_t t = date->MkTime();
        pint_t i = static_cast<pint_t>(t); // TODO: What if time_t is larger than pint_t?
        ctx->Push((pint_t)i);
        return 1;
    }
}

void Date::StaticInitType(Package* pkg, Engine* eng)
{
    GCPAUSE_NORUN(eng);
    String* Date_String = eng->AllocString("Date");
    Type* Date_Type = Type::Create(eng, Date_String, eng->Object_Type, Date::Constructor, pkg);
    pkg->SetSlot(Date_String, Date_Type);
    SlotBinder<Date>(eng, Date_Type, pkg)
    .PropertyRW("sec",
            &Date::GetSec,      0,
            &Date::SetSec,      0)
    .PropertyRW("min",
            &Date::GetMin,      0,
            &Date::SetMin,      0)
    .PropertyRW("hour",
            &Date::GetHour,     0,
            &Date::SetHour,     0)
    .PropertyRW("mday",
            &Date::GetMDay,     0,
            &Date::SetMDay,     0)
    .PropertyRW("mon",
            &Date::GetMon,      0,
            &Date::GetMon,      0)
    .PropertyRW("year",
            &Date::GetYear,     0,
            &Date::SetYear,     0)
    .PropertyRW("wday",
            &Date::GetWDay,     0,
            &Date::SetWDay,     0)
    .PropertyRW("yday",
            &Date::GetYDay,     0,
            &Date::SetYDay,     0)
    .PropertyRW("isdst",
            &Date::GetIsDst,    0,
            &Date::SetIsDst,    0)
    .PropertyRW("gmtoff",
            &Date::GetGmtOff,   0,
            &Date::SetGmtOff,   0)
    .Register(Date_mktime,      "mktime", 0, false, true)
    .Method(&Date::LocalTime,   "localtime")
    .Method(&Date::GmTime,      "gmtime")
    .Method(&Date::Diff,        "opSub")
    .Alias("diff",              "opSub")
    ;
}

}// pika

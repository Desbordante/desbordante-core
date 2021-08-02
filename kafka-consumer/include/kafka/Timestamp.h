#pragma once

#include "kafka/Project.h"

#include "librdkafka/rdkafka.h"

#include <cassert>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <time.h>


namespace KAFKA_API {

/**
 * The time point together with the type.
 */
struct Timestamp
{
    using Value = std::int64_t;

    enum class Type { NotAvailable, CreateTime, LogAppendTime };

    /**
    * The milliseconds since epoch.
    */
    Value msSinceEpoch;

    /**
    * The type shows what the `msSinceEpoch` means (CreateTime or LogAppendTime).
    */
    Type  type;

    explicit Timestamp(Value v = 0, Type t = Type::NotAvailable): msSinceEpoch(v), type(t) {}
    Timestamp(Value v, rd_kafka_timestamp_type_t t): Timestamp(v, convertType(t)) {}

    static Type convertType(rd_kafka_timestamp_type_t tstype)
    {
        return (tstype == RD_KAFKA_TIMESTAMP_CREATE_TIME) ? Type::CreateTime :
                 (tstype == RD_KAFKA_TIMESTAMP_LOG_APPEND_TIME ? Type::LogAppendTime : Type::NotAvailable);
    }

    operator std::chrono::time_point<std::chrono::system_clock>() const // NOLINT
    {
        return std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds(msSinceEpoch));
    }

    static std::string toString(Type t)
    {
        switch (t)
        {
            case Type::CreateTime:
                return "CreateTime";
            case Type::LogAppendTime:
                return "LogAppendTime";
            default:
                assert(t == Type::NotAvailable);
                return "";
        }
    }

    static std::string toString(Value v)
    {
        auto ms = std::chrono::milliseconds(v);
        auto timepoint = std::chrono::time_point<std::chrono::system_clock>(ms);
        std::time_t time = std::chrono::system_clock::to_time_t(timepoint);
        std::ostringstream oss;
        std::tm tmBuf = {};
#if !defined(WIN32)
        oss << std::put_time(localtime_r(&time, &tmBuf), "%F %T") << "." << std::setfill('0') << std::setw(3) << (v % 1000);
#else
        localtime_s(&tmBuf, &time);
        oss << std::put_time(&tmBuf, "%F %T") << "." << std::setfill('0') << std::setw(3) << (v % 1000);
#endif
        return oss.str();
    }

    /**
    * Obtains explanatory string.
    */
    std::string toString() const
    {
        auto typeString = toString(type);
        auto timeString = toString(msSinceEpoch);
        return typeString.empty() ? timeString : (typeString + "[" + timeString + "]");
    }
};

} // end of KAFKA_API


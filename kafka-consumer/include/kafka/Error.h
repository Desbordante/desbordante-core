#pragma once

#include "kafka/Project.h"

#include "kafka/RdKafkaHelper.h"

#include "librdkafka/rdkafka.h"

#include <string>
#include <system_error>


namespace KAFKA_API {

struct ErrorCategory: public std::error_category
{
    const char* name() const noexcept override { return "KafkaError"; }
    std::string message(int ev) const override { return rd_kafka_err2str(static_cast<rd_kafka_resp_err_t>(ev)); }

    template <typename T = void>
    struct Global { static ErrorCategory category; };
};

template <typename T>
ErrorCategory ErrorCategory::Global<T>::category;

/**
 * A utility function which converts an error number to `std::error_code` (with KafkaError category)
 */
inline std::error_code ErrorCode(int errNo = 0)
{
    /**
     * The error code for external interfaces.
     * Actually, it's the same as 'rd_kafka_resp_err_t', which is defined by librdkafka.
     * 1. The negative values are for internal errors.
     * 2. Non-negative values are for external errors. See the defination at,
     *    - [Error Codes] (https://cwiki.apache.org/confluence/display/KAFKA/A+Guide+To+The+Kafka+Protocol#AGuideToTheKafkaProtocol-ErrorCodes)
     */
    return {errNo, ErrorCategory::Global<>::category};
}

/**
 * A utility fucntion which converts the `librdkafka`'s internal `rd_kafka_resp_err_t` to `std::error_code` (with KafkaError category)
 */
inline std::error_code ErrorCode(rd_kafka_resp_err_t respErr)
{
    return ErrorCode(static_cast<int>(respErr));
}

/**
 * It would contain detailed message.
 */
struct ErrorWithDetail
{
    ErrorWithDetail(std::error_code code, std::string detailedMsg)
        : error(code), detail(std::move(detailedMsg)) {}

    ErrorWithDetail(rd_kafka_resp_err_t respErr, std::string detailedMsg)
        : ErrorWithDetail(ErrorCode(respErr), std::move(detailedMsg)) {}

    explicit operator bool() const { return static_cast<bool>(error); }

    std::error_code error;
    std::string     detail;
};

/**
 * Error type with rich info.
 */
class Error
{
public:
    explicit Error(rd_kafka_error_t* error = nullptr): _rkError(error, RkErrorDeleter) {}
    Error(const Error&)            = default;
    virtual ~Error()               = default;

    explicit operator bool() const { return static_cast<bool>(errorCode()); }

    /**
     * Obtains the underlying error code.
     */
    virtual std::error_code errorCode()   const { return ErrorCode(rd_kafka_error_code(_rkError.get())); }

    /**
     * Readable error string.
     */
    virtual std::string     message()     const { return rd_kafka_error_string(_rkError.get()); }

    /**
     * Fatal error indicates that the client instance is no longer usable.
     */
    virtual Optional<bool>  isFatal()     const { return rd_kafka_error_is_fatal(_rkError.get()); }

    /**
     * Show whether the operation may be retried.
     */
    virtual Optional<bool>  isRetriable() const { return rd_kafka_error_is_retriable(_rkError.get()); }

private:
    rd_kafka_error_shared_ptr _rkError;
};

/**
 * Error type only with brief info.
 */
class SimpleError: public Error
{
public:
    explicit SimpleError(rd_kafka_resp_err_t respErr, std::string message): _respErr(respErr), _message(std::move(message)) {}
    explicit SimpleError(rd_kafka_resp_err_t respErr): SimpleError(respErr, rd_kafka_err2str(respErr)) {}

    SimpleError(const SimpleError&) = default;

    std::error_code errorCode()   const override { return ErrorCode(_respErr); }
    std::string     message()     const override { return _message; }
    Optional<bool>  isFatal()     const override { return {}; }
    Optional<bool>  isRetriable() const override { return {}; }

private:
    rd_kafka_resp_err_t _respErr;
    std::string         _message;
};


} // end of KAFKA_API


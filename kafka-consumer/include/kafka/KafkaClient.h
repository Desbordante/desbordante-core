#pragma once

#include "kafka/Project.h"

#include "kafka/BrokerMetadata.h"
#include "kafka/Error.h"
#include "kafka/KafkaException.h"
#include "kafka/Log.h"
#include "kafka/Properties.h"
#include "kafka/RdKafkaHelper.h"
#include "kafka/Types.h"

#include "librdkafka/rdkafka.h"

#include <atomic>
#include <cassert>
#include <climits>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>


namespace KAFKA_API {

/**
 * The base class for Kafka clients (i.e, KafkaConsumer, KafkaProducer, and AdminClient).
 */
class KafkaClient
{
protected:
    using ConfigCallbacksRegister = std::function<void(rd_kafka_conf_t*)>;
    using StatsCallback           = std::function<void(std::string)>;

    enum class ClientType { KafkaConsumer, KafkaProducer, AdminClient };
    static std::string getClientTypeString(ClientType type)
    {
        switch (type)
        {
            case ClientType::KafkaConsumer: return "KafkaConsumer";
            case ClientType::KafkaProducer: return "KafkaProducer";
            case ClientType::AdminClient:   return "AdminClient";
            default: assert(false);         return "Invalid Type";
        }
    }

    static constexpr int TIMEOUT_INFINITE  = -1;

    static int convertMsDurationToInt(std::chrono::milliseconds ms)
    {
        return ms > std::chrono::milliseconds(INT_MAX) ? TIMEOUT_INFINITE : static_cast<int>(ms.count());
    }

public:
    enum class EventsPollingOption { Manual, Auto };

    KafkaClient(ClientType                     clientType,
                const Properties&              properties,
                const ConfigCallbacksRegister& registerCallbacks   = ConfigCallbacksRegister(),
                const std::set<std::string>&   privatePropertyKeys = {});

    virtual ~KafkaClient() = default;

    const std::string& clientId()   const { return _clientId; }
    const std::string& name()       const { return _clientName; }

    /**
     * Set a log callback for kafka clients, which do not have a client specific logging callback configured (see `setLogger`).
     */
    static void setGlobalLogger(Logger logger = NoneLogger)
    {
        std::call_once(Global<>::initOnce, [](){}); // Then no need to init within KafkaClient constructor
        Global<>::logger = std::move(logger);
    }

    /**
     * Set the log callback for the kafka client (it's a per-client setting).
     */
    void setLogger(Logger logger) { _logger = std::move(logger); }

    /**
     * Set log level for the kafka client (the default value: 5).
     */
    void setLogLevel(int level);

    /**
     * Set callback to receive the periodic statistics info.
     * Note: 1) It only works while the "statistics.interval.ms" property is configured with a non-0 value.
     *       2) The callback would be triggered periodically, receiving the internal statistics info (with JSON format) emited from librdkafka.
     */
    void setStatsCallback(StatsCallback cb) { _statsCb = std::move(cb); }

    /**
     * Return the properties which took effect.
     */
    const Properties& properties() const { return _properties; }

    /**
     * Fetch the effected property (including the property internally set by librdkafka).
     */
    Optional<std::string> getProperty(const std::string& name) const;

    /**
     * Fetch matadata from a available broker.
     */
    Optional<BrokerMetadata> fetchBrokerMetadata(const std::string& topic,
                                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_METADATA_TIMEOUT_MS),
                                                 bool disableErrorLogging = false);

    template<class ...Args>
    void doLog(int level, const char* filename, int lineno, const char* format, Args... args) const
    {
        const auto& logger = _logger ? _logger : Global<>::logger;
        if (level >= 0 && level <= _logLevel && logger)
        {
            LogBuffer<LOG_BUFFER_SIZE> logBuffer;
            logBuffer.print("%s ", name().c_str()).print(format, args...);
            logger(level, filename, lineno, logBuffer.c_str());
        }
    }

    void doLog(int level, const char* filename, int lineno, const char* msg) const
    {
        doLog(level, filename, lineno, "%s", msg);
    }

#define KAFKA_API_DO_LOG(lvl, ...) doLog(lvl, __FILE__, __LINE__, ##__VA_ARGS__)

    template<class ...Args>
    static void doGlobalLog(int level, const char* filename, int lineno, const char* format, Args... args)
    {
        if (!Global<>::logger) return;

        LogBuffer<LOG_BUFFER_SIZE> logBuffer;
        logBuffer.print(format, args...);
        Global<>::logger(level, filename, lineno, logBuffer.c_str());
    }
    static void doGlobalLog(int level, const char* filename, int lineno, const char* msg)
    {
        doGlobalLog(level, filename, lineno, "%s", msg);
    }

/**
 * Log for kafka clients, with the callback which `setGlobalLogger` assigned.
 *
 * E.g,
 *     KAFKA_API_LOG(Log::Level::Err, "something wrong happened! %s", detailedInfo.c_str());
 */
#define KAFKA_API_LOG(lvl, ...) KafkaClient::doGlobalLog(lvl, __FILE__, __LINE__, ##__VA_ARGS__)

protected:

    rd_kafka_t* getClientHandle() const { return _rk.get(); }

    static const KafkaClient& kafkaClient(const rd_kafka_t* rk) { return *static_cast<const KafkaClient*>(rd_kafka_opaque(rk)); }
    static       KafkaClient& kafkaClient(rd_kafka_t* rk)       { return *static_cast<KafkaClient*>(rd_kafka_opaque(rk)); }

    static const constexpr int LOG_BUFFER_SIZE = 1024;

    template <typename T = void>
    struct Global
    {
        static Logger         logger;
        static std::once_flag initOnce;
    };

    // Log callback (for librdkafka)
    static void logCallback(const rd_kafka_t* rk, int level, const char* fac, const char* buf);

    // Statistics callback (for librdkafka)
    static int statsCallback(rd_kafka_t* rk, char* jsonStrBuf, size_t jsonStrLen, void* opaque);

    // Validate properties (and fix it if necesary)
    static Properties validateAndReformProperties(const Properties& origProperties);

    // To avoid double-close
    bool _opened = false;

private:
    std::string         _clientId;
    std::string         _clientName;
    std::atomic<int>    _logLevel = {Log::Level::Notice};
    Logger              _logger;
    Properties          _properties;
    StatsCallback       _statsCb;
    rd_kafka_unique_ptr _rk;

    // Log callback (for class instance)
    void onLog(int level, const char* fac, const char* buf) const;

    // Stats callback (for class instance)
    void onStats(std::string&& jsonString);

    static const constexpr char* BOOTSTRAP_SERVERS = "bootstrap.servers";
    static const constexpr char* CLIENT_ID         = "client.id";
    static const constexpr char* LOG_LEVEL         = "log_level";
    static const constexpr char* DEBUG             = "debug";
    static const constexpr char* SECURITY_PROTOCOL          = "security.protocol";
    static const constexpr char* SASL_KERBEROS_SERVICE_NAME = "sasl.kerberos.service.name";

#if __cplusplus >= 201703L
    static constexpr int DEFAULT_METADATA_TIMEOUT_MS = 10000;
#else
    enum { DEFAULT_METADATA_TIMEOUT_MS = 10000 };
#endif

protected:
    class Pollable
    {
    public:
        virtual ~Pollable() = default;
        virtual void poll(int timeoutMs) = 0;
    };

    template <typename T>
    class PollableCallback: public Pollable
    {
    public:
        using Func = void(*)(T*, int);
        PollableCallback(T* client, Func cb): _client(client), _cb(cb) {}

        void poll(int timeoutMs) override { _cb(_client, timeoutMs); }

    private:
        T*   _client;
        Func _cb;
    };

    class PollThread
    {
    public:
        explicit PollThread(Pollable& pollable)
            : _running(true), _thread(keepPolling, std::ref(_running), std::ref(pollable))
        {
        }

        ~PollThread()
        {
            _running = false;

            if (_thread.joinable())
            {
                _thread.join();
            }
        }

    private:
        static void keepPolling(std::atomic_bool& running, Pollable& pollable)
        {
            while (running.load())
            {
                pollable.poll(CALLBACK_POLLING_INTERVAL_MS);
            }
        }

        static constexpr int CALLBACK_POLLING_INTERVAL_MS = 10;

        std::atomic_bool _running;
        std::thread      _thread;
    };
};

template <typename T>
Logger KafkaClient::Global<T>::logger;

template <typename T>
std::once_flag KafkaClient::Global<T>::initOnce;

inline
KafkaClient::KafkaClient(ClientType                     clientType,
                         const Properties&              properties,
                         const ConfigCallbacksRegister& registerCallbacks,
                         const std::set<std::string>&   privatePropertyKeys)
{
    // Save clientID
    if (auto clientId = properties.getProperty(CLIENT_ID))
    {
        _clientId   = *clientId;
        _clientName = getClientTypeString(clientType) + "[" + _clientId + "]";
    }

    // Init global logger
    std::call_once(Global<>::initOnce, [](){ Global<>::logger = DefaultLogger; });

    // Save LogLevel
    if (auto logLevel = properties.getProperty(LOG_LEVEL))
    {
        try
        {
            _logLevel = std::stoi(*logLevel);
        }
        catch (const std::exception& e)
        {
            KAFKA_THROW_WITH_MSG(RD_KAFKA_RESP_ERR__INVALID_ARG, std::string("Invalid log_level[").append(*logLevel).append("], which must be an number!").append(e.what()));
        }

        if (_logLevel < Log::Level::Emerg || _logLevel > Log::Level::Debug)
        {
            KAFKA_THROW_WITH_MSG(RD_KAFKA_RESP_ERR__INVALID_ARG, std::string("Invalid log_level[").append(*logLevel).append("], which must be a value between 0 and 7!"));
        }
    }

    LogBuffer<LOG_BUFFER_SIZE> errInfo;

    auto rk_conf = rd_kafka_conf_unique_ptr(rd_kafka_conf_new());

    for (const auto& prop: properties.map())
    {
        // Those private properties are only available for `C++ wrapper`, not for librdkafka
        if (privatePropertyKeys.count(prop.first))
        {
            _properties.put(prop.first, prop.second);
            continue;
        }

        rd_kafka_conf_res_t result = rd_kafka_conf_set(rk_conf.get(), prop.first.c_str(), prop.second.c_str(), errInfo.str(), errInfo.capacity());
        if (result == RD_KAFKA_CONF_OK)
        {
            _properties.put(prop.first, prop.second);
        }
        else
        {
            KAFKA_API_DO_LOG(Log::Level::Err, "failed to be initialized with property[%s:%s], result[%d]", prop.first.c_str(), prop.second.c_str(), result);
        }
    }

    // Save KafkaClient's raw pointer to the "opaque" field, thus we could fetch it later (for kinds of callbacks)
    rd_kafka_conf_set_opaque(rk_conf.get(), this);

    // Log Callback
    rd_kafka_conf_set_log_cb(rk_conf.get(), KafkaClient::logCallback);

    // Statistics Callback
    rd_kafka_conf_set_stats_cb(rk_conf.get(), KafkaClient::statsCallback);

    // Other Callbacks
    if (registerCallbacks)
    {
        registerCallbacks(rk_conf.get());
    }

    // Set client handler
    _rk.reset(rd_kafka_new((clientType == ClientType::KafkaConsumer ? RD_KAFKA_CONSUMER : RD_KAFKA_PRODUCER),
                           rk_conf.release(),  // rk_conf's ownship would be transferred to rk, after the "rd_kafka_new()" call
                           errInfo.clear().str(),
                           errInfo.capacity()));
    KAFKA_THROW_IF_WITH_RESP_ERROR(rd_kafka_last_error());

    // Add brokers
    auto brokers = properties.getProperty(BOOTSTRAP_SERVERS);
    if (rd_kafka_brokers_add(getClientHandle(), brokers->c_str()) == 0)
    {
        KAFKA_THROW_WITH_MSG(RD_KAFKA_RESP_ERR__INVALID_ARG,\
                             "No broker could be added successfully, BOOTSTRAP_SERVERS=[" + *brokers + "]");
    }

    _opened = true;
}

inline Properties
KafkaClient::validateAndReformProperties(const Properties& origProperties)
{
    Properties properties(origProperties);

    // BOOTSTRAP_SERVERS property is mandatory
    if (!properties.getProperty(BOOTSTRAP_SERVERS))
    {
        KAFKA_THROW_WITH_MSG(RD_KAFKA_RESP_ERR__INVALID_ARG,\
                             "Validation failed! With no property [" + std::string(BOOTSTRAP_SERVERS) + "]");
    }

    // If no "client.id" configured, generate a random one for user
    if (!properties.getProperty(CLIENT_ID))
    {
        properties.put(CLIENT_ID, Utility::getRandomString());
    }

    // "sasl.kerberos.service.name" is mandatory for SASL connection
    if (auto securityProtocol = properties.getProperty(SECURITY_PROTOCOL))
    {
        if (securityProtocol->find("sasl") != std::string::npos)
        {
            if (!properties.getProperty(SASL_KERBEROS_SERVICE_NAME))
            {
                KAFKA_THROW_WITH_MSG(RD_KAFKA_RESP_ERR__INVALID_ARG,\
                                     "The \"sasl.kerberos.service.name\" property is mandatory for SASL connection!");
            }
        }
    }

    // If no "log_level" configured, use Log::Level::Notice as default
    if (!properties.getProperty(LOG_LEVEL))
    {
        properties.put(LOG_LEVEL, std::to_string(static_cast<int>(Log::Level::Notice)));
    }

    return properties;
}

inline Optional<std::string>
KafkaClient::getProperty(const std::string& name) const
{
    constexpr int DEFAULT_BUF_SIZE = 512;

    const rd_kafka_conf_t* conf = rd_kafka_conf(getClientHandle());

    std::vector<char> valueBuf(DEFAULT_BUF_SIZE);
    std::size_t       valueSize = valueBuf.size();

    // Firstly, try with a default buf size
    if (rd_kafka_conf_get(conf, name.c_str(), valueBuf.data(), &valueSize) != RD_KAFKA_CONF_OK)
    {
        // If doesn't exist within librdkafka, might be from the C++ wrapper
        return _properties.getProperty(name);
    }

    // If the default buf size is not big enough, retry with a larger buf
    if (valueSize > valueBuf.size())
    {
        valueBuf.resize(valueSize);
        [[maybe_unused]] rd_kafka_conf_res_t result = rd_kafka_conf_get(conf, name.c_str(), valueBuf.data(), &valueSize);
        assert(result == RD_KAFKA_CONF_OK);
    }

    return std::string(valueBuf.data());
}

inline void
KafkaClient::setLogLevel(int level)
{
    _logLevel = level < Log::Level::Emerg ? Log::Level::Emerg : (level > Log::Level::Debug ? Log::Level::Debug : level);
    rd_kafka_set_log_level(getClientHandle(), _logLevel);
}

inline void
KafkaClient::onLog(int level, const char* fac, const char* buf) const
{
    doLog(level, nullptr, 0, "%s | %s", fac, buf); // The `filename`/`lineno` here is NULL (just wouldn't help)
}

inline void
KafkaClient::logCallback(const rd_kafka_t* rk, int level, const char* fac, const char* buf)
{
    kafkaClient(rk).onLog(level, fac, buf);
}

inline void
KafkaClient::onStats(std::string&& jsonString)
{
    if (_statsCb) _statsCb(std::move(jsonString));
}

inline int
KafkaClient::statsCallback(rd_kafka_t* rk, char* jsonStrBuf, size_t jsonStrLen, void* /*opaque*/)
{
    kafkaClient(rk).onStats(std::string(jsonStrBuf, jsonStrBuf+jsonStrLen));
    return 0;
}

inline Optional<BrokerMetadata>
KafkaClient::fetchBrokerMetadata(const std::string& topic, std::chrono::milliseconds timeout, bool disableErrorLogging)
{
    Optional<BrokerMetadata> ret;
    auto rkt = rd_kafka_topic_unique_ptr(rd_kafka_topic_new(getClientHandle(), topic.c_str(), nullptr));

    const rd_kafka_metadata_t* rk_metadata = nullptr;
    rd_kafka_resp_err_t err = rd_kafka_metadata(getClientHandle(), false, rkt.get(), &rk_metadata, convertMsDurationToInt(timeout));
    auto guard = rd_kafka_metadata_unique_ptr(rk_metadata);

    if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        if (!disableErrorLogging)
        {
            KAFKA_API_DO_LOG(Log::Level::Err, "failed to get BrokerMetadata! error[%s]", rd_kafka_err2str(err));
        }
        return ret;
    }

    if (rk_metadata->topic_cnt != 1)
    {
        if (!disableErrorLogging)
        {
            KAFKA_API_DO_LOG(Log::Level::Err, "failed to construct MetaData! topic_cnt[%d]", rk_metadata->topic_cnt);
        }
        return ret;
    }

    const rd_kafka_metadata_topic& metadata_topic = rk_metadata->topics[0];
    if (metadata_topic.err != 0)
    {
        if (!disableErrorLogging)
        {
            KAFKA_API_DO_LOG(Log::Level::Err, "failed to construct MetaData!  topic.err[%s]", rd_kafka_err2str(metadata_topic.err));
        }
        return ret;
    }

    // Construct the BrokerMetadata
    BrokerMetadata metadata(metadata_topic.topic);
    metadata.setOrigNodeName(rk_metadata->orig_broker_name ? std::string(rk_metadata->orig_broker_name) : "");

    for (int i = 0; i < rk_metadata->broker_cnt; ++i)
    {
        metadata.addNode(rk_metadata->brokers[i].id, rk_metadata->brokers[i].host, rk_metadata->brokers[i].port);
    }

    for (int i = 0; i < metadata_topic.partition_cnt; ++i)
    {
        const rd_kafka_metadata_partition& metadata_partition = metadata_topic.partitions[i];

        Partition partition = metadata_partition.id;

        if (metadata_partition.err != 0)
        {
            if (!disableErrorLogging)
            {
                KAFKA_API_DO_LOG(Log::Level::Err, "got error[%s] while constructing BrokerMetadata for topic[%s]-partition[%d]", rd_kafka_err2str(metadata_partition.err), topic.c_str(), partition);
            }

            continue;
        }

        BrokerMetadata::PartitionInfo partitionInfo(metadata_partition.leader);

        for (int j = 0; j < metadata_partition.replica_cnt; ++j)
        {
            partitionInfo.addReplica(metadata_partition.replicas[j]);
        }

        for (int j = 0; j < metadata_partition.isr_cnt; ++j)
        {
            partitionInfo.addInSyncReplica(metadata_partition.isrs[j]);
        }

        metadata.addPartitionInfo(partition, partitionInfo);
    }

    ret = metadata;
    return ret;
}


} // end of KAFKA_API


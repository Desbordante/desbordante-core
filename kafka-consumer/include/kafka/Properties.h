#pragma once

#include "kafka/Project.h"

#include "kafka/Types.h"

#include <algorithm>
#include <map>
#include <string>


namespace KAFKA_API {

/**
 * The properties for Kafka clients.
 */
class Properties
{
public:
    // Just make sure key will printed in order
    using PropertiesMap = std::map<std::string, std::string>;

    Properties() = default;
    Properties(const Properties&) = default;
    explicit Properties(PropertiesMap kvMap): _kvMap(std::move(kvMap)) {}

    virtual ~Properties() = default;

    bool operator==(const Properties& rhs) const { return map() == rhs.map(); }

    /**
     * Set a property.
     * If the map previously contained a mapping for the key, the old value is replaced by the specified value.
     */
    Properties& put(const std::string& key, const std::string& value)
    {
        _kvMap[key] = value;
        return *this;
    }

    /**
     * Get a property.
     * If the map previously contained a mapping for the key, the old value is replaced by the specified value.
     */
    Optional<std::string> getProperty(const std::string& key) const
    {
        Optional<std::string> ret;
        auto search = _kvMap.find(key);
        if (search != _kvMap.end())
        {
            ret = search->second;
        }
        return ret;
    }

    /**
     * Remove a property.
     */
    void eraseProperty(const std::string& key)
    {
      _kvMap.erase(key);
    }

    std::string toString() const
    {
        std::string ret;
        std::for_each(_kvMap.cbegin(), _kvMap.cend(),
                      [&ret](const auto& kv) {
                          ret.append(ret.empty() ? "" : "|").append(kv.first).append("=").append(kv.second);
                      });
        return ret;
    }

    /**
     * Get all properties with a map.
     */
    const PropertiesMap& map() const { return _kvMap; }

private:
    PropertiesMap _kvMap;
};

} // end of KAFKA_API


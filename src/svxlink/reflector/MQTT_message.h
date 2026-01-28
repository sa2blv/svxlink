#pragma once

#include <string>
#include <memory>
#include <mqtt/async_client.h>
#include <mutex>
#include <thread>
#include <atomic>
#include <json/json.h>

#include <map>
#include <string>

// Add this at the top of MQTT_message.h
using TopicMap = std::map<std::string, std::string>;


class MQTT_message : public virtual mqtt::callback,
    public virtual mqtt::iaction_listener
{
public:
    // Singleton pointer access
    static MQTT_message* instance();

    // Enable / disable via config
    bool init(const std::string& serverURI,
        const std::string& clientId,
        bool enable,
        const std::string& username = "",
        const std::string& password = "");

    void shutdown();

    // Safe no-op if disabled or not connected
    bool publish(const std::string& topic,
        const std::string& payload,
        int qos = 1,
        bool retained = false);

    bool subscribe(const std::string& topic, int qos = 1);

    bool isEnabled()   const { return enabled_; }
    bool isConnected() const { return connected_; }
    void startBufferThread();
    void stopBufferThread();
    void publishBuffered(const Json::Value& newVal, const std::string& baseTopic = "");
    void removeNode(const std::string& nodeTopic);
    void publishBufferedFull(const Json::Value& newVal,
        const std::string& baseTopic);

    void publishJsonTreeFull(const Json::Value& value, const std::string& baseTopic);

private:
    MQTT_message();
    ~MQTT_message();

    MQTT_message(const MQTT_message&) = delete;
    MQTT_message& operator=(const MQTT_message&) = delete;

    // Callbacks
    void connected(const std::string& cause) override;
    void connection_lost(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
    void delivery_complete(mqtt::delivery_token_ptr tok) override;

    void on_success(const mqtt::token& tok) override;
    void on_failure(const mqtt::token& tok) override;

    void reconnect();



    void publishJsonDiff(const Json::Value& oldVal,
        const Json::Value& newVal,
        const std::string& baseTopic = "");


    void markAllTopicsEmpty(const Json::Value& node, const std::string& prefix, TopicMap& diffs);

    static MQTT_message* instance_;

    bool enabled_{ false };
    bool connected_{ false };

    std::unique_ptr<mqtt::async_client> client_;
    mqtt::connect_options connOpts_;

    std::string serverURI_;
    std::string clientId_;

    Json::Value buffer_;                // latest JSON to publish
    std::mutex bufferMutex_;
    std::atomic<bool> bufferDirty_{ false };
    std::condition_variable bufferCv_;
    std::thread bufferThread_;
    std::atomic<bool> bufferThreadRunning_{ false };


    Json::Value prev_status_;       // last published JSON, for diffing
    std::string bufferBaseTopic_;   // base topic for buffered JSON


    void bufferWorker();                // worker thread to process buffered messages
    bool bufferForceFull_{ false };
    void computeDiffWithRemovals(const Json::Value& oldVal,
        const Json::Value& newVal,
        const std::string& prefix,
        TopicMap& diffs);


};

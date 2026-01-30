/**
@file	 mqtt_message.cpp
@brief   The Mqtt service class
@author  Peter Lundberg / SA2BLV
@date	 2026-01-26

\verbatim
SvxReflector - An mqtt service  for  svxreflector for connecting SvxLink Servers
Copyright (C) 2025-2026 Peter Lundberg / SA2BLV

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/

/****************************************************************************
 *
 * System Includes
 *
 *
 * */


#include "MQTT_message.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <json/json.h>
#include <map>
#include <set>
#include <sstream>
#include "Reflector.h" 

 /****************************************************************************
  *
  * Exported Global Variables
  *
  ****************************************************************************/



  /****************************************************************************
   *
   * Local Global Variables
   *
   ****************************************************************************/


   /****************************************************************************
    *
    * Public static functions
    *
    ****************************************************************************/



MQTT_message* MQTT_message::instance_ = nullptr;

MQTT_message* MQTT_message::instance()
{
    if (!instance_) {
        instance_ = new MQTT_message();
    }
    return instance_;
}

MQTT_message::MQTT_message() = default;

MQTT_message::~MQTT_message()
{
    shutdown();
}

bool MQTT_message::init(const std::string& serverURI,
    const std::string& clientId,
    bool enable,
    const std::string& username ,
    const std::string& password )
{
    enabled_ = enable;

    if (!enabled_) {
        std::cout << "MQTT disabled by configuration" << std::endl;
        return false;
    }

    serverURI_ = serverURI;
    clientId_ = clientId;

    try {
        client_ = std::make_unique<mqtt::async_client>(serverURI_, clientId_);
        client_->set_callback(*this);

        connOpts_.set_clean_session(true);
        connOpts_.set_automatic_reconnect(true);
        connOpts_.set_keep_alive_interval(20);
        mqtt::ssl_options sslopts;
        sslopts.set_enable_server_cert_auth(false);
       // connOpts_.set_ssl(sslopts);


        // Set username and password if provided
        if (!username.empty())
            connOpts_.set_user_name(username);
        if (!password.empty())
            connOpts_.set_password(password);

        std::cout << "MQTT enabled, connecting..." << std::endl;
        client_->connect(connOpts_, nullptr, *this);
    }
    catch (const mqtt::exception& e) {
        std::cerr << "MQTT init error: " << e.what() << std::endl;
        enabled_ = false;
        return false;
    }

    return true;
}

void MQTT_message::shutdown()
{
    if (client_ && connected_) {
        try {
            client_->disconnect()->wait();
        }
        catch (...) {}
    }
    connected_ = false;
}

bool MQTT_message::publish(const std::string& topic,
    const std::string& payload,
    int qos,
    bool retained)
{
    if (!enabled_ || !connected_)
        return false;

    try {
        auto msg = mqtt::make_message(topic, payload);
        msg->set_qos(qos);
        msg->set_retained(retained);
        client_->publish(msg);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool MQTT_message::subscribe(const std::string& topic, int qos)
{

    std::cout << "MQTT: Subscribing to " << topic << " \r\n";
    if (!enabled_ || !connected_)
        return false;

    try {
        client_->subscribe(topic, qos);
        return true;
    }
    catch (...) {
        return false;
    }
}

// ===== Callbacks =====

void MQTT_message::connected(const std::string& cause)
{
    connected_ = true;
    std::cout << "MQTT connected" << std::endl;

    MQTT_message::instance()->subscribe("reflector_ctrl/" + my_id + "/#", 0);
    MQTT_message::instance()->subscribe("reflector_ctrl/all/#", 0);

}

void MQTT_message::connection_lost(const std::string& cause)
{
    connected_ = false;
    std::cerr << "MQTT connection lost: " << cause << std::endl;
}

void MQTT_message::message_arrived(mqtt::const_message_ptr msg)
{
    std::cout << "Incoming message: "
        << msg->get_topic()
        << " -> "
        << msg->to_string()
        << std::endl;

    std::string topic = msg->get_topic();
    std::vector<std::string> parts;
    std::stringstream ss(topic);
    std::string token;

    while (std::getline(ss, token, '/')) {
        parts.push_back(token);
    }


    if (parts.size() > 2 && (parts[1] == "all" || parts[1] == my_id)
        && parts[2] == "PTY")
    {
//        std::cout << "Send message to pty \r\n";
        m_reflector->mqtt_pty_received(msg->to_string());
    }
}

void MQTT_message::delivery_complete(mqtt::delivery_token_ptr)
{
}

void MQTT_message::on_success(const mqtt::token&)
{
}

void MQTT_message::on_failure(const mqtt::token&)
{
}

void MQTT_message::computeDiffWithRemovals(
    const Json::Value& oldVal,
    const Json::Value& newVal,
    const std::string& prefix,
    TopicMap& diffs)
{
    if (newVal.isObject()) {
        std::set<std::string> keys;
        if (oldVal.isObject()) for (const auto& k : oldVal.getMemberNames()) keys.insert(k);
        for (const auto& k : newVal.getMemberNames()) keys.insert(k);

        for (const auto& key : keys) {
            const Json::Value oldChild = oldVal.get(key, Json::nullValue);
            const Json::Value newChild = newVal.get(key, Json::nullValue);
            std::string topic = prefix.empty() ? key : prefix + "/" + key;

            if (newChild.isNull()) {
                diffs[topic] = "";  // removed
            }
            else if (oldChild.isNull() || oldChild != newChild) {
                computeDiffWithRemovals(oldChild, newChild, topic, diffs);
            }
        }
    }
    else if (newVal.isArray()) {
        // flatten array by index
        Json::ArrayIndex size = newVal.size();
        for (Json::ArrayIndex i = 0; i < size; ++i) {
            const Json::Value oldChild = oldVal.isArray() && i < oldVal.size() ? oldVal[i] : Json::nullValue;
            const Json::Value newChild = newVal[i];
            std::string topic = prefix + "/" + std::to_string(i);
            computeDiffWithRemovals(oldChild, newChild, topic, diffs);
        }
        // if old array was longer → remove extra elements
        if (oldVal.isArray() && oldVal.size() > size) {
            for (Json::ArrayIndex i = size; i < oldVal.size(); ++i) {
                std::string topic = prefix + "/" + std::to_string(i);
                diffs[topic] = "";
            }
        }
    }
    else {
        // primitive value
        diffs[prefix] = newVal.asString();
    }
}

void MQTT_message::startBufferThread() {
    bufferThreadRunning_ = true;
    bufferThread_ = std::thread(&MQTT_message::bufferWorker, this);
}

void MQTT_message::stopBufferThread() {
    bufferThreadRunning_ = false;
    bufferCv_.notify_all();
    if (bufferThread_.joinable()) bufferThread_.join();
}

void MQTT_message::publishBuffered(const Json::Value& newVal, const std::string& baseTopic) {
    std::lock_guard<std::mutex> lock(bufferMutex_);
    buffer_ = newVal;
    bufferBaseTopic_ = baseTopic;
    bufferDirty_ = true;
    bufferCv_.notify_all();
}

void MQTT_message::publishBufferedFull(const Json::Value& newVal,const std::string& baseTopic)
{
    std::lock_guard<std::mutex> lock(bufferMutex_);
    buffer_ = newVal;
    bufferBaseTopic_ = baseTopic;
    bufferForceFull_ = true;   
    bufferDirty_ = true;
    bufferCv_.notify_all();
}

void MQTT_message::bufferWorker() {
    while (bufferThreadRunning_) {
        std::unique_lock<std::mutex> lock(bufferMutex_);
        bufferCv_.wait(lock, [this] {
            return bufferDirty_ || !bufferThreadRunning_;
            });

        if (!bufferThreadRunning_)
            break;

        Json::Value toPublish = buffer_;
        std::string baseTopic = bufferBaseTopic_;
        bool forceFull = bufferForceFull_;

        bufferDirty_ = false;
        bufferForceFull_ = false;
        lock.unlock();

        if (forceFull) {
            //  FULL publish: clear old retained topics first
            TopicMap clears;
            markAllTopicsEmpty(prev_status_, baseTopic, clears);

            for (const auto& [topic, value] : clears)
                publish(topic, value, 1, true);

            // publish everything as new
            TopicMap full;
            computeDiffWithRemovals(Json::Value(), toPublish, baseTopic, full);

            for (const auto& [topic, value] : full)
                publish(topic, value, 1, true);
        }
        else {
            //  normal diff publish
            publishJsonDiff(prev_status_, toPublish, baseTopic);
        }

        prev_status_ = toPublish;
    }
}

void MQTT_message::publishJsonDiff(const Json::Value& oldVal,
    const Json::Value& newVal,
    const std::string& baseTopic)
{
    if (!enabled_ || !connected_) return;

    TopicMap diffs;
    computeDiffWithRemovals(oldVal, newVal, baseTopic, diffs);

    for (const auto& [topic, value] : diffs) {
        // use your existing publish() method
        publish(topic, value, 1, true);  // QoS=1, retained=true
    }
}

void MQTT_message::removeNode(const std::string& nodeTopic)
{
    std::lock_guard<std::mutex> lock(bufferMutex_);
    // If you stored prev_status_ for that node:
    if (prev_status_.isObject() && prev_status_.isMember(nodeTopic)) {
        TopicMap diffs;
        // recursively mark all topics under node as empty
        markAllTopicsEmpty(prev_status_[nodeTopic], nodeTopic, diffs);
        for (const auto& [topic, value] : diffs)
            publish(topic, value, 1, true);
        prev_status_.removeMember(nodeTopic); // remove from previous state
    }
}


void MQTT_message::markAllTopicsEmpty(const Json::Value& node,
    const std::string& prefix,
    TopicMap& diffs)
{
    if (node.isObject()) {
        for (const auto& key : node.getMemberNames()) {
            std::string topic = prefix.empty() ? key : prefix + "/" + key;
            markAllTopicsEmpty(node[key], topic, diffs);
        }
    }
    else if (node.isArray()) {
        for (Json::ArrayIndex i = 0; i < node.size(); ++i) {
            std::string topic = prefix + "/" + std::to_string(i);
            markAllTopicsEmpty(node[i], topic, diffs);
        }
    }
    else {
        diffs[prefix] = "";
    }
}

void MQTT_message::publishJsonTreeFullAsync(
    Json::Value node,
    std::string topic)
{
    std::thread(
        [this, node = std::move(node), topic = std::move(topic)]() {
            publishJsonTreeFull(node, topic);
        }
    ).detach();
}




void MQTT_message::publishJsonTreeFull(
    const Json::Value& node,
    const std::string& topic)
{
    if (node.isObject()) {
        for (const auto& key : node.getMemberNames()) {
            publishJsonTreeFull(
                node[key],
                topic.empty() ? key : topic + "/" + key
            );
        }
    }
    else if (node.isArray()) {
        for (Json::ArrayIndex i = 0; i < node.size(); ++i) {
            publishJsonTreeFull(
                node[i],
                topic + "/" + std::to_string(i)
            );
        }
    }
    else {
        // primitive
        publish(topic, node.asString(), 1, true);
    }
}
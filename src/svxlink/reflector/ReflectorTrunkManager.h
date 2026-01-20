#ifndef REFLECTOR_TRUNK_MANAGER_H
#define REFLECTOR_TRUNK_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <regex>
#include <iostream>
#include <sstream> 
#include "ReflectorClientUdp.h"
#include "AsyncConfig.h"

#include <AsyncCppApplication.h>
#include <AsyncUdpSocket.h>
#include <AsyncIpAddress.h>
#include <AsyncTcpClient.h>




enum class TrunkQsoType { QSO_START, QSO_END };

class MsgTrunkQso {
public:
    MsgTrunkQso(TrunkQsoType type, int tg, const std::string &gatewayId)
        : m_type(type), m_tg(tg), m_gatewayId(gatewayId) {}

    TrunkQsoType type() const { return m_type; }
    int tg() const { return m_tg; }
    const std::string &gatewayId() const { return m_gatewayId; }

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buf;
        buf.push_back(static_cast<uint8_t>(m_type));
        buf.push_back((m_tg >> 8) & 0xFF);
        buf.push_back(m_tg & 0xFF);
        buf.insert(buf.end(), m_gatewayId.begin(), m_gatewayId.end());
        return buf;
    }

    static MsgTrunkQso deserialize(const std::vector<uint8_t> &buf) {
        if (buf.size() < 3) throw std::runtime_error("Buffer too short");
        TrunkQsoType type = static_cast<TrunkQsoType>(buf[0]);
        int tg = (buf[1] << 8) | buf[2];
        std::string gatewayId(buf.begin() + 3, buf.end());
        return MsgTrunkQso(type, tg, gatewayId);
    }

private:

    TrunkQsoType m_type;
    int m_tg;
    std::string m_gatewayId;
};





// Typ av QSO-meddelande
enum class ReflectorQsoType { QSO_START, QSO_END };

// MsgReflectorQso klass (ersätter den saknade)
class MsgReflectorQso {
public:
    MsgReflectorQso()
        : m_type(ReflectorQsoType::QSO_START), m_tg(0), m_gatewayId("") {}

    MsgReflectorQso(ReflectorQsoType type, int tg, const std::string &gatewayId)
        : m_type(type), m_tg(tg), m_gatewayId(gatewayId) {}

    // Getters och setters
    ReflectorQsoType type() const { return m_type; }
    void setType(ReflectorQsoType t) { m_type = t; }

    int tg() const { return m_tg; }
    void setTg(int t) { m_tg = t; }

    const std::string &gatewayId() const { return m_gatewayId; }
    void setGatewayId(const std::string &id) { m_gatewayId = id; }

    // Serialisering
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buf;
        buf.push_back(static_cast<uint8_t>(m_type));
        buf.push_back((m_tg >> 8) & 0xFF);
        buf.push_back(m_tg & 0xFF);
        buf.insert(buf.end(), m_gatewayId.begin(), m_gatewayId.end());
        return buf;
    }

    // Deserialisering
    static MsgReflectorQso deserialize(const std::vector<uint8_t> &buf) {
        if (buf.size() < 3) throw std::runtime_error("Buffer too short");
        ReflectorQsoType type = static_cast<ReflectorQsoType>(buf[0]);
        int tg = (buf[1] << 8) | buf[2];
        std::string gatewayId(buf.begin() + 3, buf.end());
        return MsgReflectorQso(type, tg, gatewayId);
    }

private:
    ReflectorQsoType m_type;
    int m_tg;
    std::string m_gatewayId;
};

struct TrunkPeerTnaslation {
    int src_tg;
    int dest_tg;
};


struct TrunkPeer {
    std::string name;
    std::string host;
    int audioPort = 0;
    int controlPort = 0;
    int Retransmitt = 0;
    int Active_filter;
    std::regex tgRule;
    bool Use_tg_translation = false;
    std::string tranlationfile = "";
    std::vector<TrunkPeerTnaslation> Translation;
    std::vector<TrunkPeerTnaslation> Translation_dest;
    std::shared_ptr<ReflectorClientUdp> client;
    std::vector<int> activeTalkgroups;
    std::string Trunk_type;
    std::string Trunk_type_send;
    std::string Crypt_key;

//    std::shared_ptr Async::TcpClient<> con;
};



using namespace std;
using namespace Async;

class ReflectorTrunkManager : public sigc::trackable  {
public:
    static ReflectorTrunkManager* instance(void)
    {
      static ReflectorTrunkManager *inst  = new ReflectorTrunkManager;
      return inst;
    }

    /**
     * @brief   Default constructor
     */
    ReflectorTrunkManager(void);

    /**
     * @brief   Destructor
     */
    ~ReflectorTrunkManager(void);

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */
     
     

    void init();
    const std::vector<TrunkPeer>& getPeers() const { return peers; }
    void handleOutgoingAudio(int tg, std::ostringstream &msg);
    
    void notifyQsoStart(int tg, std::string Talker);
    void notifyQsoEnd(int tg, std::string Talker);
    bool handleIncomingQso(ReflectorClientUdp *client, const MsgTrunkQso &qso);
    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */
    void setConfig(const Async::Config* cfg) { m_cfg = cfg; }


	void addPeer(const std::string& str) {
	    config_peers.push_back(str);  // <-- pass the string here
	}


    bool udp_init(uint16_t port = 0);
    bool sendMessage(const std::string& ip, uint16_t port, const std::string& msg);
    void handleOutgoingAudio_resend( int tg,  std::stringstream& msg, std::string host);
    std::string receiveMessage();

    	void print_config_peers() const {
	for (const auto& s : config_peers) {
	    std::cout << s << " ";
	}
	std::cout << std::endl;
	}


    int is_ip_allowed(  std::string host);
  //  void  addTranslation(int src_tg, int dest_tg);
   void handleFilter_tunks(std::vector<int>& tgs);
   void incomming_filter(std::vector<int>& tgs, std::string host);
   void send_hello();
   void handleOutgoingAudio_width_remap(int tg,  MsgUdpAudio_trunk& msg);
   void handleOutgoingMessage_width_remap(int tg, MSG_Trunk_Change& msg_trunk);
   std::vector<unsigned char> decryptAES(
       const void* data,
       size_t len,
       const std::string& key
   );
   std::string get_key(std::string host);
   void handleOutgoingAudio_resend_newmsg( int tg, ReflectorUdpMsg& msg_new, std::string host);
   int get_tg_from_dest_table(int tg, std::string host);
   void handleOutgoingAudio_resend_status_newmsg( int tg, MSG_Trunk_Change& msg_new, std::string host);

private:

    const Async::Config*  m_cfg = nullptr;
    int m_localAudioPort = 5401;
    int m_localControlPort = 5400;
    std::string m_gatewayId;
    std::vector<std::string>config_peers;
    std::vector<TrunkPeer> peers;
 
    void  add_peers(const std::string& str);
    int sockfd = -1;                 // socket descriptor
    struct sockaddr_in localAddr;
    
    
    UdpSocket * sock;
    bool isTalkgroupAllowed(const TrunkPeer& peer, int tg);
    void addTranslation(std::vector<TrunkPeerTnaslation>& translations, int src_tg, int dest_tg);

    void onDataReceived(const IpAddress& addr, uint16_t port,void *buf, int count)
    {
      cout << "Data received from " << addr << ":" << port << ": "
           << static_cast<int *>(buf);
    }
    std::string hostnameToIp(const std::string& host);
    std::vector<TrunkPeerTnaslation> read_tg_mapping(std::string filename);
    int find_remap_dest_from_tg(const std::vector<TrunkPeerTnaslation>& vec, int src_tg);
    int find_remap_src_from_tg(const std::vector<TrunkPeerTnaslation>& vec, int dest_tg);
};

#endif // REFLECTOR_TRUNK_MANAGER_H

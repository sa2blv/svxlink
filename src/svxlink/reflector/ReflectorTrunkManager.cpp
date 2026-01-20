/**
@file	 RefletkorTrunkManager.cpp
@brief   The Trunk service class
@author  Peter Lundberg / SA2BLV
@date	 2017-02-11

\verbatim
SvxReflector - An Trunk for  svxreflector for connecting SvxLink Servers
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


#include "ReflectorTrunkManager.h"
#include "TGHandler.h"
#include "ReflectorClientUdp.h"

#include "ReflectorMsg.h"


#include <iostream>
#include <regex>
#include "ReflectorClientUdp.h"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>


#include <iostream>
#include <string>
#include <cstring>      // for memset
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>     // for close()
#include <netdb.h>
#include <fstream>
#include <vector>
#include <algorithm>


 /****************************************************************************
  *
  * Project Includes
  *
  ****************************************************************************/



#include <AsyncCppApplication.h>
#include <AsyncUdpSocket.h>
#include <AsyncIpAddress.h>

#include <openssl/evp.h>
#include <openssl/rand.h>


/****************************************************************************
*
* Local Includes
*
****************************************************************************/




/****************************************************************************
*
* Namespaces to use
*
****************************************************************************/

using namespace std;
using namespace Async;


/****************************************************************************
*
* Public member functions
*
****************************************************************************/



ReflectorTrunkManager::ReflectorTrunkManager(void)
  : m_cfg(0)
{



} /* ReflectorTrunkManager::ReflectorTrunkManager */


ReflectorTrunkManager::~ReflectorTrunkManager(void)
{
        if (sockfd >= 0) close(sockfd);

} /* ReflectorTrunkManager::~ReflectorTrunkManager */


void  ReflectorTrunkManager::add_peers(const std::string& str) 
{
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, ',')) {    

   
    }

}


void ReflectorTrunkManager::init()
{

    std::cout << "-- Loding trunks-- " << std::endl;
   std::cout << "m_cfg function = " << static_cast<const void *>(m_cfg) << std::endl;

   
    // --- Huvudinställningar för denna trunk ---
    m_cfg->getValue("ReflectorTrunk", "GatewayId", m_gatewayId);
    m_cfg->getValue("ReflectorTrunk", "Port", m_localAudioPort);
    m_cfg->getValue("ReflectorTrunk", "Port", m_localControlPort);

    std::cout << "Server id: "<<m_gatewayId << std::endl;
    std::cout << "Server LocalAudioPort: "<<m_localAudioPort << std::endl;
    std::cout << "Server LocalControlPort: "<<m_localControlPort << std::endl;
    
   // remove udp_init(m_localAudioPort);
        
        
 //   sock = new UdpSocket(m_localAudioPort);
 //     sock->dataReceived.connect(mem_fun(*this, &ReflectorTrunkManager::onDataReceived));

        
    std::string config_peers_str;
    m_cfg->getValue("ReflectorTrunk", "Peers", config_peers_str);
    std::istringstream iss(config_peers_str);
 
   std::string token;

   while (std::getline(iss, token, ',')) 
   {
 	addPeer(token);
   }

      
    std::cout << "Found following peers in config: " << std::endl;
    print_config_peers();
    


    for (const auto& sec : config_peers) {
    
      std::ostringstream ss;
      ss << "TrunkPeer#" << sec;
  
     
        TrunkPeer peer;
        peer.name = ss.str(); // "TrunkPeer <name>"
      //  std::string sec_cstr(sec);
          
        
        // Läs värden från config
        std::string hostStr;
        
     //   std::cout << "Getting information from peer:" <<ss.str() << std::endl;
      

        m_cfg->getValue(ss.str(), "Host", hostStr );
        peer.host = hostnameToIp(hostStr);

        std::string key_str;
        // Läs värden från config
        m_cfg->getValue(ss.str(), "Key", key_str);
        peer.Crypt_key = key_str;

     
        int port=0;
        m_cfg->getValue(ss.str(), "Port", port);
      //  std::cout << "Getting infomation from peer port1 :" << port << std::endl;
        peer.controlPort = port;
        peer.audioPort = port;
        
   
          int Retransmitt=0;
        m_cfg->getValue(ss.str(), "Retransmit", Retransmitt);

        peer.Retransmitt = Retransmitt;
        
        
        int Active_filter1 =0;
        m_cfg->getValue(ss.str(), "ActiveFilter", Active_filter1);
        peer.Active_filter = Active_filter1;
       

        // TGRule (regex) med felhantering
        std::string tgRuleStr;
        m_cfg->getValue(ss.str(), "TGRule", tgRuleStr);
        try {
            peer.tgRule = std::regex(tgRuleStr);
        } catch (const std::regex_error &e) {
            std::cerr << "[ReflectorTrunk] Warning: Invalid TGRule regex for peer " 
                      << peer.name << ": " << tgRuleStr << " (" << e.what() << ")" << std::endl;
            peer.tgRule = std::regex(".*"); // fallback
        }


        // Läs värden från config
        std::string TG_mapping_file = "";

        m_cfg->getValue(ss.str(), "TgMapFile", TG_mapping_file);
//        std::cout << "Adding mapping from file   " << TG_mapping_file << std::endl;

        if (TG_mapping_file != "")
        {
            std::cout << "Adding mapping from file " << TG_mapping_file << "to "<< peer.name << std::endl;

            peer.tranlationfile = TG_mapping_file;
            peer.Use_tg_translation = true;
            peer.Translation = read_tg_mapping(TG_mapping_file);

            peer.Translation_dest = peer.Translation;

                std::sort(peer.Translation_dest.begin(), peer.Translation_dest.end(), [](const TrunkPeerTnaslation& a, const TrunkPeerTnaslation& b) {
                return a.dest_tg < b.dest_tg;
                    });

        }
  
        std::string trunk_type = "";

        m_cfg->getValue(ss.str(), "TrunkType", trunk_type);
        peer.Trunk_type = trunk_type;


        std::string Trunk_type_send = "";

        m_cfg->getValue(ss.str(), "TrunkTypeSend", Trunk_type_send);
        peer.Trunk_type_send = Trunk_type_send;


        // Skapa UDP-klient mot remote reflektor
      peer.client = std::make_shared<ReflectorClientUdp>(peer.host, peer.audioPort,peer.controlPort);


        // Lägg till i peers-listan
        peers.push_back(peer);

        std::cout << "[ReflectorTrunk] Added trunk peer " << peer.name
                  << " -> " << peer.host << ":" << peer.audioPort
                  << " TGRule=" << tgRuleStr << std::endl;




   }  


  
    
}

/*
Reads the csv file for tg mapping
*/

std::vector<TrunkPeerTnaslation> ReflectorTrunkManager::read_tg_mapping(std::string filename)
{
    std::ifstream file(filename);  // Open the CSV file
    if (!file.is_open()) {
        std::cerr << "Could not open the file!" << std::endl;
    }


    std::vector<TrunkPeerTnaslation> peer_tg;



    std::string line;
    // Read the header line first (tg,mapped_tg,description)
    std::getline(file, line);

    // Loop through each line
    while (std::getline(file, line)) {

        TrunkPeerTnaslation tg_to_tg;
        std::stringstream ss(line);
        std::string tgStr, mappedTgStr, description;

        // Split by comma
        std::getline(ss, tgStr, ',');
        std::getline(ss, mappedTgStr, ',');
        std::getline(ss, description, ',');

        // Convert tg and mapped_tg to integers
        int tg = std::stoi(tgStr);
        int mapped_tg = std::stoi(mappedTgStr);

        tg_to_tg.src_tg = tg;
        tg_to_tg.dest_tg = mapped_tg;

    
        // Print or use the values
        std::cout << "TG: " << tg
            << ", Mapped TG: " << mapped_tg
            << ", Description: " << description << std::endl;
        peer_tg.push_back(tg_to_tg);
    }


    file.close();

    std::sort(peer_tg.begin(), peer_tg.end(), [](const TrunkPeerTnaslation& a, const TrunkPeerTnaslation& b) {
        return a.src_tg < b.src_tg;
        });


    return peer_tg;
  


}


void ReflectorTrunkManager::addTranslation(std::vector<TrunkPeerTnaslation>& translations ,int src_tg, int dest_tg)
{
    translations.push_back({ src_tg, dest_tg });

}

// --- send tg to flilterd trunk peers ---
void ReflectorTrunkManager::send_hello()
{
    for (auto& peer : peers)
    {
        // send hello to trunks if a restart has ocuured.
        ostringstream ss1;
        ss1 << "hello\r\n";

        if (peer.Crypt_key != "")
        {
            peer.client->sendUdp_crypt(ss1.str().data(), ss1.str().size(), peer.Crypt_key);
        }
        else
        {
            peer.client->sendUdp(ss1.str().data(), ss1.str().size());
        }
      


    }
}




// --- send tg to flilterd trunk peers ---
void ReflectorTrunkManager::handleFilter_tunks(std::vector<int>& tgs)
{
    MSG_Trunk_tg_subsribe msg_trunk1;

    msg_trunk1.Talkgroups = tgs;
    ReflectorUdpMsgV2 header1(msg_trunk1.type(), 10,
        10 & 0xffff);

    for (auto& peer : peers)
    {

//        std::cout << "Sending message to " << peer.host <<"Active filter is "<< peer.Active_filter << "\r\n";

        if (peer.Active_filter == 1)
        {
            std::cout << "Sending filter message to peer " << peer.host << std::endl;
            msg_trunk1.trunkid = peer.host;
            ostringstream ss;
            assert(header1.pack(ss) && msg_trunk1.pack(ss));

            if (peer.Crypt_key != "")
            {
                peer.client->sendUdp_crypt(ss.str().data(), ss.str().size(), peer.Crypt_key);
            }
            else
            {
                peer.client->sendUdp(ss.str().data(), ss.str().size());
            }

        }
        else
        {
            return;
        }
    }
}

// --- send tg to flilterd trunk peers ---
void ReflectorTrunkManager::incomming_filter(std::vector<int>& tgs, std::string host)
{

    for (auto& peer : peers)
    {

        if (peer.host == host)
        {
              std::cout << "Set talkgroup filter to peer " << host  << "\r\n" << "Tgs[";


              for (int tg : tgs)
              {
                  std::cout << tg << ",";

              }
              std::cout<< "]" << std::endl;

            peer.activeTalkgroups = tgs;
            return;

        }
     


    }
}

// --- send incomming audio to all trunk peers ---
void ReflectorTrunkManager::handleOutgoingAudio( int tg,  std::ostringstream &msg)
{

    for (auto &peer : peers)
    {
     //   if (!std::regex_match(std::to_string(tg), peer.tgRule))
     //       continue;

        if (isTalkgroupAllowed(peer, tg))
        {

            if (peer.Crypt_key != "")
            {
                peer.client->sendUdp_crypt(msg.str().data(), msg.str().size(), peer.Crypt_key);
            }
            else
            {
                peer.client->sendUdp(msg.str().data(), msg.str().size());
            }



  
        }

    }
}


// --- send tg to flilterd trunk peers ---
int ReflectorTrunkManager::get_tg_from_dest_table(int tg, std::string host)
{

    for (auto& peer : peers)
    {
    


        if (peer.host == host && peer.Use_tg_translation == true)
        {

                // returning tg from tabel
                return    find_remap_src_from_tg(peer.Translation_dest, tg);

        }



    }

    return tg;
}



/*
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
    std::shared_ptr<ReflectorClientUdp> client;
    std::vector<int> activeTalkgroups;

//    std::shared_ptr Async::TcpClient<> con;
};
*/


// --- send incomming audio to all trunk peers ---
void ReflectorTrunkManager::handleOutgoingAudio_width_remap(int tg,  MsgUdpAudio_trunk& msg_trunk)
{

    for (auto& peer : peers)
    {
        //   if (!std::regex_match(std::to_string(tg), peer.tgRule))
        //       continue;

        if (peer.Use_tg_translation == true)
        {
            msg_trunk.tg = find_remap_dest_from_tg(peer.Translation, tg);
        }

        ReflectorUdpMsgV2 header(msg_trunk.type(), 0,
            0 & 0xffff);


        ostringstream ss;
        assert(header.pack(ss) && msg_trunk.pack(ss));


        if (isTalkgroupAllowed(peer, tg))
        {
            if (peer.Crypt_key != "")
            {
                peer.client->sendUdp_crypt(ss.str().data(), ss.str().size(), peer.Crypt_key);
            }
            else
            {
                peer.client->sendUdp(ss.str().data(), ss.str().size());
            }
        }

    }
}

// --- send incomming audio to all trunk peers ---
void ReflectorTrunkManager::handleOutgoingMessage_width_remap(int tg, MSG_Trunk_Change& msg_trunk)
{

    for (auto& peer : peers)
    {
        //   if (!std::regex_match(std::to_string(tg), peer.tgRule))
        //       continue;

        if (peer.Use_tg_translation == true)
        {

            msg_trunk.tg = find_remap_dest_from_tg(peer.Translation, tg);
//            std::cout << "New_tg: " << msg_trunk.tg << "\r\n";

        }


        ReflectorUdpMsgV2 header(msg_trunk.type(), 0,
            0 & 0xffff);


        ostringstream ss;
        assert(header.pack(ss) && msg_trunk.pack(ss));


        if (isTalkgroupAllowed(peer, tg))
        {
            if (peer.Crypt_key != "")
            {
                peer.client->sendUdp_crypt(ss.str().data(), ss.str().size(), peer.Crypt_key);
            }
            else
            {
                peer.client->sendUdp(ss.str().data(), ss.str().size());
            }
        }

    }
}



int ReflectorTrunkManager::find_remap_dest_from_tg(const std::vector<TrunkPeerTnaslation>& vec, int src_tg)
{
    // Make sure vector is sorted by src_tg for binary search
    auto it = std::lower_bound(vec.begin(), vec.end(), src_tg,
        [](const TrunkPeerTnaslation& a, int value) { return a.src_tg < value; });

    if (it != vec.end() && it->src_tg == src_tg) {
        return it->dest_tg;  // found
    }
    else {
        return src_tg;       // not found, return input
    }
}

int ReflectorTrunkManager::find_remap_src_from_tg(const std::vector<TrunkPeerTnaslation>& vec, int dest_tg)
{
    // Make sure vector is sorted by dest_tg for binary search
    auto it = std::lower_bound(vec.begin(), vec.end(), dest_tg,
        [](const TrunkPeerTnaslation& a, int value) { return a.dest_tg < value; });

    if (it != vec.end() && it->dest_tg == dest_tg) {
        return it->src_tg;  // found
    }
    else {
        return dest_tg;       // not found, return input
    }
}



bool ReflectorTrunkManager::isTalkgroupAllowed(const TrunkPeer& peer, int tg)
{
//    if (!peer.Active_filter)
//        return true; // filtering disabled

    if (peer.activeTalkgroups.empty())
        return true; // no talkgroups configured → allow all
    else
    {

    }


    auto it = std::find(
        peer.activeTalkgroups.begin(),
        peer.activeTalkgroups.end(),
        tg
    );
    /*
    std::cout << "[DEBUG] result = "
        << (it == peer.activeTalkgroups.end() ? "NOT FOUND" : "FOUND")
        << std::endl;
        */

    return it != peer.activeTalkgroups.end();

}



// --- send audio to all trunk peers ---
void ReflectorTrunkManager::handleOutgoingAudio_resend( int tg, std::stringstream& msg, std::string host)
{

    std::string Trunk_type_send;

    for (auto& peer : peers)
    {
        if (peer.host == host )
        {
            Trunk_type_send = peer.Trunk_type_send;
            break;
        }
    }


    for (auto &peer : peers)
    {

        if (!std::regex_match(std::to_string(tg), peer.tgRule))
            continue;
            
            
        if(peer.host != host && peer.Retransmitt == 1
            && isTalkgroupAllowed(peer, tg) == true)
        {
           
            if (Trunk_type_send == "" || Trunk_type_send == peer.Trunk_type)
            {
//                std::cout << "truk_t  " << Trunk_type_send << " peer:" << peer.host << " -> " << peer.Trunk_type << "\r\n";
//                std::cout << "Sending\r\n  ";
                if (peer.Crypt_key != "")
                {
                    peer.client->sendUdp_crypt(msg.str().data(), msg.str().size(), peer.Crypt_key);
                }
                else
                {
                    peer.client->sendUdp(msg.str().data(), msg.str().size());
                }


            }
        }
    }
}

// --- send audio to all trunk peers ---
void ReflectorTrunkManager::handleOutgoingAudio_resend_newmsg( int tg, ReflectorUdpMsg& msg_new, std::string host)
{

            ReflectorUdpMsgV2 header(msg_new.type(), 0,
            0 & 0xffff);


        ostringstream ss;
        assert(header.pack(ss) && msg_new.pack(ss));



    std::string Trunk_type_send;

    for (auto& peer : peers)
    {
        if (peer.host == host )
        {
            Trunk_type_send = peer.Trunk_type_send;
            break;
        }
    }


    for (auto &peer : peers)
    {

        if (!std::regex_match(std::to_string(tg), peer.tgRule))
            continue;


        if(peer.host != host && peer.Retransmitt == 1
            && isTalkgroupAllowed(peer, tg) == true)
        {

            if (Trunk_type_send == "" || Trunk_type_send == peer.Trunk_type)
            {
//                std::cout << "truk_t  " << Trunk_type_send << " peer:" << peer.host << " -> " << peer.Trunk_type << "\r\n";
//                std::cout << "Sending\r\n  ";
                if (peer.Crypt_key != "")
                {
                    peer.client->sendUdp_crypt(ss.str().data(), ss.str().size(), peer.Crypt_key);
                }
                else
                {
                    peer.client->sendUdp(ss.str().data(), ss.str().size());
                }


            }
        }
    }
}

// --- send audio to all trunk peers ---
void ReflectorTrunkManager::handleOutgoingAudio_resend_status_newmsg( int tg, MSG_Trunk_Change& msg_new, std::string host)
{

            ReflectorUdpMsgV2 header(msg_new.type(), 0,
            0 & 0xffff);


        ostringstream ss;
        assert(header.pack(ss) && msg_new.pack(ss));



    std::string Trunk_type_send;

    for (auto& peer : peers)
    {
        if (peer.host == host )
        {
            Trunk_type_send = peer.Trunk_type_send;
            break;
        }
    }


    for (auto &peer : peers)
    {

        if (!std::regex_match(std::to_string(tg), peer.tgRule))
            continue;


        if(peer.host != host && peer.Retransmitt == 1
            && isTalkgroupAllowed(peer, tg) == true)
        {

            if (Trunk_type_send == "" || Trunk_type_send == peer.Trunk_type)
            {
//                std::cout << "truk_t  " << Trunk_type_send << " peer:" << peer.host << " -> " << peer.Trunk_type << "\r\n";
//                std::cout << "Sending\r\n  ";
                if (peer.Crypt_key != "")
                {
                    peer.client->sendUdp_crypt(ss.str().data(), ss.str().size(), peer.Crypt_key);
                }
                else
                {
                    peer.client->sendUdp(ss.str().data(), ss.str().size());
                }


            }
        }
    }
}






int ReflectorTrunkManager::is_ip_allowed(  std::string host)
{


    for (auto &peer : peers)
    {
                     
        if(peer.host == host  )
        {
		    return 1;
        }
    }
    
    return 0;
}

std::string ReflectorTrunkManager::get_key(std::string host)
{


    for (auto& peer : peers)
    {

        if (peer.host == host)
        {
            if (peer.Crypt_key != "")
            {
                return peer.Crypt_key;
            }
            else
            {
                return "";
            }
        }
    }

    return "";
}





void ReflectorTrunkManager::notifyQsoStart(int tg, std::string Talker)
{

    for (auto &peer : peers)
    {
        if (!std::regex_match(std::to_string(tg), peer.tgRule))
            continue;
            
            
    }


 
}

void ReflectorTrunkManager::notifyQsoEnd(int tg, std::string Talker)
{


}


std::vector<unsigned char> ReflectorTrunkManager::decryptAES(
    const void* data,
    size_t len,
    const std::string& key
) 
{
    // 🔹 No decryption
    if (key.empty()) {
        const unsigned char* p =
            static_cast<const unsigned char*>(data);
        return std::vector<unsigned char>(p, p + len);
    }

    if (len < 16) {
        return {};
    }

    const unsigned char* input =
        static_cast<const unsigned char*>(data);

    const unsigned char* iv = input;
    const unsigned char* ciphertext = input + 16;
    size_t ciphertext_len = len - 16;

    unsigned char aes_key[16] = { 0 };
    memcpy(aes_key, key.data(),
        std::min(key.size(), sizeof(aes_key)));

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    std::vector<unsigned char> plaintext(ciphertext_len);
    int out_len1 = 0, out_len2 = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, aes_key, iv);

    if (!EVP_DecryptUpdate(
        ctx,
        plaintext.data(),
        &out_len1,
        ciphertext,
        ciphertext_len)) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    if (!EVP_DecryptFinal_ex(
        ctx,
        plaintext.data() + out_len1,
        &out_len2)) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    plaintext.resize(out_len1 + out_len2);
    EVP_CIPHER_CTX_free(ctx);

    return plaintext;
}






std::string ReflectorTrunkManager::hostnameToIp(const std::string& host) {
    // 1. If already an IPv4 or IPv6 address, return it
    sockaddr_in sa4{};
    sockaddr_in6 sa6{};

    if (inet_pton(AF_INET, host.c_str(), &sa4.sin_addr) == 1) {
        return host; // IPv4
    }
    if (inet_pton(AF_INET6, host.c_str(), &sa6.sin6_addr) == 1) {
        return host; // IPv6
    }

    // 2. Resolve hostname
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* res = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0) {
        return "";
    }

    char ipstr[INET6_ADDRSTRLEN];

    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        void* addr = nullptr;

        if (p->ai_family == AF_INET) {
            addr = &((sockaddr_in*)p->ai_addr)->sin_addr;
        }
        else if (p->ai_family == AF_INET6) {
            addr = &((sockaddr_in6*)p->ai_addr)->sin6_addr;
        }

        if (addr) {
            inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
            freeaddrinfo(res);
            return ipstr;
        }
    }

    freeaddrinfo(res);
    return "";
}





    



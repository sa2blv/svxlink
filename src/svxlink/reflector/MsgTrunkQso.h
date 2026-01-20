#pragma once
#include <string>
#include <vector>
#include <stdexcept>


// Forward-deklaration, ersätt med din riktiga klass
class ReflectorClientUdp;

struct Peer {
    std::string tgRule; // t.ex. "4000|4001"
    ReflectorClientUdp *client;
};




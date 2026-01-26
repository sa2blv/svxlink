#ifndef AUDIO_PACKET_H
#define AUDIO_PACKET_H

#include <cstdint>
#include <cstddef>

struct AudioPacket
{
  uint32_t tg;                   // Talkgroup
  uint32_t origin_reflector_id;  // Ursprunglig reflector
  uint32_t talker_id;            // Vem som pratar (kan vara node ID)
  uint32_t seq;
  uint64_t timestamp;
  const void* payload;
  size_t payload_len;
};

#endif // AUDIO_PACKET_H

#include "NetworkMessage.h"
#include <cmath>
#include <cstring>

const char* GetPacketTypeName(PacketType type) {
    switch (type) {
    case PacketType::Join:         return "Join";
    case PacketType::Accept:       return "Accept";
    case PacketType::Reject:       return "Reject";
    case PacketType::PlayerJoined: return "PlayerJoined";
    case PacketType::PlayerLeft:   return "PlayerLeft";
    case PacketType::Leave:        return "Leave";
    case PacketType::SceneChange:  return "SceneChange";
    case PacketType::SceneReady:   return "SceneReady";
    case PacketType::Spawn:        return "Spawn";
    case PacketType::Despawn:      return "Despawn";
    case PacketType::Event:        return "Event";
    case PacketType::SharedValue:  return "SharedValue";
    case PacketType::Ping:         return "Ping";
    case PacketType::Pong:         return "Pong";
    case PacketType::Input:        return "Input";
    case PacketType::Snapshot:     return "Snapshot";
    default:                       return "Unknown";
    }
}

const char* GetRejectReasonText(RejectReason reason) {
    switch (reason) {
    case RejectReason::Full:            return "Room is full";
    case RejectReason::VersionMismatch: return "Protocol version mismatch";
    case RejectReason::GameIdMismatch:  return "Different game";
    default:                            return "Invalid request";
    }
}

// ─── NetworkWriter ───

void NetworkWriter::WriteBytes(const void* bytes, size_t byteCount) {
    if (!bytes || byteCount == 0) return;
    const uint8_t* p = static_cast<const uint8_t*>(bytes);
    data.insert(data.end(), p, p + byteCount);
}

void NetworkWriter::WriteU8(uint8_t value) { data.push_back(value); }

void NetworkWriter::WriteU16(uint16_t value) {
    data.push_back((uint8_t)(value & 0xFF));
    data.push_back((uint8_t)(value >> 8));
}

void NetworkWriter::WriteU32(uint32_t value) {
    for (int i = 0; i < 4; i++) data.push_back((uint8_t)((value >> (8 * i)) & 0xFF));
}

void NetworkWriter::WriteI32(int32_t value) { WriteU32((uint32_t)value); }

void NetworkWriter::WriteFloat(float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    WriteU32(bits);
}

void NetworkWriter::WriteDouble(double value) {
    uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    WriteU32((uint32_t)(bits & 0xFFFFFFFFu));
    WriteU32((uint32_t)(bits >> 32));
}

void NetworkWriter::WriteString(const std::string& value) {
    const size_t length = value.size() > 255 ? 255 : value.size();
    WriteU8((uint8_t)length);
    WriteBytes(value.data(), length);
}

NetworkWriter BeginPacket(PacketType type) {
    NetworkWriter writer;
    writer.WriteU8((uint8_t)type);
    return writer;
}

// ─── NetworkReader ───

bool NetworkReader::ReadBytes(void* destination, size_t byteCount) {
    if (failed || byteCount > size - position) {
        failed = true;
        return false;
    }
    if (byteCount > 0) std::memcpy(destination, data + position, byteCount);
    position += byteCount;
    return true;
}

uint8_t NetworkReader::ReadU8() {
    uint8_t value = 0;
    ReadBytes(&value, 1);
    return value;
}

uint16_t NetworkReader::ReadU16() {
    uint8_t b[2] = {};
    if (!ReadBytes(b, 2)) return 0;
    return (uint16_t)(b[0] | (b[1] << 8));
}

uint32_t NetworkReader::ReadU32() {
    uint8_t b[4] = {};
    if (!ReadBytes(b, 4)) return 0;
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

int32_t NetworkReader::ReadI32() { return (int32_t)ReadU32(); }

float NetworkReader::ReadFloat() {
    const uint32_t bits = ReadU32();
    float value = 0.f;
    std::memcpy(&value, &bits, sizeof(value));
    if (!std::isfinite(value)) {   // NaN・無限大は壊れたパケット
        failed = true;
        return 0.f;
    }
    return value;
}

double NetworkReader::ReadDouble() {
    const uint64_t low = ReadU32();
    const uint64_t high = ReadU32();
    const uint64_t bits = low | (high << 32);
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    if (!std::isfinite(value)) {
        failed = true;
        return 0.0;
    }
    return value;
}

std::string NetworkReader::ReadString(size_t maxLength) {
    const size_t length = ReadU8();
    if (failed || length > maxLength || length > size - position) {
        failed = true;
        return std::string();
    }
    std::string value(reinterpret_cast<const char*>(data + position), length);
    position += length;
    return value;
}

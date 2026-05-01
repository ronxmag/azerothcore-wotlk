/*
 * Dark Chaos - Unified Addon Communication Namespace
 * =====================================================
 *
 * This header defines the unified DCAddon namespace for all client-server
 * addon communication in Dark Chaos.
 *
 * Architecture:
 * - All DC addon messages use the unified "DC" prefix
 * - Subsystems are identified by MODULE byte in the message
 * - Coexists with AIO (SAIO/CAIO) without conflict
 *
 * Message Format:
 * Simple: DC|MODULE|OPCODE|DATA1|DATA2|...
 * JSON:   DC|MODULE|OPCODE|J|{"key":"value",...}
 *
 * Where MODULE is one of:
 * - AOE  (AOE Loot system)
 * - SPEC (Mythic+ Spectator)
 * - UPG  (Item Upgrade)
 * - HLBG (Hinterland BG)
 * - DUEL (Phased Duels)
 * - MPLUS (Mythic+ general)
 * - PRES (Prestige system)
 * - SEAS (Seasonal system)
 *
 * Copyright (C) 2024 Dark Chaos Development Team
 */

#ifndef DC_ADDON_NAMESPACE_H
#define DC_ADDON_NAMESPACE_H

// Core headers expected to be provided by PCH or including file
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cctype>
#include <map>

namespace DCAddon
{
    // ========================================================================
    // CONSTANTS
    // ========================================================================

    // The unified DC addon prefix - ALL DC messages use this
    constexpr const char* DC_PREFIX = "DC";

    // Message delimiter
    constexpr char DELIMITER = '|';

    // WoW 3.3.5a message limits
    constexpr uint32 MAX_CLIENT_MSG_SIZE = 255;
    constexpr uint32 MAX_SERVER_MSG_SIZE = 2560;

    // Module identifiers (first field after prefix)
    namespace Module
    {
        constexpr const char* AOE_LOOT      = "AOE";
    }

    // ========================================================================
    // OPCODES BY MODULE
    // ========================================================================

    namespace Opcode
    {
        // Core/Handshake opcodes
        namespace Core
        {
            constexpr uint8 CMSG_HANDSHAKE         = 0x01;  // Client says hello
            constexpr uint8 CMSG_VERSION_CHECK     = 0x02;  // Client sends version
            constexpr uint8 CMSG_FEATURE_QUERY     = 0x03;  // Client asks what's enabled

            constexpr uint8 SMSG_HANDSHAKE_ACK     = 0x10;  // Server acknowledges
            constexpr uint8 SMSG_VERSION_RESULT    = 0x11;  // Server version check result
            constexpr uint8 SMSG_FEATURE_LIST      = 0x12;  // Server sends enabled features
            constexpr uint8 SMSG_RELOAD_UI         = 0x13;  // Server tells client to reload
            constexpr uint8 SMSG_SERVER_CONTEXT    = 0x14;  // Season/phase server context
            constexpr uint8 SMSG_CROSS_EVENT       = 0x15;  // CrossSystem event broadcast
            constexpr uint8 SMSG_PERMISSION_DENIED = 0x1E;  // Permission denied specific
            constexpr uint8 SMSG_ERROR             = 0x1F;  // Error response (Generic)
        }

        // AOE Loot opcodes
        namespace AOE
        {
            constexpr uint8 CMSG_TOGGLE_ENABLED    = 0x01;
            constexpr uint8 CMSG_SET_QUALITY       = 0x02;
            constexpr uint8 CMSG_GET_STATS         = 0x03;
            constexpr uint8 CMSG_SET_AUTO_SKIN     = 0x04;
            constexpr uint8 CMSG_SET_RANGE         = 0x05;
            constexpr uint8 CMSG_GET_SETTINGS      = 0x06;
            constexpr uint8 CMSG_IGNORE_ITEM       = 0x07;
            constexpr uint8 CMSG_GET_QUALITY_STATS = 0x08;  // Request quality breakdown

            constexpr uint8 SMSG_STATS             = 0x10;
            constexpr uint8 SMSG_SETTINGS_SYNC     = 0x11;
            constexpr uint8 SMSG_LOOT_RESULT       = 0x12;
            constexpr uint8 SMSG_GOLD_COLLECTED    = 0x13;
            constexpr uint8 SMSG_QUALITY_STATS     = 0x14;  // Quality breakdown response
        }
    }

    // Standard addon error codes
    namespace ErrorCode
    {
        constexpr uint32 PERMISSION_DENIED = 1;
        constexpr uint32 MODULE_DISABLED   = 2;
        constexpr uint32 BAD_FORMAT        = 3;
        constexpr uint32 VERSION_MISMATCH  = 4;
        constexpr uint32 CAP_NOT_SUPPORTED = 5;
        constexpr uint32 UNKNOWN          = 255;
    }

    // ========================================================================
    // PROTOCOL VERSIONING & CAPABILITY NEGOTIATION
    // ========================================================================

    namespace ProtocolVersion
    {
        // Semantic version components
        constexpr uint8 MAJOR = 2;       // Breaking changes
        constexpr uint8 MINOR = 0;       // New features (backwards compatible)
        constexpr uint8 PATCH = 0;       // Bug fixes

        // Combined version for comparison
        constexpr uint32 VERSION = (MAJOR << 16) | (MINOR << 8) | PATCH;

        // Capability flags - bitfield for feature negotiation
        namespace Capability
        {
            constexpr uint32 NONE           = 0x00000000;
            constexpr uint32 JSON_MESSAGES  = 0x00000001;  // JSON payload support
            constexpr uint32 BATCH_MESSAGES = 0x00000002;  // Batch message support
            constexpr uint32 COMPRESSION    = 0x00000004;  // zlib compression
            constexpr uint32 BINARY_PROTO   = 0x00000008;  // Binary protocol option
            constexpr uint32 ASYNC_QUERIES  = 0x00000010;  // Async DB query responses
            constexpr uint32 DELTA_SYNC     = 0x00000020;  // Delta sync for collections
            constexpr uint32 HOT_RELOAD     = 0x00000040;  // Module hot-reload support

            // Default capabilities for current server version
            constexpr uint32 SERVER_DEFAULT = JSON_MESSAGES | BATCH_MESSAGES;
        }

        // Version info structure for handshake
        struct VersionInfo
        {
            uint8 major;
            uint8 minor;
            uint8 patch;
            uint32 capabilities;

            uint32 GetVersion() const { return (major << 16) | (minor << 8) | patch; }

            bool IsCompatible(const VersionInfo& other) const
            {
                // Major version must match, minor can be >=
                return (major == other.major);
            }

            bool HasCapability(uint32 cap) const { return (capabilities & cap) != 0; }
        };

        // Get server version info
        inline VersionInfo GetServerVersion()
        {
            return { MAJOR, MINOR, PATCH, Capability::SERVER_DEFAULT };
        }

        // Parse client version string "MAJOR.MINOR.PATCH" or "MAJOR.MINOR.PATCH|CAPS"
        inline VersionInfo ParseClientVersion(const std::string& versionStr)
        {
            VersionInfo info = { 0, 0, 0, 0 };
            size_t pipePos = versionStr.find('|');
            std::string version = (pipePos != std::string::npos)
                                  ? versionStr.substr(0, pipePos)
                                  : versionStr;

            // Parse "MAJOR.MINOR.PATCH"
            sscanf(version.c_str(), "%hhu.%hhu.%hhu", &info.major, &info.minor, &info.patch);

            // Parse capabilities if present
            if (pipePos != std::string::npos)
            {
                try {
                    info.capabilities = std::stoul(versionStr.substr(pipePos + 1));
                } catch (...) {
                    info.capabilities = 0;
                }
            }

            return info;
        }

        // Build version string for client
        inline std::string BuildVersionString(const VersionInfo& info)
        {
            return std::to_string(info.major) + "." +
                   std::to_string(info.minor) + "." +
                   std::to_string(info.patch) + "|" +
                   std::to_string(info.capabilities);
        }
    }

    inline bool IsSafeRequestId(const std::string& id)
    {
        if (id.empty() || id.size() > 64)
            return false;
        for (unsigned char c : id)
        {
            if (!(std::isalnum(c) || c == '-' || c == '_' || c == ':' || c == '.'))
                return false;
        }
        return true;
    }

    // ========================================================================
    // ParsedMessage - Core parser for incoming DC addon messages
    // ========================================================================

    class ParsedMessage
    {
    public:
        ParsedMessage(const std::string& raw)
        {
            Parse(raw);
        }

        bool IsValid() const { return _valid; }
        const std::string& GetModule() const { return _module; }
        uint8 GetOpcode() const { return _opcode; }
        size_t GetDataCount() const { return _data.size(); }
        bool HasRequestId() const { return !_requestId.empty(); }
        const std::string& GetRequestId() const { return _requestId; }
        bool HasMore() const { return _currentIndex < _data.size(); }

        // Get data at index with type conversion
        std::string GetString(size_t index) const
        {
            return (index < _data.size()) ? _data[index] : "";
        }

        // Sequential read methods for parser-style access
        std::string NextString()
        {
            return HasMore() ? _data[_currentIndex++] : "";
        }

        int32 GetInt32(size_t index) const
        {
            try {
                return (index < _data.size()) ? std::stoi(_data[index]) : 0;
            } catch (...) {
                return 0;
            }
        }

        uint32 GetUInt32(size_t index) const
        {
            try {
                return (index < _data.size()) ? static_cast<uint32>(std::stoul(_data[index])) : 0;
            } catch (...) {
                return 0;
            }
        }

        float GetFloat(size_t index) const
        {
            try {
                return (index < _data.size()) ? std::stof(_data[index]) : 0.0f;
            } catch (...) {
                return 0.0f;
            }
        }

        bool GetBool(size_t index) const
        {
            return GetString(index) == "1";
        }

        uint64 GetUInt64(size_t index) const
        {
            try {
                return (index < _data.size()) ? std::stoull(_data[index]) : 0;
            } catch (...) {
                return 0;
            }
        }

    private:
        void Parse(const std::string& raw)
        {
            std::stringstream ss(raw);
            std::string token;
            std::vector<std::string> tokens;

            while (std::getline(ss, token, DELIMITER))
            {
                tokens.push_back(token);
            }

            if (tokens.size() < 2)
            {
                _valid = false;
                return;
            }

            _module = tokens[0];
            if (_module.empty() || _module.size() > 8)
            {
                _valid = false;
                return;
            }

            for (unsigned char c : _module)
            {
                if (!(std::isalnum(c) || c == '_'))
                {
                    _valid = false;
                    return;
                }
            }

            try {
                int32 parsedOpcode = std::stoi(tokens[1]);
                if (parsedOpcode < 0 || parsedOpcode > 255)
                {
                    _valid = false;
                    return;
                }

                _opcode = static_cast<uint8>(parsedOpcode);
            } catch (...) {
                _valid = false;
                return;
            }

            // Optional request ID token (RID:<id>) as first data field
            size_t dataStart = 2;
            if (tokens.size() > 2)
            {
                const std::string& maybeRid = tokens[2];
                if (maybeRid.rfind("RID:", 0) == 0 || maybeRid.rfind("RID=", 0) == 0)
                {
                    _requestId = maybeRid.substr(4);
                    if (!IsSafeRequestId(_requestId))
                        _requestId.clear();
                    dataStart = 3;
                }
            }

            // Remaining tokens are data
            for (size_t i = dataStart; i < tokens.size(); ++i)
            {
                _data.push_back(tokens[i]);
            }

            _valid = true;
        }

        bool _valid = false;
        std::string _module;
        uint8 _opcode = 0;
        std::vector<std::string> _data;
        mutable size_t _currentIndex = 0;
        std::string _requestId;
    };

    // Parser - alias for ParsedMessage with sequential read support
    class Parser
    {
    public:
        Parser(const ParsedMessage& msg) : _msg(msg), _index(0) {}

        uint8 GetOpcode() const { return _msg.GetOpcode(); }
        bool HasMore() const { return _index < _msg.GetDataCount(); }

        std::string GetString() { return _msg.GetString(_index++); }
        int32 GetInt32() { return _msg.GetInt32(_index++); }
        uint32 GetUInt32() { return _msg.GetUInt32(_index++); }
        float GetFloat() { return _msg.GetFloat(_index++); }
        bool GetBool() { return _msg.GetBool(_index++); }
        uint64 GetUInt64() { return _msg.GetUInt64(_index++); }

        // Peek at next without consuming
        std::string PeekString() const { return HasMore() ? _msg.GetString(_index) : ""; }

        // Skip N fields
        void Skip(size_t count = 1) { _index += count; }

        // Reset to beginning of data
        void Reset() { _index = 0; }

    private:
        const ParsedMessage& _msg;
        size_t _index;
    };

    // ========================================================================
    // BATCH MESSAGE SUPPORT (DC|BATCH|count|MOD1|op|data|MOD2|op|data|...)
    // ========================================================================

    namespace Batch
    {
        constexpr const char* MODULE = "BATCH";
        constexpr size_t MAX_MESSAGES_PER_BATCH = 10;

        // Batch message is parsed as: BATCH|count|MOD|op|...|MOD|op|...
        struct BatchEntry
        {
            std::string module;
            uint8 opcode;
            std::vector<std::string> data;
        };

        // Parse a batch message into individual entries
        // Format: BATCH|count|MOD|op|d1|d2|...|MOD|op|d1|...
        // Each sub-message ends when next MOD is found or end of data
        inline std::vector<BatchEntry> ParseBatch(const ParsedMessage& msg)
        {
            std::vector<BatchEntry> entries;

            // First data field is the count
            if (msg.GetDataCount() < 1)
                return entries;

            uint32 declaredCount = msg.GetUInt32(0);
            if (declaredCount == 0 || declaredCount > MAX_MESSAGES_PER_BATCH)
                return entries;

            // Parse remaining fields as sub-messages
            size_t idx = 1;  // Start after count
            while (idx < msg.GetDataCount() && entries.size() < declaredCount)
            {
                BatchEntry entry;

                // Module
                if (idx >= msg.GetDataCount()) break;
                entry.module = msg.GetString(idx++);

                // Opcode
                if (idx >= msg.GetDataCount()) break;
                entry.opcode = static_cast<uint8>(msg.GetUInt32(idx++));

                // Data fields until next module keyword or end
                // We detect a new sub-message when we see a known module ID
                while (idx < msg.GetDataCount())
                {
                    std::string val = msg.GetString(idx);
                    // Check if this looks like a module identifier (3-5 uppercase chars)
                    bool isModule = (val.length() >= 3 && val.length() <= 5);
                    if (isModule)
                    {
                        bool allUpper = true;
                        for (char c : val) if (!isupper(c)) { allUpper = false; break; }
                        if (allUpper && entries.size() < declaredCount - 1)
                            break;  // Start of next sub-message
                    }
                    entry.data.push_back(val);
                    idx++;
                }

                entries.push_back(entry);
            }

            return entries;
        }
    }

    // ========================================================================
    // MESSAGE UTILITIES
    // ========================================================================

    class Message
    {
    public:
        Message() = default;
        Message(const std::string& module, uint8 opcode)
            : _module(module), _opcode(opcode) {}

        Message& SetRequestId(const std::string& requestId)
        {
            _requestId = IsSafeRequestId(requestId) ? requestId : std::string();
            return *this;
        }

        // Build message for sending
        Message& Add(const std::string& value)
        {
            _data.push_back(value);
            return *this;
        }

        Message& Add(int32 value)
        {
            _data.push_back(std::to_string(value));
            return *this;
        }

        Message& Add(uint32 value)
        {
            _data.push_back(std::to_string(value));
            return *this;
        }

        Message& Add(float value)
        {
            _data.push_back(std::to_string(value));
            return *this;
        }

        Message& Add(bool value)
        {
            _data.push_back(value ? "1" : "0");
            return *this;
        }

        Message& Add(ObjectGuid guid)
        {
            _data.push_back(std::to_string(guid.GetRawValue()));
            return *this;
        }

        // Build final message string
        std::string Build() const
        {
            std::string result = _module;
            result += DELIMITER;
            result += std::to_string(_opcode);

            if (!_requestId.empty())
            {
                result += DELIMITER;
                result += "RID:";
                result += _requestId;
            }

            for (auto const& d : _data)
            {
                result += DELIMITER;
                result += d;
            }

            return result;
        }

        // Send to player
        void Send(Player* player) const;

        // Alias for Send (convenience method)
        void SendTo(Player* player) const { Send(player); }

        // Send to multiple players
        void SendToList(const std::vector<Player*>& players) const
        {
            for (Player* p : players)
            {
                if (p)
                    Send(p);
            }
        }

    private:
        std::string _module;
        uint8 _opcode;
        std::vector<std::string> _data;
        std::string _requestId;
    };

    // Forward declarations for helpers used by MessageRouter::Route
    inline void SendError(Player* player, const std::string& module, const std::string& errorMsg, uint32 errorCode, uint8 opcode);
    inline void SendPermissionDenied(Player* player, const std::string& module, const std::string& errorMsg);

    // Request context helpers (defined in dc_addon_protocol.cpp)
    void SetCurrentRequestContext(const std::string& requestId);
    void ClearCurrentRequestContext();
    const std::string& GetCurrentRequestId();
    void NotifyResponseSent(Player* player, const std::string& requestId);

    // Quick permission helper: ensure module enabled and player has minimum security
    // (Moved below MessageRouter declaration to avoid forward-declare/ordering issues)

    // ========================================================================
    // MESSAGE HANDLER REGISTRATION
    // ========================================================================

    using MessageHandler = std::function<void(Player*, const ParsedMessage&)>;

    class MessageRouter
    {
    public:
        static MessageRouter& Instance()
        {
            static MessageRouter instance;
            return instance;
        }

        // Register a handler for a module + opcode combination
        void RegisterHandler(const std::string& module, uint8 opcode, MessageHandler handler)
        {
            std::string key = module + "_" + std::to_string(opcode);
            _handlers[key] = handler;
        }

        bool HasHandler(const std::string& module, uint8 opcode) const
        {
            std::string key = module + "_" + std::to_string(opcode);
            return _handlers.find(key) != _handlers.end();
        }

        // Route an incoming message to the appropriate handler
        bool Route(Player* player, const std::string& rawMessage)
        {
            ParsedMessage msg(rawMessage);
            if (!msg.IsValid())
                return false;

            std::string key = msg.GetModule() + "_" + std::to_string(msg.GetOpcode());

            // Debug: log all routed messages
            LOG_DEBUG("module.dc", "[MessageRouter] Route: player={}, module={}, opcode=0x{:02X}, key={}",
                player ? player->GetName() : "NULL", msg.GetModule(), msg.GetOpcode(), key);

            // If the module is disabled globally, send a structured addon error
            if (!IsModuleEnabled(msg.GetModule()))
            {
                LOG_DEBUG("module.dc", "[MessageRouter] Module '{}' is DISABLED, rejecting opcode {}", msg.GetModule(), msg.GetOpcode());
                if (player && player->GetSession())
                    SendError(player, msg.GetModule(), "Module is disabled on server", ErrorCode::MODULE_DISABLED, Opcode::Core::SMSG_ERROR);
                return false;
            }
            auto it = _handlers.find(key);

            LOG_DEBUG("module.dc", "[MessageRouter] Looking for handler key='{}', found={}", key, (it != _handlers.end()));

            if (it != _handlers.end())
            {
                // Set current request context so SendError can echo request ID
                struct RequestContextScope
                {
                    RequestContextScope(const std::string& reqId) { DCAddon::SetCurrentRequestContext(reqId); }
                    ~RequestContextScope() { DCAddon::ClearCurrentRequestContext(); }
                } scope(msg.GetRequestId());

                // Check module-wise min security if configured
                uint32_t minSec = 0;
                auto minIt = _moduleMinSecurity.find(msg.GetModule());
                if (minIt != _moduleMinSecurity.end())
                    minSec = minIt->second;

                if (player && player->GetSession() && player->GetSession()->GetSecurity() < minSec)
                {
                    // Inform the player they lack sufficient permission via structured addon error
                    DCAddon::SendPermissionDenied(player, msg.GetModule(), "Insufficient GM level to execute addon commands for this module");
                    return false;
                }

                try
                {
                    it->second(player, msg);
                }
                catch (...)
                {
                    LOG_ERROR(
                        "module.dc",
                        "[MessageRouter] Handler exception: player={}, module={}, opcode=0x{:02X}",
                        player ? player->GetName() : "NULL",
                        msg.GetModule(),
                        msg.GetOpcode());

                    if (player && player->GetSession())
                    {
                        SendError(
                            player,
                            msg.GetModule(),
                            "Internal handler error",
                            ErrorCode::UNKNOWN,
                            Opcode::Core::SMSG_ERROR);
                    }

                    return false;
                }

                return true;
            }

            LOG_DEBUG("module.dc", "[MessageRouter] No handler found for key='{}', returning false", key);
            return false;  // No handler registered
        }

        // Check if a module is enabled
        bool IsModuleEnabled(const std::string& module) const
        {
            auto it = _enabledModules.find(module);
            return (it != _enabledModules.end()) ? it->second : false;
        }

        void SetModuleEnabled(const std::string& module, bool enabled)
        {
            _enabledModules[module] = enabled;
        }

        void SetModuleMinSecurity(const std::string& module, uint32 minSecurity)
        {
            _moduleMinSecurity[module] = minSecurity;
        }

    private:
        MessageRouter() = default;
        std::unordered_map<std::string, MessageHandler> _handlers;
        std::unordered_map<std::string, bool> _enabledModules;
        std::unordered_map<std::string, uint32_t> _moduleMinSecurity;
    };

    // Quick permission helper: ensure module enabled and player has minimum security
    inline bool CheckAddonPermission(Player* player, const std::string& module, uint32 minSecurity = SEC_MODERATOR)
    {
        if (!MessageRouter::Instance().IsModuleEnabled(module))
            return false;
        if (!player || !player->GetSession())
            return false;
        return (player->GetSession()->GetSecurity() >= minSecurity);
    }

    // Send a standard error response via addon protocol for module
    inline void SendError(Player* player, const std::string& module, const std::string& errorMsg, uint32 errorCode = 1, uint8 opcode = Opcode::Core::SMSG_ERROR)
    {
        if (!player || !player->GetSession())
            return;
        Message errorMsgObj(module, opcode);
        const std::string& reqId = GetCurrentRequestId();
        if (!reqId.empty())
            errorMsgObj.SetRequestId(reqId);
        errorMsgObj.Add(std::to_string(errorCode));
        errorMsgObj.Add(errorMsg);
        errorMsgObj.Send(player);
    }

    inline void SendPermissionDenied(Player* player, const std::string& module, const std::string& errorMsg = "Permission denied")
    {
        SendError(player, module, errorMsg, ErrorCode::PERMISSION_DENIED, Opcode::Core::SMSG_PERMISSION_DENIED);
    }

    // ========================================================================
    // HELPER MACROS FOR HANDLER REGISTRATION
    // ========================================================================

    #define DC_REGISTER_HANDLER(module, opcode, handler) \
        DCAddon::MessageRouter::Instance().RegisterHandler(module, opcode, handler)

    #define DC_SEND_MESSAGE(player, module, opcode) \
        DCAddon::Message(module, opcode)

    // ========================================================================
    // CHUNKING SUPPORT (for messages > 255 bytes)
    // ========================================================================

    class ChunkedMessage
    {
    public:
        // Split a large message into chunks
        static std::vector<std::string> Chunk(const std::string& message, uint32 maxSize = MAX_CLIENT_MSG_SIZE - 10)
        {
            std::vector<std::string> chunks;

            if (message.size() <= maxSize)
            {
                // No chunking needed, but mark as single chunk
                chunks.push_back("0|1|" + message);
                return chunks;
            }

            uint32 totalChunks = (message.size() + maxSize - 1) / maxSize;

            for (uint32 i = 0; i < totalChunks; ++i)
            {
                std::string chunk = std::to_string(i) + "|" + std::to_string(totalChunks) + "|";
                chunk += message.substr(i * maxSize, maxSize);
                chunks.push_back(chunk);
            }

            return chunks;
        }

        // Reassemble chunks (call per incoming chunk, returns complete message when done)
        bool AddChunk(const std::string& chunk)
        {
            // Parse chunk header: INDEX|TOTAL|DATA
            std::stringstream ss(chunk);
            std::string indexStr, totalStr;

            if (!std::getline(ss, indexStr, '|') || !std::getline(ss, totalStr, '|'))
                return false;

            uint32 index = 0;
            uint32 total = 0;
            try
            {
                index = static_cast<uint32>(std::stoul(indexStr));
                total = static_cast<uint32>(std::stoul(totalStr));
            }
            catch (...)
            {
                return false;
            }

            if (total == 0 || index >= total)
                return false;

            if (_totalChunks == 0)
            {
                _totalChunks = total;
                _chunks.resize(total);
                _received.resize(total, false);
            }

            if (index >= _totalChunks || total != _totalChunks)
                return false;

            // Get remaining data after second |
            std::string data;
            std::getline(ss, data);

            _chunks[index] = data;
            if (!_received[index])
            {
                _received[index] = true;
                _receivedCount++;
            }

            return _receivedCount == _totalChunks;
        }

        std::string GetCompleteMessage() const
        {
            std::string result;
            for (auto const& chunk : _chunks)
                result += chunk;
            return result;
        }

        bool IsComplete() const { return _receivedCount == _totalChunks; }
        void Reset()
        {
            _chunks.clear();
            _received.clear();
            _totalChunks = 0;
            _receivedCount = 0;
        }

    private:
        std::vector<std::string> _chunks;
        std::vector<bool> _received;
        uint32 _totalChunks = 0;
        uint32 _receivedCount = 0;
    };

    // ========================================================================
    // JSON SUPPORT
    // ========================================================================

    // JSON marker for detecting JSON payloads
    constexpr const char* JSON_MARKER = "J";

    // Simple JSON value class for addon communication
    class JsonValue
    {
    public:
        enum Type { Null, Bool, Number, String, Array, Object };

        JsonValue() : _type(Null) {}
        JsonValue(bool v) : _type(Bool), _bool(v) {}
        JsonValue(int32 v) : _type(Number), _number(static_cast<double>(v)) {}
        JsonValue(uint32 v) : _type(Number), _number(static_cast<double>(v)) {}
        JsonValue(double v) : _type(Number), _number(v) {}
        JsonValue(const std::string& v) : _type(String), _string(v) {}
        JsonValue(const char* v) : _type(String), _string(v) {}

        Type GetType() const { return _type; }
        bool IsNull() const { return _type == Null; }
        bool IsBool() const { return _type == Bool; }
        bool IsNumber() const { return _type == Number; }
        bool IsString() const { return _type == String; }
        bool IsArray() const { return _type == Array; }
        bool IsObject() const { return _type == Object; }

        bool AsBool() const { return _bool; }
        double AsNumber() const { return _number; }
        int32 AsInt32() const { return static_cast<int32>(_number); }
        uint32 AsUInt32() const { return static_cast<uint32>(_number); }
        const std::string& AsString() const { return _string; }
        const std::vector<JsonValue>& AsArray() const { return _array; }
        const std::map<std::string, JsonValue>& AsObject() const { return _object; }

        // Object access
        bool HasKey(const std::string& key) const
        {
            return _type == Object && _object.find(key) != _object.end();
        }

        const JsonValue& operator[](const std::string& key) const
        {
            static JsonValue null;
            if (_type != Object) return null;
            auto it = _object.find(key);
            return (it != _object.end()) ? it->second : null;
        }

        // Array access
        const JsonValue& operator[](size_t index) const
        {
            static JsonValue null;
            return (_type == Array && index < _array.size()) ? _array[index] : null;
        }

        size_t Size() const
        {
            if (_type == Array) return _array.size();
            if (_type == Object) return _object.size();
            return 0;
        }

        // Building JSON
        void SetNull() { _type = Null; }
        void Set(bool v) { _type = Bool; _bool = v; }
        void Set(double v) { _type = Number; _number = v; }
        void Set(const std::string& v) { _type = String; _string = v; }

        void SetArray() { _type = Array; _array.clear(); }
        void Push(const JsonValue& v) { if (_type == Array) _array.push_back(v); }

        void SetObject() { _type = Object; _object.clear(); }
        void Set(const std::string& key, const JsonValue& v)
        {
            if (_type == Object) _object[key] = v;
        }

        // Encode to JSON string
        std::string Encode() const
        {
            switch (_type)
            {
                case Null: return "null";
                case Bool: return _bool ? "true" : "false";
                case Number: {
                    // IMPORTANT: Preserve integer fidelity for large IDs (e.g. spawnId ~= 9,000,000).
                    // Default iostream precision (6) would round 9000189/9000191 to 9.00019e+06.
                    if (std::isfinite(_number))
                    {
                        double intPart = 0.0;
                        if (std::modf(_number, &intPart) == 0.0)
                        {
                            // Integer: emit without scientific notation.
                            std::ostringstream ss;
                            ss.setf(std::ios::fmtflags(0), std::ios::floatfield);
                            ss << static_cast<long long>(intPart);
                            return ss.str();
                        }
                    }

                    // Non-integer: emit with enough precision to round-trip.
                    std::ostringstream ss;
                    ss << std::setprecision(15) << _number;
                    return ss.str();
                }
                case String: {
                    std::string result = "\"";
                    for (char c : _string) {
                        if (c == '"') result += "\\\"";
                        else if (c == '\\') result += "\\\\";
                        else if (c == '\n') result += "\\n";
                        else if (c == '\r') result += "\\r";
                        else if (c == '\t') result += "\\t";
                        else result += c;
                    }
                    result += "\"";
                    return result;
                }
                case Array: {
                    std::string result = "[";
                    for (size_t i = 0; i < _array.size(); ++i) {
                        if (i > 0) result += ",";
                        result += _array[i].Encode();
                    }
                    result += "]";
                    return result;
                }
                case Object: {
                    std::string result = "{";
                    bool first = true;
                    for (auto const& [k, v] : _object)
                    {
                        if (!first) result += ",";
                        first = false;
                        result += "\"" + k + "\":" + v.Encode();
                    }
                    result += "}";
                    return result;
                }
            }
            return "null";
        }

    private:
        Type _type = Null;
        bool _bool = false;
        double _number = 0.0;
        std::string _string;
        std::vector<JsonValue> _array;
        std::map<std::string, JsonValue> _object;
    };

    // Simple JSON parser
    class JsonParser
    {
    public:
        static JsonValue Parse(const std::string& json)
        {
            size_t pos = 0;
            return ParseValue(json, pos);
        }

    private:
        static void SkipWhitespace(const std::string& s, size_t& pos)
        {
            while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r'))
                ++pos;
        }

        static JsonValue ParseValue(const std::string& s, size_t& pos)
        {
            SkipWhitespace(s, pos);
            if (pos >= s.size()) return JsonValue();

            char c = s[pos];
            if (c == '"') return ParseString(s, pos);
            if (c == '{') return ParseObject(s, pos);
            if (c == '[') return ParseArray(s, pos);
            if (c == 't' && s.substr(pos, 4) == "true") { pos += 4; return JsonValue(true); }
            if (c == 'f' && s.substr(pos, 5) == "false") { pos += 5; return JsonValue(false); }
            if (c == 'n' && s.substr(pos, 4) == "null") { pos += 4; return JsonValue(); }
            if (c == '-' || (c >= '0' && c <= '9')) return ParseNumber(s, pos);

            return JsonValue();
        }

        static JsonValue ParseString(const std::string& s, size_t& pos)
        {
            if (s[pos] != '"') return JsonValue();
            ++pos;
            std::string result;
            while (pos < s.size() && s[pos] != '"')
            {
                if (s[pos] == '\\' && pos + 1 < s.size())
                {
                    ++pos;
                    char esc = s[pos];
                    if (esc == '"') result += '"';
                    else if (esc == '\\') result += '\\';
                    else if (esc == 'n') result += '\n';
                    else if (esc == 'r') result += '\r';
                    else if (esc == 't') result += '\t';
                    else if (esc == 'u') { pos += 4; result += '?'; }  // Skip unicode
                    ++pos;
                }
                else
                {
                    result += s[pos++];
                }
            }
            if (pos < s.size()) ++pos;  // Skip closing "
            return JsonValue(result);
        }

        static JsonValue ParseNumber(const std::string& s, size_t& pos)
        {
            size_t start = pos;
            if (s[pos] == '-') ++pos;
            while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
            if (pos < s.size() && s[pos] == '.')
            {
                ++pos;
                while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
            }
            if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E'))
            {
                ++pos;
                if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) ++pos;
                while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
            }
            return JsonValue(std::stod(s.substr(start, pos - start)));
        }

        static JsonValue ParseArray(const std::string& s, size_t& pos)
        {
            if (s[pos] != '[') return JsonValue();
            ++pos;
            JsonValue arr;
            arr.SetArray();
            SkipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ']') { ++pos; return arr; }
            while (pos < s.size()) {
                arr.Push(ParseValue(s, pos));
                SkipWhitespace(s, pos);
                if (pos >= s.size()) break;
                if (s[pos] == ']') { ++pos; break; }
                if (s[pos] == ',') ++pos;
            }
            return arr;
        }

        static JsonValue ParseObject(const std::string& s, size_t& pos)
        {
            if (s[pos] != '{') return JsonValue();
            ++pos;
            JsonValue obj;
            obj.SetObject();
            SkipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == '}') { ++pos; return obj; }
            while (pos < s.size()) {
                SkipWhitespace(s, pos);
                JsonValue keyVal = ParseString(s, pos);
                if (!keyVal.IsString()) break;
                SkipWhitespace(s, pos);
                if (pos >= s.size() || s[pos] != ':') break;
                ++pos;
                obj.Set(keyVal.AsString(), ParseValue(s, pos));
                SkipWhitespace(s, pos);
                if (pos >= s.size()) break;
                if (s[pos] == '}') { ++pos; break; }
                if (s[pos] == ',') ++pos;
            }
            return obj;
        }
    };

    // JSON Message builder
    class JsonMessage
    {
    public:
        JsonMessage(const std::string& module, uint8 opcode, const JsonValue& json)
            : _module(module), _opcode(opcode), _json(json) {}

        JsonMessage(const std::string& module, uint8 opcode)
            : _module(module), _opcode(opcode)
        {
            _json.SetObject();
        }

        JsonMessage& SetRequestId(const std::string& requestId)
        {
            _requestId = IsSafeRequestId(requestId) ? requestId : std::string();
            return *this;
        }

        JsonMessage& Set(const std::string& key, bool v) { _json.Set(key, JsonValue(v)); return *this; }
        JsonMessage& Set(const std::string& key, int32 v) { _json.Set(key, JsonValue(v)); return *this; }
        JsonMessage& Set(const std::string& key, uint32 v) { _json.Set(key, JsonValue(v)); return *this; }
        JsonMessage& Set(const std::string& key, double v) { _json.Set(key, JsonValue(v)); return *this; }
        JsonMessage& Set(const std::string& key, const std::string& v) { _json.Set(key, JsonValue(v)); return *this; }
        JsonMessage& Set(const std::string& key, const char* v) { _json.Set(key, JsonValue(v)); return *this; }
        JsonMessage& Set(const std::string& key, const JsonValue& v) { _json.Set(key, v); return *this; }

        std::string Build() const
        {
            std::string result = _module;
            result += DELIMITER;
            result += std::to_string(_opcode);
            result += DELIMITER;
            if (!_requestId.empty())
            {
                result += "RID:";
                result += _requestId;
                result += DELIMITER;
            }
            result += JSON_MARKER;
            result += DELIMITER;
            result += _json.Encode();
            return result;
        }

        void Send(Player* player) const
        {
            if (!player || !player->GetSession())
                return;

            std::string effectiveRequestId = _requestId;
            if (effectiveRequestId.empty())
            {
                const std::string& ctxReqId = GetCurrentRequestId();
                if (IsSafeRequestId(ctxReqId))
                    effectiveRequestId = ctxReqId;
            }

            std::string payload;
            if (!effectiveRequestId.empty() && effectiveRequestId != _requestId)
            {
                payload = _module;
                payload += DELIMITER;
                payload += std::to_string(_opcode);
                payload += DELIMITER;
                payload += "RID:";
                payload += effectiveRequestId;
                payload += DELIMITER;
                payload += JSON_MARKER;
                payload += DELIMITER;
                payload += _json.Encode();
            }
            else
            {
                payload = Build();
            }

            // If the payload is large, split it into chunk frames.
            // Client-side DCAddonProtocol reassembles INDEX|TOTAL|DATA before parsing.
            if (payload.length() > MAX_CLIENT_MSG_SIZE - 10)
            {
                auto chunks = ChunkedMessage::Chunk(payload);
                for (auto const& chunk : chunks)
                {
                    std::string fullMsg = std::string(DC_PREFIX) + "\t" + chunk;
                    WorldPacket data;
                    ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, fullMsg);
                    player->SendDirectMessage(&data);
                }
                if (!effectiveRequestId.empty())
                    NotifyResponseSent(player, effectiveRequestId);
                return;
            }

            // Build the full message with DC prefix and tab separator
            std::string fullMsg = std::string(DC_PREFIX) + "\t" + payload;

            // Use proper ChatHandler to build addon message packet
            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, fullMsg);
            player->SendDirectMessage(&data);

            if (!effectiveRequestId.empty())
                NotifyResponseSent(player, effectiveRequestId);
        }

    private:
        std::string _module;
        uint8 _opcode;
        JsonValue _json;
        std::string _requestId;
    };

    // Check if a parsed message contains JSON
    inline bool IsJsonMessage(const ParsedMessage& msg)
    {
        return msg.GetDataCount() > 0 && msg.GetString(0) == JSON_MARKER;
    }

    // Get JSON data from a message (returns empty JsonValue if not JSON)
    inline JsonValue GetJsonData(const ParsedMessage& msg)
    {
        if (!IsJsonMessage(msg) || msg.GetDataCount() < 2)
            return JsonValue();

        return JsonParser::Parse(msg.GetString(1));
    }

}  // namespace DCAddon

#endif // DC_ADDON_NAMESPACE_H

#pragma once

// =============================================================================
// FiveInvader - GTA V Script Native Engine Dispatcher Template
// Target: GTA V Xbox 360 (RAGE scrThread / scrProgram Native Interception)
// =============================================================================
//
// In the GTA V RAGE engine, gameplay scripts (.csc / .ysc) invoke engine-level
// services through a 64-bit (or 32-bit on Xbox 360) native hash table.
// When a script instruction like `native` or `callnative` executes:
//   1. The VM pushes arguments onto the script stack.
//   2. An `scrNativeCallContext` is populated pointing to the arguments and
//      return buffer.
//   3. The engine looks up the native function pointer via hash.
//
// If a native is unregistered or unhandled, FiveInvader's dispatcher intercepts
// the call via FallbackHandler, zero-initializes the return buffer, logs the
// missing hash, and returns execution cleanly without crashing the runtime.
// =============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <string_view>
#include <unordered_map>

// Try to include project logger if available in include path
#if __has_include("os/logger.h")
    #include "os/logger.h"
    #define GTA5_NATIVE_LOG_WARN(fmt_str, ...) LOGF_WARNING(fmt_str, ##__VA_ARGS__)
    #define GTA5_NATIVE_LOG_INFO(fmt_str, ...) LOGF_INFO(fmt_str, ##__VA_ARGS__)
#else
    #define GTA5_NATIVE_LOG_WARN(fmt_str, ...) \
        std::fprintf(stderr, "[FiveInvader:Native:WARN] " fmt_str "\n", ##__VA_ARGS__)
    #define GTA5_NATIVE_LOG_INFO(fmt_str, ...) \
        std::printf("[FiveInvader:Native:INFO] " fmt_str "\n", ##__VA_ARGS__)
#endif

namespace rage
{
    // =========================================================================
    // scrNativeCallContext
    // Matches the Xbox 360 RAGE memory layout for native invocations.
    // =========================================================================
    #pragma pack(push, 4)
    struct scrNativeCallContext
    {
        void*     m_pReturn;       // Pointer to return buffer on script stack
        uint32_t  m_nArgCount;     // Number of arguments passed to the native
        void*     m_pArgs;         // Pointer to arguments array (guest 32-bit/64-bit slots)
        uint32_t  m_nDataCount;    // Number of vector/out parameters to write back
        void*     m_pData[4];      // Storage for out vector parameters

        // Helper: retrieve argument by index (typed)
        template <typename T>
        inline T GetArgument(uint32_t index) const
        {
            if (!m_pArgs || index >= m_nArgCount)
            {
                return T{};
            }
            // Xbox 360 script stack stores 32-bit aligned slots (or 64-bit on newer builds)
            const uintptr_t* slot = reinterpret_cast<const uintptr_t*>(m_pArgs) + index;
            return *reinterpret_cast<const T*>(slot);
        }

        // Helper: set return value (typed)
        template <typename T>
        inline void SetResult(T value)
        {
            if (m_pReturn)
            {
                *reinterpret_cast<T*>(m_pReturn) = value;
            }
        }

        // Helper: set 64-bit or pointer return value
        inline void SetResultU64(uint64_t value)
        {
            if (m_pReturn)
            {
                *reinterpret_cast<uint64_t*>(m_pReturn) = value;
            }
        }

        // Helper: zero-initialize return slot
        inline void ClearResult()
        {
            if (m_pReturn)
            {
                std::memset(m_pReturn, 0, sizeof(uint64_t));
            }
        }
    };
    #pragma pack(pop)

    // Function signature for a script native implementation
    using scrNativeHandler = void (*)(scrNativeCallContext* pContext);

} // namespace rage

namespace gta5::natives
{
    // Structure holding metadata for a registered native
    struct NativeRegistration
    {
        uint64_t hash;
        std::string_view name;
        rage::scrNativeHandler handler;
    };

    // =========================================================================
    // Native Dispatcher Class
    // =========================================================================
    class NativeDispatcher
    {
    public:
        using RegistryMap = std::unordered_map<uint64_t, NativeRegistration>;

        // Singleton registry access
        static RegistryMap& GetRegistry()
        {
            static RegistryMap s_registry;
            return s_registry;
        }

        // Register a native handler with associated hash
        static void Register(uint64_t hash, std::string_view name, rage::scrNativeHandler handler)
        {
            GetRegistry()[hash] = NativeRegistration{ hash, name, handler };
        }

        // Fallback handler for unmapped/unhandled hashes
        // Logs the missing hash and prevents crashes by returning 0
        static void FallbackHandler(uint64_t hash, rage::scrNativeCallContext* pContext)
        {
            pContext->ClearResult();
            static std::unordered_map<uint64_t, uint32_t> s_warnedHashes;
            if (s_warnedHashes[hash]++ == 0)
            {
                GTA5_NATIVE_LOG_WARN("Unhandled GTA V script native called: hash=0x%016llX (args=%u)",
                    static_cast<unsigned long long>(hash),
                    pContext->m_nArgCount);
            }
        }

        // Dispatch a native by hash
        static void Dispatch(uint64_t hash, rage::scrNativeCallContext* pContext)
        {
            auto& reg = GetRegistry();
            auto it = reg.find(hash);
            if (it != reg.end() && it->second.handler != nullptr)
            {
                it->second.handler(pContext);
            }
            else
            {
                FallbackHandler(hash, pContext);
            }
        }
    };

    // Auto-registration helper class
    struct AutoRegister
    {
        AutoRegister(uint64_t hash, std::string_view name, rage::scrNativeHandler handler)
        {
            NativeDispatcher::Register(hash, name, handler);
        }
    };

} // namespace gta5::natives

// =============================================================================
// Macros for Native Definition and Registration
// =============================================================================

// Define a native function implementation
// Example:
//   GTA5_NATIVE(Native_WAIT) {
//       uint32_t ms = ctx->GetArgument<uint32_t>(0);
//       // sleep or yield thread
//   }
#define GTA5_NATIVE(func_name) \
    void func_name(rage::scrNativeCallContext* ctx)

// Register a native handler with its hash
// Example:
//   REGISTER_NATIVE(0x4EDE34FB8897CD05, Native_WAIT);
#define REGISTER_NATIVE(hash_val, func_name) \
    static const ::gta5::natives::AutoRegister _reg_##func_name( \
        (hash_val), #func_name, (func_name))

// =============================================================================
// Core Boot Native Stubs (Commonly called during script initialization)
// =============================================================================

namespace gta5::natives::stubs
{
    // SYSTEM::WAIT(int ms)
    inline GTA5_NATIVE(SYSTEM_WAIT)
    {
        // No-op or yield thread in recomp VM
        ctx->ClearResult();
    }

    // SCRIPT::TERMINATE_THIS_THREAD()
    inline GTA5_NATIVE(SCRIPT_TERMINATE_THIS_THREAD)
    {
        ctx->ClearResult();
    }

    // PLAYER::PLAYER_PED_ID() -> Ped
    inline GTA5_NATIVE(PLAYER_PLAYER_PED_ID)
    {
        ctx->SetResult<int32_t>(1); // Default player ped handle
    }

    // PLAYER::PLAYER_ID() -> Player
    inline GTA5_NATIVE(PLAYER_PLAYER_ID)
    {
        ctx->SetResult<int32_t>(0); // Player 0
    }

    // PLAYER::IS_PLAYER_PLAYING(Player player) -> BOOL
    inline GTA5_NATIVE(PLAYER_IS_PLAYER_PLAYING)
    {
        ctx->SetResult<uint32_t>(1); // True
    }

    // MISC::GET_GAME_TIMER() -> int
    inline GTA5_NATIVE(MISC_GET_GAME_TIMER)
    {
        static uint32_t s_dummyTimer = 1000;
        ctx->SetResult<uint32_t>(s_dummyTimer += 16);
    }

    // NETWORK::NETWORK_IS_GAME_IN_PROGRESS() -> BOOL
    inline GTA5_NATIVE(NETWORK_NETWORK_IS_GAME_IN_PROGRESS)
    {
        ctx->SetResult<uint32_t>(0); // False during early single player boot
    }

    // Initialize initial native registration table
    inline void InitializeCoreNatives()
    {
        // SYSTEM::WAIT (0x4EDE34FB8897CD05)
        NativeDispatcher::Register(0x4EDE34FB8897CD05ULL, "SYSTEM_WAIT", SYSTEM_WAIT);
        // SCRIPT::TERMINATE_THIS_THREAD (0x1090044AD1DA7664)
        NativeDispatcher::Register(0x1090044AD1DA7664ULL, "SCRIPT_TERMINATE_THIS_THREAD", SCRIPT_TERMINATE_THIS_THREAD);
        // PLAYER::PLAYER_PED_ID (0xD80958FC74E988A4)
        NativeDispatcher::Register(0xD80958FC74E988A4ULL, "PLAYER_PLAYER_PED_ID", PLAYER_PLAYER_PED_ID);
        // PLAYER::PLAYER_ID (0x4F8644AF03D0E0D6)
        NativeDispatcher::Register(0x4F8644AF03D0E0D6ULL, "PLAYER_PLAYER_ID", PLAYER_PLAYER_ID);
        // PLAYER::IS_PLAYER_PLAYING (0x7480E83141E73482)
        NativeDispatcher::Register(0x7480E83141E73482ULL, "PLAYER_IS_PLAYER_PLAYING", PLAYER_IS_PLAYER_PLAYING);
        // MISC::GET_GAME_TIMER (0x9CD27B0045628463)
        NativeDispatcher::Register(0x9CD27B0045628463ULL, "MISC_GET_GAME_TIMER", MISC_GET_GAME_TIMER);
        // NETWORK::NETWORK_IS_GAME_IN_PROGRESS (0x10DC0736EEABE6E3)
        NativeDispatcher::Register(0x10DC0736EEABE6E3ULL, "NETWORK_NETWORK_IS_GAME_IN_PROGRESS", NETWORK_NETWORK_IS_GAME_IN_PROGRESS);
    }
} // namespace gta5::natives::stubs

// Philipp Neufeld, 2023

#ifndef CP_Util_UUID_H_
#define CP_Util_UUID_H_

#include <cstdint>
#include <string>

#if defined(CP_SSE2)
#include <xmmintrin.h> // SSE
#include <emmintrin.h> // SSE2
#endif

namespace CP
{

#if defined(CP_SSE2)
    using UUIDv4Register = __m128i;
#else
    struct UUIDv4Register
    {
        std::uint64_t lo;
        std::uint64_t hi;
    };
#endif

    class UUIDv4
    {
    private:
        UUIDv4(UUIDv4Register uuid);

    public:
        UUIDv4();

        UUIDv4(const UUIDv4& rhs) = default;
        UUIDv4& operator=(const UUIDv4& rhs) = default;

        bool operator==(const UUIDv4& rhs) const;
        bool operator!=(const UUIDv4& rhs) const;
        bool operator<(const UUIDv4& rhs) const;
        bool operator<=(const UUIDv4& rhs) const;
        bool operator>(const UUIDv4& rhs) const;
        bool operator>=(const UUIDv4& rhs) const;

        bool IsNullUUID() const;
        operator bool() const;

        std::size_t Hash() const;

        void ToString(char* buffer) const;
        void ToString(std::string& buffer) const;
        std::string ToString() const;

        static UUIDv4 FromString(const char* uuidStr);
        static UUIDv4 FromString(const std::string& uuidStr);

        void StoreToBufferLE(void* buffer) const;
        void StoreToBufferBE(void* buffer) const;

        static UUIDv4 LoadFromBufferLE(const void* buffer);
        static UUIDv4 LoadFromBufferBE(const void* buffer);

        static UUIDv4 NullUUID();

    private:
        static UUIDv4Register LoadFromBufferLEReg(const void* buffer);
        static UUIDv4Register LoadFromBufferBEReg(const void* buffer);
        static void StoreToBufferLEReg(UUIDv4Register uuid, void* buffer);
        static void StoreToBufferBEReg(UUIDv4Register uuid, void* buffer);

        UUIDv4Register Load() const;
        void Store(UUIDv4Register uuid);

        static UUIDv4Register Generate();
        static UUIDv4Register SwapEndianess(UUIDv4Register v);

#if defined(CP_SSE2)
        static __m128i PackUUID(__m128i hi, __m128i lo);
#endif

    private:
        std::uint8_t m_data[16];
    };

}

namespace std
{
    template<>
    struct hash<CP::UUIDv4>
    {
        std::size_t operator()(const CP::UUIDv4& uuid) const
        {
            return uuid.Hash();
        }
    };
}

#endif

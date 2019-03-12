#pragma once
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>

#ifndef UNCHECKED_BITSTREAM_READER
#define UNCHECKED_BITSTREAM_READER !CONFIG_SAFE_BITSTREAM_READER
#endif

#ifndef uint8_t
typedef unsigned char               uint8_t;
#endif

#define QP_MAX_NUM                  (51 + 6*6) 
#define EXTENDED_SAR                255
#define MAX_SPS_COUNT               32
#define MAX_PPS_COUNT               256
#define MIN_LOG2_MAX_FRAME_NUM      4
#define MAX_LOG2_MAX_FRAME_NUM      (12 + 4)
#define AV_INPUT_BUFFER_PADDING_SIZE 64
#define FFDIFFSIGN(x,y)             (((x)>(y)) - ((x)<(y)))
#define FFMAX(a,b)                  ((a) > (b) ? (a) : (b))
#define FFMAX3(a,b,c)               FFMAX(FFMAX(a,b),c)
#define FFMIN(a,b)                  ((a) > (b) ? (b) : (a))
#define FFMIN3(a,b,c)               FFMIN(FFMIN(a,b),c)
#define FFSWAP(type,a,b)            do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)
#define FF_ARRAY_ELEMS(a)           (sizeof(a) / sizeof((a)[0]))


#define INVALID_VLC   0x80000000

#define MAX_SPATIAL_SEGMENTATION   4096

// 短整型大小端互换
#define BigLittleSwap16(x)  ((((uint16_t)(x) & 0xff00) >> 8) | \
    (((uint16_t)(x) & 0x00ff) << 8))

// 长整型大小端互换
#define BigLittleSwap32(x)  ((((uint32_t)(x) & 0xff000000) >> 24) | \
    (((uint32_t)(x) & 0x00ff0000) >> 8) | \
    (((uint32_t)(x) & 0x0000ff00) << 8) | \
    (((uint32_t)(x) & 0x000000ff) << 24))

#ifdef __GNUC__
#    define AV_GCC_VERSION_AT_LEAST(x,y) (__GNUC__ > (x) || __GNUC__ == (x) && __GNUC_MINOR__ >= (y))
#    define AV_GCC_VERSION_AT_MOST(x,y)  (__GNUC__ < (x) || __GNUC__ == (x) && __GNUC_MINOR__ <= (y))
#else
#    define AV_GCC_VERSION_AT_LEAST(x,y) 0
#    define AV_GCC_VERSION_AT_MOST(x,y)  0
#endif

#ifndef av_always_inline
#if AV_GCC_VERSION_AT_LEAST(3,1)
#    define av_always_inline __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#    define av_always_inline __forceinline
#else
#    define av_always_inline inline
#endif
#endif

#ifndef av_extern_inline
#if defined(__ICL) && __ICL >= 1210 || defined(__GNUC_STDC_INLINE__)
#    define av_extern_inline extern inline
#else
#    define av_extern_inline inline
#endif
#endif

#if AV_GCC_VERSION_AT_LEAST(3,3) || defined(__clang__)
#   define av_alias __attribute__((may_alias))
#else
#   define av_alias
#endif

#if defined(__GNUC__) || defined(__clang__)
#define av_unused __attribute__((unused))
#else
#define av_unused
#endif

#define AV_BSWAP16C(x) (((x) << 8 & 0xff00)  | ((x) >> 8 & 0x00ff))
#define AV_BSWAP32C(x) (AV_BSWAP16C(x) << 16 | AV_BSWAP16C((x) >> 16))
#define AV_BSWAP64C(x) (AV_BSWAP32C(x) << 32 | AV_BSWAP32C((x) >> 32))

#define AV_BSWAPC(s, x) AV_BSWAP##s##C(x)
#ifndef av_bswap32
static av_always_inline const uint32_t av_bswap32(uint32_t x)
{
    return AV_BSWAP32C(x);
}
#endif

#if defined(__GNUC__)

union unaligned_64 { uint64_t l; } __attribute__((packed)) av_alias;
union unaligned_32 { uint32_t l; } __attribute__((packed)) av_alias;

#define AV_RN(s, p) (((const union unaligned_##s *) (p))->l)
#define AV_WN(s, p, v) ((((union unaligned_##s *) (p))->l) = (v))

#elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_X64) || defined(_M_ARM64)) && AV_HAVE_FAST_UNALIGNED

#define AV_RN(s, p) (*((const __unaligned uint##s##_t*)(p)))
#define AV_WN(s, p, v) (*((__unaligned uint##s##_t*)(p)) = (v))

#elif AV_HAVE_FAST_UNALIGNED

#define AV_RN(s, p) (((const av_alias##s*)(p))->u##s)
#define AV_WN(s, p, v) (((av_alias##s*)(p))->u##s = (v))

#else

#ifndef AV_RB32
#define AV_RB32(x)                                  \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
               (((const uint8_t*)(x))[1] << 16) |    \
               (((const uint8_t*)(x))[2] <<  8) |    \
                ((const uint8_t*)(x))[3])
#endif

#ifndef AV_WB32
#define AV_WB32(p, val) do {                    \
        uint32_t d = (val);                     \
        ((uint8_t*)(p))[3] = (d);               \
        ((uint8_t*)(p))[2] = (d)>>8;            \
        ((uint8_t*)(p))[1] = (d)>>16;           \
        ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)
#endif

#ifndef AV_RL32
#define AV_RL32(x)                                   \
    (((uint32_t)((const uint8_t*)(x))[3] << 24) |    \
               (((const uint8_t*)(x))[2] << 16) |    \
               (((const uint8_t*)(x))[1] <<  8) |    \
                ((const uint8_t*)(x))[0])
#endif

#ifndef AV_WL32
#define AV_WL32(p, val) do {                    \
        uint32_t d = (val);                     \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
    } while(0)
#endif

#ifndef AV_RB64
#define AV_RB64(x)                                      \
    (((uint64_t)((const uint8_t*)(x))[0] << 56) |       \
     ((uint64_t)((const uint8_t*)(x))[1] << 48) |       \
     ((uint64_t)((const uint8_t*)(x))[2] << 40) |       \
     ((uint64_t)((const uint8_t*)(x))[3] << 32) |       \
     ((uint64_t)((const uint8_t*)(x))[4] << 24) |       \
     ((uint64_t)((const uint8_t*)(x))[5] << 16) |       \
     ((uint64_t)((const uint8_t*)(x))[6] <<  8) |       \
      (uint64_t)((const uint8_t*)(x))[7])
#endif

#ifndef AV_WB64
#define AV_WB64(p, val) do {                    \
        uint64_t d = (val);                     \
        ((uint8_t*)(p))[7] = (d);               \
        ((uint8_t*)(p))[6] = (d)>>8;            \
        ((uint8_t*)(p))[5] = (d)>>16;           \
        ((uint8_t*)(p))[4] = (d)>>24;           \
        ((uint8_t*)(p))[3] = (d)>>32;           \
        ((uint8_t*)(p))[2] = (d)>>40;           \
        ((uint8_t*)(p))[1] = (d)>>48;           \
        ((uint8_t*)(p))[0] = (d)>>56;           \
    } while(0)
#endif

#ifndef AV_RL64
#define AV_RL64(x)                                      \
    (((uint64_t)((const uint8_t*)(x))[7] << 56) |       \
     ((uint64_t)((const uint8_t*)(x))[6] << 48) |       \
     ((uint64_t)((const uint8_t*)(x))[5] << 40) |       \
     ((uint64_t)((const uint8_t*)(x))[4] << 32) |       \
     ((uint64_t)((const uint8_t*)(x))[3] << 24) |       \
     ((uint64_t)((const uint8_t*)(x))[2] << 16) |       \
     ((uint64_t)((const uint8_t*)(x))[1] <<  8) |       \
      (uint64_t)((const uint8_t*)(x))[0])
#endif

#ifndef AV_WL64
#define AV_WL64(p, val) do {                    \
        uint64_t d = (val);                     \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
        ((uint8_t*)(p))[4] = (d)>>32;           \
        ((uint8_t*)(p))[5] = (d)>>40;           \
        ((uint8_t*)(p))[6] = (d)>>48;           \
        ((uint8_t*)(p))[7] = (d)>>56;           \
    } while(0)
#endif

#if AV_HAVE_BIGENDIAN
#define AV_RN(s, p)    AV_RB##s(p)
#define AV_WN(s, p, v) AV_WB##s(p, v)
#else
#define AV_RN(s, p)    AV_RL##s(p)
#define AV_WN(s, p, v) AV_WL##s(p, v)
#endif/*AV_HAVE_BIGENDIAN*/
#endif

#if AV_HAVE_BIGENDIAN
#if    defined(AV_RN32) && !defined(AV_RB32)
#define AV_RB32(p) AV_RN32(p)
#elif !defined(AV_RN32) &&  defined(AV_RB32)
#define AV_RN32(p) AV_RB32(p)
#endif

#if    defined(AV_WN32) && !defined(AV_WB32)
#define AV_WB32(p, v) AV_WN32(p, v)
#elif !defined(AV_WN32) &&  defined(AV_WB32)
#define AV_WN32(p, v) AV_WB32(p, v)
#endif

#if    defined(AV_RN64) && !defined(AV_RB64)
#define AV_RB64(p) AV_RN64(p)
#elif !defined(AV_RN64) &&  defined(AV_RB64)
#define AV_RN64(p) AV_RB64(p)
#endif

#if    defined(AV_WN64) && !defined(AV_WB64)
#define AV_WB64(p, v) AV_WN64(p, v)
#elif !defined(AV_WN64) &&  defined(AV_WB64)
#define AV_WN64(p, v) AV_WB64(p, v)
#endif

#else /* AV_HAVE_BIGENDIAN */

#if    defined(AV_RN32) && !defined(AV_RL32)
#define AV_RL32(p) AV_RN32(p)
#elif !defined(AV_RN32) &&  defined(AV_RL32)
#define AV_RN32(p) AV_RL32(p)
#endif

#if    defined(AV_WN32) && !defined(AV_WL32)
#define AV_WL32(p, v) AV_WN32(p, v)
#elif !defined(AV_WN32) &&  defined(AV_WL32)
#define AV_WN32(p, v) AV_WL32(p, v)
#endif

#if    defined(AV_RN64) && !defined(AV_RL64)
#define AV_RL64(p) AV_RN64(p)
#elif !defined(AV_RN64) &&  defined(AV_RL64)
#define AV_RN64(p) AV_RL64(p)
#endif

#if    defined(AV_WN64) && !defined(AV_WL64)
#define AV_WL64(p, v) AV_WN64(p, v)
#elif !defined(AV_WN64) &&  defined(AV_WL64)
#define AV_WN64(p, v) AV_WL64(p, v)
#endif
#endif /* !AV_HAVE_BIGENDIAN */



#if AV_HAVE_BIGENDIAN
#define AV_RB(s, p)    AV_RN##s(p)
#define AV_WB(s, p, v) AV_WN##s(p, v)
#define AV_RL(s, p)    av_bswap##s(AV_RN##s(p))
#define AV_WL(s, p, v) AV_WN##s(p, av_bswap##s(v))
#else
#define AV_RB(s, p)    av_bswap##s(AV_RN##s(p))
#define AV_WB(s, p, v) AV_WN##s(p, av_bswap##s(v))
#define AV_RL(s, p)    AV_RN##s(p)
#define AV_WL(s, p, v) AV_WN##s(p, v)
#endif

#ifndef AV_RB32
#define AV_RB32(p)    AV_RB(32, p)
#endif
#ifndef AV_WB32
#define AV_WB32(p, v) AV_WB(32, p, v)
#endif

#ifndef AV_RL32
#define AV_RL32(p)    AV_RL(32, p)
#endif
#ifndef AV_WL32
#define AV_WL32(p, v) AV_WL(32, p, v)
#endif

#ifndef AV_RB64
#define AV_RB64(p)    AV_RB(64, p)
#endif
#ifndef AV_WB64
#define AV_WB64(p, v) AV_WB(64, p, v)
#endif

#ifndef AV_RL64
#define AV_RL64(p)    AV_RL(64, p)
#endif
#ifndef AV_WL64
#define AV_WL64(p, v) AV_WL(64, p, v)
#endif

#ifndef AV_RN32
#   define AV_RN32(p) AV_RN(32, p)
#endif

#ifndef AV_RN64
#   define AV_RN64(p) AV_RN(64, p)
#endif

#ifndef AV_WN16
#   define AV_WN16(p, v) AV_WN(16, p, v)
#endif

#ifndef AV_WN32
#   define AV_WN32(p, v) AV_WN(32, p, v)
#endif

#ifndef AV_WN64
#   define AV_WN64(p, v) AV_WN(64, p, v)
#endif

#define AV_RNA(s, p)    (((const av_alias##s*)(p))->u##s)
#define AV_WNA(s, p, v) (((av_alias##s*)(p))->u##s = (v))

#ifndef AV_RN32A
#   define AV_RN32A(p) AV_RNA(32, p)
#endif

#ifndef AV_RN64A
#   define AV_RN64A(p) AV_RNA(64, p)
#endif

#ifndef AV_WN32A
#   define AV_WN32A(p, v) AV_WNA(32, p, v)
#endif

#ifndef AV_WN64A
#   define AV_WN64A(p, v) AV_WNA(64, p, v)
#endif

#define AV_COPYU(n, d, s) AV_WN##n(d, AV_RN##n(s));

#ifndef AV_COPY32U
#   define AV_COPY32U(d, s) AV_COPYU(32, d, s)
#endif

#ifndef AV_COPY64U
#   define AV_COPY64U(d, s) AV_COPYU(64, d, s)
#endif

#define AV_COPY(n, d, s) \
    (((av_alias##n*)(d))->u##n = ((const av_alias##n*)(s))->u##n)

#ifndef AV_COPY32
#   define AV_COPY32(d, s) AV_COPY(32, d, s)
#endif

#ifndef AV_COPY64
#   define AV_COPY64(d, s) AV_COPY(64, d, s)
#endif

#define AV_SWAP(n, a, b) FFSWAP(av_alias##n, *(av_alias##n*)(a), *(av_alias##n*)(b))

#ifndef AV_SWAP64
#   define AV_SWAP64(a, b) AV_SWAP(64, a, b)
#endif

#define AV_ZERO(n, d) (((av_alias##n*)(d))->u##n = 0)

#ifndef AV_ZERO32
#   define AV_ZERO32(d) AV_ZERO(32, d)
#endif

#ifndef AV_ZERO64
#   define AV_ZERO64(d) AV_ZERO(64, d)
#endif


#ifdef LONG_BITSTREAM_READER
#define MIN_CACHE_BITS 32
#else
#define MIN_CACHE_BITS 25
#endif

#define OPEN_READER_NOSIZE(name, gb)            \
    unsigned int name ## _index = (gb)->index;  \
    unsigned int av_unused name ## _cache

#if UNCHECKED_BITSTREAM_READER
#define OPEN_READER(name, gb) OPEN_READER_NOSIZE(name, gb)

#define BITS_AVAILABLE(name, gb) 1
#else
#define OPEN_READER(name, gb)                   \
    OPEN_READER_NOSIZE(name, gb);               \
    unsigned int name ## _size_plus8 = (gb)->size_in_bits_plus8

#define BITS_AVAILABLE(name, gb) name ## _index < name ## _size_plus8
#endif

#define CLOSE_READER(name, gb) (gb)->index = name ## _index

#ifdef LONG_BITSTREAM_READER

#define UPDATE_CACHE_LE(name, gb) name ## _cache = \
      AV_RL64((gb)->buffer + (name ## _index >> 3)) >> (name ## _index & 7)

# define UPDATE_CACHE_BE(name, gb) name ## _cache = \
      AV_RB64((gb)->buffer + (name ## _index >> 3)) >> (32 - (name ## _index & 7))

#else

# define UPDATE_CACHE_LE(name, gb) name ## _cache = \
      AV_RL32((gb)->buffer + (name ## _index >> 3)) >> (name ## _index & 7)

# define UPDATE_CACHE_BE(name, gb) name ## _cache = \
      AV_RB32((gb)->buffer + (name ## _index >> 3)) << (name ## _index & 7)

#endif


#ifdef BITSTREAM_READER_LE

#define UPDATE_CACHE(name, gb) UPDATE_CACHE_LE(name, gb)

#define SKIP_CACHE(name, gb, num) name ## _cache >>= (num)

#else

#define UPDATE_CACHE(name, gb) UPDATE_CACHE_BE(name, gb)

#define SKIP_CACHE(name, gb, num) name ## _cache <<= (num)

#endif

#if UNCHECKED_BITSTREAM_READER
#define SKIP_COUNTER(name, gb, num) name ## _index += (num)
#else
#define SKIP_COUNTER(name, gb, num) \
    name ## _index = FFMIN(name ## _size_plus8, name ## _index + (num))
#endif

#define BITS_LEFT(name, gb) ((int)((gb)->size_in_bits - name ## _index))

#define SKIP_BITS(name, gb, num)                \
    do {                                        \
        SKIP_CACHE(name, gb, num);              \
        SKIP_COUNTER(name, gb, num);            \
    } while (0)

#define LAST_SKIP_BITS(name, gb, num) SKIP_COUNTER(name, gb, num)

#define SHOW_UBITS_LE(name, gb, num) zero_extend(name ## _cache, num)
#define SHOW_SBITS_LE(name, gb, num) sign_extend(name ## _cache, num)

#define SHOW_UBITS_BE(name, gb, num) NEG_USR32(name ## _cache, num)
#define SHOW_SBITS_BE(name, gb, num) NEG_SSR32(name ## _cache, num)

#ifdef BITSTREAM_READER_LE
#define SHOW_UBITS(name, gb, num) SHOW_UBITS_LE(name, gb, num)
#define SHOW_SBITS(name, gb, num) SHOW_SBITS_LE(name, gb, num)
#else
#define SHOW_UBITS(name, gb, num) SHOW_UBITS_BE(name, gb, num)
#define SHOW_SBITS(name, gb, num) SHOW_SBITS_BE(name, gb, num)
#endif

#define GET_CACHE(name, gb) ((uint32_t) name ## _cache)

#ifndef NEG_SSR32
#define NEG_SSR32(a,s) ((( int32_t)(a))>>(32-(s)))
#endif

#ifndef NEG_USR32
#define NEG_USR32(a,s) (((uint32_t)(a))>>(32-(s)))
#endif

/* NAL unit types */
enum {
    H264_NAL_SLICE = 1,
    H264_NAL_DPA = 2,
    H264_NAL_DPB = 3,
    H264_NAL_DPC = 4,
    H264_NAL_IDR_SLICE = 5,
    H264_NAL_SEI = 6,
    H264_NAL_SPS = 7,
    H264_NAL_PPS = 8,
    H264_NAL_AUD = 9,
    H264_NAL_END_SEQUENCE = 10,
    H264_NAL_END_STREAM = 11,
    H264_NAL_FILLER_DATA = 12,
    H264_NAL_SPS_EXT = 13,
    H264_NAL_AUXILIARY_SLICE = 19,
};

enum {
    H264_MAX_SPS_COUNT = 32,
    H264_MAX_PPS_COUNT = 256,
    H264_MAX_DPB_FRAMES = 16,
    H264_MAX_REFS = 2 * H264_MAX_DPB_FRAMES,
    H264_MAX_RPLM_COUNT = H264_MAX_REFS + 1,
    H264_MAX_MMCO_COUNT = H264_MAX_REFS * 2 + 3,
    H264_MAX_SLICE_GROUPS = 8,
    H264_MAX_CPB_CNT = 32,
    H264_MAX_MB_PIC_SIZE = 139264,
    H264_MAX_MB_WIDTH = 1055,
    H264_MAX_MB_HEIGHT = 1055,
    H264_MAX_WIDTH = H264_MAX_MB_WIDTH * 16,
    H264_MAX_HEIGHT = H264_MAX_MB_HEIGHT * 16,
};

enum AVColorPrimaries {
    AVCOL_PRI_RESERVED0 = 0,
    AVCOL_PRI_BT709 = 1,  ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
    AVCOL_PRI_UNSPECIFIED = 2,
    AVCOL_PRI_RESERVED = 3,
    AVCOL_PRI_BT470M = 4,  ///< also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)

    AVCOL_PRI_BT470BG = 5,  ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
    AVCOL_PRI_SMPTE170M = 6,  ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
    AVCOL_PRI_SMPTE240M = 7,  ///< functionally identical to above
    AVCOL_PRI_FILM = 8,  ///< colour filters using Illuminant C
    AVCOL_PRI_BT2020 = 9,  ///< ITU-R BT2020
    AVCOL_PRI_SMPTE428 = 10, ///< SMPTE ST 428-1 (CIE 1931 XYZ)
    AVCOL_PRI_SMPTEST428_1 = AVCOL_PRI_SMPTE428,
    AVCOL_PRI_SMPTE431 = 11, ///< SMPTE ST 431-2 (2011) / DCI P3
    AVCOL_PRI_SMPTE432 = 12, ///< SMPTE ST 432-1 (2010) / P3 D65 / Display P3
    AVCOL_PRI_JEDEC_P22 = 22, ///< JEDEC P22 phosphors
    AVCOL_PRI_NB                ///< Not part of ABI
};

enum AVColorTransferCharacteristic {
    AVCOL_TRC_RESERVED0 = 0,
    AVCOL_TRC_BT709 = 1,  ///< also ITU-R BT1361
    AVCOL_TRC_UNSPECIFIED = 2,
    AVCOL_TRC_RESERVED = 3,
    AVCOL_TRC_GAMMA22 = 4,  ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
    AVCOL_TRC_GAMMA28 = 5,  ///< also ITU-R BT470BG
    AVCOL_TRC_SMPTE170M = 6,  ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
    AVCOL_TRC_SMPTE240M = 7,
    AVCOL_TRC_LINEAR = 8,  ///< "Linear transfer characteristics"
    AVCOL_TRC_LOG = 9,  ///< "Logarithmic transfer characteristic (100:1 range)"
    AVCOL_TRC_LOG_SQRT = 10, ///< "Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
    AVCOL_TRC_IEC61966_2_4 = 11, ///< IEC 61966-2-4
    AVCOL_TRC_BT1361_ECG = 12, ///< ITU-R BT1361 Extended Colour Gamut
    AVCOL_TRC_IEC61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
    AVCOL_TRC_BT2020_10 = 14, ///< ITU-R BT2020 for 10-bit system
    AVCOL_TRC_BT2020_12 = 15, ///< ITU-R BT2020 for 12-bit system
    AVCOL_TRC_SMPTE2084 = 16, ///< SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems
    AVCOL_TRC_SMPTEST2084 = AVCOL_TRC_SMPTE2084,
    AVCOL_TRC_SMPTE428 = 17, ///< SMPTE ST 428-1
    AVCOL_TRC_SMPTEST428_1 = AVCOL_TRC_SMPTE428,
    AVCOL_TRC_ARIB_STD_B67 = 18, ///< ARIB STD-B67, known as "Hybrid log-gamma"
    AVCOL_TRC_NB                 ///< Not part of ABI
};

enum AVColorSpace {
    AVCOL_SPC_RGB = 0,  ///< order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB)
    AVCOL_SPC_BT709 = 1,  ///< also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
    AVCOL_SPC_UNSPECIFIED = 2,
    AVCOL_SPC_RESERVED = 3,
    AVCOL_SPC_FCC = 4,  ///< FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
    AVCOL_SPC_BT470BG = 5,  ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601
    AVCOL_SPC_SMPTE170M = 6,  ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
    AVCOL_SPC_SMPTE240M = 7,  ///< functionally identical to above
    AVCOL_SPC_YCGCO = 8,  ///< Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
    AVCOL_SPC_YCOCG = AVCOL_SPC_YCGCO,
    AVCOL_SPC_BT2020_NCL = 9,  ///< ITU-R BT2020 non-constant luminance system
    AVCOL_SPC_BT2020_CL = 10, ///< ITU-R BT2020 constant luminance system
    AVCOL_SPC_SMPTE2085 = 11, ///< SMPTE 2085, Y'D'zD'x
    AVCOL_SPC_CHROMA_DERIVED_NCL = 12, ///< Chromaticity-derived non-constant luminance system
    AVCOL_SPC_CHROMA_DERIVED_CL = 13, ///< Chromaticity-derived constant luminance system
    AVCOL_SPC_ICTCP = 14, ///< ITU-R BT.2100-0, ICtCp
    AVCOL_SPC_NB                ///< Not part of ABI
};

typedef struct _AVRational {
    int num; ///< Numerator
    int den; ///< Denominator
} AVRational;

typedef struct _SPS {
    unsigned int sps_id;
    int profile_idc;
    int level_idc;
    int chroma_format_idc;
    int transform_bypass;              ///< qpprime_y_zero_transform_bypass_flag
    int log2_max_frame_num;            ///< log2_max_frame_num_minus4 + 4
    int poc_type;                      ///< pic_order_cnt_type
    int log2_max_poc_lsb;              ///< log2_max_pic_order_cnt_lsb_minus4
    int delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int poc_cycle_length;              ///< num_ref_frames_in_pic_order_cnt_cycle
    int ref_frame_count;               ///< num_ref_frames
    int gaps_in_frame_num_allowed_flag;
    int mb_width;                      ///< pic_width_in_mbs_minus1 + 1
                                       ///< (pic_height_in_map_units_minus1 + 1) * (2 - frame_mbs_only_flag)
    int mb_height;
    int frame_mbs_only_flag;
    int mb_aff;                        ///< mb_adaptive_frame_field_flag
    int direct_8x8_inference_flag;
    int crop;                          ///< frame_cropping_flag

                                       /* those 4 are already in luma samples */
    unsigned int crop_left;            ///< frame_cropping_rect_left_offset
    unsigned int crop_right;           ///< frame_cropping_rect_right_offset
    unsigned int crop_top;             ///< frame_cropping_rect_top_offset
    unsigned int crop_bottom;          ///< frame_cropping_rect_bottom_offset
    int vui_parameters_present_flag;
    AVRational sar;
    int video_signal_type_present_flag;
    int full_range;
    int colour_description_present_flag;
    enum AVColorPrimaries color_primaries;
    enum AVColorTransferCharacteristic color_trc;
    enum AVColorSpace colorspace;
    int timing_info_present_flag;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    int fixed_frame_rate_flag;
    short offset_for_ref_frame[256]; // FIXME dyn aloc?
    int bitstream_restriction_flag;
    int num_reorder_frames;
    int scaling_matrix_present;
    uint8_t scaling_matrix4[6][16];
    uint8_t scaling_matrix8[6][64];
    int nal_hrd_parameters_present_flag;
    int vcl_hrd_parameters_present_flag;
    int pic_struct_present_flag;
    int time_offset_length;
    int cpb_cnt;                          ///< See H.264 E.1.2
    int initial_cpb_removal_delay_length; ///< initial_cpb_removal_delay_length_minus1 + 1
    int cpb_removal_delay_length;         ///< cpb_removal_delay_length_minus1 + 1
    int dpb_output_delay_length;          ///< dpb_output_delay_length_minus1 + 1
    int bit_depth_luma;                   ///< bit_depth_luma_minus8 + 8
    int bit_depth_chroma;                 ///< bit_depth_chroma_minus8 + 8
    int residual_color_transform_flag;    ///< residual_colour_transform_flag
    int constraint_set_flags;             ///< constraint_set[0-3]_flag
    uint8_t data[4096];
    size_t data_size;
} SPS;

int get_h264_sps_info(uint8_t* buff, int nLen, int& nWidth, int& nHeight, int& nFrameRate);

enum HEVCNALUnitType {
     HEVC_NAL_TRAIL_N = 0,
     HEVC_NAL_TRAIL_R = 1,
     HEVC_NAL_TSA_N = 2,
     HEVC_NAL_TSA_R = 3,
     HEVC_NAL_STSA_N = 4,
     HEVC_NAL_STSA_R = 5,
     HEVC_NAL_RADL_N = 6,
     HEVC_NAL_RADL_R = 7,
     HEVC_NAL_RASL_N = 8,
     HEVC_NAL_RASL_R = 9,
     HEVC_NAL_VCL_N10 = 10,
     HEVC_NAL_VCL_R11 = 11,
     HEVC_NAL_VCL_N12 = 12,
     HEVC_NAL_VCL_R13 = 13,
     HEVC_NAL_VCL_N14 = 14,
     HEVC_NAL_VCL_R15 = 15,
     HEVC_NAL_BLA_W_LP = 16,
     HEVC_NAL_BLA_W_RADL = 17,
     HEVC_NAL_BLA_N_LP = 18,
     HEVC_NAL_IDR_W_RADL = 19,
     HEVC_NAL_IDR_N_LP = 20,
     HEVC_NAL_CRA_NUT = 21,
     HEVC_NAL_IRAP_VCL22 = 22,
     HEVC_NAL_IRAP_VCL23 = 23,
     HEVC_NAL_RSV_VCL24 = 24,
     HEVC_NAL_RSV_VCL25 = 25,
     HEVC_NAL_RSV_VCL26 = 26,
     HEVC_NAL_RSV_VCL27 = 27,
     HEVC_NAL_RSV_VCL28 = 28,
     HEVC_NAL_RSV_VCL29 = 29,
     HEVC_NAL_RSV_VCL30 = 30,
     HEVC_NAL_RSV_VCL31 = 31,
     HEVC_NAL_VPS = 32,
     HEVC_NAL_SPS = 33,
     HEVC_NAL_PPS = 34,
     HEVC_NAL_AUD = 35,
     HEVC_NAL_EOS_NUT = 36,
     HEVC_NAL_EOB_NUT = 37,
     HEVC_NAL_FD_NUT = 38,
     HEVC_NAL_SEI_PREFIX = 39,
     HEVC_NAL_SEI_SUFFIX = 40,
     HEVC_NAL_RSV_NVCL41 = 41,
     HEVC_NAL_RSV_NVCL42 = 42,
     HEVC_NAL_RSV_NVCL43 = 43,
     HEVC_NAL_RSV_NVCL44 = 44,
     HEVC_NAL_RSV_NVCL45 = 45,
     HEVC_NAL_RSV_NVCL46 = 46,
     HEVC_NAL_RSV_NVCL47 = 47,
     HEVC_NAL_UNSPEC48 = 48,
     HEVC_NAL_UNSPEC49 = 49,
     HEVC_NAL_UNSPEC50 = 50,
     HEVC_NAL_UNSPEC51 = 51,
     HEVC_NAL_UNSPEC52 = 52,
     HEVC_NAL_UNSPEC53 = 53,
     HEVC_NAL_UNSPEC54 = 54,
     HEVC_NAL_UNSPEC55 = 55,
     HEVC_NAL_UNSPEC56 = 56,
     HEVC_NAL_UNSPEC57 = 57,
     HEVC_NAL_UNSPEC58 = 58,
     HEVC_NAL_UNSPEC59 = 59,
     HEVC_NAL_UNSPEC60 = 60,
     HEVC_NAL_UNSPEC61 = 61,
     HEVC_NAL_UNSPEC62 = 62,
     HEVC_NAL_UNSPEC63 = 63,
};

enum HEVCSliceType {
     HEVC_SLICE_B = 0,
     HEVC_SLICE_P = 1,
     HEVC_SLICE_I = 2,
};

enum {
     // 7.4.3.1: vps_max_layers_minus1 is in [0, 62].
     HEVC_MAX_LAYERS = 63,
     // 7.4.3.1: vps_max_sub_layers_minus1 is in [0, 6].
     HEVC_MAX_SUB_LAYERS = 7,
     // 7.4.3.1: vps_num_layer_sets_minus1 is in [0, 1023].
     HEVC_MAX_LAYER_SETS = 1024,

     // 7.4.2.1: vps_video_parameter_set_id is u(4).
     HEVC_MAX_VPS_COUNT = 16,
     // 7.4.3.2.1: sps_seq_parameter_set_id is in [0, 15].
     HEVC_MAX_SPS_COUNT = 16,
     // 7.4.3.3.1: pps_pic_parameter_set_id is in [0, 63].
     HEVC_MAX_PPS_COUNT = 64,

     // A.4.2: MaxDpbSize is bounded above by 16.
     HEVC_MAX_DPB_SIZE = 16,
     // 7.4.3.1: vps_max_dec_pic_buffering_minus1[i] is in [0, MaxDpbSize - 1].
     HEVC_MAX_REFS = HEVC_MAX_DPB_SIZE,

     // 7.4.3.2.1: num_short_term_ref_pic_sets is in [0, 64].
     HEVC_MAX_SHORT_TERM_REF_PIC_SETS = 64,
     // 7.4.3.2.1: num_long_term_ref_pics_sps is in [0, 32].
     HEVC_MAX_LONG_TERM_REF_PICS = 32,

     // A.3: all profiles require that CtbLog2SizeY is in [4, 6].
     HEVC_MIN_LOG2_CTB_SIZE = 4,
     HEVC_MAX_LOG2_CTB_SIZE = 6,

     // E.3.2: cpb_cnt_minus1[i] is in [0, 31].
     HEVC_MAX_CPB_CNT = 32,

     // A.4.1: in table A.6 the highest level allows a MaxLumaPs of 35 651 584.
     HEVC_MAX_LUMA_PS = 35651584,
     // A.4.1: pic_width_in_luma_samples and pic_height_in_luma_samples are
     // constrained to be not greater than sqrt(MaxLumaPs * 8).  Hence height/
     // width are bounded above by sqrt(8 * 35651584) = 16888.2 samples.
     HEVC_MAX_WIDTH = 16888,
     HEVC_MAX_HEIGHT = 16888,

     // A.4.1: table A.6 allows at most 22 tile rows for any level.
     HEVC_MAX_TILE_ROWS = 22,
     // A.4.1: table A.6 allows at most 20 tile columns for any level.
     HEVC_MAX_TILE_COLUMNS = 20,

     // A.4.2: table A.6 allows at most 600 slice segments for any level.
     HEVC_MAX_SLICE_SEGMENTS = 600,

     // 7.4.7.1: in the worst case (tiles_enabled_flag and
     // entropy_coding_sync_enabled_flag are both set), entry points can be
     // placed at the beginning of every Ctb row in every tile, giving an
     // upper bound of (num_tile_columns_minus1 + 1) * PicHeightInCtbsY - 1.
     // Only a stream with very high resolution and perverse parameters could
     // get near that, though, so set a lower limit here with the maximum
     // possible value for 4K video (at most 135 16x16 Ctb rows).
     HEVC_MAX_ENTRY_POINT_OFFSETS = HEVC_MAX_TILE_COLUMNS * 135,
};

typedef struct _HVCCNALUnitArray {
	uint8_t  array_completeness;
	uint8_t  NAL_unit_type;
	uint16_t numNalus;
	uint16_t *nalUnitLength;
	uint8_t  **nalUnit;
}HVCCNALUnitArray;

typedef struct _HEVCDecoderConfigurationRecord {
	uint8_t  configurationVersion;
	uint8_t  general_profile_space;
	uint8_t  general_tier_flag;
	uint8_t  general_profile_idc;
	uint32_t general_profile_compatibility_flags;
	uint64_t general_constraint_indicator_flags;
	uint8_t  general_level_idc;
	uint16_t min_spatial_segmentation_idc;
	uint8_t  parallelismType;
	uint8_t  chromaFormat;
	uint8_t  bitDepthLumaMinus8;
	uint8_t  bitDepthChromaMinus8;
	uint16_t avgFrameRate;
	uint8_t  constantFrameRate;
	uint8_t  numTemporalLayers;
	uint8_t  temporalIdNested;
	uint8_t  lengthSizeMinusOne;
	uint8_t  numOfArrays;
	HVCCNALUnitArray *array;
}HEVCDecoderConfigurationRecord;

typedef struct _HVCCProfileTierLevel {
    uint8_t  profile_space;
    uint8_t  tier_flag;
    uint8_t  profile_idc;
    uint32_t profile_compatibility_flags;
    uint64_t constraint_indicator_flags;
    uint8_t  level_idc;
} HVCCProfileTierLevel;

typedef struct GetBitContext {
    const uint8_t *buffer, *buffer_end;
    int index;
    int size_in_bits;
    int size_in_bits_plus8;
} GetBitContext;

int get_sequence_header(
	unsigned char *out_data,
	unsigned char *pps,
	unsigned short pps_len,
	unsigned char * sps,
	unsigned short sps_len,
	unsigned char * vps,
	unsigned short vps_len);


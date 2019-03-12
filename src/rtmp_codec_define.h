
#ifndef _RTMP_CLIENT_CODE_DEFIND_H_
#define _RTMP_CLIENT_CODE_DEFIND_H_

#pragma once



/**
* E.4.1 FLV Tag, page 75
*/
enum RTMPCodecFlvTag
{
    // set to the zero to reserved, for array map.
    RTMPCodecFlvTagReserved     = 0,

    // 8 = audio
    RTMPCodecFlvTagAudio        = 8,
    // 9 = video
    RTMPCodecFlvTagVideo        = 9,
    // 18 = script data
    RTMPCodecFlvTagScript       = 18,
};


///* Audio Codec RTMPCodecAudio */
enum eAudioCodecType
{
    // 16 bit uniform PCM values. 原始 pcm 数据 
    AudioCodec_Law_PCM16        = 0, 		

    // A law 
    AudioCodec_Law_G711A        = 7, 		
    // U law
    AudioCodec_Law_G711U        = 8, 		
    // G726   
    AudioCodec_Law_G726         = 9,		
    // AAC  
    AudioCodec_AAC              = 10		
};


///* Audio Rate Bits */
enum AudioRateBit
{
    // 16k bits per second (2 bits per ADPCM sample)
    Rate16kBits                 = 2,	
    // 24k bits per second (3 bits per ADPCM sample)
    Rate24kBits                 = 3,	
    // 32k bits per second (4 bits per ADPCM sample)
    Rate32kBits                 = 4,	
    // 40k bits per second (5 bits per ADPCM sample)
    Rate40kBits                 = 5	    
};


struct G726Param
{
    unsigned char   ucRateBits;             //AudioRateBit:Rate16kBits Rate24kBits Rate32kBits Rate40kBits
};

struct G711Param
{
    ;
};

struct InitAudioParam
{
    unsigned char	ucInAudioFrame;			// 1为完整的单帧,0为非完整帧(有截断的buff)
    unsigned char	ucAudioCodec;			// Law_uLaw  Law_ALaw Law_PCM16 Law_G726
    unsigned char	ucAudioChannel;			// 1
    unsigned int	u32AudioSamplerate;		// 8000
    unsigned int	u32PCMBitSize;			// 16
    union
    {
        G711Param   g711param;
        G726Param   g726param;
    };

};

/// Audio Encoder info
struct AACEncoderInfo 
{
    char            aac_object;            // RTMPAacObjectType
    char            aac_sample_rate;       // RTMPCodecAudioSampleRate
    char            aac_channels;          // 声道
    char            got_sequence_header;   // 0
};

/// Video Encoder info
struct VideoEncoderInfo
{
    int             codectype;              // 0:h264 1:h265
    int             init;
    int             profile_idc;
    int             level_idc;
    int             tier_idc;
    int             width;                  // 宽
    int             height;                 // 高
    int             crop_left;
    int             crop_right;
    int             crop_top;
    int             crop_bottom;
    float           max_framerate;          // 由SPS计算得到的帧率，为0表示SPS中没有相应的字段计算
    int             chroma_format_idc;      // YUV颜色空间 0: monochrome 1:420 2:422 3:444
    int             encoding_type;          // 为1表示CABAC 0表示CAVLC
    int             bit_depth_luma;
    int             bit_depth_chroma;
} ;




// E.4.3.1 VIDEODATA
// Frame Type UB [4]
// Type of video frame. The following values are defined:
//     1 = key frame (for AVC, a seekable frame)
//     2 = inter frame (for AVC, a non-seekable frame)
//     3 = disposable inter frame (H.263 only)
//     4 = generated key frame (reserved for server use only)
//     5 = video info/command frame
enum RTMPCodecVideoAVCFrame
{
    // set to the zero to reserved, for array map.
    RTMPCodecVideoAVCFrameReserved                  = 0,
    RTMPCodecVideoAVCFrameReserved1                 = 6,
    
    RTMPCodecVideoAVCFrameKeyFrame                  = 1,
    RTMPCodecVideoAVCFrameInterFrame                = 2,
    RTMPCodecVideoAVCFrameDisposableInterFrame      = 3,
    RTMPCodecVideoAVCFrameGeneratedKeyFrame         = 4,
    RTMPCodecVideoAVCFrameVideoInfoFrame            = 5,
};

// AVCPacketType IF CodecID == 7 UI8
// The following values are defined:
//     0 = AVC sequence header
//     1 = AVC NALU
//     2 = AVC end of sequence (lower level NALU sequence ender is
//         not required or supported)
enum RTMPCodecVideoAVCType
{
    // set to the max value to reserved, for array map.
    RTMPCodecVideoAVCTypeReserved                   = 3,
    
    RTMPCodecVideoAVCTypeSequenceHeader             = 0,
    RTMPCodecVideoAVCTypeNALU                       = 1,
    RTMPCodecVideoAVCTypeSequenceHeaderEOF          = 2,
};

// E.4.3.1 VIDEODATA
// CodecID UB [4]
// Codec Identifier. The following values are defined:
//     2 = Sorenson H.263
//     3 = Screen video
//     4 = On2 VP6
//     5 = On2 VP6 with alpha channel
//     6 = Screen video version 2
//     7 = AVC
//	  12 = HEVC
enum RTMPCodecVideo
{
    // set to the zero to reserved, for array map.
    RTMPCodecVideoReserved                          = 0,
    RTMPCodecVideoReserved1                         = 1,
    RTMPCodecVideoReserved2                         = 9,
    
    // for user to disable video, for example, use pure audio hls.
    RTMPCodecVideoDisabled                          = 8,
    
    RTMPCodecVideoSorensonH263                      = 2,
    RTMPCodecVideoScreenVideo                       = 3,
    RTMPCodecVideoOn2VP6                            = 4,
    RTMPCodecVideoOn2VP6WithAlphaChannel            = 5,
    RTMPCodecVideoScreenVideoVersion2               = 6,
    RTMPCodecVideoAVC                               = 7, //h264
	RTMPCodecVideoHEVC								= 12,//H265
};


/**
 * Table 7-1 - NAL unit type codes, syntax element categories, and NAL unit type classes
 * H.264-AVC-ISO_IEC_14496-10-2012.pdf, page 83.
 */
enum RTMPAvcNaluType
{
    // Unspecified
    RTMPAvcNaluTypeReserved = 0,
    
    // Coded slice of a non-IDR picture slice_layer_without_partitioning_rbsp( )
    RTMPAvcNaluTypeNonIDR = 1,
    // Coded slice data partition A slice_data_partition_a_layer_rbsp( )
    RTMPAvcNaluTypeDataPartitionA = 2,
    // Coded slice data partition B slice_data_partition_b_layer_rbsp( )
    RTMPAvcNaluTypeDataPartitionB = 3,
    // Coded slice data partition C slice_data_partition_c_layer_rbsp( )
    RTMPAvcNaluTypeDataPartitionC = 4,
    // Coded slice of an IDR picture slice_layer_without_partitioning_rbsp( )
    RTMPAvcNaluTypeIDR = 5,
    // Supplemental enhancement information (SEI) sei_rbsp( )
    RTMPAvcNaluTypeSEI = 6,
    // Sequence parameter set seq_parameter_set_rbsp( )
    RTMPAvcNaluTypeSPS = 7,
    // Picture parameter set pic_parameter_set_rbsp( )
    RTMPAvcNaluTypePPS = 8,
    // Access unit delimiter access_unit_delimiter_rbsp( )
    RTMPAvcNaluTypeAccessUnitDelimiter = 9,
    // End of sequence end_of_seq_rbsp( )
    RTMPAvcNaluTypeEOSequence = 10,
    // End of stream end_of_stream_rbsp( )
    RTMPAvcNaluTypeEOStream = 11,
    // Filler data filler_data_rbsp( )
    RTMPAvcNaluTypeFilterData = 12,
    // Sequence parameter set extension seq_parameter_set_extension_rbsp( )
    RTMPAvcNaluTypeSPSExt = 13,
    // Prefix NAL unit prefix_nal_unit_rbsp( )
    RTMPAvcNaluTypePrefixNALU = 14,
    // Subset sequence parameter set subset_seq_parameter_set_rbsp( )
    RTMPAvcNaluTypeSubsetSPS = 15,
    // Coded slice of an auxiliary coded picture without partitioning slice_layer_without_partitioning_rbsp( )
    RTMPAvcNaluTypeLayerWithoutPartition = 19,
    // Coded slice extension slice_layer_extension_rbsp( )
    RTMPAvcNaluTypeCodedSliceExt = 20,
};


/**
* the profile for avc/h.264.
* @see Annex A Profiles and levels, H.264-AVC-ISO_IEC_14496-10.pdf, page 205.
*/
enum RTMPAvcProfile
{
    RTMPAvcProfileReserved = 0,

    // @see ffmpeg, libavcodec/avcodec.h:2713
    RTMPAvcProfileBaseline = 66,
    // FF_PROFILE_H264_CONSTRAINED  (1<<9)  // 8+1; constraint_set1_flag
    // FF_PROFILE_H264_CONSTRAINED_BASELINE (66|FF_PROFILE_H264_CONSTRAINED)
    RTMPAvcProfileConstrainedBaseline = 578,
    RTMPAvcProfileMain = 77,
    RTMPAvcProfileExtended = 88,
    RTMPAvcProfileHigh = 100,
    RTMPAvcProfileHigh10 = 110,
    RTMPAvcProfileHigh10Intra = 2158,
    RTMPAvcProfileHigh422 = 122,
    RTMPAvcProfileHigh422Intra = 2170,
    RTMPAvcProfileHigh444 = 144,
    RTMPAvcProfileHigh444Predictive = 244,
    RTMPAvcProfileHigh444Intra = 2192,
};


/**
* the level for avc/h.264.
* @see Annex A Profiles and levels, H.264-AVC-ISO_IEC_14496-10.pdf, page 207.
*/
enum RTMPAvcLevel
{
    RTMPAvcLevelReserved    = 0,

    RTMPAvcLevel_1          = 10,
    RTMPAvcLevel_11         = 11,
    RTMPAvcLevel_12         = 12,
    RTMPAvcLevel_13         = 13,
    RTMPAvcLevel_2          = 20,
    RTMPAvcLevel_21         = 21,
    RTMPAvcLevel_22         = 22,
    RTMPAvcLevel_3          = 30,
    RTMPAvcLevel_31         = 31,
    RTMPAvcLevel_32         = 32,
    RTMPAvcLevel_4          = 40,
    RTMPAvcLevel_41         = 41,
    RTMPAvcLevel_5          = 50,
    RTMPAvcLevel_51         = 51,
};


/**
* the avc payload format, must be ibmf or annexb format.
* we guess by annexb first, then ibmf for the first time,
* and we always use the guessed format for the next time.
*/
enum RTMPAvcPayloadFormat
{
    RTMPAvcPayloadFormatGuess = 0,
    RTMPAvcPayloadFormatAnnexb,
    RTMPAvcPayloadFormatIbmf,
};


//////////////////////////////////////////////////////////////////////////

// AACPacketType IF SoundFormat == 10 UI8
// The following values are defined:
//     0 = AAC sequence header
//     1 = AAC raw
enum RTMPCodecAudioType
{
    // set to the max value to reserved, for array map.
    RTMPCodecAudioTypeReserved                      = 2,

    RTMPCodecAudioTypeSequenceHeader                = 0,
    RTMPCodecAudioTypeRawData                       = 1,
};

// SoundFormat UB [4] 
// Format of SoundData. The following values are defined:
//     0 = Linear PCM, platform endian
//     1 = ADPCM
//     2 = MP3
//     3 = Linear PCM, little endian
//     4 = Nellymoser 16 kHz mono
//     5 = Nellymoser 8 kHz mono
//     6 = Nellymoser
//     7 = G.711 A-law logarithmic PCM
//     8 = G.711 mu-law logarithmic PCM
//     9 = reserved
//     10 = AAC
//     11 = Speex
//     14 = MP3 8 kHz
//     15 = Device-specific sound
// Formats 7, 8, 14, and 15 are reserved.
// AAC is supported in Flash Player 9,0,115,0 and higher.
// Speex is supported in Flash Player 10 and higher.
enum RTMPCodecAudio
{
    // set to the max value to reserved, for array map.
    RTMPCodecAudioReserved1                         = 16,
    
    // for user to disable audio, for example, use pure video hls.
    RTMPCodecAudioDisabled                          = 17,
    
    RTMPCodecAudioLinearPCMPlatformEndian           = 0,
    RTMPCodecAudioADPCM                             = 1,
    RTMPCodecAudioMP3                               = 2,
    RTMPCodecAudioLinearPCMLittleEndian             = 3,
    RTMPCodecAudioNellymoser16kHzMono               = 4,
    RTMPCodecAudioNellymoser8kHzMono                = 5,
    RTMPCodecAudioNellymoser                        = 6,
    RTMPCodecAudioReservedG711AlawLogarithmicPCM    = 7,
    RTMPCodecAudioReservedG711MuLawLogarithmicPCM   = 8,
    RTMPCodecAudioReserved                          = 9,
    RTMPCodecAudioAAC                               = 10,
    RTMPCodecAudioSpeex                             = 11,
    RTMPCodecAudioReservedMP3_8kHz                  = 14,
    RTMPCodecAudioReservedDeviceSpecificSound       = 15,
};


/**
* the FLV/RTMP supported audio sample rate.
* Sampling rate. The following values are defined:
* 0 = 5.5 kHz = 5512 Hz
* 1 = 11 kHz = 11025 Hz
* 2 = 22 kHz = 22050 Hz
* 3 = 44 kHz = 44100 Hz
*/
enum RTMPCodecAudioSampleRate
{
    // set to the max value to reserved, for array map.
    RTMPCodecAudioSampleRateReserved                 = 4,
    
    RTMPCodecAudioSampleRate5512                     = 0,
    RTMPCodecAudioSampleRate11025                    = 1,
    RTMPCodecAudioSampleRate22050                    = 2,
    RTMPCodecAudioSampleRate44100                    = 3,

    RTMPCodecAudioSampleRate8000                     = 11,
};


/**
* the FLV/RTMP supported audio sample size.
* Size of each audio sample. This parameter only pertains to
* uncompressed formats. Compressed formats always decode
* to 16 bits internally.
* 0 = 8-bit samples
* 1 = 16-bit samples
*/
enum RTMPCodecAudioSampleSize
{
    // set to the max value to reserved, for array map.
    RTMPCodecAudioSampleSizeReserved                 = 2,
    
    RTMPCodecAudioSampleSize8bit                     = 0,
    RTMPCodecAudioSampleSize16bit                    = 1,
};

/**
* the FLV/RTMP supported audio sound type/channel.
* Mono or stereo sound
* 0 = Mono sound
* 1 = Stereo sound
*/
enum RTMPCodecAudioSoundType
{
    // set to the max value to reserved, for array map.
    RTMPCodecAudioSoundTypeReserved                  = 2, 
    
    RTMPCodecAudioSoundTypeMono                      = 0,
    RTMPCodecAudioSoundTypeStereo                    = 1,
};



/**
* the aac profile, for ADTS(HLS/TS)
* @see https://github.com/ossrs/srs/issues/310
*/
enum RTMPAacProfile
{
    RTMPAacProfileReserved  = 3,
    
    // @see 7.1 Profiles,MPEG-2, aac-iso-13818-7.pdf, page 40
    RTMPAacProfileMain      = 0,
    RTMPAacProfileLC        = 1,
    RTMPAacProfileSSR       = 2,
};


/**
* the aac object type, for RTMP sequence header,MPEG-4
* for AudioSpecificConfig, @see aac-mp4a-format-ISO_IEC_14496-3+2001.pdf, page 33
* for audioObjectType, @see aac-mp4a-format-ISO_IEC_14496-3+2001.pdf, page 23
*/
enum RTMPAacObjectType
{
    RTMPAacObjectTypeReserved   = 0,
    
    // Table 1.1 - Audio Object Type definition
    // @see @see aac-mp4a-format-ISO_IEC_14496-3+2001.pdf, page 23
    RTMPAacObjectTypeAacMain    = 1,
    RTMPAacObjectTypeAacLC      = 2,
    RTMPAacObjectTypeAacSSR     = 3,
    RTMPAacObjectTypeAacLTP     = 4,    

    // AAC HE = LC+SBR
    RTMPAacObjectTypeAacHE      = 5,
    // AAC HEv2 = LC+SBR+PS
    RTMPAacObjectTypeAacHEV2    = 29,
};


#endif//_RTMP_CLIENT_CODE_DEFIND_H_
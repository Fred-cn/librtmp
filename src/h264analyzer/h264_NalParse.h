#ifndef PALPARSE_H
#define PALPARSE_H


typedef struct
{
	int type;                       // 0 -- h.264; 1 -- h.265
	unsigned int num;               // 序号
	unsigned int len;               // 含起始码的总的长度
	unsigned int offset;            // nal包在文件中的偏移
	int sliceType;                  // 帧类型
	int nalType;                    // NAL类型
	int startcodeLen;               // start code长度
	char startcodeBuffer[16];       // 起始码，字符串形式
} NALU_t;

typedef struct
{
	int profile_idc;
	int level_idc;
	int width;
	int height;
	int crop_left;
	int crop_right;
	int crop_top;
	int crop_bottom;
	float max_framerate;        // 由SPS计算得到的帧率，为0表示SPS中没有相应的字段计算
	int chroma_format_idc;      // YUV颜色空间 0: monochrome 1:420 2:422 3:444
}SPSInfo_t;

typedef struct
{
	int encoding_type;  // 为1表示CABAC 0表示CAVLC

}PPSInfo_t;

enum FileType
{
	FILE_H264 = 0,
	FILE_H265 = 1,
	FILE_UNK = 2,
};

typedef struct
{
	int type;           // 0:h264 1:h265
	int init;
	int profile_idc;
	int level_idc;
	int tier_idc;
	int width;
	int height;
	int crop_left;
	int crop_right;
	int crop_top;
	int crop_bottom;
	float max_framerate;  // 由SPS计算得到的帧率，为0表示SPS中没有相应的字段计算
	int chroma_format_idc;  // YUV颜色空间 0: monochrome 1:420 2:422 3:444
	int encoding_type;  // 为1表示CABAC 0表示CAVLC
	int bit_depth_luma;
	int bit_depth_chroma;
} videoinfo_t;



//NAL ref idc codes
#define NAL_REF_IDC_PRIORITY_HIGHEST    3
#define NAL_REF_IDC_PRIORITY_HIGH       2
#define NAL_REF_IDC_PRIORITY_LOW        1
#define NAL_REF_IDC_PRIORITY_DISPOSABLE 0

//Table 7-1 NAL unit type codes
#define NAL_UNIT_TYPE_UNSPECIFIED                    0    // Unspecified
#define NAL_UNIT_TYPE_CODED_SLICE_NON_IDR            1    // Coded slice of a non-IDR picture
#define NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A   2    // Coded slice data partition A
#define NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B   3    // Coded slice data partition B
#define NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C   4    // Coded slice data partition C
#define NAL_UNIT_TYPE_CODED_SLICE_IDR                5    // Coded slice of an IDR picture
#define NAL_UNIT_TYPE_SEI                            6    // Supplemental enhancement information (SEI)
#define NAL_UNIT_TYPE_SPS                            7    // Sequence parameter set
#define NAL_UNIT_TYPE_PPS                            8    // Picture parameter set
#define NAL_UNIT_TYPE_AUD                            9    // Access unit delimiter
#define NAL_UNIT_TYPE_END_OF_SEQUENCE               10    // End of sequence
#define NAL_UNIT_TYPE_END_OF_STREAM                 11    // End of stream
#define NAL_UNIT_TYPE_FILLER                        12    // Filler data
#define NAL_UNIT_TYPE_SPS_EXT                       13    // Sequence parameter set extension
// 14..18    // Reserved
#define NAL_UNIT_TYPE_CODED_SLICE_AUX               19    // Coded slice of an auxiliary coded picture without partitioning
// 20..23    // Reserved
// 24..31    // Unspecified



//7.4.3 Table 7-6. Name association to slice_type
#define SH_SLICE_TYPE_P        0        // P (P slice)
#define SH_SLICE_TYPE_B        1        // B (B slice)
#define SH_SLICE_TYPE_I        2        // I (I slice)
#define SH_SLICE_TYPE_SP       3        // SP (SP slice)
#define SH_SLICE_TYPE_SI       4        // SI (SI slice)
//as per footnote to Table 7-6, the *_ONLY slice types indicate that all other slices in that picture are of the same type
#define SH_SLICE_TYPE_P_ONLY    5        // P (P slice)
#define SH_SLICE_TYPE_B_ONLY    6        // B (B slice)
#define SH_SLICE_TYPE_I_ONLY    7        // I (I slice)
#define SH_SLICE_TYPE_SP_ONLY   8        // SP (SP slice)
#define SH_SLICE_TYPE_SI_ONLY   9        // SI (SI slice)

//Appendix E. Table E-1  Meaning of sample aspect ratio indicator
#define SAR_Unspecified  0           // Unspecified
#define SAR_1_1        1             //  1:1
#define SAR_12_11      2             // 12:11
#define SAR_10_11      3             // 10:11
#define SAR_16_11      4             // 16:11
#define SAR_40_33      5             // 40:33
#define SAR_24_11      6             // 24:11
#define SAR_20_11      7             // 20:11
#define SAR_32_11      8             // 32:11
#define SAR_80_33      9             // 80:33
#define SAR_18_11     10             // 18:11
#define SAR_15_11     11             // 15:11
#define SAR_64_33     12             // 64:33
#define SAR_160_99    13             // 160:99
// 14..254           Reserved
#define SAR_Extended      255        // Extended_SAR

//7.4.3.1 Table 7-7 modification_of_pic_nums_idc operations for modification of reference picture lists
#define RPLR_IDC_ABS_DIFF_ADD       0
#define RPLR_IDC_ABS_DIFF_SUBTRACT  1
#define RPLR_IDC_LONG_TERM          2
#define RPLR_IDC_END                3

//7.4.3.3 Table 7-9 Memory management control operation (memory_management_control_operation) values
#define MMCO_END                     0
#define MMCO_SHORT_TERM_UNUSED       1
#define MMCO_LONG_TERM_UNUSED        2
#define MMCO_SHORT_TERM_TO_LONG_TERM 3
#define MMCO_LONG_TERM_MAX_INDEX     4
#define MMCO_ALL_UNUSED              5
#define MMCO_CURRENT_TO_LONG_TERM    6

//7.4.2.4 Table 7-5 Meaning of primary_pic_type
#define AUD_PRIMARY_PIC_TYPE_I       0                // I
#define AUD_PRIMARY_PIC_TYPE_IP      1                // I, P
#define AUD_PRIMARY_PIC_TYPE_IPB     2                // I, P, B
#define AUD_PRIMARY_PIC_TYPE_SI      3                // SI
#define AUD_PRIMARY_PIC_TYPE_SISP    4                // SI, SP
#define AUD_PRIMARY_PIC_TYPE_ISI     5                // I, SI
#define AUD_PRIMARY_PIC_TYPE_ISIPSP  6                // I, SI, P, SP
#define AUD_PRIMARY_PIC_TYPE_ISIPSPB 7                // I, SI, P, SP, B

#define H264_PROFILE_BASELINE  66
#define H264_PROFILE_MAIN      77
#define H264_PROFILE_EXTENDED  88
#define H264_PROFILE_HIGH     100


#endif

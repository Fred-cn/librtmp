/*
 * h264bitstream - a library for reading and writing H.264 video
 * Copyright (C) 2005-2007 Auroras Entertainment, LLC
 * Copyright (C) 2008-2011 Avail-TVN
 *
 * Written by Alex Izvorski <aizvorski@gmail.com> and Alex Giladi <alex.giladi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _H264_STREAM_H
#define _H264_STREAM_H        1


#include <stdio.h>
#include <assert.h>

#include "h264_NalParse.h"
#include "bs.h"
#include "h264_sei.h"

#include <vector>
using std::vector;

/**
   Sequence Parameter Set
   @see 7.3.2.1 Sequence parameter set RBSP syntax
   @see read_seq_parameter_set_rbsp
   @see write_seq_parameter_set_rbsp
   @see debug_sps
*/
typedef struct
{
	int profile_idc;
	int constraint_set0_flag;
	int constraint_set1_flag;
	int constraint_set2_flag;
	int constraint_set3_flag;
	int constraint_set4_flag;
	int constraint_set5_flag;
	int reserved_zero_2bits;
	int level_idc;
	int seq_parameter_set_id;
	int chroma_format_idc;
	int separate_colour_plane_flag;
	int ChromaArrayType;
	int bit_depth_luma_minus8;
	int bit_depth_chroma_minus8;
	int qpprime_y_zero_transform_bypass_flag;
	int seq_scaling_matrix_present_flag;
	int seq_scaling_list_present_flag[8];
	int ScalingList4x4[6];
	int UseDefaultScalingMatrix4x4Flag[6];
	int ScalingList8x8[2];
	int UseDefaultScalingMatrix8x8Flag[2];
	int log2_max_frame_num_minus4;
	int pic_order_cnt_type;
	int log2_max_pic_order_cnt_lsb_minus4;
	int delta_pic_order_always_zero_flag;
	int offset_for_non_ref_pic;
	int offset_for_top_to_bottom_field;
	int num_ref_frames_in_pic_order_cnt_cycle;
	int offset_for_ref_frame[256];
	int max_num_ref_frames;
	int gaps_in_frame_num_value_allowed_flag;
	int pic_width_in_mbs_minus1;
	int pic_height_in_map_units_minus1;
	int frame_mbs_only_flag;
	int mb_adaptive_frame_field_flag;
	int direct_8x8_inference_flag;
	int frame_cropping_flag;
	int frame_crop_left_offset;
	int frame_crop_right_offset;
	int frame_crop_top_offset;
	int frame_crop_bottom_offset;
	int vui_parameters_present_flag;

	struct
	{
		int aspect_ratio_info_present_flag;
		int aspect_ratio_idc;
		int sar_width;
		int sar_height;
		int overscan_info_present_flag;
		int overscan_appropriate_flag;
		int video_signal_type_present_flag;
		int video_format;
		int video_full_range_flag;
		int colour_description_present_flag;
		int colour_primaries;
		int transfer_characteristics;
		int matrix_coefficients;
		int chroma_loc_info_present_flag;
		int chroma_sample_loc_type_top_field;
		int chroma_sample_loc_type_bottom_field;
		int timing_info_present_flag;
		int num_units_in_tick;
		int time_scale;
		int fixed_frame_rate_flag;
		int nal_hrd_parameters_present_flag;
		int vcl_hrd_parameters_present_flag;
		int low_delay_hrd_flag;
		int pic_struct_present_flag;
		int bitstream_restriction_flag;
		int motion_vectors_over_pic_boundaries_flag;
		int max_bytes_per_pic_denom;
		int max_bits_per_mb_denom;
		int log2_max_mv_length_horizontal;
		int log2_max_mv_length_vertical;
		int num_reorder_frames;
		int max_dec_frame_buffering;
	} vui;

	struct
	{
		int cpb_cnt_minus1;
		int bit_rate_scale;
		int cpb_size_scale;
		int bit_rate_value_minus1[32]; // up to cpb_cnt_minus1, which is <= 31
		int cpb_size_value_minus1[32];
		int cbr_flag[32];
		int initial_cpb_removal_delay_length_minus1;
		int cpb_removal_delay_length_minus1;
		int dpb_output_delay_length_minus1;
		int time_offset_length;
	} hrd;

} sps_t;


/**
   Picture Parameter Set
   @see 7.3.2.2 Picture parameter set RBSP syntax
   @see read_pic_parameter_set_rbsp
   @see write_pic_parameter_set_rbsp
   @see debug_pps
*/
typedef struct
{
	int pic_parameter_set_id;
	int seq_parameter_set_id;
	int entropy_coding_mode_flag;
	int pic_order_present_flag; // 2005版本为此字段名 保留，不影响库本身write的编译，但实际不使用
	int bottom_field_pic_order_in_frame_present_flag; // 2013版本为此字段名
	int num_slice_groups_minus1;
	int slice_group_map_type;
	int run_length_minus1[8]; // up to num_slice_groups_minus1, which is <= 7 in Baseline and Extended, 0 otheriwse
	int top_left[8];
	int bottom_right[8];
	int slice_group_change_direction_flag;
	int slice_group_change_rate_minus1;
	int pic_size_in_map_units_minus1;
	int slice_group_id_bytes;
	vector<int> slice_group_id; // FIXME what size?
	int num_ref_idx_l0_active_minus1;
	int num_ref_idx_l1_active_minus1;
	int weighted_pred_flag;
	int weighted_bipred_idc;
	int pic_init_qp_minus26;
	int pic_init_qs_minus26;
	int chroma_qp_index_offset;
	int deblocking_filter_control_present_flag;
	int constrained_intra_pred_flag;
	int redundant_pic_cnt_present_flag;

	// see if we carry any of the optional headers
	int _more_rbsp_data_present;

	int transform_8x8_mode_flag;
	int pic_scaling_matrix_present_flag;
	int pic_scaling_list_present_flag[8];
	int* ScalingList4x4[6];
	int UseDefaultScalingMatrix4x4Flag[6];
	int* ScalingList8x8[2];
	int UseDefaultScalingMatrix8x8Flag[2];
	int second_chroma_qp_index_offset;
} pps_t;

// predictive weight table
typedef struct
{
	int luma_log2_weight_denom;
	int chroma_log2_weight_denom;
	int luma_weight_l0_flag[64];
	int luma_weight_l0[64];
	int luma_offset_l0[64];
	int chroma_weight_l0_flag[64];
	int chroma_weight_l0[64][2];
	int chroma_offset_l0[64][2];
	int luma_weight_l1_flag[64];
	int luma_weight_l1[64];
	int luma_offset_l1[64];
	int chroma_weight_l1_flag[64];
	int chroma_weight_l1[64][2];
	int chroma_offset_l1[64][2];
} pwt_t;

// ref pic list modification
typedef struct
{
	int modification_of_pic_nums_idc;
	int abs_diff_pic_num_minus1;
	int long_term_pic_num;
} rplm_tt;

typedef struct
{
	int ref_pic_list_modification_flag_l0;
	int ref_pic_list_modification_flag_l1;

	vector<rplm_tt> rplm;
} rplm_t;

// decoded ref pic marking
typedef struct
{
	int memory_management_control_operation;
	int difference_of_pic_nums_minus1;
	int long_term_pic_num;
	int long_term_frame_idx;
	int max_long_term_frame_idx_plus1;
} drpm_tt;
typedef struct
{
	int no_output_of_prior_pics_flag;
	int long_term_reference_flag;
	int adaptive_ref_pic_marking_mode_flag;

	vector<drpm_tt> drpm;
} drpm_t;

/**
  Slice Header
  @see 7.3.3 Slice header syntax
  @see read_slice_header_rbsp
  @see write_slice_header_rbsp
  @see debug_slice_header_rbsp
*/
typedef struct
{
	int read_slice_type; // see if we only read slice type and return

	int first_mb_in_slice;
	int slice_type;
	int pic_parameter_set_id;
	int colour_plane_id;
	int frame_num_bytes;
	int frame_num;
	int field_pic_flag;
	int bottom_field_flag;
	int idr_pic_id;
	int pic_order_cnt_lsb_bytes;
	int pic_order_cnt_lsb;
	int delta_pic_order_cnt_bottom;
	int delta_pic_order_cnt[2];
	int redundant_pic_cnt;
	int direct_spatial_mv_pred_flag;
	int num_ref_idx_active_override_flag;
	int num_ref_idx_l0_active_minus1;
	int num_ref_idx_l1_active_minus1;
	int cabac_init_idc;
	int slice_qp_delta;
	int sp_for_switch_flag;
	int slice_qs_delta;
	int disable_deblocking_filter_idc;
	int slice_alpha_c0_offset_div2;
	int slice_beta_offset_div2;
	int slice_group_change_cycle_bytes;
	int slice_group_change_cycle;

	pwt_t pwt;
	rplm_t rplm;
	drpm_t drpm;
} slice_header_t;


/**
   Access unit delimiter
   @see 7.3.1 NAL unit syntax
   @see read_nal_unit
   @see write_nal_unit
   @see debug_nal
*/
typedef struct
{
	int primary_pic_type;
} aud_t;

/**
   Network Abstraction Layer (NAL) unit
   @see 7.3.1 NAL unit syntax
   @see read_nal_unit
   @see write_nal_unit
   @see debug_nal
*/
typedef struct
{
	int forbidden_zero_bit;
	int nal_ref_idc;
	int nal_unit_type;
	void* parsed; // FIXME
	int sizeof_parsed;

	//uint8_t* rbsp_buf;
	//int rbsp_size;
} nal_t;

typedef struct
{
	int _is_initialized;
	int sps_id;
	int initial_cpb_removal_delay;
	int initial_cpb_delay_offset;
} sei_buffering_t;

typedef struct
{
	int clock_timestamp_flag;
	int ct_type;
	int nuit_field_based_flag;
	int counting_type;
	int full_timestamp_flag;
	int discontinuity_flag;
	int cnt_dropped_flag;
	int n_frames;

	int seconds_value;
	int minutes_value;
	int hours_value;

	int seconds_flag;
	int minutes_flag;
	int hours_flag;

	int time_offset;
} picture_timestamp_t;

typedef struct
{
	int _is_initialized;
	int cpb_removal_delay;
	int dpb_output_delay;
	int pic_struct;
	picture_timestamp_t clock_timestamps[3]; // 3 is the maximum possible value
} sei_picture_timing_t;


typedef struct
{
	int rbsp_size;
	uint8_t* rbsp_buf;
} slice_data_rbsp_t;




/**
   H264 stream
   Contains data structures for all NAL types that can be handled by this library.
   When reading, data is read into those, and when writing it is written from those.
   The reason why they are all contained in one place is that some of them depend on others, we need to
   have all of them available to read or write correctly.
 */
typedef struct
{
	nal_t* nal;
	sps_t* sps;
	pps_t* pps;
	aud_t* aud;
	sei_t* sei; //This is a TEMP pointer at whats in h->seis...    
	int num_seis;
	slice_header_t* sh;
	slice_data_rbsp_t* slice_data;

	sps_t* sps_table[32];
	pps_t* pps_table[256];
	sei_t** seis;
	videoinfo_t* info;
} h264_stream_t;


#ifdef __cplusplus
extern "C" {
#endif

	h264_stream_t* h264_new();
	void h264_free(h264_stream_t* h);

	int find_nal_unit(uint8_t* buf, int size, int* nal_start, int* nal_end);

	int rbsp_to_nal(const uint8_t* rbsp_buf, const int* rbsp_size, uint8_t* nal_buf, int* nal_size);
	int nal_to_rbsp(const int nal_header_size, const uint8_t* nal_buf, int* nal_size, uint8_t* rbsp_buf, int* rbsp_size);

	int read_nal_unit(h264_stream_t* h, uint8_t* buf, int size);
	int peek_nal_unit(h264_stream_t* h, uint8_t* buf, int size);

	void read_seq_parameter_set_rbsp(h264_stream_t* h, bs_t* b);
	void read_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList, int useDefaultScalingMatrixFlag);
	void read_vui_parameters(h264_stream_t* h, bs_t* b);
	void read_hrd_parameters(h264_stream_t* h, bs_t* b);

	void read_pic_parameter_set_rbsp(h264_stream_t* h, bs_t* b);

	void read_sei_rbsp(h264_stream_t* h, bs_t* b);
	void read_sei_message(h264_stream_t* h, bs_t* b);
	void read_access_unit_delimiter_rbsp(h264_stream_t* h, bs_t* b);
	void read_end_of_seq_rbsp(h264_stream_t* h, bs_t* b);
	void read_end_of_stream_rbsp(h264_stream_t* h, bs_t* b);
	void read_filler_data_rbsp(h264_stream_t* h, bs_t* b);

	void read_slice_layer_rbsp(h264_stream_t* h, bs_t* b);
	void read_rbsp_slice_trailing_bits(h264_stream_t* h, bs_t* b);
	void read_rbsp_trailing_bits(h264_stream_t* h, bs_t* b);
	void read_slice_header(h264_stream_t* h, bs_t* b);
	void read_ref_pic_list_modification(h264_stream_t* h, bs_t* b);
	void read_pred_weight_table(h264_stream_t* h, bs_t* b);
	void read_dec_ref_pic_marking(h264_stream_t* h, bs_t* b);

	int more_rbsp_trailing_data(h264_stream_t* h, bs_t* b);

	int is_slice_type(int slice_type, int cmp_type);

	int write_nal_unit(h264_stream_t* h, uint8_t* buf, int size);

	void write_seq_parameter_set_rbsp(h264_stream_t* h, bs_t* b);
	void write_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList, int useDefaultScalingMatrixFlag);
	void write_vui_parameters(h264_stream_t* h, bs_t* b);
	void write_hrd_parameters(h264_stream_t* h, bs_t* b);

	void write_pic_parameter_set_rbsp(h264_stream_t* h, bs_t* b);

	void write_sei_rbsp(h264_stream_t* h, bs_t* b);
	void write_sei_message(h264_stream_t* h, bs_t* b);
	void write_access_unit_delimiter_rbsp(h264_stream_t* h, bs_t* b);
	void write_end_of_seq_rbsp(h264_stream_t* h, bs_t* b);
	void write_end_of_stream_rbsp(h264_stream_t* h, bs_t* b);
	void write_filler_data_rbsp(h264_stream_t* h, bs_t* b);

	void write_slice_layer_rbsp(h264_stream_t* h, bs_t* b);
	void write_rbsp_slice_trailing_bits(h264_stream_t* h, bs_t* b);
	void write_rbsp_trailing_bits(h264_stream_t* h, bs_t* b);
	void write_slice_header(h264_stream_t* h, bs_t* b);
	void write_ref_pic_list_modification(h264_stream_t* h, bs_t* b);
	void write_pred_weight_table(h264_stream_t* h, bs_t* b);
	void write_dec_ref_pic_marking(h264_stream_t* h, bs_t* b);

	void debug_sps(sps_t* sps);
	void debug_pps(pps_t* pps);
	void debug_slice_header(slice_header_t* sh);
	void debug_nal(h264_stream_t* h, nal_t* nal);

	void debug_bytes(uint8_t* buf, int len);

	void read_sei_payload(h264_stream_t* h, bs_t* b, int payloadType, int payloadSize);
	void write_sei_payload(h264_stream_t* h, bs_t* b, int payloadType, int payloadSize);


#ifdef __cplusplus
}
#endif

#endif

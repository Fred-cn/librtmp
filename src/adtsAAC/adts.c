//
//  adts.c
//  AdtsTest
//
//  Created by zg.shao on 15/4/9.
//  Copyright (c) 2015年 shaozg.cn. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "adts.h"


void set_fixed_header(adts_fixed_header *header) {
    header->syncword                 = 0xFF;// 代表着一个帧的开始
    header->id                       = 0;// MPEG Version: 0 for MPEG-4, 1 for MPEG-2
    header->layer                    = 0;// always 00
    header->protection_absent        = 1;// 表示是否误码校验
    header->profile                  = 1;// 表示使用哪个级别的AAC, 有些芯片只支持AAC LC.
    header->sampling_frequency_index = 4;// 表示使用的采样率的下标
    header->private_bit              = 0;
    header->channel_configuration    = 2;// 表示声道数
    header->original_copy            = 0;
    header->home                     = 0;
}

void set_variable_header(adts_variable_header *header, const int aac_raw_data_length) {
    header->copyright_identification_bit       = 0;
    header->copyright_identification_start     = 0;
    header->aac_frame_length                   = aac_raw_data_length + 7;
    header->adts_buffer_fullness               = 0x7f;
    header->number_of_raw_data_blocks_in_frame = 2;
}


// decode the ADTS.
// @see aac-iso-13818-7.pdf, page 26
//      6.2 Audio Data Transport Stream, ADTS
// @see https://github.com/ossrs/srs/issues/212#issuecomment-64145885
// byte_alignment()

// adts_fixed_header:
//      12bits syncword,
//      16bits left.
// adts_variable_header:
//      28bits
//      12+16+28=56bits
// adts_error_check:
//      16bits if protection_absent
//      56+16=72bits
// if protection_absent:
//      require(7bytes)=56bits
// else
//      require(9bytes)=72bits
void get_fixed_header0(const unsigned char *buff, adts_fixed_header *header) {
    unsigned long long adts = 0;
    const unsigned char *p = buff;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++;


    header->syncword                 = (adts >> 44) & 0xfff;
    header->id                       = (adts >> 43) & 0x01;
    header->layer                    = (adts >> 41) & 0x03;
    header->protection_absent        = (adts >> 40) & 0x01;
    header->profile                  = (adts >> 38) & 0x03;
    header->sampling_frequency_index = (adts >> 34) & 0x0f;
    header->private_bit              = (adts >> 33) & 0x01;
    header->channel_configuration    = (adts >> 30) & 0x07;
    header->original_copy            = (adts >> 29) & 0x01;
    header->home                     = (adts >> 28) & 0x01;
}

void get_fixed_header(const unsigned char *buff, adts_fixed_header *header) {
    unsigned short adts = 0;
    unsigned short adts2 = 0;
    const unsigned char *p = buff;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; 

    adts2 |= *p ++; adts2 <<= 8;
    adts2 |= *p ++; 
    
    header->syncword                 = (adts >> 4);
    header->id                       = (adts >> 3) & 0x01;
    header->layer                    = (adts >> 1) & 0x03;
    header->protection_absent        = (adts) & 0x01;

    header->profile                  = (adts2 >> 14) & 0x03;
    header->sampling_frequency_index = (adts2 >> 10) & 0x0f;
    header->private_bit              = (adts2 >> 9) & 0x01;
    header->channel_configuration    = (adts2 >> 6) & 0x07;
    header->original_copy            = (adts2 >> 5) & 0x01;
    header->home                     = (adts2 >> 4) & 0x01;

}

void get_variable_header(const unsigned char buff[7], adts_variable_header *header) {
    unsigned long long adts = 0;
    adts  = buff[0]; adts <<= 8;
    adts |= buff[1]; adts <<= 8;
    adts |= buff[2]; adts <<= 8;
    adts |= buff[3]; adts <<= 8;
    adts |= buff[4]; adts <<= 8;
    adts |= buff[5]; adts <<= 8;
    adts |= buff[6];
    
    header->copyright_identification_bit    = (adts >> 27) & 0x01;
    header->copyright_identification_start  = (adts >> 26) & 0x01;
    header->aac_frame_length                = (adts >> 13) & ((int)pow(2.0, 14) - 1);
    header->adts_buffer_fullness            = (adts >> 2 ) & ((int)pow(2.0, 11) - 1);
    header->number_of_raw_data_blocks_in_frame = adts & 0x03;
}

void convert_adts_header2int64(const adts_fixed_header *fixed_header, const adts_variable_header *variable_header, unsigned long long *header) {
    uint64_t ret_value = 0;
    ret_value = fixed_header->syncword;
    ret_value <<= 1;
    ret_value |= fixed_header->id;
    ret_value <<= 2;
    ret_value |= fixed_header->layer;
    ret_value <<= 1;
    ret_value |= (fixed_header->protection_absent) & 0x01;
    ret_value <<= 2;
    ret_value |= fixed_header->profile;
    ret_value <<= 4;
    ret_value |= (fixed_header->sampling_frequency_index) & 0x0f;
    ret_value <<= 1;
    ret_value |= (fixed_header->private_bit) & 0x01;
    ret_value <<= 3;
    ret_value |= (fixed_header->channel_configuration) & 0x07;
    ret_value <<= 1;
    ret_value |= (fixed_header->original_copy) & 0x01;
    ret_value <<= 1;
    ret_value |= (fixed_header->home) & 0x01;
    
    ret_value <<= 1;
    ret_value |= (variable_header->copyright_identification_bit) & 0x01;
    ret_value <<= 1;
    ret_value |= (variable_header->copyright_identification_start) & 0x01;
    ret_value <<= 13;
    ret_value |= (variable_header->aac_frame_length) & ((int)pow(2.0, 13) - 1);
    ret_value <<= 11;
    ret_value |= (variable_header->adts_buffer_fullness) & ((int)pow(2.0, 11) - 1);
    ret_value <<= 2;
    ret_value |= (variable_header->number_of_raw_data_blocks_in_frame) & ((int)pow(2.0, 2) - 1);
    
    *header = ret_value;
}

void convert_adts_header2char(const adts_fixed_header *fixed_header, const adts_variable_header *variable_header, unsigned char buffer[7]) {
    uint64_t value = 0;
    convert_adts_header2int64(fixed_header, variable_header, &value);
    
    buffer[0] = (value >> 48) & 0xff;
    buffer[1] = (value >> 40) & 0xff;
    buffer[2] = (value >> 32) & 0xff;
    buffer[3] = (value >> 24) & 0xff;
    buffer[4] = (value >> 16) & 0xff;
    buffer[5] = (value >> 8) & 0xff;
    buffer[6] = (value) & 0xff;
}

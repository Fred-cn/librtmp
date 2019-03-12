/*
The MIT License (MIT)

Copyright (c) 2013-2016 SRS(ossrs)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <srs_protocol_utility.hpp>

// for srs-librtmp, @see https://github.com/ossrs/srs/issues/213
#ifndef _WIN32
#include <unistd.h>
#endif

#include <stdlib.h>
#include <sstream>
using namespace std;

#include <srs_kernel_log.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_kernel_buffer.hpp>
#include <srs_rtmp_stack.hpp>
#include <srs_kernel_codec.hpp>
#include <srs_kernel_consts.hpp>
#include <srs_rtmp_stack.hpp>
#include <srs_protocol_io.hpp>

/**
 * resolve the vhost in query string
 * @pram vhost, update the vhost if query contains the vhost.
 * @param app, may contains the vhost in query string format:
 *   app?vhost=request_vhost
 *   app...vhost...request_vhost
 * @param param, the query, for example, ?vhost=xxx
 */
void srs_vhost_resolve(string& vhost, string& app, string& param)
{
    // get original param
    size_t pos = 0;
    if ((pos = app.find("?")) != std::string::npos) {
        param = app.substr(pos);
    }
    
    // filter tcUrl
    app = srs_string_replace(app, ",", "?");
    app = srs_string_replace(app, "...", "?");
    app = srs_string_replace(app, "&&", "?");
    app = srs_string_replace(app, "=", "?");
    
    if ((pos = app.find("?")) != std::string::npos) {
        std::string query = app.substr(pos + 1);
        app = app.substr(0, pos);
        
        if ((pos = query.find("vhost?")) != std::string::npos) {
            query = query.substr(pos + 6);
            if (!query.empty()) {
                vhost = query;
            }
            if ((pos = vhost.find("?")) != std::string::npos) {
                vhost = vhost.substr(0, pos);
            }
        }
    }
    
    /* others */
}

void srs_discovery_tc_url(
    string tcUrl, 
    string& schema, string& host, string& vhost, 
    string& app, int& port, string& param
) {
    size_t pos = std::string::npos;
    std::string url = tcUrl;
    
    if ((pos = url.find("://")) != std::string::npos) {
        schema = url.substr(0, pos);
        url = url.substr(schema.length() + 3);
        srs_info("discovery schema=%s", schema.c_str());
    }
    
    if ((pos = url.find("/")) != std::string::npos) {
        host = url.substr(0, pos);
        url = url.substr(host.length() + 1);
        srs_info("discovery host=%s", host.c_str());
    }

    port = SRS_CONSTS_RTMP_DEFAULT_PORT;
    if ((pos = host.find(":")) != std::string::npos) {
        srs_parse_hostport(host, host, port);
        srs_info("discovery host=%s, port=%s", host.c_str(), port.c_str());
    }

    if (url.empty()) {
        app = SRS_CONSTS_RTMP_DEFAULT_APP;
    } else {
        app = url;
    }

    vhost = host;
    srs_vhost_resolve(vhost, app, param);
}

void srs_parse_query_string(string q, map<string,string>& query)
{
    // query string flags.
    static vector<string> flags;
    if (flags.empty()) {
        flags.push_back("=");
        flags.push_back(",");
        flags.push_back("&&");
        flags.push_back("&");
        flags.push_back(";");
    }
    
    vector<string> kvs = srs_string_split(q, flags);
    for (int i = 0; i < (int)kvs.size(); i+=2) {
        string k = kvs.at(i);
        string v = (i < (int)kvs.size() - 1)? kvs.at(i+1):"";
        
        query[k] = v;
    }
}

void srs_random_generate(char* bytes, int size)
{
    static bool _random_initialized = false;
    if (!_random_initialized) {
        srand(0);
        _random_initialized = true;
        srs_trace("srand initialized the random.");
    }
    
    for (int i = 0; i < size; i++) {
        // the common value in [0x0f, 0xf0]
        bytes[i] = 0x0f + (rand() % (256 - 0x0f - 0x0f));
    }
}

string srs_generate_tc_url(string ip, string vhost, string app, int port, string param)
{
    string tcUrl = "rtmp://";
    
    if (vhost == SRS_CONSTS_RTMP_DEFAULT_VHOST) {
        tcUrl += ip;
    } else {
        tcUrl += vhost;
    }
    
    if (port != SRS_CONSTS_RTMP_DEFAULT_PORT) {
        tcUrl += ":";
        tcUrl += srs_int2str(port);
    }
    
    tcUrl += "/";
    tcUrl += app;
    if (!param.empty()) {
        tcUrl += "?" + param;
    }
    
    return tcUrl;
}

string srs_generate_normal_tc_url(string ip, string vhost, string app, int port)
{
    return "rtmp://" + vhost + ":" + srs_int2str(port) + "/" + app;
}

string srs_generate_via_tc_url(string ip, string vhost, string app, int port)
{
    return "rtmp://" + ip + ":" + srs_int2str(port) + "/" + vhost + "/" + app;
}

string srs_generate_vis_tc_url(string ip, string vhost, string app, int port)
{
    return "rtmp://" + ip + ":" + srs_int2str(port) + "/" + app;
}

/**
* compare the memory in bytes.
*/
bool srs_bytes_equals(void* pa, void* pb, int size)
{
    u_int8_t* a = (u_int8_t*)pa;
    u_int8_t* b = (u_int8_t*)pb;
    
    if (!a && !b) {
        return true;
    }
    
    if (!a || !b) {
        return false;
    }
    
    for(int i = 0; i < size; i++){
        if(a[i] != b[i]){
            return false;
        }
    }

    return true;
}

template<typename T>
int srs_do_rtmp_create_msg(char type, u_int32_t timestamp, char* data, int size, int stream_id, T** ppmsg)
{
    int ret = ERROR_SUCCESS;
    
    *ppmsg = NULL;
    T* msg = NULL;
    
    if (type == SrsCodecFlvTagAudio) {
        SrsMessageHeader header;
        header.initialize_audio(size, timestamp, stream_id);
        
        msg = new T();
        if ((ret = msg->create(&header, data, size)) != ERROR_SUCCESS) {
            srs_freep(msg);
            return ret;
        }
    } else if (type == SrsCodecFlvTagVideo) {
        SrsMessageHeader header;
        header.initialize_video(size, timestamp, stream_id);
        
        msg = new T();
        if ((ret = msg->create(&header, data, size)) != ERROR_SUCCESS) {
            srs_freep(msg);
            return ret;
        }
    } else if (type == SrsCodecFlvTagScript) {
        SrsMessageHeader header;
        header.initialize_amf0_script(size, stream_id);
        
        msg = new T();
        if ((ret = msg->create(&header, data, size)) != ERROR_SUCCESS) {
            srs_freep(msg);
            return ret;
        }
    } else {
        ret = ERROR_STREAM_CASTER_FLV_TAG;
        srs_error("rtmp unknown tag type=%#x. ret=%d", type, ret);
        return ret;
    }

    *ppmsg = msg;

    return ret;
}

int srs_rtmp_create_msg(char type, u_int32_t timestamp, char* data, int size, int stream_id, SrsSharedPtrMessage** ppmsg)
{
    int ret = ERROR_SUCCESS;

    // only when failed, we must free the data.
    if ((ret = srs_do_rtmp_create_msg(type, timestamp, data, size, stream_id, ppmsg)) != ERROR_SUCCESS) {
        srs_freepa(data);
        return ret;
    }

    return ret;
}

int srs_rtmp_create_msg(char type, u_int32_t timestamp, char* data, int size, int stream_id, SrsCommonMessage** ppmsg)
{
    int ret = ERROR_SUCCESS;
    
    // only when failed, we must free the data.
    if ((ret = srs_do_rtmp_create_msg(type, timestamp, data, size, stream_id, ppmsg)) != ERROR_SUCCESS) {
        srs_freepa(data);
        return ret;
    }
    
    return ret;
}

string srs_generate_stream_url(string vhost, string app, string stream)
{
    std::string url = "";
    
    if (SRS_CONSTS_RTMP_DEFAULT_VHOST != vhost){
        url += vhost;
    }
    url += "/";
    url += app;
    url += "/";
    url += stream;

    return url;
}

void srs_parse_rtmp_url(string url, string& tcUrl, string& stream)
{
    size_t pos;
    
    if ((pos = url.rfind("/")) != string::npos) {
        stream = url.substr(pos + 1);
        tcUrl = url.substr(0, pos);
    } else {
        tcUrl = url;
    }
}

string srs_generate_rtmp_url(string server, int port, string vhost, string app, string stream)
{
    std::stringstream ss;
    
    ss << "rtmp://" << server << ":" << std::dec << port << "/" << app;
    
    // when default or server is vhost, donot specifies the vhost in params.
    if (SRS_CONSTS_RTMP_DEFAULT_VHOST != vhost && server != vhost) {
        ss << "...vhost..." << vhost;
    }
    
    if (!stream.empty()) {
        ss << "/" << stream;
    }
    
    return ss.str();
}

int srs_write_large_iovs(ISrsProtocolReaderWriter* skt, iovec* iovs, int size, ssize_t* pnwrite)
{
    int ret = ERROR_SUCCESS;
    
    // the limits of writev iovs.
    // for srs-librtmp, @see https://github.com/ossrs/srs/issues/213
#ifndef _WIN32
    // for linux, generally it's 1024.
    static int limits = (int)sysconf(_SC_IOV_MAX);
#else
    static int limits = 1024;
#endif
    
    // send in a time.
    if (size < limits) {
        if ((ret = skt->writev(iovs, size, pnwrite)) != ERROR_SUCCESS) {
            if (!srs_is_client_gracefully_close(ret)) {
                srs_error("send with writev failed. ret=%d", ret);
            }
            return ret;
        }
        return ret;
    }
    
    // send in multiple times.
    int cur_iov = 0;
    while (cur_iov < size) {
        int cur_count = srs_min(limits, size - cur_iov);
        if ((ret = skt->writev(iovs + cur_iov, cur_count, pnwrite)) != ERROR_SUCCESS) {
            if (!srs_is_client_gracefully_close(ret)) {
                srs_error("send with writev failed. ret=%d", ret);
            }
            return ret;
        }
        cur_iov += cur_count;
    }
    
    return ret;
}

string srs_join_vector_string(vector<string>& vs, string separator)
{
    string str = "";
    
    for (int i = 0; i < (int)vs.size(); i++) {
        str += vs.at(i);
        if (i != (int)vs.size() - 1) {
            str += separator;
        }
    }
    
    return str;
}

bool srs_is_ipv4(string domain)
{
    for (int i = 0; i < (int)domain.length(); i++) {
        char ch = domain.at(i);
        if (ch == '.') {
            continue;
        }
        if (ch >= '0' && ch <= '9') {
            continue;
        }
        
        return false;
    }
    
    return true;
}


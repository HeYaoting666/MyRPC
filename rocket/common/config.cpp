//
// Created by 20771 on 2024/1/10.
//

#include "config.h"

namespace rocket {

static Config* g_config = nullptr;

Config *Config::GetGlobalConfig() {
    return g_config;
}

Config::Config():
    m_log_level("DEBUG"),
    m_log_max_file_size(1000000000),
    m_log_sync_interval(500),
    m_port(0),
    m_io_threads_nums(1),
    m_xml_document(nullptr) { }

/*
 *  xml 文件结构 有三个子结点
 *  <root>
 *      <log>
 *      <log>
 *
 *      <server>
 *      <server>
 *
 *      <stubs>
 *      <stubs>
 *  <root>
 */
Config::Config(const char *xmlfile) {
    m_xml_document = new TiXmlDocument();
    bool ret = m_xml_document->LoadFile(xmlfile);
    if(!ret) {
        printf("Start rocket server error, failed to read config file %s, error info[%s] \n", xmlfile, m_xml_document->ErrorDesc());
        exit(0);
    }

    READ_XML_NODE(root, m_xml_document) // 获取 root_node
    READ_XML_NODE(log, root_node)       // 获取 log_node
    READ_XML_NODE(server, root_node)    // 获取 server_node

    // 获取 log 配置信息
    READ_STR_FROM_XML_NODE(log_level, log_node)
    READ_STR_FROM_XML_NODE(log_file_name, log_node)
    READ_STR_FROM_XML_NODE(log_file_path, log_node)
    READ_STR_FROM_XML_NODE(log_max_file_size, log_node)
    READ_STR_FROM_XML_NODE(log_sync_interval, log_node)
    m_log_level = log_level_str;
    m_log_file_name = log_file_name_str;
    m_log_file_path = log_file_path_str;
    m_log_max_file_size = std::stoi(log_max_file_size_str) ;
    m_log_sync_interval = std::stoi(log_sync_interval_str);

    printf("LOG -- CONFIG LEVEL[%s], FILE_NAME[%s],FILE_PATH[%s] MAX_FILE_SIZE[%d B], SYNC_INTERVAL[%d ms]\n",
           m_log_level.c_str(), m_log_file_name.c_str(), m_log_file_path.c_str(), m_log_max_file_size, m_log_sync_interval);

    // 获取 server 配置信息
    READ_STR_FROM_XML_NODE(port, server_node)
    READ_STR_FROM_XML_NODE(io_threads, server_node)
    m_port = std::stoi(port_str);
    m_io_threads_nums = std::stoi(io_threads_str);

}

Config::~Config() {
    if (m_xml_document) {
        delete m_xml_document;
        m_xml_document = nullptr;
    }
}

void Config::SetGlobalConfig(const char* xmlfile) {
    if(!g_config) {
        if(!xmlfile)
            g_config = new Config();
        else
            g_config = new Config(xmlfile);
    }
}


}
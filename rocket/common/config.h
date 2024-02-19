//
// Created by 20771 on 2024/1/10.
//

#ifndef ROCKET_CONFIG_H
#define ROCKET_CONFIG_H

#include <string>
#include <tinyxml/tinyxml.h>

#define READ_XML_NODE(name, parent) \
TiXmlElement* name##_node = parent->FirstChildElement(#name); \
if (!name##_node) { \
  printf("Start rocket server error, failed to read node [%s]\n", #name); \
  exit(0); \
}                                   \

#define READ_STR_FROM_XML_NODE(name, parent) \
  TiXmlElement* name##_node = parent->FirstChildElement(#name); \
  if (!name##_node|| !name##_node->GetText()) { \
    printf("Start rocket server error, failed to read config file %s\n", #name); \
    exit(0); \
  } \
  std::string name##_str = std::string(name##_node->GetText()); \

namespace rocket {

class Config {
public:
    std::string m_log_level;
    std::string m_log_file_name;
    std::string m_log_file_path;
    int m_log_max_file_size;
    int m_log_sync_interval;   // 日志同步间隔，ms

    int m_port;
    int m_io_threads_nums;

    TiXmlDocument* m_xml_document;



public:
    static Config* GetGlobalConfig();
    static void SetGlobalConfig(const char* xmlfile);

private:
    explicit Config(const char* xmlfile);

    Config();

    ~Config();
};

}

#endif //ROCKET_CONFIG_H

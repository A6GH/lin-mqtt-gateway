#ifndef XML_PARSER_H
#define XML_PARSER_H

#include "lin_utils.h" // LIN_params
#include "mqtt_client.h" // MQTT_params
#include "libxml/parser.h" // libxml

int is_leaf(xmlNode*);
void parse_LIN_IDs(int*, xmlChar*);
void read_MQTT_values(xmlNode*, MQTT_params*);
MQTT_params *read_MQTT_params(char*);
void read_LIN_values(xmlNode*, LIN_params*);
LIN_params *read_LIN_params(char*);

#endif // XML_PARSER_H
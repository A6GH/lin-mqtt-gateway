#include <string.h> // strtol, strtok

#include "xml_parser.h"

int is_leaf(xmlNode *node)
{
	xmlNode *child = node->children;
	while(child)
	{
		if(child->type == XML_ELEMENT_NODE) return 0;
		child = child->next;
	}

	return 1;
}

void parse_LIN_IDs(int *ids, xmlChar *str)
{
	if(str == NULL)
		return;
	else if(xmlStrcmp(str,"all") == 0 || xmlStrcmp(str,"all/") == 0)
		for(int i = 0; i < NUM_LIN_IDS; ++i) ids[i] = 1;
	else{
		char d[4] = "/, ";
		char *token;
		int id;
		
		token = strtok(str, d);
		if(xmlStrcmp(token,"all") == 0){
			for(int i = 0; i < NUM_LIN_IDS; ++i) ids[i] = 1;
			while(token != NULL){
				token = strtok(NULL, d);
				id = strtol(token, NULL, 0);
				if(id != 0 && id < NUM_LIN_IDS)
					ids[id] = 0;
				else if(id == 0 && strcmp(token,"0") == 0)
					ids[id] = 0;
			}
		}
		else{
			while(token != NULL){
				id = strtol(token, NULL, 0);
				if(id != 0 && id < NUM_LIN_IDS)
					ids[id] = 1;
				else if(id == 0 && strcmp(token,"0") == 0)
					ids[id] = 1;
				token = strtok(NULL, d);
			}
		}
	}
	xmlFree(str);
}

void read_MQTT_values(xmlNode *node, MQTT_params *params)
{
	static int pt_num = 0, pq_num = 0, pl_num = 0, st_num = 0, sq_num = 0, sl_num = 0;
	xmlChar *token = NULL;
	
	while(node){
		if(node->type == XML_ELEMENT_NODE && is_leaf(node)){
			if(xmlStrcmp(node->parent->name,"broker") == 0){
				if(xmlStrcmp(node->name,"name") == 0) params->broker_name = xmlNodeGetContent(node);
				else if(xmlStrcmp(node->name,"address") == 0) params->broker_address = xmlNodeGetContent(node);
			}
			else if(xmlStrcmp(node->parent->name,"client") == 0){
				if(xmlStrcmp(node->name,"name") == 0) params->client_name = xmlNodeGetContent(node);
			}
			else if(xmlStrcmp(node->parent->parent->name,"publication") == 0){
				if(params->publication_topics == NULL){
					params->num_pt = (int)xmlChildElementCount(node->parent->parent);
					params->publication_topics = (char**)malloc(params->num_pt*sizeof(char*));
					params->p_qos_levels = (int*)malloc(params->num_pt*sizeof(int));
					params->p_lin_ids = (int**)malloc(params->num_pt*sizeof(int*));
					for(int i = 0; i < params->num_pt; ++i){
						params->p_lin_ids[i] = (int*)malloc(NUM_LIN_IDS*sizeof(int));
						memset(params->p_lin_ids[i], 0, NUM_LIN_IDS);
					}
				}
				if(params->num_pt != 0){
					if(xmlStrcmp(node->name,"name") == 0) params->publication_topics[pt_num++] = xmlNodeGetContent(node);
					else if(xmlStrcmp(node->name,"qos") == 0){
						token = xmlNodeGetContent(node);
						params->p_qos_levels[pq_num++] = strtol(token, NULL, 0);
						xmlFree(token);
						token = NULL;
					}
					else if(xmlStrcmp(node->name,"lin_id") == 0) parse_LIN_IDs(params->p_lin_ids[pl_num++], xmlNodeGetContent(node));
				}
			}
			else if(xmlStrcmp(node->parent->parent->name,"subscription") == 0){
				if(params->subscription_topics == NULL){
					params->num_st = (int)xmlChildElementCount(node->parent->parent);
					params->subscription_topics = (char**)malloc(params->num_st*sizeof(char*));
					params->s_qos_levels = (int*)malloc(params->num_st*sizeof(int));
					params->s_lin_ids = (int**)malloc(params->num_st*sizeof(int*));
					for(int i = 0; i < params->num_st; ++i){
						params->s_lin_ids[i] = (int*)malloc(NUM_LIN_IDS*sizeof(int));
						memset(params->s_lin_ids[i], 0, NUM_LIN_IDS);
					}
				}
				if(params->num_st != 0){
					if(xmlStrcmp(node->name,"name") == 0) params->subscription_topics[st_num++] = xmlNodeGetContent(node);
					else if(xmlStrcmp(node->name,"qos") == 0){
						token = xmlNodeGetContent(node);
						params->s_qos_levels[sq_num++] = strtol(token, NULL, 0);
						xmlFree(token);
						token = NULL;
					}
					else if(xmlStrcmp(node->name,"lin_id") == 0) parse_LIN_IDs(params->s_lin_ids[sl_num++], xmlNodeGetContent(node));
				}
			}
			else if(xmlStrcmp(node->name,"timeout") == 0){
				token = xmlNodeGetContent(node);
				params->timeout = strtol(token, NULL, 0);
				xmlFree(token);
				token = NULL;
			}
		}
		read_MQTT_values(node->children, params);
		node = node->next;
	}
}

MQTT_params *read_MQTT_params(char *file_path)
{
	xmlDoc *doc = NULL;
	xmlNode *node = NULL;
	doc = xmlReadFile(file_path, NULL, 0);
	if (doc == NULL){
		fprintf(stderr, "Could not parse the XML config file\n");
		return NULL;
	}
	
	MQTT_params *params = (MQTT_params*)malloc(sizeof(MQTT_params));
	*params = MQTT_params_initializer;
	
	node = xmlDocGetRootElement(doc)->children;
	
	while(node && (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name,"mqtt_params") != 0)){
		node = node->next;
	}
	node = node->children;
	
	read_MQTT_values(node, params);

	xmlFreeDoc(doc);
	xmlCleanupParser();
	
	return params;
}

void read_LIN_values(xmlNode *node, LIN_params *params)
{
	while(node){
		if(node->type == XML_ELEMENT_NODE && is_leaf(node))
		{
			if(xmlStrcmp(node->parent->name,"lin_params") == 0){
				if(xmlStrcmp(node->name,"device_path") == 0) params->device_path = xmlNodeGetContent(node);
				else if(xmlStrcmp(node->name,"module_path") == 0) params->module_path = xmlNodeGetContent(node);
				else if(xmlStrcmp(node->name,"baud_rate") == 0) params->baud_rate = strtol(xmlNodeGetContent(node), NULL, 0);
			}
		}
		read_LIN_values(node->children, params);
		node = node->next;
	}
}

LIN_params *read_LIN_params(char *file_path)
{
	xmlDoc *doc = NULL;
	xmlNode *node = NULL;
	doc = xmlReadFile(file_path, NULL, 0);
	if (doc == NULL){
		fprintf(stderr, "Could not parse the XML config file\n");
		return NULL;
	}
	
	LIN_params *params = (LIN_params*)malloc(sizeof(LIN_params));
	*params = LIN_params_initializer;
	
	node = xmlDocGetRootElement(doc)->children;
	
	while(node && (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name,"lin_params") != 0)){
		node = node->next;
	}
	node = node->children;
	
	read_LIN_values(node, params);

	xmlFreeDoc(doc);
	xmlCleanupParser();
	
	return params;
}
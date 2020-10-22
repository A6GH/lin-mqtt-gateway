#include <stdio.h> // printf
#include <stdlib.h> // free
#include <string.h> // strncpy, memset, sprintf

#include "mqtt_client.h"

extern int running;
extern int receive_frame(char*);
extern int get_num_LIN_IDs();
extern int rdy_to_send;

void free_MQTT_params(MQTT_params *params)
{
	free(params->broker_name);
	params->broker_name = NULL;
	free(params->broker_address);
	params->broker_address = NULL;
	free(params->client_name);
	params->client_name = NULL;
	for(int i = 0; i < params->num_pt; ++i){
		free(params->publication_topics[i]);
		params->publication_topics[i] = NULL;
	}
	free(params->publication_topics);
	params->publication_topics = NULL;
	for(int i = 0; i < params->num_st; ++i){
		free(params->subscription_topics[i]);
		params->subscription_topics[i] = NULL;
	}
	free(params->subscription_topics);
	params->subscription_topics = NULL;
	free(params->p_qos_levels);
	params->p_qos_levels = NULL;
	free(params->s_qos_levels);
	params->s_qos_levels = NULL;
	free(params->p_lin_ids);
	params->p_lin_ids = NULL;
	free(params->s_lin_ids);
	params->s_lin_ids = NULL;
	free(params);
	params = NULL;
}

// mode - 0 for pub, 1 for sub
char *get_LIN_IDs(MQTT_params *params, int mode, int i)
{
	char *ids;
	int nlids = get_num_LIN_IDs();
	int len = nlids*4+(nlids-1)*2+1;
	if(mode == 0){
		ids = (char*)calloc(len,sizeof(char));
		int sum = 0;
		for(int j = 0; j < nlids; ++j){
			if(params->p_lin_ids[i][j] == 1){
				sum++;
			}
		}
		if(sum == nlids){
			strncpy(ids,"all",3);
			return ids;
		}
		else{
			int k = 0;
			char hex[5]={'\0'};
			if(sum < nlids/2){
				for(int j = 0; j < nlids; ++j){
					if(params->p_lin_ids[i][j] == 1){
						sprintf(hex, "0x%02X", j);
						strncpy(ids+k,hex,4);
						k += 4;
						strncpy(ids+k,", ",2);
						k += 2;
					}
				}
			}
			else{
				strncpy(ids,"all/",3);
				k = 4;
				for(int j = 0; j < nlids; ++j){
					if(params->p_lin_ids[i][j] == 0){
						sprintf(hex, "0x%02X", j);
						strncpy(ids+k,hex,4);
						k += 4;
						strncpy(ids+k,", ",2);
						k += 2;
					}
				}
			}
			memset(ids+k-2,'\0',2);
			return ids;
		}
	}
	else if(mode == 1){
		ids = (char*)calloc(len,sizeof(char));
		int sum = 0;
		for(int j = 0; j < nlids; ++j){
			if(params->s_lin_ids[i][j] == 1){
				sum++;
			}
		}
		if(sum == nlids){
			strncpy(ids,"all",3);
			return ids;
		}
		else{
			int k = 0;
			char hex[5]={0};
			if(sum < nlids/2){
				for(int j = 0; j < nlids; ++j){
					if(params->s_lin_ids[i][j] == 1){
						sprintf(hex, "0x%02X", j);
						strncpy(ids+k,hex,4);
						k += 4;
						strncpy(ids+k,", ",2);
						k += 2;
					}
				}
			}
			else{
				strncpy(ids,"all/",3);
				k = 4;
				for(int j = 0; j < nlids; ++j){
					if(params->s_lin_ids[i][j] == 0){
						sprintf(hex, "0x%02X", j);
						strncpy(ids+k,hex,4);
						k += 4;
						strncpy(ids+k,", ",2);
						k += 2;
					}
				}
			}
			memset(ids+k-2,'\0',2);
			return ids;
		}
	}
	return NULL;
}

void print_MQTT_params(MQTT_params *params)
{
	printf("MQTT Params ------------------------------------------------\n");
	printf("Broker name:     %s\n", params->broker_name);
	printf("Broker address:  %s\n", params->broker_address);
	printf("Client name:     %s\n", params->client_name);
	int topic_padding = 20; // must be greater than 3
	printf("Publication topics:\n");
	for(int i = 0; i < params->num_pt; ++i){
		char *ids = get_LIN_IDs(params, 0, i);
		if(strlen(params->publication_topics[i]) <= topic_padding)
			printf("  %d) %-*s -> mapped to LIN IDs - %s\n", i+1, topic_padding, params->publication_topics[i], ids);
		else
			printf("  %d) %-*.*s... -> mapped to LIN IDs - %s\n", i+1, topic_padding-3, topic_padding-3, params->publication_topics[i], ids);
		free(ids);
	}
	printf("Subscription topics:\n");
	for(int i = 0; i < params->num_st; ++i){
		char *ids = get_LIN_IDs(params, 1, i);
		if(strlen(params->subscription_topics[i]) <= topic_padding)
			printf("  %d) %-*s -> mapped to LIN IDs - %s\n", i+1, topic_padding, params->subscription_topics[i], ids);
		else
			printf("  %d) %-*.*s... -> mapped to LIN IDs - %s\n", i+1, topic_padding-3, topic_padding-3, params->subscription_topics[i], ids);
		free(ids);
	}
	printf("Timeout: %ldus\n", params->timeout);
	printf("------------------------------------------------------------\n");
}

void connectionLost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	if (cause) printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		running = 0;
	}
}

int messageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
	MQTTAsync client = (MQTTAsync)context;
    printf("\nMessage arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n\n", message->payloadlen, (char*)message->payload);
	receive_frame((char*)message->payload);
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Disconnect failed, rc %d\n", response ? response->code : 0);
	disconnected = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	disconnected = 1;
}

void onSendFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Message with token value %d delivery failed, error code %d\n", response ? response->token, response->code : 0, 0);
}

void onSend(void* context, MQTTAsync_successData* response)
{
	printf("Message with token value %d delivery confirmed\n", response ? response->token : 0);
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response? response->code : 0);
	subscribe_count += 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded\n");
	subscribe_count += 1;
}

void onConnectFailure(void *context, MQTTAsync_failureData *response)
{
	printf("Connect failed, rc %d\n", response ? response->code : 0);
	running = 0;
}

void onConnect(void *context, MQTTAsync_successData *response)
{
	printf("Successful connection to MQTT broker\n");
	connected = 1;
}

void subscribe(void *context, MQTT_params* params)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;
	
	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client;
	
	for(int i = 0; i < params->num_st; ++i)
	{
		printf("Subscribing to topic %s for client %s using QoS %d\n", params->subscription_topics[i], params->client_name, params->s_qos_levels[i]);
		if(rc = MQTTAsync_subscribe(client, params->subscription_topics[i], params->s_qos_levels[i], &opts) != MQTTASYNC_SUCCESS){
			printf("Failed to start subscribe for topic %s, return code %d\n", params->subscription_topics[i], rc);
			memset(params->p_lin_ids[i], 0, get_num_LIN_IDs());
		}
	}
}

int is_LIN_ID_included_in_topic(char *payload, MQTT_params *params, int i)
{
	int check;
	char pl_copy[20];
	strncpy(pl_copy, payload, 20);
	
	char *id_str = strtok(pl_copy, "#");
	int id = (int)strtol(id_str, NULL, 16);
	if(id == 0 && strcmp(id_str, "000") != 0)
		check = 0;
	else if(params->p_lin_ids[i][id] == 1)
		check = 1;
	
	return check;
}

void publish(void *context, char *payload, MQTT_params *params)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;
	int check;

	opts.onSuccess = onSend;
	opts.onFailure = onSendFailure;
	opts.context = client;
	pubmsg.payload = payload;
	pubmsg.payloadlen = (int)strlen(payload);
	pubmsg.retained = 0;
	for(int i = 0; i < params->num_pt; ++i){
		check = is_LIN_ID_included_in_topic(payload, params, i);
		if(check == 1){
			pubmsg.qos = params->p_qos_levels[i];
			if ((rc = MQTTAsync_sendMessage(client, params->publication_topics[i], &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
				printf("Failed to publish %s to topic %s, return code %d\n", payload, params->publication_topics[i], rc);
			else
				printf("Successfully published %s to topic %s using QoS %d\n", payload, params->publication_topics[i], params->p_qos_levels[i]);
		}
	}
}
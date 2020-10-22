#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "MQTTAsync.h" // MQTT Async library

int connected; // MQTT connected to broker flag
int disconnected; // MQTT disconnected from broker flag
int subscribe_count; // number of successful and failed subscribe attempts

typedef struct{
	char *broker_name;
	char *broker_address;
	char *client_name;
	int num_pt; // number of publication topics
	int num_st; // number of subscription topics
	char **publication_topics;
	char **subscription_topics;
	int *p_qos_levels; // qos levels for publication topics
	int *s_qos_levels; // qos levels for subscription topics
	int **p_lin_ids; // lin ids for publication topics
	int **s_lin_ids; // lin ids for subscription topics
	long timeout; // in microseconds
} MQTT_params;

#define MQTT_params_initializer (MQTT_params){NULL, NULL, NULL, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, 0}

void free_MQTT_params(MQTT_params*);
char *get_LIN_IDs(MQTT_params*, int, int);
void print_MQTT_params(MQTT_params*);

void connectionLost(void*, char*);
int messageArrived(void*, char*, int, MQTTAsync_message*);

void onDisconnectFailure(void*, MQTTAsync_failureData*);
void onDisconnect(void*, MQTTAsync_successData*);

void onSendFailure(void*, MQTTAsync_failureData*);
void onSend(void*, MQTTAsync_successData*);

void onSubscribeFailure(void*, MQTTAsync_failureData*);
void onSubscribe(void*, MQTTAsync_successData*);

void onConnectFailure(void*, MQTTAsync_failureData*);
void onConnect(void*, MQTTAsync_successData*);

void subscribe(void*, MQTT_params*);
int is_LIN_ID_included_in_topic(char*, MQTT_params*, int);
void publish(void*, char*, MQTT_params*);

#endif // MQTT_CLIENT_H
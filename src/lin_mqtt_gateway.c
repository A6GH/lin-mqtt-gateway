#include <stdio.h> // printf, fprintf
#include <unistd.h> // usleep
#include <string.h> //strncpy
#include <sys/time.h> // timespec
#include <sys/select.h> // pselect
#include <pthread.h> // pthread for publishing
#include "lin_utils.h" // LIN_params
#include "mqtt_client.h" // MQTT_params, MQTTAsync
#include "xml_parser.h" // readLINParams, readMQTTParams

#define STDIN 0

extern int connected;
extern int disconnected;
extern int subscribe_count;
int running = 1; // thread synchronization flag

extern int load_module(LIN_params*);
extern int unload_module(LIN_params*);

int main(int argc, char *argv[])
{
	if(argc < 2){
		printf("Please include path to lin_mqtt_config.xml as an argument\n");
		return 1;
	}
	
	LIN_params *lin_params = read_LIN_params(argv[1]);
	MQTT_params *mqtt_params = read_MQTT_params(argv[1]);
	int rc;
	
	if(lin_params == NULL || mqtt_params == NULL){
		rc = 1;
		goto first_stage;
	}
	print_LIN_params(lin_params);
	print_MQTT_params(mqtt_params);
	printf("\n");
	
	// MODULE -----------------------------------------------------------------
	rc = load_module(lin_params);
	if(rc < 0){
		fprintf(stderr, "Could not load kernel module, return code %d\n", rc);
		goto first_stage;
	}
	
	// LIN --------------------------------------------------------------------
	rc = sllin_config_up(lin_params);
	if(rc < 0) goto second_stage;
	
	rc = open_socket();
	if(rc < 0) goto third_stage;
	
	rc = bind_socket(lin_params);
	if(rc < 0) goto fourth_stage;
	
	// MQTT -------------------------------------------------------------------
	MQTTAsync client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;

	if ((rc = MQTTAsync_create(&client, mqtt_params->broker_address, mqtt_params->client_name, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
		fprintf(stderr, "Failed to create client, return code %d\n", rc);
		goto fourth_stage;
	}

	if ((rc = MQTTAsync_setCallbacks(client, client, connectionLost, messageArrived, NULL)) != MQTTASYNC_SUCCESS)
	{
		fprintf(stderr, "Failed to set callbacks, return code %d\n", rc);
		goto fifth_stage;
	}

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		fprintf(stderr, "Failed to start connect, return code %d\n", rc);
		goto fifth_stage;
	}
	while (!connected && running) usleep(10000L);
	if(!running) goto fifth_stage;
	
	subscribe(client, mqtt_params);
	while(subscribe_count != mqtt_params->num_st);
	
	// creating LIN listener thread
	pthread_t pub_t;
	rc = pthread_create(&pub_t, NULL, send_frame, lin_params);
	if(rc != 0){
		fprintf(stderr, "Could not create LIN listener thread\n");
		goto fifth_stage;
	}
	
	printf("Enter q / Q to exit\n\n");
	
	
	// checking to see if user pressed q/Q or if there is data to be sent from LIN to MQTT
	struct timespec ts = {1, 0}; // 1 second
	fd_set fds;
	char payload[20];
	char ch = '\0';
	do{
		FD_ZERO(&fds);
		FD_SET(STDIN, &fds);
		rc = pselect(STDIN+1, &fds, NULL, NULL, &ts, NULL);
		if(rc > 0)
			ch = getchar();
		if(rdy_to_send){
			memset(payload, '\0', 20);
			strncpy(payload, out, 20);
			publish(client, payload, mqtt_params);
			rdy_to_send = 0;
		}
	}while((ch!='Q' && ch!='q') && running);
	
	running = 0;
	pthread_join(pub_t, NULL);
	
	disc_opts.onSuccess = onDisconnect;
	disc_opts.onFailure = onDisconnectFailure;
	if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		fprintf(stderr, "Failed to start disconnect, return code %d\n", rc);
		rc = 1;
		goto fifth_stage;
	}
 	while(!disconnected) usleep(10000L);
	
	rc = 0;

fifth_stage:
	MQTTAsync_destroy(&client);
fourth_stage:
	close_socket();
third_stage:
	sllin_config_down(lin_params);
second_stage:
	unload_module(lin_params);
first_stage:
	if(lin_params != NULL ) free_LIN_params(lin_params);
	if(mqtt_params != NULL) free_MQTT_params(mqtt_params);
	printf("Terminating..\n");
	return rc;
}


#include <stdio.h> // printf, fprintf, perror
#include <stdlib.h> // free
#include <fcntl.h> // open, O_NOCTTY
#include <unistd.h> // close, write

#include <sys/time.h> // timespec
#include <sys/select.h> // pselect
#include <sys/socket.h> // socket, SOCK_RAW
#include <sys/ioctl.h> // ioctl, TIOCSETD

#include <net/if.h> // SIOCGIFNAME, IFNAMSIZ, if_nametoindex 

#include <netlink/netlink.h> // nl_connect
#include <netlink/route/link.h> // rtnl_link_*, nl_cache
#include <netlink/socket.h> // nl_sock, nl_socket_alloc, nl_socket_free

#include <linux/can/raw.h> // CAN_RAW, CAN_MTU, CAN_MAX_DLEN, struct sockaddr_can, struct canfd_frame

#include "canutils_lib.h" // can-utils lib.h file // parse_canframe
#include "lin_utils.h" // SLLIN_LDISC, LIN_params, LIN_params_initializer, function prototypes

extern int running;

int tty; // global tty identifier
int s; // global socket identifier

int get_num_LIN_IDs()
{
	return NUM_LIN_IDS;
}

void free_LIN_params(LIN_params *params)
{
	free(params->device_path);
	params->device_path = NULL;
	free(params->module_path);
	params->module_path = NULL;
	//free(params->if_name);
	//params->if_name = NULL;
	memset(params->if_name, '\0', sizeof(params->if_name));
	free(params);
	params = NULL;
}

void print_LIN_params(LIN_params *params)
{
	printf("LIN Params -------------------------------------------------\n");
	printf("Device path:     %s\n", params->device_path);
	printf("Module path:     %s\n", params->module_path);
	printf("Baud rate:       %d\n", params->baud_rate);
	printf("------------------------------------------------------------\n");
}

int sllin_interface_up_down(LIN_params *params, int up)
{
	struct nl_sock *nls;
	struct rtnl_link *request;
	struct rtnl_link *link;
	int rc;

	// Allocate and initialize a new netlink socket
	nls = nl_socket_alloc();

	// Bind and connect the socket to a protocol, NETLINK_ROUTE in this case
	nl_connect(nls, NETLINK_ROUTE);
	
	// A specific link may be looked up by either interface index or interface name
	rc = rtnl_link_get_kernel(nls, 0, params->if_name, &link);
	if(rc < 0){
		fprintf(stderr, "Could not get %s link\n", params->if_name);
		return -1;
	}

	// In order to change any attributes of an existing link, we must allocate
	// a new link to hold the change requests:
	request = rtnl_link_alloc();

	// Set the interface up or down (same as ip link set interface up/down)
	if(up) rtnl_link_set_flags(request, rtnl_link_str2flags("up"));
	else   rtnl_link_unset_flags(request, rtnl_link_str2flags("up"));
	
	// Two ways exist to commit this change request, the first one is to
	// build the required netlink message and send it out in one single
	// step:
	rc = rtnl_link_change(nls, link, request, 0);
	if(rc < 0){
		fprintf(stderr, "Could not set interface %s\n", (up)? "up":"down");
		return -1;
	}

	// An alternative way is to build the netlink message and send it
	// out yourself using nl_send_auto_complete()
	// struct nl_msg *msg = rtnl_link_build_change_request(old, request);
	// nl_send_auto_complete(nl_handle, nlmsg_hdr(msg));
	// nlmsg_free(msg);

	// After successful usage, the object must be given back to the cache
	rtnl_link_put(link);
	nl_socket_free(nls);

	return 0;
}

int sllin_config_up(LIN_params *params)
{
	int ldisc = SLLIN_LDISC;
	int rc;

	tty = open(params->device_path, O_NOCTTY);
	if (tty < 0){
		perror("open");
		return -1;
	}

	/* Set sllin line discipline on given tty */
	rc = ioctl(tty, TIOCSETD, &ldisc);
	if (rc < 0){
		perror("ioctl TIOCSETD");
		return -1;
	}
	printf("Registered sllin line discipline to %s\n", params->device_path);

	params->if_name = (char*)calloc(IFNAMSIZ,sizeof(char));
	/* Retrieve the name of the created netdevice */
	rc = ioctl(tty, SIOCGIFNAME, params->if_name);
	if (rc < 0){
		perror("ioctl SIOCGIFNAME");
		return -1;
	}
	
	rc = sllin_interface_up_down(params, 1);
	if(rc < 0) printf("Could not attach netdevice %s to %s\n", params->if_name, params->device_path);
	else printf("Attached netdevice %s to %s\n", params->if_name, params->device_path);

	return rc;
}

int sllin_config_down(LIN_params *params)
{
	int ldisc = N_TTY;
	int rc;

	rc = sllin_interface_up_down(params, 0);
	if(rc < 0) printf("Could not detach netdevice %s from %s\n", params->if_name, params->device_path);
	else printf("Detached netdevice %s from %s\n", params->if_name, params->device_path);
	
	/* Remove sllin line discipline from given tty */
	rc = ioctl(tty, TIOCSETD, &ldisc);
	if (rc < 0){
		perror("ioctl TIOCSETD");
		return -1;
	}
	close(tty);
	
	printf("Unregistered sllin line discipline from %s\n", params->device_path);
	
	return rc;
}

int open_socket()
{
	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0){
		perror("socket");
		return -1;
	}
	return 0;
}

int bind_socket(LIN_params *params)
{
	struct sockaddr_can addr;

	params->if_index = if_nametoindex(params->if_name);
	if (params->if_index == 0) {
		perror("if_nametoindex");
		return -1;
	}
	
	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = params->if_index;
	
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return -1;
	}

	return 0;
}

int close_socket()
{
	return close(s);
}

int receive_frame(char *frm_str)
{	
	int required_mtu; // rquired transfer unit byte count
	int mtu; // maximum transfer unit byte count
	struct canfd_frame frame;
	
	/* parse CAN frame */
	required_mtu = parse_canframe(frm_str, &frame);
	if (required_mtu == 0){
		fprintf(stderr, "Wrong frame format!\n\n");
		return -1;
	}
	
	if (write(s, &frame, required_mtu) != required_mtu){
		perror("Write to LIN");
		return -1;
	}
	
	return 0;
}

void *send_frame(void *vargp)
{
	LIN_params *params = (LIN_params*)vargp;
	fd_set rdfs;
	struct sockaddr_can addr;
	struct iovec iov;
	struct msghdr msg;
	struct canfd_frame frame;
	int nbytes;
	struct timespec ts = {1, 0}; // 1 second
	
	addr.can_family = AF_CAN;
	addr.can_ifindex = params->if_index;
	
	// msghdr struct fields (from
	/*	struct msghdr {
			void    *   msg_name;   // Socket name
			int     msg_namelen;    // Length of name
			struct iovec *  msg_iov;    // Data blocks
			__kernel_size_t msg_iovlen; // Number of blocks
			void    *   msg_control;    // Per protocol magic (eg BSD file descriptor passing)
			__kernel_size_t msg_controllen; // Length of cmsg list
			unsigned int    msg_flags;
		}; */

	iov.iov_base = &frame;
	msg.msg_name = &addr;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	
	while(running){
		FD_ZERO(&rdfs);
		FD_SET(s, &rdfs);

		if ((pselect(s+1, &rdfs, NULL, NULL, &ts, NULL)) < 0){
			perror("select");
			continue;
		}
		
		if (FD_ISSET(s, &rdfs)){
			
			iov.iov_len = sizeof(frame);
			msg.msg_namelen = sizeof(addr); 
			msg.msg_flags = 0;
			
			nbytes = recvmsg(s, &msg, 0);
			
			if (nbytes < 0){
				perror("read");
				continue;
			}
			
			if (nbytes != CAN_MTU){
				fprintf(stderr, "read: incomplete CAN frame\n");
				continue;
			}
			
			memset(out, '\0', 20);
			sprint_canframe(out, &frame, 0, CAN_MAX_DLEN);
			rdy_to_send = 1;
			
			while(rdy_to_send && running);
		}
	}
}
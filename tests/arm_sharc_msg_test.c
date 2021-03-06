/* Test: arm_sharc_msg_test
   Description: 1. It calls all mcapi APIs supported and checks the result;     
	            2. It tests all the examples of blocking and nonblocking        
                   msgsend/msgrecv between two different endpoints on different nodes.
   Result: It'll give passed log if test passes, otherwise it'll give failed log.
*/

#include <mcapi.h>
#include <mcapi_test.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#define BUFF_SIZE 64
#define DOMAIN 0
#define NODE 0
#define MAX_MODE 4

#define CHECK_STATUS(ops, status, line) check_status(ops, status,line);
#define CHECK_SIZE(max, check, line) check_size(max, check, line);

#define SEND_STRING "hello mcapi core0"
#define RECV_STRING "hello mcapi core1"

#define MICROSECS(tv) ((tv).tv_sec*1000*1000 + (tv).tv_usec)

void check_size(size_t max_size, size_t check_size, unsigned line)
{
	if (check_size > max_size || check_size < 0) {
		printf("check_size:%d is out of range [0,%d]!\n",check_size, max_size);
		wrong(line);
	}
}

void check_status(char* ops, mcapi_status_t status, unsigned line)
{
	char status_message[32];
	char print_buf[BUFF_SIZE];
	mcapi_display_status(status, status_message, 32);
	if (NULL != ops) {
		snprintf(print_buf, sizeof(print_buf), "%s:%s", ops, status_message);
		printf("CHECK_STATUS---%s\n", print_buf);
	}
	if ((status != MCAPI_PENDING) && (status != MCAPI_SUCCESS)) {
		wrong(line);
	}
}

void wrong(unsigned line)
{
	fprintf(stderr,"WRONG: line=%u\n", line);
	fflush(stdout);
	_exit(1);
}

static int help(void)
{
	printf("Usage: arm_sharc_msg_test <options>\n");
	printf("\nAvailable options:\n");
	printf("\t-h,--help\t\tthis help\n");
	printf("\t-t,--timeout\t\ttimeout value in jiffies(default:5000)\n");
	printf("\t-r,--round\t\tnumber of test round(default:100)\n");
	printf("\t-d,--delay\t\tblock&wait function test delay time in us(default:0us)\n");
	return 0;
}

void send (mcapi_endpoint_t send, mcapi_endpoint_t recv, char *msg, size_t send_size,
			mcapi_status_t *status, mcapi_request_t *request, int mode, int timeout, long delay_us)
{
	int priority = 1;
	size_t size;
	long wait = 0;
	struct timeval tv1;
	struct timeval tv2;

	printf("send() start......\n");
	switch(mode) {
		case 0:
		case 1: 
			mcapi_msg_send_i(send,recv,msg,send_size,priority,request,status);
			CHECK_STATUS("send_i", *status, __LINE__);
			do{
				mcapi_test(request, &size, status);
			}while(*status == MCAPI_PENDING);
			CHECK_STATUS("test", *status, __LINE__);
			CHECK_SIZE(send_size, size, __LINE__);
			/* use mcapi_wait to release request */
			mcapi_wait(request, &size, timeout, status);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(send_size, size, __LINE__);
			break;
		case 2:
			mcapi_msg_send_i(send,recv,msg,send_size,priority,request,status);
			CHECK_STATUS("send_i", *status, __LINE__);
			gettimeofday(&tv1, NULL);
			mcapi_wait(request,&size,timeout,status);
			CHECK_SIZE(send_size, size, __LINE__);
			gettimeofday(&tv2, NULL);
			CHECK_STATUS("wait", *status, __LINE__);
			wait = MICROSECS(tv2) - MICROSECS(tv1);
			if (wait < delay_us) {
				*status = MCAPI_ERR_GENERAL;
				CHECK_STATUS("test_send_delay", *status, __LINE__);
			}
			break;
		case 3:
			gettimeofday(&tv1, NULL);
			mcapi_msg_send(send, recv, msg, send_size, priority, status);
			gettimeofday(&tv2, NULL);
			CHECK_STATUS("send", *status, __LINE__);
			wait = MICROSECS(tv2) - MICROSECS(tv1);
			if (wait < delay_us) {
				*status = MCAPI_ERR_GENERAL;
				CHECK_STATUS("test_send_delay", *status, __LINE__);
			}
			break;
		default:
			printf("Invalid test_round value.\
					\nIt should be in range of 1 to 100\n");
			wrong(__LINE__);
	}
	printf("end of send() - endpoint=%i has sent: [%s]\n", (int)send, msg);
}

void recv (mcapi_endpoint_t recv, char *buffer, size_t buffer_size, mcapi_status_t *status,
			mcapi_request_t *request, int mode, int timeout, long delay_us)
{
	long wait = 0;
	size_t size;
	mcapi_uint_t avail;
	struct timeval tv1;
	struct timeval tv2;
	if (buffer == NULL) {
		printf("recv() Invalid buffer ptr\n");
		wrong(__LINE__);
	}
	printf("recv() start......\n");
	switch(mode) {
		case 0:
			do {
				avail = mcapi_msg_available(recv, status);
			}while(avail <= 0);
			CHECK_STATUS("available", *status, __LINE__);

			mcapi_msg_recv_i(recv, buffer, buffer_size, request, status);
			CHECK_STATUS("recv_i", *status, __LINE__);
			/* use mcapi_wait to release request */
			mcapi_wait(request, &size, timeout, status);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			break;
		case 1:
			mcapi_msg_recv_i(recv, buffer, buffer_size, request, status);
			CHECK_STATUS("recv_i", *status, __LINE__);
			do {
				mcapi_test(request, &size, status);
			}while(*status == MCAPI_PENDING);
			CHECK_STATUS("test", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);

			/* use mcapi_wait to release request */
			mcapi_wait(request, &size, timeout, status);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			break;
		case 2:
			mcapi_msg_recv_i(recv, buffer, buffer_size, request, status);
			CHECK_STATUS("recv_i", *status, __LINE__);
			gettimeofday(&tv1, NULL);
			mcapi_wait(request, &size, timeout, status);
			gettimeofday(&tv2, NULL);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			wait = MICROSECS(tv2) - MICROSECS(tv1);
			if (wait < delay_us) {
				*status = MCAPI_ERR_GENERAL;
				CHECK_STATUS("test_recv_delay", *status, __LINE__);
			}
			break;
		case 3:
			gettimeofday(&tv1, NULL);
			mcapi_msg_recv(recv, buffer, buffer_size, &size, status);
			gettimeofday(&tv2, NULL);
			CHECK_STATUS("recv", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			wait = MICROSECS(tv2) - MICROSECS(tv1);
			if (wait < delay_us) {
				*status = MCAPI_ERR_GENERAL;
				CHECK_STATUS("test_recv_delay", *status, __LINE__);
			}
			break;
		default:
			printf("Invalid test_round value.\
					\nIt should be 4*n\n");
			wrong(__LINE__);
	}
	buffer[size] = '\0';
	printf("end of recv() - endpoint=%i size 0x%x has received: [%s]\n", (int)recv, size, buffer);
}

int main (int argc, char *argv[])
{
	mcapi_status_t status;
	mcapi_param_t parms;
	mcapi_info_t version;
	mcapi_endpoint_t ep1,ep2;
	mcapi_domain_t domain;
	mcapi_node_t node;
	mcapi_node_attributes_t mcapi_node_attributes;
	char attr[4];
	char str[] = "123";
	char send_buf[32] = "";
	char recv_buf[BUFF_SIZE];
	char cmp_buf[BUFF_SIZE];
	int s = 0, pass_num = 0;
	size_t size;
	int mode = 0;
	int b_block = 0;
	int timeout = 5*1000;
	int test_round = 100;
	long delay_us = 0;
	static const char short_options[] = "ht:r:d:";
	static const struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"timeout", 1, NULL, 't'},
		{"round", 1, NULL, 'r'},
		{"delay", 1, NULL, 'd'},
		{NULL, 0, NULL, 0},
	};
	mcapi_request_t request;

	if (!strstr(argv[0], "arm_sharc_msg_test")) {
		printf("command should be named arm_sharc_msg_test\n");
		return 1;
	}
	while (1) {
		int c;
		if ((c = getopt_long(argc, argv, short_options, long_options, NULL)) < 0)
			break;
		switch (c) {
		case 'h':
			help();
			return 0;
		case 't':
			timeout = strtol(optarg, NULL, 0);
			break;
		case 'r':
			test_round = strtol(optarg, NULL, 0);
			break;
		case 'd':
			delay_us = strtol(optarg, NULL, 0);
			break;
		default:
			printf("Invalid switch or option needs an argument.\
				  \ntry arm_sharc_msg_test --help for more information.\n");
			return 1;
		}
	}
	if (timeout <= 0) {
		printf("Invalid timeout value: %d\
			  \nIt should be in the range of 1 to MCA_INFINITE\n", timeout);
		return 1;
	}
	if (test_round <= 0 || test_round > 100) {
		printf("Invalid round value: %d\
			  \nIt should be in the range of 1 to 100\n", test_round);
		return 1;
	}
	if (delay_us < 0 || delay_us > 1000000) {
		printf("Invalid delay value: %d\
			  \nIt should be in the range of 0 to 1000000us\n", delay_us);
		return 1;
	}
	/* init node attributes*/
	mcapi_node_init_attributes(&mcapi_node_attributes, &status);
	CHECK_STATUS("init_node_attr", status, __LINE__);
	/* set default attributes */
	mcapi_node_set_attribute(&mcapi_node_attributes, 0, (void *)str, sizeof(str), &status);
	CHECK_STATUS("set_node_attr", status, __LINE__);
	/* create a node */
	mcapi_initialize(DOMAIN,NODE,&mcapi_node_attributes,&parms,&version,&status);
	CHECK_STATUS("initialize", status, __LINE__);
	domain = mcapi_domain_id_get(&status);
	CHECK_STATUS("get_domain_id", status, __LINE__);
	if (domain != DOMAIN) {
		printf("get domain:%d error\n", domain);
		return 1;
	}
	node = mcapi_node_id_get(&status);
	CHECK_STATUS("get_node_id", status, __LINE__);
	if (node != NODE) {
		printf("get node:%d error\n", node);
		return 1;
	}
	/* get attributes of node */
	mcapi_node_get_attribute(DOMAIN, NODE, 0, attr, sizeof(str), &status);
	CHECK_STATUS("get_node_attr", status, __LINE__);
	if (strncmp(attr, str, sizeof(str))) {
		printf("get_node_attr:%s error\n", (char *)attr);
		return 1;
	}
	/* create endpoints */
	ep1 = mcapi_endpoint_create(MASTER_PORT_NUM1,&status);
	CHECK_STATUS("create_ep", status, __LINE__);
	printf("ep1 %x   \n", ep1);
	ep2 = mcapi_endpoint_get(DOMAIN,SLAVE_NODE_NUM,SLAVE_PORT_NUM1,
					timeout,&status);
	CHECK_STATUS("get_ep", status, __LINE__);
	printf("t1: ep2 %x   \n", ep2);
	mcapi_endpoint_get_i(DOMAIN,SLAVE_NODE_NUM,SLAVE_PORT_NUM1,
					&ep2,&request,&status);
	CHECK_STATUS("get_ep_i", status, __LINE__);
	do {
		mcapi_test(&request, &size, &status);
	}while(status == MCAPI_PENDING);

	CHECK_STATUS("test", status, __LINE__);
	mcapi_wait(&request, &size, timeout, &status);
	CHECK_STATUS("wait", status, __LINE__);
	printf("t2: ep2 %x   \n", ep2);
	mcapi_endpoint_get_i(DOMAIN,SLAVE_NODE_NUM,SLAVE_PORT_NUM1,
					&ep2,&request,&status);
	CHECK_STATUS("get_ep_i", status, __LINE__);
	mcapi_wait(&request, &size, timeout, &status);
	CHECK_STATUS("wait", status, __LINE__);
	printf("t3: ep2 %x   \n", ep2);

	/* send and recv messages on the endpoints */
	/* start full test process */
	for (s = 0; s < test_round; s++) {
		mode = s % MAX_MODE;
		snprintf(send_buf, sizeof(send_buf), "%s %d", SEND_STRING, s);
		send(ep1, ep2, send_buf, strlen(send_buf), &status, &request, mode, timeout, delay_us);
		printf("Core0: mode(%d) message send. The %d time sending\n",mode, s);
		recv(ep1, recv_buf, sizeof(recv_buf)-1, &status, &request, mode, timeout, delay_us);
		snprintf(cmp_buf, sizeof(cmp_buf), "%s %d", RECV_STRING, s+1);
		if (!strncmp(recv_buf, cmp_buf, sizeof(recv_buf))) {
			pass_num++;
			printf("Core0: mode(%d) message recv. The %d time receiving\n",mode, s);
		} else
		  printf("Core0: mode(%d) %d time message recv failed\n", mode, s);
	}

	mcapi_endpoint_delete(ep1,&status);
	CHECK_STATUS("del_ep", status, __LINE__);

	mcapi_finalize(&status);
	CHECK_STATUS("finalize", status, __LINE__);

	if (pass_num == test_round)
		printf("Core0 %d rounds full Test PASSED!!\n", test_round);
	else
		printf("Core0 only %d rounds test passed, full Test FAILED!!\n", pass_num);

	return 0;
}

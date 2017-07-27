#ifndef _EPSON_DAEMON_H_
#define _EPSON_DAEMON_H_

typedef struct _CBTD_INFO
{
	char printer_name[CONF_BUF_MAX]; /* the printer title that is set with lpr */
	char devprt_path[CONF_BUF_MAX]; /* open device path */
	char infifo_path[CONF_BUF_MAX]; /* fifo path */
	unsigned int comsock_port; /* INET socket port */
	int devfd;		/* device file descriptor */

	char prt_status[PRT_STATUS_MAX]; /* printer status */
	int prt_status_len;	/* size of printer status strings */
	long status_post_time; /* the time that updated prt_status in the last */

	int sysflags;		/* CBTD status flags */
	void* sysflags_critical;

	/* Parent thread uses status of each thread for authentication.
	Not need to manage it with semaphore. */
	void* datatrans_thread;
	int datatrans_thread_status;
	void* comserv_thread;
	int comserv_thread_status;

	/* CBT control */
	void* ecbt_handle;
	void* ecbt_accsess_critical;
} CBTD_INFO, *P_CBTD_INFO;









#endif // !_EPSOND_H_


#ifndef __CONGESTION_CTRL_H__
#define __CONGESTION_CTRL_H__

#define CONGEST_TIMEOUT 1
#define CONGEST_DUPACK 2

#define TRANSMIT_SLOW_START 1
#define TRANSMIT_CONGEST_AV 2

//because in congestion avoidance mode, increase cwnd by 1/cwnd, if cwnd is an integer, 
//this number will always be 0, therefore I add another double rcwnd to save this increase
//
typedef struct _congestion_ctrl_info {
	double rcwnd;			
	int cwnd;
	int ssthresh;
	int recvwin;		/* receiver Advertised Window */
        int send_conf_win;      /* Configured Send Size.. i.e size of the Send Buffer */
	int sendwin; 		/* the minimum of cwnd, recvwin and send_conf_win */
} congestion_ctrl_info_t;

void init_congestion_ctrl_info(congestion_ctrl_info_t* pctrl, int conf_window);

void congest_occur(congestion_ctrl_info_t* pctrl, int mode);

void transmit_occur(congestion_ctrl_info_t* pctrl);

void adjust_send_win_size(congestion_ctrl_info_t* pccinfo);

#endif /* __CONGESTION_CTRL_H__ */


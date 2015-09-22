#include "congestion_ctrl.h"
#include <stdio.h>

void init_congestion_ctrl_info(congestion_ctrl_info_t* pctrl, int send_conf_win)
{
  pctrl->rcwnd = 1.0;
  pctrl->cwnd = 1;
  pctrl->ssthresh = 128; // 66536 / 512
  pctrl->recvwin = 0;
  pctrl->sendwin = pctrl->cwnd;
  pctrl->send_conf_win = send_conf_win;
}

void congest_occur(congestion_ctrl_info_t* pctrl, int mode)
{
  pctrl->ssthresh = pctrl->cwnd / 2;	
  if (pctrl->ssthresh < 2) {
    pctrl->ssthresh = 2;
  }
  printf("[Congest Occur] Congest type:%s, adjust ssthresh to:%d\n", 
      mode == CONGEST_TIMEOUT ? "timeout" : "dup ack", pctrl->ssthresh);
  if (mode == CONGEST_TIMEOUT) {
    pctrl->cwnd = 1;
    pctrl->rcwnd = 1.0;
  }
}

void transmit_occur(congestion_ctrl_info_t* pctrl)
{
  if (pctrl->cwnd <= pctrl->ssthresh) {
    // slow start happen
    pctrl->cwnd += 1;
    pctrl->rcwnd += 1.0;
    printf("[Cwnd] In 'Slow Start', adjust cwnd to %d.\n", pctrl->cwnd);
  }
  else {
    // congestion avoidance
    pctrl->rcwnd += 1.0 / pctrl->cwnd;
    pctrl->cwnd = (int)(pctrl->rcwnd + 0.5);
    printf("[Cwnd] In 'Congestion Avoidance', adjust cwnd to %lf by add (1/cwnd), round to %d.\n", pctrl->rcwnd, pctrl->cwnd);
  }
}

#define MIN(a,b) (((a) < (b))? (a): (b))

void recalc_send_win_size(congestion_ctrl_info_t* pccinfo)
{
  pccinfo->sendwin = MIN(pccinfo->cwnd, pccinfo->recvwin);
  pccinfo->sendwin = MIN(pccinfo->sendwin, pccinfo->send_conf_win);
}


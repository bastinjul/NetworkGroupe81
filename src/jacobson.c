#include <math.h>
#include <stdio.h>
#include "jacobson.h"
// ============================== JACOBSON ALGORITHM VARIABLES ==============================

int              counter=1;              // segment counter 
float            rto_jaco_now;           // rto(k+1)           - Jacobson
float            srtt3_last, srtt3_now;  // srtt(k), srtt(k+1) - Jacobson
float            serr_last, serr_now;    // serr(k), serr(k+1) - Jacobson
float            sdev_last, sdev_now;    // sdev(k), sdev(k+1) - Jacobson

// ============================== JACOBSON ALGORITHM COMPUTING FUNCTION ==============================

// All credits go to the University of South Florida
// Inspired of the program found at http://www.csee.usf.edu/~christen/tools/rtimeout.c
// Returns the retransmission timeout value
float compute_retransmission_time(float rtt){
	if (counter == 1)
	{
       srtt3_now = (1-PARA_G)*SRTT_0+PARA_G*rtt;
       serr_now = rtt - SRTT_0;
       sdev_now = (1-PARA_H)*SDEV_0+PARA_H*fabs(serr_now);   
       rto_jaco_now = srtt3_now+PARA_F*sdev_now;
       counter++;
	} else {
       srtt3_now = (1-PARA_G)*srtt3_last+PARA_G*rtt;
       serr_now = rtt - srtt3_last;
       sdev_now = (1-PARA_H)*sdev_last+PARA_H*fabs(serr_now);
       rto_jaco_now = srtt3_now+PARA_F*sdev_now;
	}
       srtt3_last = srtt3_now;
        serr_last = serr_now;
        sdev_last = sdev_now;
	return rto_jaco_now;
}
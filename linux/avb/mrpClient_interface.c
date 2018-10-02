#include "mrpClient_interface.h"


extern int errno;


int mrpClient_getDomain(FILE* filepointer, mrp_ctx_t *mrp_ctx)
{
	int ret=0;

    /*
     * we may not get a notification if we are joining late,
     * so query for what is already there ...
     */
	char* msgbuf= malloc(1500);
    if (NULL == msgbuf){
        fprintf(filepointer,  "failed to create msgbuf. %d %s\n",  errno, strerror(errno));fflush(filepointer);
        return RETURN_VALUE_FAILURE;
    }

    memset(msgbuf, 0, 1500);
    sprintf(msgbuf, "S??");
	fprintf(filepointer, "Get Domain %s\n",msgbuf);fflush(filepointer);
    ret = mrpClient_send_mrp_msg( filepointer, mrpClient_get_Control_socket(), msgbuf, 1500);


    fprintf(filepointer,  "Query SRP Domain: %s\n",  msgbuf);fflush(filepointer);
    free(msgbuf);
    if (ret != 1500){
        fprintf(filepointer,  "failed to create socket. %d %s\n",  errno, strerror(errno));fflush(filepointer);
        return RETURN_VALUE_FAILURE;
    }


    while ( (mrp_ctx->domain_a_valid == 0) || (mrp_ctx->domain_b_valid == 0 ) ||
                (mrp_ctx->domain_class_a_vid == 0) || (mrp_ctx->domain_class_b_vid == 0) ){
        usleep(20000);
    }

//    if (mrp_ctx->domain_a_valid > 0) {
//        class_a->id = mrp_ctx->domain_class_a_id;
//        class_a->priority = mrp_ctx->domain_class_a_priority;
//        class_a->vid = mrp_ctx->domain_class_a_vid;
//    }
//    if (mrp_ctx->domain_b_valid > 0) {
//        class_b->id = mrp_ctx->domain_class_b_id;
//        class_b->priority = mrp_ctx->domain_class_b_priority;
//        class_b->vid = mrp_ctx->domain_class_b_vid;
//    }

	return RETURN_VALUE_SUCCESS;
}


int mrpClient_report_domain_status(FILE* filepointer, mrp_ctx_t *mrp_ctx)
{
	char* msgbuf= malloc(1500);
	int rc=0;


	if (NULL == msgbuf){
		fprintf(filepointer,  "mrp_listener_report_domain_status - NULL == msgbuf \t LISTENER_FAILED \n");fflush(filepointer);
		mrp_ctx->mrp_status = LISTENER_FAILED;


		return RETURN_VALUE_FAILURE;
	}
	memset(msgbuf, 0, 1500);
	sprintf(msgbuf, "S+D:C=%d,P=%d,V=%04x", mrp_ctx->domain_class_a_id, mrp_ctx->domain_class_a_priority, mrp_ctx->domain_class_a_vid);
	fprintf(filepointer, "Report Domain Status %s\n",msgbuf);fflush(filepointer);
	rc = mrpClient_send_mrp_msg( filepointer, mrpClient_get_Control_socket(), msgbuf, 1500);
	free(msgbuf);

	if (rc != 1500){
		fprintf(filepointer,  "mrp_listener_report_domain_status - rc != 1500 \t LISTENER_FAILED \n");fflush(filepointer);
		mrp_ctx->mrp_status = LISTENER_FAILED;

		return RETURN_VALUE_FAILURE;
	}

    fprintf(filepointer,  "mrp_listener_report_domain_status\t LISTENER_IDLE \n");fflush(filepointer);
    mrp_ctx->mrp_status = LISTENER_IDLE;

	return RETURN_VALUE_SUCCESS;
}




int mrpClient_joinVLAN(FILE* filepointer, mrp_ctx_t *mrp_ctx)
{
	char* msgbuf= malloc(1500);
	int rc=0;

	if (NULL == msgbuf)		return RETURN_VALUE_FAILURE;

	memset(msgbuf, 0, 1500);
	sprintf(msgbuf, "V++:I=%04x",mrp_ctx->domain_class_a_vid);
	fprintf(filepointer, "Joing VLAN %s\n",msgbuf);fflush(filepointer);

    rc = mrpClient_send_mrp_msg( filepointer, mrpClient_get_Control_socket(), msgbuf, 1500);
    free(msgbuf);

    if( rc != 1500){
        fprintf(filepointer,  "int mrp_listener_join_vlan - rc != 1500\t LISTENER_FAILED\n");fflush(filepointer);
        mrp_ctx->mrp_status = LISTENER_FAILED;
        return RETURN_VALUE_FAILURE;
    } else {
        fprintf(filepointer,  "int mrp_listener_join_vlan - rc != 1500\t LISTENER_IDLE\n");fflush(filepointer);
        mrp_ctx->mrp_status = LISTENER_IDLE;
        return RETURN_VALUE_SUCCESS;
    }


    return RETURN_VALUE_FAILURE;
}



int mrpClient_getDomain_joinVLAN(FILE* filepointer, ieee1722_avtp_driver_state **ieee1722mc, mrp_ctx_t *mrp_ctx)
{
    fprintf(filepointer,  "calling mrp_get_domain()\n");fflush(filepointer);

    if ( mrpClient_getDomain( filepointer, mrp_ctx) > RETURN_VALUE_FAILURE)	{
        fprintf(filepointer,  "success calling mrp_get_domain()\n");fflush(filepointer);
    } else {
        fprintf(filepointer,  "failed calling mrp_get_domain()\n");fflush(filepointer);
        return RETURN_VALUE_FAILURE;
    }

    fprintf(filepointer,  "detected domain Class A PRIO=%d VID=%04x...\n",mrp_ctx->domain_class_a_priority,
                                                                            mrp_ctx->domain_class_a_vid);fflush(filepointer);

    if ( mrpClient_report_domain_status( filepointer, mrp_ctx) > RETURN_VALUE_FAILURE ) {
        fprintf(filepointer,  "report_domain_status success\n");fflush(filepointer);
    } else {
        fprintf(filepointer,  "report_domain_status failed\n");fflush(filepointer);

        return RETURN_VALUE_FAILURE;
    }

    if ( mrpClient_joinVLAN( filepointer, mrp_ctx) > RETURN_VALUE_FAILURE) {
        fprintf(filepointer,  "join_vlan success\n");fflush(filepointer);
    } else {
        fprintf(filepointer,  "join_vlan failed\n");fflush(filepointer);

        return RETURN_VALUE_FAILURE;
    }


	return RETURN_VALUE_SUCCESS;
}






int mrpClient_listener_await_talker(FILE* filepointer, ieee1722_avtp_driver_state **ieee1722mc, mrp_ctx_t *mrp_ctx)
{
    if( mrp_ctx->mrp_status  == LISTENER_READY)   {
        fprintf(filepointer,  "Already connected to a talker...\n");fflush(filepointer);
        return RETURN_VALUE_FAILURE;
    } else {
        mrp_ctx->mrp_status = LISTENER_WAITING;
        fprintf(filepointer,  "Waiting for talker...\n");fflush(filepointer);
        fprintf(filepointer,  "int mrp_listener_await_talker - rc != 1500\t LISTENER_WAITING\n");fflush(filepointer);

        struct timespec tim, tim2;
        tim.tv_sec = 0;
        tim.tv_nsec = MILISLEEP_TIME * 5;
        while(mrp_ctx->mrp_status == LISTENER_WAITING
                && mrp_ctx->mrp_status != LISTENER_READY ){
            nanosleep(&tim , &tim2);
        }

        if ( mrpClient_listener_send_ready( filepointer, ieee1722mc, mrp_ctx ) > RETURN_VALUE_FAILURE) {
            fprintf(filepointer,  "send_ready success\n");fflush(filepointer);
            return RETURN_VALUE_SUCCESS;
        }
    }

    return RETURN_VALUE_FAILURE;
}

int mrpClient_listener_send_ready(FILE* filepointer, ieee1722_avtp_driver_state **ieee1722mc, mrp_ctx_t *mrp_ctx)
{
	char *databuf= malloc(1500);
	int rc=0;

	if (NULL == databuf) return RETURN_VALUE_FAILURE;

	memset(databuf, 0, 1500);
	sprintf(databuf, "S+L:L=%02x%02x%02x%02x%02x%02x%02x%02x,D=2",
			(*ieee1722mc)->streamid8[0], (*ieee1722mc)->streamid8[1],
			(*ieee1722mc)->streamid8[2], (*ieee1722mc)->streamid8[3],
			 (*ieee1722mc)->streamid8[4], (*ieee1722mc)->streamid8[5],
			 (*ieee1722mc)->streamid8[6], (*ieee1722mc)->streamid8[7]);
	rc = mrpClient_send_mrp_msg( filepointer, mrpClient_get_Control_socket(), databuf, 1500);
	fprintf(filepointer, "Send Ready %s\n",databuf);fflush(filepointer);
	free(databuf);

	if (rc != 1500){
        fprintf(filepointer,  "mrp_listener_send_ready - rc != 1500\t LISTENER_FAILED\n");fflush(filepointer);
		mrp_ctx->mrp_status = LISTENER_FAILED;

		return RETURN_VALUE_FAILURE;
	}

    fprintf(filepointer,  "mrp_listener_send_ready - rc != 1500\t LISTENER_IDLE\n");fflush(filepointer);

	return RETURN_VALUE_SUCCESS;
}


int mrpClient_listener_send_leave(FILE* filepointer, ieee1722_avtp_driver_state **ieee1722mc, mrp_ctx_t *mrp_ctx)
{
	char *databuf= malloc(1500);
	int rc=0;

	if (NULL == databuf){
		return RETURN_VALUE_FAILURE;
	}
	memset(databuf, 0, 1500);
	sprintf(databuf, "S-L:L=%02x%02x%02x%02x%02x%02x%02x%02x,D=3",
			(*ieee1722mc)->streamid8[0], (*ieee1722mc)->streamid8[1],
			 (*ieee1722mc)->streamid8[2], (*ieee1722mc)->streamid8[3],
			 (*ieee1722mc)->streamid8[4], (*ieee1722mc)->streamid8[5],
			 (*ieee1722mc)->streamid8[6], (*ieee1722mc)->streamid8[7]);
	rc = mrpClient_send_mrp_msg(filepointer, mrpClient_get_Control_socket(), databuf, 1500);
	fprintf(filepointer, "Send Leave %s\n",databuf);fflush(filepointer);
	free(databuf);

	if (rc != 1500){
		fprintf(filepointer,  "mrp_listener_send_leave - rc != 1500\t LISTENER_FAILED\n");fflush(filepointer);
		mrp_ctx->mrp_status = LISTENER_FAILED;

		return RETURN_VALUE_FAILURE;
	} else {
        fprintf(filepointer,  "mrp_listener_send_leave - rc != 1500\t LISTENER_IDLE\n");fflush(filepointer);
		mrp_ctx->mrp_status = LISTENER_IDLE;

        return RETURN_VALUE_SUCCESS;
	}
}



/* The code is subject to Purdue University copyright policies.
 * DO NOT SHARE, DISTRIBUTE, OR POST ONLINE
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "ring_buffer.h"
#include "tinytcp.h"
#include "handle.h"

uint32_t last_ack_num = 0;
int ISN = 0;


uint32_t ring_buffer_get_data(ring_buffer_t* buffer,
                            char* dst_buff,
                            uint32_t bytes, uint32_t seq_num)
{
    if (buffer == NULL) {
        fprintf(stderr, "error ring_buffer_remove: buffer is NULL\n");
        exit(1);
    }

    uint32_t occupied = occupied_space(buffer, &seq_num);
    if (bytes > occupied) {
        bytes = occupied;
    }

    uint32_t start_idx = seq_num % get_ring_buffer_capcity(buffer);

    if (dst_buff != NULL && bytes > 0) {
        if (start_idx + bytes <= get_ring_buffer_capcity(buffer)) {
            memcpy(dst_buff, (get_ring_buffer_data(buffer) + start_idx), bytes);
        } else {
            uint32_t diff = bytes - (get_ring_buffer_capcity(buffer) - start_idx);
            memcpy(dst_buff, (get_ring_buffer_data(buffer) + start_idx),
                    (get_ring_buffer_capcity(buffer) - start_idx));
            memcpy(dst_buff + (get_ring_buffer_capcity(buffer) - start_idx), get_ring_buffer_data(buffer), diff);
        }
    }
    
    return bytes;
}

void* handle_send_to_network(void* args)
{
    fprintf(stderr, "### started send thread\n");
    //printf("here\n");
    while (1) {

        int call_send_to_network = 0;

        for (int i = 0; i < tinytcp_conn_list_size; ++i) {
            tinytcp_conn_t* tinytcp_conn = tinytcp_conn_list[i];

            if (tinytcp_conn->curr_state == CONN_ESTABLISHED
            || tinytcp_conn->curr_state == READY_TO_TERMINATE) {

                if (tinytcp_conn->curr_state == READY_TO_TERMINATE) {
                    if (occupied_space(tinytcp_conn->send_buffer, NULL) == 0) {
                        handle_close(tinytcp_conn);
                        num_of_closed_conn++;
                        continue;
                    }
                }

<<<<<<< HEAD
                //TODO do something else
                if(timer_expired(tinytcp_conn->time_last_new_data_acked)){
                    //TODO do someting
                    pthread_spin_lock(&tinytcp_conn->mtx);
                    tinytcp_conn->seq_num = get_ring_buffer_head(tinytcp_conn->send_buffer);
                    pthread_spin_unlock(&tinytcp_conn->mtx);

                    tinytcp_conn->time_last_new_data_acked = clock();
                }
                
                if (occupied_space(tinytcp_conn->send_buffer, &tinytcp_conn->seq_num) > 0){
                    if(tinytcp_conn->num_of_dup_acks >= 3){
                        pthread_spin_lock(&tinytcp_conn->mtx);
                        tinytcp_conn->seq_num = get_ring_buffer_head(tinytcp_conn->send_buffer);
                        pthread_spin_unlock(&tinytcp_conn->mtx);

                        tinytcp_conn->num_of_dup_acks = 0;
                    }

                    char data[MSS];

                    pthread_spin_lock(&tinytcp_conn->mtx);
                    uint32_t num_bytes_removed = ring_buffer_get_data(tinytcp_conn->send_buffer, data, MSS, tinytcp_conn->seq_num);
                    pthread_spin_unlock(&tinytcp_conn->mtx);
=======
                if (timer_expired(tinytcp_conn->time_last_new_data_acked)) {
                    //TODO do someting
                    tinytcp_conn->time_last_new_data_acked = 0;
                    char* tinytcp_pkt = create_tinytcp_pkt(tinytcp_conn->src_port,
                        tinytcp_conn->dst_port, tinytcp_conn->seq_num,
                        tinytcp_conn->ack_num, 1, 1, 0, NULL, 0);
                    send_to_network(tinytcp_pkt, TINYTCP_HDR_SIZE + 0);

                }

                //TODO do something else
>>>>>>> 2602ee0004c753383617d49db3ebfb088316dfbf

                    char* tinytcp_pkt = create_tinytcp_pkt(tinytcp_conn->src_port,
                            tinytcp_conn->dst_port, tinytcp_conn->seq_num,
                            tinytcp_conn->ack_num, 1, 0, 0, data, num_bytes_removed);
                    send_to_network(tinytcp_pkt, TINYTCP_HDR_SIZE + num_bytes_removed);
                    
                    tinytcp_conn->seq_num += num_bytes_removed;

                    //TODO make call_send_to_network = 1 everytime you make a call to send_to_network()
                    call_send_to_network = 1;
                }
            }
        }

        if (call_send_to_network == 0) {
            usleep(100);
        }
    }
}


void handle_recv_from_network(char* tinytcp_pkt,
                              uint16_t tinytcp_pkt_size)
{
    //parse received tinytcp packet
    tinytcp_hdr_t* tinytcp_hdr = (tinytcp_hdr_t *) tinytcp_pkt;

    uint16_t src_port = ntohs(tinytcp_hdr->src_port);
    uint16_t dst_port = ntohs(tinytcp_hdr->dst_port);
    uint32_t seq_num = ntohl(tinytcp_hdr->seq_num);
    uint32_t ack_num = ntohl(tinytcp_hdr->ack_num);
    uint16_t data_offset_and_flags = ntohs(tinytcp_hdr->data_offset_and_flags);
    uint8_t tinytcp_hdr_size = ((data_offset_and_flags & 0xF000) >> 12) * 4; //bytes
    uint8_t ack = (data_offset_and_flags & 0x0010) >> 4;
    uint8_t syn = (data_offset_and_flags & 0x0002) >> 1;
    uint8_t fin = data_offset_and_flags & 0x0001;
    char* data = tinytcp_pkt + TINYTCP_HDR_SIZE;
    uint16_t data_size = tinytcp_pkt_size - TINYTCP_HDR_SIZE;

    if (syn == 1 && ack == 0) { //SYN recvd
        //create tinytcp connection
        tinytcp_conn_t* tinytcp_conn = tinytcp_create_conn();

        //TODO initialize tinytcp_conn attributes. filename is contained in data
        tinytcp_conn->curr_state = SYN_RECVD;
<<<<<<< HEAD

=======
>>>>>>> 2602ee0004c753383617d49db3ebfb088316dfbf
        tinytcp_conn->src_port = dst_port;
        tinytcp_conn->dst_port = src_port;
        tinytcp_conn->ack_num = seq_num + 1;
        tinytcp_conn->seq_num = (rand() % (50000 - 100 + 1)) + 100;
        ISN = tinytcp_conn->seq_num;
        strncpy(tinytcp_conn->filename, data, data_size);

        pthread_spin_lock(&tinytcp_conn->mtx);
        tinytcp_conn->send_buffer = create_ring_buffer(tinytcp_conn->seq_num + 1);
        tinytcp_conn->recv_buffer = create_ring_buffer(tinytcp_conn->seq_num + 1);
        pthread_spin_unlock(&tinytcp_conn->mtx);

        char filepath[500];
        strcpy(filepath, "recvfiles/");
        strncat(filepath, data, data_size);
        strcat(filepath, "\0");

        tinytcp_conn->r_fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        assert(tinytcp_conn->r_fd >= 0);

        fprintf(stderr, "\nSYN recvd "
                "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
                src_port, dst_port, seq_num, ack_num);

        //TODO update tinytcp_conn attributes
        tinytcp_conn->curr_state = SYN_ACK_SENT;

        fprintf(stderr, "\nSYN-ACK sending "
                "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
                tinytcp_conn->src_port, tinytcp_conn->dst_port,
                tinytcp_conn->seq_num, tinytcp_conn->ack_num);

        //TODO send SYN-ACK
<<<<<<< HEAD
=======
        tinytcp_conn->curr_state = SYN_ACK_SENT;
>>>>>>> 2602ee0004c753383617d49db3ebfb088316dfbf
        char* tinytcp_pkt = create_tinytcp_pkt(tinytcp_conn->src_port,
            tinytcp_conn->dst_port, tinytcp_conn->seq_num,
            tinytcp_conn->ack_num, 1, 1, 0, NULL, 0);
        send_to_network(tinytcp_pkt, TINYTCP_HDR_SIZE);




    } else if (syn == 1 && ack == 1) { //SYN-ACK recvd
        //get tinytcp connection

        tinytcp_conn_t* tinytcp_conn = tinytcp_get_conn(dst_port, src_port);
        assert(tinytcp_conn != NULL);

        if (tinytcp_conn->curr_state == SYN_SENT) {
            //TODO update tinytcp_conn attributes
            tinytcp_conn->curr_state = SYN_ACK_RECVD;
<<<<<<< HEAD
            //pthread_spin_unlock(&tinytcp_conn->mtx);
=======
>>>>>>> 2602ee0004c753383617d49db3ebfb088316dfbf
            tinytcp_conn->ack_num = seq_num + 1;
            tinytcp_conn->seq_num = ack_num;

            fprintf(stderr, "\nSYN-ACK recvd "
                    "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
                    src_port, dst_port, seq_num, ack_num);

        }

    } else if (fin == 1 && ack == 1) {
        //get tinytcp connection
        tinytcp_conn_t* tinytcp_conn = tinytcp_get_conn(dst_port, src_port);
        assert(tinytcp_conn != NULL);

        if (tinytcp_conn->curr_state == CONN_ESTABLISHED) { //FIN recvd
            fprintf(stderr, "\nFIN recvd "
                    "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
                    src_port, dst_port, seq_num, ack_num);

            //flush the recv_buffer
            while (occupied_space(tinytcp_conn->recv_buffer, NULL) != 0) {
                usleep(10);
            }

            //TODO update tinytcp_conn attributes
            tinytcp_conn->ack_num = seq_num + 1;
            tinytcp_conn->seq_num = ack_num;
            tinytcp_conn->curr_state = FIN_ACK_SENT;


            fprintf(stderr, "\nFIN-ACK sending "
                    "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
                    tinytcp_conn->src_port, tinytcp_conn->dst_port,
                    tinytcp_conn->seq_num, tinytcp_conn->ack_num);

            //TODO send FIN-ACK
<<<<<<< HEAD
=======
            tinytcp_conn->curr_state = FIN_ACK_SENT;
>>>>>>> 2602ee0004c753383617d49db3ebfb088316dfbf
            char* tinytcp_pkt = create_tinytcp_pkt(tinytcp_conn->src_port,
            tinytcp_conn->dst_port, tinytcp_conn->seq_num,
            tinytcp_conn->ack_num, 1, 0, 1, NULL, 0);
            send_to_network(tinytcp_pkt, TINYTCP_HDR_SIZE);




        } else if (tinytcp_conn->curr_state == FIN_SENT) { //FIN_ACK recvd
            //TODO update tinytcp_conn attributes
            tinytcp_conn->ack_num = seq_num + 1;
            tinytcp_conn->seq_num = ack_num;
            tinytcp_conn->curr_state = FIN_ACK_RECVD;

            fprintf(stderr, "\nFIN-ACK recvd "
                    "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
                    src_port, dst_port, seq_num, ack_num);
        }

    } else if (ack == 1) {
        //get tinytcp connection
        tinytcp_conn_t* tinytcp_conn = tinytcp_get_conn(dst_port, src_port);
        assert(tinytcp_conn != NULL);

        if (tinytcp_conn->curr_state == SYN_ACK_SENT) { //conn set up ACK
            //TODO update tinytcp_conn attributes
            tinytcp_conn->ack_num = seq_num;
            tinytcp_conn->seq_num = ack_num;
<<<<<<< HEAD

=======
>>>>>>> 2602ee0004c753383617d49db3ebfb088316dfbf
            tinytcp_conn->curr_state = CONN_ESTABLISHED;

            fprintf(stderr, "\nACK recvd "
                    "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
                    src_port, dst_port, seq_num, ack_num);

            fprintf(stderr, "\nconnection established...receiving file %s\n\n",
                    tinytcp_conn->filename);

        } else if (tinytcp_conn->curr_state == FIN_ACK_SENT) { //conn terminate ACK
            //TODO update tinytcp_conn attributes
<<<<<<< HEAD
=======
            tinytcp_conn->ack_num = seq_num + 1;
            tinytcp_conn->seq_num = ack_num;
>>>>>>> 2602ee0004c753383617d49db3ebfb088316dfbf
            tinytcp_conn->curr_state = CONN_TERMINATED;

            fprintf(stderr, "\nACK recvd "
                    "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
                    src_port, dst_port, seq_num, ack_num);

            tinytcp_free_conn(tinytcp_conn);

            fprintf(stderr, "\nfile %s received...connection terminated\n\n",
                    tinytcp_conn->filename);

        } else if (tinytcp_conn->curr_state == CONN_ESTABLISHED
            || tinytcp_conn->curr_state == READY_TO_TERMINATE) { //data ACK
            //implement this only if you are sending any data.. not necessary for
            //initial parts of the assignment!

            //TODO handle received data packets
<<<<<<< HEAD
            if (data_size == 0){
                if(ack_num == get_ring_buffer_head(tinytcp_conn->send_buffer)){
                    tinytcp_conn->num_of_dup_acks += 1;
                }
                else{
                    if(ack_num > get_ring_buffer_head(tinytcp_conn->send_buffer)){
                        pthread_spin_lock(&tinytcp_conn->mtx);
                        update_ring_buffer_head(tinytcp_conn->send_buffer, ack_num);
                        pthread_spin_unlock(&tinytcp_conn->mtx);

                        //TODO reset timer (i.e., set time_last_new_data_acked to clock())
                        //every time some *new* data has been ACKed
                        tinytcp_conn->time_last_new_data_acked = clock();
                        tinytcp_conn->num_of_dup_acks = 0;
                    }
                }
            }
            else{
                if(seq_num == tinytcp_conn->ack_num){
                    pthread_spin_lock(&tinytcp_conn->mtx);
                    uint32_t num_bytes_added = ring_buffer_add(tinytcp_conn->recv_buffer, data, data_size);
                    pthread_spin_unlock(&tinytcp_conn->mtx);

                    tinytcp_conn->ack_num = seq_num + num_bytes_added;
                }

                //TODO send back an ACK (if needed).
                char* tinytcp_pkt = create_tinytcp_pkt(tinytcp_conn->src_port,
                            tinytcp_conn->dst_port, tinytcp_conn->seq_num,
                            tinytcp_conn->ack_num, 1, 0, 0, NULL, 0);

                send_to_network(tinytcp_pkt, TINYTCP_HDR_SIZE);
            }
=======

            //TODO reset timer (i.e., set time_last_new_data_acked to clock())
            //every time some *new* data has been ACKed

            //TODO send back an ACK (if needed).
>>>>>>> 2602ee0004c753383617d49db3ebfb088316dfbf
        }
    }
}


int tinytcp_connect(tinytcp_conn_t* tinytcp_conn,
                    uint16_t cliport, //use this to initialize src port
                    uint16_t servport, //use this to initialize dst port
                    char* data, //filename is contained in the data
                    uint16_t data_size)
{
    //TODO initialize tinytcp_conn attributes.

<<<<<<< HEAD
    // updates attributes of the tiny tcp
=======
   // updates attributes of the tiny tcp
>>>>>>> 2602ee0004c753383617d49db3ebfb088316dfbf
    tinytcp_conn->curr_state = SYN_SENT;
    tinytcp_conn->src_port = cliport;
    tinytcp_conn->dst_port = servport;
    tinytcp_conn->ack_num = 0;
    strncpy(tinytcp_conn->filename, data, data_size);
    tinytcp_conn->seq_num = (rand() % (50000 - 100 + 1)) + 100;
    ISN = tinytcp_conn->seq_num;

    pthread_spin_lock(&tinytcp_conn->mtx);
    tinytcp_conn->send_buffer = create_ring_buffer(ISN + 1);
    tinytcp_conn->recv_buffer = create_ring_buffer(ISN + 1);
    pthread_spin_unlock(&tinytcp_conn->mtx);


    fprintf(stderr, "\nSYN sending "
            "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
            tinytcp_conn->src_port, tinytcp_conn->dst_port,
            tinytcp_conn->seq_num, tinytcp_conn->ack_num);

    //send SYN, put data (filename) into the packet
    char* tinytcp_pkt = create_tinytcp_pkt(tinytcp_conn->src_port,
            tinytcp_conn->dst_port, tinytcp_conn->seq_num,
            tinytcp_conn->ack_num, 0, 1, 0, data, data_size);
    send_to_network(tinytcp_pkt, TINYTCP_HDR_SIZE + data_size);

    //wait for SYN-ACK
    while (tinytcp_conn->curr_state != SYN_ACK_RECVD) {
        usleep(10);
    }

    //TODO update tinytcp_conn attributes
    // create send buffer size to 
    // create recieve buffer to

    fprintf(stderr, "\nACK sending "
            "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
            tinytcp_conn->src_port, tinytcp_conn->dst_port,
            tinytcp_conn->seq_num, tinytcp_conn->ack_num);

    //TODO send ACK
<<<<<<< HEAD
    tinytcp_conn->curr_state = CONN_ESTABLISHED;

    tinytcp_pkt = create_tinytcp_pkt(tinytcp_conn->src_port,
            tinytcp_conn->dst_port, tinytcp_conn->seq_num,
            tinytcp_conn->ack_num, 1, 0, 0, NULL, 0);
    send_to_network(tinytcp_pkt, TINYTCP_HDR_SIZE);
=======
    tinytcp_conn->curr_state = SYN_ACK_SENT;
    tinytcp_pkt = create_tinytcp_pkt(tinytcp_conn->src_port,
            tinytcp_conn->dst_port, tinytcp_conn->seq_num,
            tinytcp_conn->ack_num, 1, 0, 0, data, data_size);
    send_to_network(tinytcp_pkt, TINYTCP_HDR_SIZE + data_size);
    tinytcp_conn->curr_state = CONN_ESTABLISHED;
>>>>>>> 2602ee0004c753383617d49db3ebfb088316dfbf

    // set to connection established
    fprintf(stderr, "\nconnection established...sending file %s\n\n",
            tinytcp_conn->filename);

    return 0;
}


void handle_close(tinytcp_conn_t* tinytcp_conn)
{
    //TODO update tinytcp_conn attributes
    // change cur state to fin state
    tinytcp_conn->curr_state = FIN_SENT;

    fprintf(stderr, "\nFIN sending "
            "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
            tinytcp_conn->src_port, tinytcp_conn->dst_port,
            tinytcp_conn->seq_num, tinytcp_conn->ack_num);

    //TODO send FIN
    char* tinytcp_pkt = create_tinytcp_pkt(tinytcp_conn->src_port,
            tinytcp_conn->dst_port, tinytcp_conn->seq_num,
            tinytcp_conn->ack_num, 1, 0, 1, NULL, 0);
    send_to_network(tinytcp_pkt, TINYTCP_HDR_SIZE + 0);


    //wait for FIN-ACK
    while (tinytcp_conn->curr_state != FIN_ACK_RECVD) {
        usleep(10);
    }

    //TODO update tinytcp_conn attributes
    tinytcp_conn->curr_state = CONN_TERMINATED;

    fprintf(stderr, "\nACK sending "
            "(src_port:%u dst_port:%u seq_num:%u ack_num:%u)\n",
            tinytcp_conn->src_port, tinytcp_conn->dst_port,
            tinytcp_conn->seq_num, tinytcp_conn->ack_num);

    //TODO send ACK
<<<<<<< HEAD
=======
    tinytcp_conn->curr_state = FIN_ACK_SENT;
>>>>>>> 2602ee0004c753383617d49db3ebfb088316dfbf
    tinytcp_pkt = create_tinytcp_pkt(tinytcp_conn->src_port,
            tinytcp_conn->dst_port, tinytcp_conn->seq_num,
            tinytcp_conn->ack_num, 1, 0, 0, NULL, 0);
    send_to_network(tinytcp_pkt, TINYTCP_HDR_SIZE + 0);


    tinytcp_free_conn(tinytcp_conn);

    fprintf(stderr, "\nfile %s sent...connection terminated\n\n",
            tinytcp_conn->filename);

    return;
}

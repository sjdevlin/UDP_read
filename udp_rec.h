//
//  udp_rec.h
//  
//
//  Created by Stephen Devlin on 01/02/2021.
//

#ifndef udp_rec_h
#define udp_rec_h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <math.h>
#include <time.h>

#include "json_parsing.h"

#define TARGETPORT 9001
#define CATEGORYPORT 9000
#define APPPORT 9072
#define MAXLINE 1024
#define MAXPART 7
#define MAXSILENCE 500
#define NUM_CHANNELS 2
#define ANGLE_SPREAD 10
#define MINTURNSILENCE 30
#define MINENERGY 0.5

// to do :
// define talker as the highest average energy over last 5 secs
// interrupter is the other target if their energy is over 0.8
// sucessful interrurpt is if they then take over as the strongest

// function protos
void process_sound_data(meeting *, participant_data *, odas_data *);
void initialise_meeting_data(meeting *, participant_data *, odas_data *);
void write_to_file(char * ) ;

// structs
typedef struct odas_data{
    double x;
    double y;
    double activity;
    double frequency;
 } odas_data;

typedef struct participant_data{
    int participant_angle;
    int participant_is_talking;
    int participant_silent_time;
    int participant_total_talk_time;
    int participant_num_turns;
    float participant_frequency;
 } participant_data;

typedef struct meeting{
    int angle_array[360];
    int num_participants = 0;
    int total_silence = 0;
    int total_meeting_time = 0;
    int timestamp_last_target = 0;
    int timestamp_last_category = 0;
 } meeting;


#endif /* udp_rec_h */

// Server side implementation of UDP client-server model
#include "udp_rec.h"
#include "json_parsing.h"


int main()
{

// Interface buffers
char input_buffer[MAXLINE];
char output_buffer[MAXLINE];

// UDP variables
int in_sockfd;
int bytes_returned, buffer_size;
struct sockaddr_in in_addr;

int timestamp;

    // initialise UDP sockets input and output

    // Create socket file descriptor for app
    if ((in_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("output socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Filling app information
    in_addr.sin_family = AF_INET; // IPv4
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    in_addr.sin_port = htons(INPORT);
    
    
    // Bind the input socket for target with the server address
    if (bind(in_sockfd, (const struct sockaddr *)&in_addr,
             sizeof(in_addr)) < 0)
    {
        perror("target bind failed");
        exit(EXIT_FAILURE);
    }

    socklen_t len;
    len = sizeof(in_addr); //length data is neeeded for receive call

    // initialise arrays for input and output data

    meeting meeting_data;  // "meeting" is a struct
    participant_data * participant_data_array = (participant_data *) malloc(MAXPART * sizeof(meeting_data));  // "participant data" is a struct
    odas_data * odas_data_array = (odas_data *) malloc (NUM_CHANNELS * sizeof(odas_data));  // "odas data" is a struct

    initialise_meeting_data(&meeting_data, participant_data_array, odas_data_array);  // set everything to zero

  // need to change the "while" loop below to wait for GPIO socket

    timestamp = 0;

  while (1)
  {
      bytes_returned = recvfrom(in_sockfd, (char *)input_buffer, MAXLINE,
                                MSG_WAITALL, (struct sockaddr *)&in_addr,
                                &len);
      input_buffer[bytes_returned] = 0x00; // sets end for json parser
      if (bytes_returned > 0)
      {
        timestamp = json_parse(input_buffer, odas_data_array);
      }

    process_sound_data(&meeting_data, participant_data_array, odas_data_array, output_buffer);
    buffer_size = strlen(output_buffer);
    // send out UDP message to app
	printf ("%d  %s\n", buffer_size, output_buffer);
      
    if (meeting_data.total_silence > MAXSILENCE)
      {
          // reset all the meeting stuff and write to file
          if (meeting_data.num_participants > 0)
          {
              write_to_file (output_buffer);
              initialise_meeting_data(&meeting_data, participant_data_array, odas_data_array);
          }
      }
  }
  return 0;
}

void process_sound_data(meeting *  meeting_data, participant_data * participant_data_array, odas_data * odas_data_array, char * buffer)

{
    int target_angle;
    int iChannel, iAngle;
  
    meeting_data->total_meeting_time++;
    
    for (iChannel = 0; iChannel < NUM_CHANNELS; iChannel++)
    {
        // check if energy at the channel is above threshold and if it has been identifies as speech
        if (odas_data_array[iChannel].activity > MINENERGY)
        {
            meeting_data->total_silence = 0;
            target_angle = 180 - (atan2(odas_data_array[iChannel].x, odas_data_array[iChannel].y) * 57.3);
            
            // angle_array holds a int for every angle position.  Once an angle is set to true a person is registered there
            // so if tracked source is picked up we check to see if it is coming from a known participant
            // if it is not yet known then we also check that we havent reached max particpants before trying to add a new one
            
            //max_num_participants -1 so that we dont go out of bounds - means 0 is never used so will need to optimise
            
            if (meeting_data->angle_array[target_angle] == 0x00 && meeting_data->num_participants < (MAXPART - 1))
            {
                meeting_data->num_participants++;
                
                meeting_data->angle_array[target_angle] = meeting_data->num_participants;
//                printf ("new person %d\n", num_participants);
                participant_data_array[meeting_data->num_participants].participant_angle = target_angle;
                
                // set intial frequency high
                participant_data_array[meeting_data->num_participants].participant_frequency = 200.0;
                // write a buffer around them
                
                for (iAngle = 1; iAngle < ANGLE_SPREAD; iAngle++)
                {
                    if (target_angle + iAngle < 360)
                    {
                        // could check if already set here - but for now will just overwrite
                        // 360 is for going round the clock face
                        
                        meeting_data->angle_array[target_angle + iAngle] = meeting_data->num_participants;
                    }
                    else
                    {
                        meeting_data->angle_array[iAngle - 1] = meeting_data->num_participants;
                    }
                    if (target_angle - iAngle >= 0)
                    {
                        // could check if already set here - but for now will just overwrite
                        // 360 is for going round the clock face
                        meeting_data->angle_array[target_angle - iAngle] = meeting_data->num_participants;
                    }
                    else
                    {
                        meeting_data->angle_array[361 - iAngle] = meeting_data->num_participants;
                    }
                }
                
                participant_data_array[meeting_data->num_participants].participant_is_talking = 10 * odas_data_array[iChannel].activity;
            }
            else // its an existing talker we're hearing
            {
                // could put logic in here to count turns
                participant_data_array[meeting_data->angle_array[target_angle]].participant_is_talking = 10 * odas_data_array[iChannel].activity;
                participant_data_array[meeting_data->angle_array[target_angle]].participant_total_talk_time++;
                
                if (odas_data_array[iChannel].frequency > 0.0) {
                    participant_data_array[meeting_data->angle_array[target_angle]].participant_frequency = (0.9 *  participant_data_array[meeting_data->angle_array[target_angle]].participant_frequency) + (0.1 * odas_data_array[iChannel].frequency);
                }
            }
        }
        else
        {
            meeting_data->total_silence++;
        }
    }
    
    // now preapre the buffer for socket and file output
    buffer[0] = 0x00;
    
    sprintf(buffer, "%s{\n", buffer);
    sprintf(buffer, "%s    \"totalMeetingTime\": %d,\n", buffer, meeting_data->total_meeting_time);
    sprintf(buffer, "%s    \"message\": [\n", buffer);
    
    int i;
    for (i = 1; i <= meeting_data->num_participants ; i++)
    {
        sprintf(buffer, "%s { \"memNum\": %d,", buffer, i);
        sprintf(buffer, "%s \"angle\": %d,", buffer, participant_data_array[i].participant_angle);
        sprintf(buffer, "%s \"talking\": %d,", buffer, participant_data_array[i].participant_is_talking);
        sprintf(buffer, "%s \"numTurns\": %d,", buffer, participant_data_array[i].participant_num_turns);
        sprintf(buffer, "%s \"freq\": %3.0f,", buffer, participant_data_array[i].participant_frequency);
        sprintf(buffer, "%s \"totalTalk\": %d}", buffer, participant_data_array[i].participant_total_talk_time);
        
        if (participant_data_array[i].participant_is_talking>0) {
            // not sure this logic is necessary - prob all targets go to zero before dropping out
            participant_data_array[i].participant_is_talking = 0x00;
            if (participant_data_array[i].participant_silent_time > MINTURNSILENCE) {
                participant_data_array[i].participant_num_turns++;
                participant_data_array[i].participant_silent_time=0;}
        } else
        {
            participant_data_array[i].participant_silent_time++;
        };
        
        if (i != (meeting_data->num_participants))
        {
            sprintf(buffer, "%s,", buffer);
        }
        sprintf(buffer, "%s\n", buffer);
    }
    
    sprintf(buffer, "%s    ]\n", buffer);
    sprintf(buffer, "%s}\n", buffer);
//    printf ("%s\n",buffer);    
}

void initialise_meeting_data(meeting * meeting_data, participant_data * participant_data_array, odas_data * odas_data_array) {
    
    int i;
    
    // initialise meeting array
    for (i=0;i<MAXPART;i++) {
        participant_data_array[i].participant_angle = 0;
        participant_data_array[i].participant_is_talking= 0;
        participant_data_array[i].participant_silent_time= 0;
        participant_data_array[i].participant_total_talk_time= 0;
        participant_data_array[i].participant_num_turns= 0;
        participant_data_array[i].participant_frequency= 150.0;        
    }
    
    for (i=0;i<NUM_CHANNELS;i++) {
        odas_data_array[i].x = 0.0;
        odas_data_array[i].y = 0.0;
        odas_data_array[i].activity = 0.0;
        odas_data_array[i].frequency = 0.0;
    }
    
    for (i=0;i<360;i++) {
        meeting_data->angle_array[i] = 0;
    }
    
    meeting_data->total_silence = 0;
    meeting_data->total_meeting_time = 0;
    meeting_data->num_participants = 0;

}

void write_to_file(char * buffer) {
    
    struct tm *timenow;
    char filename[62];
    FILE *filepointer;
    
    memset(&filename, 0, sizeof(filename));
    time_t now = time(NULL);
    timenow = gmtime(&now);
    memset(filename, 0, sizeof(filename));
    sprintf(filename, "MP_%d", (int)now);
    
    filepointer = fopen(filename, "wb");
    
    if (filepointer == NULL)
    {
        printf("Cannot open file %s\n", filename);
        exit(EXIT_FAILURE);
    }
    
    fwrite(buffer, sizeof(char), strlen(buffer), filepointer);
    fclose(filepointer);

    
    
}

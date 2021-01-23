// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <json-c/json.h>
#include <time.h>

#define TARGETPORT 9001
#define CATEGORYPORT 9000
#define MAXLINE 512
#define MAXPART 7
#define MAXSILENCE 90

// function protos
unsigned int json_parse(json_object *, const int);
void json_parse_array(json_object *, char *);
void process_sound_data(const int);

double targetX[4];
double targetY[4];
double energy[4];
double frequency[4];
int angle_array[360];
int participant_angle[MAXPART];
int participant_is_talking[MAXPART];
int participant_silent_time[MAXPART];
int participant_total_talk_time[MAXPART];
float participant_frequency[MAXPART];

int num_participants = 0;
int total_silence = 0;
int total_meeting_time = 0;

// main JSON parser

unsigned int json_parse(json_object *jobj, const int index)
{
  enum json_type type;
  unsigned int timeStamp = 0;

  json_object_object_foreach(jobj, key, val)
  {

    type = json_object_get_type(val);
    switch (type)
    {
    case json_type_int:
      if (!strcmp(key, "timeStamp"))
        timeStamp = json_object_get_int(val) / 20;
      break;
    case json_type_double:
      if (!strcmp(key, "x"))
      {
        targetX[index] = json_object_get_double(val);
      }
      else if (!strcmp(key, "y"))
      {
        targetY[index] = json_object_get_double(val);
      }
      else if (!strcmp(key, "activity"))
      {
        energy[index] = json_object_get_double(val);
      }
      else if (!strcmp(key, "freq"))
      {
        frequency[index] = json_object_get_double(val);
      }
      break;
    case json_type_array:
      json_parse_array(jobj, key);
      break;
    }
  }
  return timeStamp;
}

void json_parse_array(json_object *jobj, char *key)
{

  enum json_type type;

  json_object *jarray = jobj;

  if (key)
  {
    if (!json_object_object_get_ex(jobj, key, &jarray))
    {
      printf("Error parsing json object\n");
      return;
    }
  }

  int arraylen = json_object_array_length(jarray);
  int i;
  json_object *jvalue;

  for (i = 0; i < arraylen; i++)
  {
    jvalue = json_object_array_get_idx(jarray, i);
    type = json_object_get_type(jvalue);

    if (type == json_type_array)
    {
      json_parse_array(jvalue, NULL);
    }
    else if (type != json_type_object)
    {
    }
    else
    {
      json_parse(jvalue, i);
    }
  }
}

// Driver code
int main()
{

  int target_socket = 9001;
  int category_socket = 9000;

  int target_sockfd, category_sockfd;
  char target_buffer[MAXLINE];
  char category_buffer[MAXLINE];

  json_object *jobj;

  struct sockaddr_in target_addr;
  struct sockaddr_in category_addr;

  // Create socket file descriptor for target
  if ((target_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("target socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Create socket file descriptor for category
  if ((category_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("category socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&target_addr, 0, sizeof(target_addr));
  memset(&category_addr, 0, sizeof(category_addr));
  memset(&angle_array, 0, sizeof(angle_array));
  memset(&participant_angle, 0, sizeof(participant_angle));
  memset(&participant_is_talking, 0, sizeof(participant_is_talking));
  memset(&participant_total_talk_time, 0, sizeof(participant_total_talk_time));

  // Filling target information
  target_addr.sin_family = AF_INET; // IPv4
  target_addr.sin_addr.s_addr = INADDR_ANY;
  target_addr.sin_port = htons(target_socket);

  // Filling catgeory information
  category_addr.sin_family = AF_INET; // IPv4
  category_addr.sin_addr.s_addr = INADDR_ANY;
  category_addr.sin_port = htons(category_socket);

  // Bind the socket with the server address
  if (bind(target_sockfd, (const struct sockaddr *)&target_addr,
           sizeof(target_addr)) < 0)
  {
    perror("target bind failed");
    exit(EXIT_FAILURE);
  }

  // Bind the socket with the server address
  if (bind(category_sockfd, (const struct sockaddr *)&category_addr,
           sizeof(category_addr)) < 0)
  {
    perror("category bind failed");
    exit(EXIT_FAILURE);
  }

  socklen_t len;
  int bytes_returned;
  int timestamp_last_target = 0;
  int timestamp_last_category = 0;

  len = sizeof(category_addr); //len is value/resuslt

  // while timestamp  < timesyamp

  while (1)
  {

    while (timestamp_last_target <= timestamp_last_category)
    {

      bytes_returned = recvfrom(target_sockfd, (char *)target_buffer, MAXLINE,
                                MSG_WAITALL, (struct sockaddr *)&target_addr,
                                &len);

      target_buffer[bytes_returned] = 0x00; // sets end for json parser
      if (bytes_returned > 0)
      {
        jobj = json_tokener_parse(target_buffer);
        timestamp_last_target = json_parse(jobj, 0);

        //printf("Target : %s\n", target_buffer);
        //printf("timestamp: %d", timestamp_last_target);
      }
    }

    while (timestamp_last_category < timestamp_last_target)
    {

      bytes_returned = recvfrom(category_sockfd, (char *)category_buffer, MAXLINE,
                                MSG_WAITALL, (struct sockaddr *)&category_addr,
                                &len);

      category_buffer[bytes_returned] = 0x00; // sets end for json parser

      if (bytes_returned > 0)
      {
        jobj = json_tokener_parse(category_buffer);
        timestamp_last_category = json_parse(jobj, 0);
      }
    }
    process_sound_data(timestamp_last_target);
  }
  //    printf("Category : %s\n", category_buffer);

  /*    for (int i = 0; i<3; i++ ) {
        if (energy[i] > 0.1) {
        printf ("X: %3.2f, Y: %3.2f, E: %3.2f, f: %3.2f\n", targetX[i], targetY[i], energy[i], frequency[i]);
        }
    }
        }
      }*/

  return 0;
}

void process_sound_data(const int timeStamp)
{

  int target_angle;
  int iChannel, iAngle;
  int angle_spread = 15;

  total_meeting_time++;

  for (iChannel = 0; iChannel < 4; iChannel++)
  {

    // check if energy at the channel is above threshold and if it has been identifies as speech

    if (energy[iChannel] > 0.3)
    {

      total_silence = 0;

      target_angle = 180 - (atan2(targetX[iChannel], targetY[iChannel]) * 57.3);

      // angle_array holds a booelan for every angle position.  Once an angle is set to true a person is registered there
      // so if tracked source is picked up we check to see if it is coming from a known participant
      // if it is not yet known then we also check that we havent reached max particpants before trying to add a new one

      //max_num_participants -1 so that we dont go out of bounds - means 0 is never used so will need to optimise

      if (angle_array[target_angle] == 0x00 && num_participants < MAXPART - 1)
      {

        num_participants++;

        angle_array[target_angle] = num_participants;

        participant_angle[num_participants] = target_angle;
        // write a buffer around them

        for (iAngle = 1; iAngle < angle_spread; iAngle++)
        {

          if (target_angle + iAngle < 360)
          {
            // could check if already set here - but for now will just overwrite
            // 360 is for going round the clock face

            angle_array[target_angle + iAngle] = num_participants;
          }
          else
          {
            angle_array[iAngle - 1] = num_participants;
          }

          if (target_angle - iAngle >= 0)
          {
            // could check if already set here - but for now will just overwrite
            // 360 is for going round the clock face
            angle_array[target_angle - iAngle] = num_participants;
          }
          else
          {
            angle_array[361 - iAngle] = num_participants;
          }
        }

        participant_is_talking[num_participants] = 0x01;
        printf("\nnew entrant %d  Angle:%d\n", num_participants, target_angle);
      }
      else // its an existing talker we're hearing

      {

        //                    last_talker=i;
        //                    this is to start getting an average angle
        //                    participant[buffer[target_angle]].angle = .9 * new_sample + (.1) * ma_old;
        //
        participant_is_talking[angle_array[target_angle]] = 1;
        participant_total_talk_time[angle_array[target_angle]]++;
      }
    }
    else
    {

      total_silence++;
    }
  }

  // now preapre the buffer for socket and file output
  char buffer[MAXLINE];

  buffer[0] = 0x00;
  int i, bufferSize;

  sprintf(buffer, "%s{\n", buffer);
  sprintf(buffer, "%s    \"timeStamp\": %d,\n", buffer, total_meeting_time);
  sprintf(buffer, "%s    \"message\": [\n", buffer);

  for (i = 1; i < (num_participants + 1); i++)
  {
    sprintf(buffer, "%s { \"memNum\": %d,", buffer, i);
    sprintf(buffer, "%s \"angle\": %d,", buffer, participant_angle[i]);
    sprintf(buffer, "%s \"talking\": %d,", buffer, participant_is_talking[i]);
    sprintf(buffer, "%s \"totalTalk\": %d}", buffer, participant_total_talk_time[i]);

    participant_is_talking[i] = 0x00;

    if (i != (num_participants))
    {

      sprintf(buffer, "%s,", buffer);
    }

    sprintf(buffer, "%s\n", buffer);
  }

  sprintf(buffer, "%s    ]\n", buffer);
  sprintf(buffer, "%s}\n", buffer);
  printf("%s", buffer);

  bufferSize = strlen(buffer);

  if (total_silence > MAXSILENCE)
  {
    // reset all the meeting stuff and write to file
    total_silence = 0;
    total_meeting_time = 0;

    if (num_participants > 0)
    {
      memset(&angle_array, 0, sizeof(angle_array));
      memset(&participant_angle, 0, sizeof(participant_angle));
      memset(&participant_is_talking, 0, sizeof(participant_is_talking));
      memset(&participant_total_talk_time, 0, sizeof(participant_total_talk_time));
      num_participants = 0;

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

      fwrite(buffer, sizeof(char), bufferSize, filepointer);
      fclose(filepointer);
    }
  }
}

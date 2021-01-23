// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <json-c/json.h>
  
#define PORT     8080 
#define MAXLINE 1024 

// function protos
unsigned int json_parse(json_object * jobj);
void json_parse_array(json_object *jobj, char *key); 

double targetX [4];
double targetY [4];
double energy [4];
double frequency [4];

// Driver code 
int main() { 



    int target_socket = 9000;
    int category_socket = 9001;

    int target_sockfd, category_sockfd; 
    char target_buffer[MAXLINE]; 
    char category_buffer[MAXLINE]; 

    json_object *jobj;

    struct sockaddr_in target_addr; 
    struct sockaddr_in category_addr; 

  

    // Create socket file descriptor for target 
    if ( (target_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("target socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    // Create socket file descriptor for category 
    if ( (category_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("category socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    memset(&target_addr, 0, sizeof(target_addr)); 
    memset(&category_addr, 0, sizeof(category_addr)); 
      
    // Filling target information 
    target_addr.sin_family    = AF_INET; // IPv4 
    target_addr.sin_addr.s_addr = INADDR_ANY; 
    target_addr.sin_port = htons(target_socket); 
      
    // Filling catgeory information 
    category_addr.sin_family    = AF_INET; // IPv4 
    category_addr.sin_addr.s_addr = INADDR_ANY; 
    category_addr.sin_port = htons(category_socket); 


    // Bind the socket with the server address 
    if ( bind(target_sockfd, (const struct sockaddr *)&target_addr,  
            sizeof(target_addr)) < 0 ) 
    { 
        perror("target bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    // Bind the socket with the server address 
    if ( bind(category_sockfd, (const struct sockaddr *)&category_addr,  
            sizeof(category_addr)) < 0 ) 
    { 
        perror("category bind failed"); 
        exit(EXIT_FAILURE); 
    } 


    socklen_t len;
    int bytes_returned; 
    int timestamp_last_target = 0;
    int timestamp_last_category = 0;

    len = sizeof(category_addr);  //len is value/resuslt 
  
  // while timestamp  < timesyamp

    while (timestamp_last_target <= timestamp_last_category) {

    bytes_returned = recvfrom(target_sockfd, (char *)target_buffer, MAXLINE,  
                MSG_WAITALL, ( struct sockaddr *) &target_addr, 
                &len); 

    target_buffer[bytes_returned] = 0x00;  // sets end for json parser
    if (bytes_returned > 0) {
    jobj = json_tokener_parse(target_buffer);
    timestamp_last_target = json_parse(jobj);

    printf("Client : %s\n", target_buffer); 
        }

    }

    while (timestamp_last_category < timestamp_last_category) {

    bytes_returned = recvfrom(category_sockfd, (char *)category_buffer, MAXLINE,  
                MSG_WAITALL, ( struct sockaddr *) &category_addr, 
                &len); 

    category_buffer[bytes_returned] = 0x00;  // sets end for json parser

    if (bytes_returned > 0) {
    jobj = json_tokener_parse(category_buffer);
    timestamp_last_category = json_parse(jobj);

    printf("Client : %s\n", category_buffer); 
    }
    }

    return 0; 

    for (int i = 0; i<4; i++ ) {

        printf ("X: %3.2f, Y: %3.2f, E: %3.2f, f: %3.2f", targetX[i], targetY[i], energy[i], frequency[i]);

    }


} 


///  Json parsing...



// main JSON parser

unsigned int json_parse(json_object *jobj) {
  enum json_type type;
  unsigned int count = 0;
  unsigned int timeStamp = 0;

  json_object_object_foreach(jobj, key, val) {

    type = json_object_get_type(val);
    switch (type) {
      case json_type_int:
        if (!strcmp (key,"timeStamp")) timeStamp = json_object_get_int(val);
        if (!strcmp(key, "id"))  count = json_object_get_int(val);
        break;
      case json_type_double:
        if (!strcmp(key, "x")) {
          targetX[count] = json_object_get_double(val);
        } else if (!strcmp(key, "y")) {
          targetY[count] = json_object_get_double(val);
        } else if (!strcmp(key, "E")) {
          energy[count] = json_object_get_double(val);
        } else if (!strcmp(key, "f")) {
          frequency[count] = json_object_get_double(val);
        }
        break;
      case json_type_array:
        json_parse_array(jobj, key);
        break;
    }
  }
return timeStamp;
}

void json_parse_array(json_object *jobj, char *key) {

  enum json_type type;

  json_object *jarray = jobj;

  if (key) {
    if (json_object_object_get_ex(jobj, key, &jarray) == false) {
      printf("Error parsing json object\n");
      return;
    }
  }

  int arraylen = json_object_array_length(jarray);
  int i;
  json_object *jvalue;

  for (i = 0; i < arraylen; i++) {
    jvalue = json_object_array_get_idx(jarray, i);
    type = json_object_get_type(jvalue);

    if (type == json_type_array) {
      json_parse_array(jvalue, NULL);
    } else if (type != json_type_object) {
    } else {
      json_parse(jvalue);
    }
  }
}

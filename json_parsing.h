//
//  json_parsing.h
//  
//
//  Created by Stephen Devlin on 01/02/2021.
//

#ifndef json_parsing_h
#define json_parsing_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <json-c/json.h>

unsigned int json_parse(json_object *, const int);
void json_parse_array(json_object *, char *);


#endif /* json_parsing_h */

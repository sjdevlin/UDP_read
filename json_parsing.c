//
//  json_parsing.c
//  
//
//  Created by Stephen Devlin on 01/02/2021.
//

#include "json_parsing.h"

typedef struct odas_data{
    double x;
    double y;
    double activity;
    double frequency;
 } odas_data;

unsigned int json_parse(char *buffer, odas_data * odas_array, const int index)
{
    json_object *jobj
    enum json_type type;
    jobj = json_tokener_parse(buffer);

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
        odas_array[index].x = json_object_get_double(val);
      }
      else if (!strcmp(key, "y"))
      {
          odas_array[index].y = json_object_get_double(val);
      }
      else if (!strcmp(key, "activity"))
      {
          odas_array[index].activity = json_object_get_double(val);
      }
      else if (!strcmp(key, "freq"))
      {
        odas_array[index].frequency = json_object_get_double(val);
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


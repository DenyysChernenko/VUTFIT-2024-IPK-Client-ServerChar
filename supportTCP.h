#ifndef supportTCP_H
#define supportTCP_H

#include <stdbool.h>

// Declaration

bool is_valid_secret(const char* str); 
bool is_valid_dname(const char* str); 
bool is_valid_id(const char* str);
bool is_valid_content(const char* content);
#endif
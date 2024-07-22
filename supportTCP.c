#include "supportTCP.h"
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// SUPPORT IS FUNCTIONS, WHICH NOT RELATED TO SERVER BUT HELPING IN HANDLING CORRECT GRAMMAR

// Function to validate ID
bool is_valid_id(const char* id) {
    int len = strlen(id);
    if (len < 1 || len > 20) return false;
    for (int i = 0; i < len; ++i) {
        if (!isalnum(id[i]) && id[i] != '-') return false;
    }
    return true;
}

// Function to validate SECRET
bool is_valid_secret(const char* secret) {
    int len = strlen(secret);
    if (len < 1 || len > 128) return false;
    for (int i = 0; i < len; ++i) {
        if (!isalnum(secret[i]) && secret[i] != '-') return false;
    }
    return true;
}

// Function to validate CONTENT
bool is_valid_content(const char* content) {
    int len = strlen(content);
    if (len < 1 || len > 1400) return false;
    for (int i = 0; i < len; ++i) {
        // Skip (allow) carriage return (CR) and line feed (LF) characters
        if (content[i] == '\r' || content[i] == '\n') continue;

        // Validate character is printable or space
        if (!isprint(content[i]) && content[i] != ' ') {
            fprintf(stderr, "Invalid character at position %d: [0x%02X]\n", i, (unsigned char)content[i]);
            return false;
        }
    }
    return true;
}

// Function to validate DNAME
bool is_valid_dname(const char* dname) {
    int len = strlen(dname);
    if (len < 1 || len > 20) return false;
    for (int i = 0; i < len; ++i) {
        // Ensure every character is printable; control characters are not allowed
        if (!isprint(dname[i])) return false;
    }
    return true;
}
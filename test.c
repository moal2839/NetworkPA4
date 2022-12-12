#include<stdio.h>
#include<stdlib.h>
#include "md5.h"

void print_hash(uint8_t *p){
	char digest[33];
    bzero(digest, 33);

    sprintf(digest, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", p[0], 
    p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13],
    p[14], p[15]);

    printf("%s\n", digest);
}

int main(int argc, char** argv) {  
    uint8_t *result1;
    uint8_t *result2;

    char* test1 = "foo.txt";
    char* test2 = "bar.txt";

    result1 = md5String(test1);
    result2 = md5String(test2);

    printf("%d %d\n", *result1, *result2);

    print_hash(result1);
    print_hash(result2);

    return 0;
}
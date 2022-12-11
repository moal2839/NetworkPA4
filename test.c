#include<stdio.h>
#include<stdlib.h>
#include "md5.h"

void print_hash(uint8_t *p){
	for(unsigned int i = 0; i < 16; ++i){
		printf("%02x", p[i]);
	}
	printf("\n");
}

int main(int argc, char** argv) {  
    uint8_t *result1;
    uint8_t *result2;

    char* test1 = "foo.txt";
    char* test2 = "bar.txt";

    result1 = md5String(test1);
    result2 = md5String(test2);

    print_hash(result1);
    print_hash(result2);

    return 0;
}
#include "llcirq.h"
#include <stdio.h>

#define TESTQ_1 8

int main(){
	llcir_qp q = init_llcirq(1,TESTQ_1,1);
	char datum_1[] = {'a','b','c','d','e','f','g','h','i'};;
	char datum_2[] = {'x','y','z'};;

	for(int i = 0; i < sizeof(datum_1); i++){
		enqueue(q,&datum_1[i]);
	}

	char check;
	for(int i = 0; i < sizeof(datum_1); i++){
		if(!dequeue(q,&check) && i < TESTQ_1 - 1)
			printf("Attempt to check element %d failure\n",i);
	}
	for(int i = 0; i < sizeof(datum_2); i++){
		enqueue(q,&datum_2[i]);
	}
	for(int i = 0; i < 10; i++){
		if(!dequeue(q,&check) && i < sizeof(datum_2)){
			printf("Attempt to check element %d failure\n",i);
		}
		if(i < sizeof(datum_2) && check != datum_2[i]){
			printf("Data check failed, expected to return %c but returned %c\n",datum_2[i],check);
		}
	}

	return 1;
}

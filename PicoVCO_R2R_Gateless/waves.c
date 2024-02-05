/*
TODO
* square
* sine
* some interesting wave tables perhaps?
*/

#include "waves.h"

void generate_triange(int *wave, uint length, uint peak) {
	for(int i=0; i< length/2; i++) {
		wave[i] = (int)((float)i*((float)peak/(length/2)));
	}
	for(int i=0; i<length/2; i++) {
		wave[(length/2)+i] = peak - (int)((float)i*((float)peak/(length/2)));
	}	
}
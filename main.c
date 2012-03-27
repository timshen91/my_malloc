#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "malloc.h"

#define MAX 100000
void * a[MAX];

int main() {
    srandom(0);
    int i = 0;
    while (1) {
        if (i == 0 || (i < MAX && random() > RAND_MAX / 2)) {
            int size = 16384;
            if ((a[i] = malloc(size)) != NULL) {
                i++;
            }
            //fprintf(stderr, "malloc(%llu) = %p\n", size, a[i-1]);
        } else {
            int now = random() % i;
            //fprintf(stderr, "free(%p)\n", a[now]);
            free(a[now]);
            a[now] = a[--i];
        }
        //printf("\n");
    }
    return 0;
};

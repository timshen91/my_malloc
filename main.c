#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 100000
void * a[MAX];

int main() {
    srandom(0);
    int i = 0;
    int times = 1000000;
    while (times--) {
        long cho = random();
        if (i == 0 || (i < MAX && cho >= RAND_MAX / 3 * 2)) {
            int size = random() % 16384;
            //fprintf(stderr, "malloc(%u)\n", size);
            if ((a[i] = malloc(size)) != NULL) {
                i++;
            }
        } else if (cho < RAND_MAX / 3){
            int now = random() % i;
            //fprintf(stderr, "free(%p)\n", a[now]);
            free(a[now]);
            a[now] = a[--i];
        } else {
            int now = random() % i;
            int size = random() % 16384;
            //fprintf(stderr, "realloc(%p, %u)\n", a[now], size);
            if ((a[now] = realloc(a[now], size)) == NULL) {
                a[now] = a[--i];
            }
        }
        //fprintf(stderr, "\n");
    }
    while (i) {
        free(a[--i]);
    }
    return 0;
};

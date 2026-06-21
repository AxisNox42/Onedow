#include <stdio.h>
#include "WiNILL/Entity/Augment.h"
int main() {
    int n = (int)(sizeof(ALL_AUGS)/sizeof(ALL_AUGS[0]));
    printf("sizeof count=%d AUG_TOTAL=%d\n", n, AUG_TOTAL);
    return n != AUG_TOTAL;
}

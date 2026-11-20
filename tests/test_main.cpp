#include "test.h"

int testFailures = 0;

int main()
{
    testMotion();
    testBleCodec();

    if (testFailures == 0)
    {
        printf("all tests passed\n");
        return 0;
    }

    printf("%d test(s) failed\n", testFailures);
    return 1;
}

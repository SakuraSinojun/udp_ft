
#include "UfConvert.h"
#include <stdio.h>

std::string UfConvert::data2Hex(const char * data, int len)
{
    std::string str;
    int i;
    for (i = 0; i < len; i++) {
        char    temp[100];
        snprintf(temp, sizeof(temp), "%02X", data[i] & 0xff);
        str += temp;
    }
    return str;
}

#ifdef __UNITTEST__
#include <stdio.h>
#include <string.h>
#include "tools/dump.h"
int main()
{
    const char * a = "show me the money.";
    std::string str = UfConvert::data2Hex(a, strlen(a));

    printf("%s\n", str.c_str());
    tools::dump(a, strlen(a));
    return 0;
}
#endif


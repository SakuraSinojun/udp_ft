
#include <string.h>
#include "tools/logging.h"
#include "Sft.h"
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>

class SftDelegate : public Sft::Delegate {
public:
    SftDelegate * o;

    virtual void send(const char * buffer, int len) {
        o->onDataReceived(buffer, len);
    }

    virtual void onResult(int result) {
        RUN_HERE() << "result = " << result;
    }

    virtual void onPercent(float percent) {
        // RUN_HERE() << "percent = " << percent;
        printf("\r%0.2f        ", percent);
        fflush(stdout);
        usleep(100);
        if (percent >= 100) {
            printf("\n");
        }
    }
};

int main()
{
    SftDelegate     ds, dc;

    ds.o = &dc;
    dc.o = &ds;

    Sft s(ds);
    Sft c(dc);

    unlink("aaa.tmp");
    c.requestFile("/home/kazuki/projects/Trunk.rar", "aaa.tmp");

    return 0;
}


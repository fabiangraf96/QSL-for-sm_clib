#include "dn_qsl_api.h"

#define NETID           0       // factory default value used if zero (1229)
#define JOINKEY         NULL    // factory default value used if NULL (44 55 53 54 4E 45 54 57 4F 52 4B 53 52 4F 43 4B)
#define BANDWIDTH_MS    0       // not changed if zero (default base bandwidth given by manager is 9 s)
#define DEST_IP         NULL    // NULL defaults to the manager (FF02::2)


int main(void)
{
    my_data_struct_t myData;
    uint8_t bytesRead;
    uint8_t* myBuffer;

    if (!dn_qsl_init())
    {
        return; // Initialization failed
    }

    while (dn_qsl_isConnected())
    {
        /*
        Prepare myData
        */
        if (!dn_qsl_send(&myData, sizeof(myData)), DEST_IP)
        {
            // Pushing data to the manager failed
        }
        bytesRead = dn_qsl_read(&myBuffer)
        if (bytesRead > 0)
        {
            /*
            Handle received data
            */
        }
        /*
        Wait for next period
        */
    }
    else
    {
        if (!dn_qsl_connect(NETID, JOINKEY, BANDWIDTH_S))
        {
            // Connecting to network failed
        }
    }
}

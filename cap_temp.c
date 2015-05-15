#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h> /* for strncpy */
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <net/if.h>


/// DEFS ///
#define BUFLEN 512
#define INTERFACE "eth0"
#define PORT 9930
#define SRV_IP "192.168.75.131 " // UPDATE VALUE FOR NEW SERVER IP
#define CAP_NAME "Temperature Sensor";


#define PORT2 1337
/// END OF DEFS ///


/// UTIL ///
int getTemp()
{
    srand(time(NULL));

    int r = 0;
    do
    {
        r = rand();
    }
    while(r>45 || r<0);
    return r;
}


int setTemp(int t)
{
    return t;
}

void diep(char *s)
{
    perror(s);
    exit(1);
}


char* getIP()
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, INTERFACE, IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);
    return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}
/// END UTIL ///


/// STRUCT ////
struct Sensor
{
    char* ip;
    char* port;
    char* label;
    char* actions[3];
};


struct Sensor *createSensor()
{
    struct Sensor *mySensor = malloc(sizeof(struct Sensor));
    assert(mySensor != NULL);
    mySensor->label = "";
    mySensor->actions[0] = "NotAv";
    mySensor->actions[1] = "NotAv";
    mySensor->actions[2] = "NotAv";
    mySensor->ip = getIP();
    mySensor->port = "NotDEF";
    return mySensor;
}

char * getInitialMessage(struct Sensor *s)
{
    char* msg;
    char port[20];
    msg = malloc(512);
    strcpy(msg, s->label);
    strcat(msg, "#");
    strcat(msg, s->actions[0]);
    strcat(msg, "#");
    strcat(msg, s->actions[1]);
    strcat(msg, "#");
    strcat(msg, s->actions[2]);
    strcat(msg, "#");
    strcat(msg, s->ip);
    strcat(msg, "#");
    snprintf (port, sizeof(port), "%d",PORT2);
    strcat(msg,port); /// CAST TO STRING @TODO

    return msg;
}
/// END STRUCT ///




int main(void)
{
    /// SOCKET ONE
    struct sockaddr_in si_other;
    int s, i, slen=sizeof(si_other);
    char buf[BUFLEN];

    /// SOCKET 2
    struct sockaddr_in si_me, si_other2;
    int sock2, i2, slen2=sizeof(si_other2);
    char buf2[BUFLEN];
    char tmpBUF[BUFLEN];
    ////

    //TEST IF First RUN
    int firstRun = 1;
    //VARS FOR SENSOR
    struct Sensor *mySensor = createSensor();
    mySensor->label = CAP_NAME;
    mySensor->actions[0] = "GetTemp";
    mySensor->actions[1] = "SetTemp";

    char* msg;
    msg = malloc(512);

    msg = getInitialMessage(mySensor);

    /////
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        diep("socket");

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    if (inet_aton(SRV_IP, &si_other.sin_addr)==0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    ////
    /// SOCKET PREP ///
    if ((sock2=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        diep("socket 2");

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT2);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock2,(struct sockaddr *) &si_me, sizeof(si_me))==-1)
        diep("bind");
    /// END SOCKET PREP ///



    while(1)
    {
        if(firstRun!=0)
        {
            printf("FIRST RUN \n");
            printf("%s", mySensor->label);
            firstRun =0;
            //SEND INITIAL DISVOVERY MESSAGE
            if (sendto(s, msg, BUFLEN, 0,(struct sockaddr *) &si_other, slen)==-1)
            {
                diep("sendto()");
            }

        }
        else
        {
            /// GET ACTIVATED ///
            sleep(1);
            int k;
            for(k=0; k<100000 ; k++) {}


            if (recvfrom(s, buf2, BUFLEN, 0,(struct sockaddr *) &si_other, &slen)==-1)
            {
                printf("error");
                diep("recvfrom()");
            }

            strcpy(tmpBUF,buf2);

            if(tmpBUF[0]=='1')
            {
                printf("\nTEMPS : %d \n",getTemp());

                char temperature[5];
                snprintf (temperature, sizeof(temperature), "%d",getTemp());

                if (sendto(s, temperature, BUFLEN, 0,(struct sockaddr *) &si_other, slen)==-1)
                {
                    diep("sendto()");
                }

            }




            printf("\nGOT SOCK2 : %s \n",buf2);



            /// END GET ACTIVATED ////


        }
    }
    close(s);
    close(sock2);
    return 0;
}

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>


#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <semaphore.h>

#define BUFLEN 512

#define SHMSZ 1024

sem_t * sem_id;

/// UTIL ///
void diep(char *s)
{
    perror(s);
    exit(1);
}

/**
* @Name str_split
* @Param char* a_str , const char a_delim
* @Return char** result
*/
char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void signal_callback_handler(int signum)
{

        /**
         * Semaphore unlink: Remove a named semaphore  from the system.
         */
        if ( shm_unlink("/mysem") < 0 )
        {
                perror("shm_unlink");
        }

        /**
         * Semaphore Close: Close a named semaphore
         */
        if ( sem_close(sem_id) < 0 )
        {
        	perror("sem_close");
        }

        /**
         * Semaphore unlink: Remove a named semaphore  from the system.
         */
        if ( sem_unlink("/mysem") < 0 )
        {
        	perror("sem_unlink");
        }
   // Terminate program
   exit(signum);
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

/**
* @Name createSensor
* @Param char* msg
* @Return Sensor : mySensor
*/
struct Sensor *createSensor(char * msg)
{
    struct Sensor *mySensor = malloc(sizeof(struct Sensor));
    assert(mySensor != NULL);

    char** tokens;
    tokens = str_split(msg, '#');

    if (tokens)
    {
        mySensor->label = *(tokens +0);
        mySensor->actions[0] = *(tokens +1);
        mySensor->actions[1] = *(tokens +2);
        mySensor->actions[2] = *(tokens +3);
        mySensor->ip = *(tokens +4);
        mySensor->port = *(tokens +5);

        free(tokens);
    }

    return mySensor;
}

/// END SENSOR ///

int main()
{
    /// SOCKET ONE
    struct sockaddr_in si_other;
    int s, i, slen=sizeof(si_other);
    char buf[BUFLEN];
    /// END OF SOCKET ONE ///


    /// VARS FOR SHARED MEMORY ///
    int shmid;
    key_t key;
    char *shm, *s1, *tmp;

    key = 5678;
    /// END OF VARS FOR SHARED MEMORY ///


    /// RES VARS FOR SHARED MEMORY ///
    int shmid2;
    key_t key2;
    char *shm2, *s2;

    key2 = 1234;
    /// RES ŖEND OF VARS FOR SHARED MEMORY ///

	signal(SIGINT, signal_callback_handler);
	/**
	 * Semaphore open
	 */
	sem_id=sem_open("/mysem", O_CREAT, S_IRUSR | S_IWUSR, 1);


    /// SHARED MEMORY PREP ///
    /*
     * Locate the segment.
     */
    if ((shmid = shmget(key, 27, 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1)
    {
        perror("shmat");
        exit(1);
    }
    /// END OF SHARED MEMORY 1 ///

     /// (2) SHARED MEMORY PREP ///
    if ((shmid2 = shmget(key2, 27, IPC_CREAT | 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }

    if ((shm2 = shmat(shmid2, NULL, 0)) == (char *) -1)
    {
        perror("shmat");
        exit(1);
    }

    ///(2)  END OF SHARED MEMORY PREP ///



    for(;;)
    {


        if (strcmp(shm,tmp) == 0)
        {
	    sleep(2);
   	    printf("Waiting \n");
            sem_wait(sem_id);
            printf("Locked, About to sleep \n");
            printf("Got %s \n",shm);
            printf("RES %s \n",shm2);
	    sleep(2);
    	    sem_post(sem_id);
            //  printf("same");
         
        }
        else
        {
	    sleep(2);
   	    printf("Waiting \n");
            sem_wait(sem_id);
            printf("Locked, About to sleep \n");
            /// GET A SENSOR OBJECT OUT OF A STRING ///
            struct Sensor *mySensor = createSensor(shm);
            printf("%s | %s : %s",mySensor->label,mySensor->ip,mySensor->port);
            tmp = shm;
	    sleep(2);
    	    sem_post(sem_id);


            /// SOCKET PREP///
            if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
                diep("socket");

            memset((char *) &si_other, 0, sizeof(si_other));
            si_other.sin_family = AF_INET;
            si_other.sin_port = htons(atoi(mySensor->port));
            if (inet_aton(mySensor->ip, &si_other.sin_addr)==0)
            {
                fprintf(stderr, "inet_aton() failed\n");
                exit(1);
            }
            /// SOCKET PREP ///




            /// SEND SOCKET
            if (sendto(s, "a", BUFLEN, 0,(struct sockaddr *) &si_other, slen)==-1)
            {
                diep("sendto()");
            }else{
                printf("socket sent");
            }


        }




    }






    return 0;
}

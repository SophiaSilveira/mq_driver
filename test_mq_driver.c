#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LENGTH 57

int main()
{
	int ret, fd, len;
    int len_buffer;
	
	printf("Starting device test code example...\n");
	
	fd = open("/dev/mq_driver", O_RDWR);
	if (fd < 0) {
		perror("Failed to open the device...");
		return errno;
	}

    const char buf[1] = {'\0'};
    len_buffer = write(fd, buf, 1);
    
    printf("buffer size %d\n", len_buffer);

    char *receive;
    char *stringToSend;


    //registro
    while (1) {
        printf("Type /reg name, to register(or just ENTER to finish):\n");

        stringToSend = malloc(BUFFER_LENGTH);

		memset(stringToSend, 0, BUFFER_LENGTH);
		fgets(stringToSend, BUFFER_LENGTH - 1, stdin);
		len = strnlen(stringToSend, BUFFER_LENGTH);
		stringToSend[len - 1] = '\0';
		if (len == 1) return 0;

		ret = write(fd, stringToSend, strlen(stringToSend));
        if (ret < 0) {
			perror("Failed to write the message to the device.");
			return errno;
		}
        if (ret == 0) break;
        //malloc quando for
	}

    //envio ou leitura de mensagens
    while (1) {
        printf("Type /name_process message, whith %d syze of bytes.\n", len_buffer);
        printf("ENTER to read back the oldest message\n");
        printf("or /unregister to finish the program\n");

        stringToSend = malloc(len_buffer + BUFFER_LENGTH);

        memset(stringToSend, 0, len_buffer + BUFFER_LENGTH);
		fgets(stringToSend, (len_buffer + BUFFER_LENGTH) - 1, stdin);
        len = strnlen(stringToSend, len_buffer + BUFFER_LENGTH);
        stringToSend[len - 1] = '\0';

		if (len == 1) {
            receive = malloc(len_buffer);
            memset(receive, 0, len_buffer);
            ret = read(fd, receive, len_buffer);
            if(ret < 0){
			    perror("Failed to read the message from the device.");
			    return errno;
		    }
            printf("Read message: [%s]\n", receive);
            free(receive);
        }else if(strcmp(stringToSend, "/unregister") == 0){
            free(stringToSend);
            break;
        }
        else{
            ret = write(fd, stringToSend, strlen(stringToSend));
        }
        free(stringToSend);
    }

	printf("End of the program\n");
	
	return 0;
}

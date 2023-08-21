#include "defs-client.h"

int main(void){
    // L'indirizzo passato per le chiamate di `sendto` e `recvfrom`
    // sarà, per il client, quello del server
    // addr = &serv_addr;
    chosen = 0;

    send_msg.data = (char*) calloc(1, UDP_RFTP_MAXLINE); 
    recv_msg.data = (char*) calloc(1, UDP_RFTP_MAXLINE);
    
    memset((void*) &serv_addr, 0, sizeof(serv_addr));
     
    memset((void*) &serv_addr, 0, sizeof(serv_addr));
    memset((void*) &addr, 0, sizeof(addr));
    addr.sin_family         = serv_addr.sin_family        = AF_INET;
    addr.sin_port           = serv_addr.sin_port          = htons(UDP_RFTP_SERV_PT);
    addr.sin_addr.s_addr    = serv_addr.sin_addr.s_addr   = htonl(INADDR_ANY);
    if(inet_pton(AF_INET, UDP_RFTP_SERV_IP, &serv_addr.sin_addr) <= 0) {
		perror("errore in inet_pton");
	    exit(-1);
    }
    
    char command[UDP_RFTP_MAXLINE];
     
    while(1){
        if(chosen >= UDP_RFTP_MAXCLIENT){
            wait(NULL);
            --chosen;
        }

        if(fgets(command, UDP_RFTP_MAXLINE, stdin) == NULL){
            perror("errore in fgets");
            exit(-1);
        }

        fflush(stdin);
        
        char *cmd;
        if((cmd = strtok(command, " ")) == NULL){
            printf("nessun comando trovato\n");
            fflush(stdout);
        }
        
        if(strncmp(cmd, "get", 3) == 0){
            if((cmd = strtok(NULL, " \n")) == NULL){
                printf("nessun file specificato\n");
            }else
                UDP_RFTP_generate_recv(cmd);
            
            ++chosen;
            continue;
        }
        
        if(strncmp(cmd, "put", 3) == 0){
            if((cmd = strtok(NULL, " \n")) == NULL){
                printf("nessun file specificato\n");
           
                continue;
            }
            
            FILE* fd;
            chdir("server_files");
            if((fd = fopen(cmd, "r")) == NULL)
                printf("file %s non esistente\n", cmd);
            else{
                fclose(fd);
                chdir("..");
                UDP_RFTP_generate_put(cmd);
                ++chosen;
            }
            
            continue;
        }
         
        if(strncmp(cmd, "list", 4) == 0){
            UDP_RFTP_generate_recv(NULL);
            ++chosen;
            continue;
        }

        printf("unknown command %s\n", cmd);
    }
}

#include <stdio.h>
#include <dirent.h>
#include <string>
#include <string.h>
#include <vector>

#include <cstring>    // sizeof()
#include <iostream>
#include <string>   

// headers for socket(), getaddrinfo() and friends
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>

#include <unistd.h>    // close()

using namespace std;


#define PATH_LIMIT 256 //path to dir/file
#define PCKT_S_MAX 15000 //packet max buf size
#define FILE_LINE_BUF_S 2048 //for reading files 


#define PORT 9999

#define REQCODE_DIR_LIST 0x0a
#define REQCODE_DOWNLOAD_FILE 0x0b

//Read directory, save to vector
//Return 0 upon success, else return not 0
int read_dir(char *path, vector<string> *res) {
    struct dirent *entry = nullptr;
    DIR *dp = nullptr;
    

    dp = opendir(path);
    if (dp != nullptr) {
        while ((entry = readdir(dp))) {

            //Skip current directory and parent directory
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            //path + file name
            string a = path;
            string b = entry->d_name;
            string c = a + b;
            
            res->push_back(c);
            
        }
    } else {
        printf("Could not open %s\n", path);
        return -1;
    }

    closedir(dp);
    return 0;
}



//Read file, save to vector
//Vector is list of lines
int read_file(char *fpath, vector<string> *res) {
    ifstream inputFile;
    string line;

    inputFile.open(fpath);
    if (inputFile.is_open()) {
        while (!inputFile.eof()) {
            getline(inputFile, line);
            res->push_back(line);
        }
    } else {
        perror("Error opening file\n");
        inputFile.close();
        return 1;
    }

    inputFile.close();

    return 0;

}


//Write to buffer the list of files in directory
//Return 0 on success, else return something else
int buf_write_list_dir(char *dir_path, char *buf, int *bytes_written) {
    if(buf == nullptr || bytes_written == nullptr) {
        return -1;
    }

    *bytes_written = 0;
    vector<string> vec;
    read_dir(dir_path, &vec);

    for(string fpath : vec) {

        //Write to buffer 
        strncpy((char*)(buf + (*bytes_written)), fpath.c_str(), fpath.length() + 1);
        printf("bytes_written: %d str: %s\n", *bytes_written,(char*) (buf + *bytes_written));

        (*bytes_written) += fpath.length() + 1; //sizeof path written + \n terminator

        
    }

    
    return 0;
}

//Write to buffer the file to open (aka Download File function of program)
//Return 0 on success
int buf_write_file(char *fpath, char *buf, int *bytes_written) {
    vector<string> lines;
    int res = read_file(fpath, &lines);
    if(res != 0)
        return res;
    
    for(string s : lines) {
        //Write to buffer
        strncpy((char*)(buf + (*bytes_written)), s.c_str(), s.length() + 1);
        printf("bytes_written: %d str: %s\n", *bytes_written,(char*) (buf + *bytes_written));

        (*bytes_written) += s.length() + 1; //sizeof string + \n terminator
    }

    return 0;
}

//On success return 0
int send_list_dir(int sockfd, char *dir) {
    char buf[PCKT_S_MAX];
    bzero(buf, PCKT_S_MAX);
    int bytes_written = 0;
    int ret = buf_write_list_dir(dir, buf, &bytes_written);
    
    if(ret != 0) {
        return -1;
    }

    int flags = 0;
    size_t num_bytes_send = send(sockfd, buf, bytes_written, flags);
    if(num_bytes_send == -1) {
        perror("Error sending list dir\n");
        exit(errno);
    }
    printf("Sent %d bytes\n", num_bytes_send);

    return 0;
}

//Send the file to download
int send_file(int sockfd, char *file_to_download) {
    char buf [PCKT_S_MAX];
    int bytes_written = 0;

    bzero(buf, PCKT_S_MAX);

    int res = buf_write_file(file_to_download, buf, &bytes_written);
    if(res != 0)
        return res;

    int flags = 0;
    size_t num_bytes_send = send(sockfd, buf, bytes_written, flags);
    if(num_bytes_send == -1) {
        perror("Error sending file\n");
        exit(errno);
    }
    printf("Sent %d bytes\n", num_bytes_send);

    return 0;
}

int main() {
    int sockfd, connfd;
    socklen_t len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    }
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    
    
    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    }   
    else
        printf("Socket successfully binded..\n"); 
  

    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 


    
    len = sizeof(cli); 

    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (struct sockaddr *)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n"); 
  
    //Receive message
    char recv_buf [PCKT_S_MAX];
    int recv_flags = 0;
    size_t bytes_recv = recv(connfd, recv_buf, PCKT_S_MAX, recv_flags);

    if(bytes_recv == -1 || bytes_recv == 0) {
        perror("Error receive\n");
        exit(1);
    }

    printf("Received %d bytes : %s\n", bytes_recv, recv_buf);

    //Process message
    char first_byte = recv_buf[0];

    if(first_byte == REQCODE_DIR_LIST) {
        printf("Directory listing request!\n");

        char *request_dir = recv_buf+1; //skip first byte
        printf("Request dir: %s\n", request_dir);

        int res = send_list_dir(connfd, request_dir);
        if(res != 0) {
            perror("send_list_dir");
            exit(res);
        }


    } else if (first_byte == REQCODE_DOWNLOAD_FILE) {
        printf("Download file request!\n");

        char *request_file = recv_buf + 1; //skip first byte
        printf("File to download: %s\n", request_file);

        int send_file_res = send_file(connfd, request_file);  
        if(send_file_res != 0) {
            perror("Error sending file\n");
            exit(send_file_res);
        }
    }
    else {
        printf("Invalid code: %d\n", (int)recv_buf[0]);
        exit(1);
    }

    close(sockfd);
    close(connfd);

    return 0;
}
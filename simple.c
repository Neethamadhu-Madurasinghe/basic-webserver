// 20001223

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 1024


enum accept_type {
    OTHER,
    JSON,
    ERROR
};


void replaceDoubleSlashes(char *path) {
    char *read = path;
    char *write = path;
    int lastWasSlash = 0;

    while (*read) {
        if (*read == '/' && lastWasSlash) {
            // Skip additional slashes
            read++;
        } else {
            // Copy current character
            *write = *read;
            lastWasSlash = (*write == '/');
            read++;
            write++;
            
        }
    }
    *write = '\0'; // Null-terminate the cleaned string
    printf("path: %s\n",  path);
}

int extractMethodAndAcceptTypes(char *buffer, char *method) {
    char tempBuffer[BUFFER_SIZE];
    strcpy(tempBuffer, buffer); 

    char *requestLine = strtok(tempBuffer, "\n");
    if (requestLine == NULL) {
        perror("No request line found\n");
        return ERROR;
    }

    // Extract the method (first word in the request line)
    char *temp_method = strtok(requestLine, " ");
    if (temp_method == NULL) {
        perror("No request line found\n");
        return ERROR;
    }
    strcpy(method, temp_method);

    // Iterate through the buffer to find the Accept header
    strcpy(tempBuffer, buffer);
    char *line = strtok(tempBuffer, "\n");
    while ((line = strtok(NULL, "\n")) != NULL) {
        if (strncmp(line, "Accept:", 7) == 0) {
            // Extract the content of the Accept header
            char *acceptValue = strchr(line, ' ');
            if (acceptValue != NULL) {
                acceptValue = acceptValue + 1; // Move past the space after "Accept:"
                 printf("Iterating...\n");
                // Check if it contains "application/json"
                if (strstr(acceptValue, "application/json") != NULL) {
                    return JSON;
                } else {
                    return OTHER;
                }
            }
        }
    }

    printf("Error extracting Accept header\n");
    return ERROR; // Error if the Accept header is not present

}


int extractResourcePath(char *buffer, char *result, enum accept_type acceptType) {
    char tempBuffer[BUFFER_SIZE];

    strcpy(tempBuffer, buffer); 

    // Find the request line (first line of the buffer)
    char *requestLine = strtok(tempBuffer, "\n");

    if (requestLine == NULL) {
        perror("No request line found\n");
        return -1;
    }

    // Extract the requested file path from the request line - first get the method (using first " " then file path using second " ")
    strtok(requestLine, " ");

    // Null is used to continue from prevous strtok call
    char *fileRequested = strtok(NULL, " ");
    
    if (fileRequested == NULL) {
        perror("No file has been requested\n");
         return -1;
    }

    // Replace double slashes with single slashes
    replaceDoubleSlashes(fileRequested);

    // Handle the root path "/"
    if (strcmp(fileRequested, "/") == 0) {
        strcpy(result, "/index.html");
    } else {
        // Check if the path has an extension - strchr for finding last occurence of a character in a string
        char *lastDot = strrchr(fileRequested, '.');
        char *lastSlash = strrchr(fileRequested, '/');

        if (lastDot == NULL || (lastSlash != NULL && lastDot < lastSlash)) {
            // No extension found, append ".html" or ".json" based on acceptType
            if (acceptType == JSON) {
                sprintf(result, "%s.json", fileRequested);
            } else {
                sprintf(result, "%s.html", fileRequested);
            }
            
        } else {
            // Path already has an extension, no change needed
            strcpy(result, fileRequested);
        }
    }

    return 0;
}


void sendBadRequest(char *responseHeader, int clientSocket) {
    sprintf(responseHeader, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nBad Request");
    send(clientSocket, responseHeader, strlen(responseHeader), 0);
    close(clientSocket);
    return;
}

void sendMethodNotAllowed(char *responseHeader, int clientSocket, enum accept_type acceptType) {
    switch (acceptType) {
        case OTHER:
            sprintf(responseHeader, "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/plain\r\n\r\nMethod Not Allowed");
            send(clientSocket, responseHeader, strlen(responseHeader), 0);
            close(clientSocket);
            break;
        case JSON:
            sprintf(responseHeader, "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: application/json\r\n\r\nMethod Not Allowed");
            send(clientSocket, responseHeader, strlen(responseHeader), 0);
            close(clientSocket);
            break;
        case ERROR:
            sendBadRequest(responseHeader, clientSocket);
        }
    
    return;
}

void sendNotFound(char *responseHeader, int clientSocket, enum accept_type acceptType) {
    switch (acceptType) {
        case OTHER:
            sprintf(responseHeader, "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nNot found");
            send(clientSocket, responseHeader, strlen(responseHeader), 0);
            close(clientSocket);
            break;
        case JSON:
            sprintf(responseHeader, "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\nResource not found");
            send(clientSocket, responseHeader, strlen(responseHeader), 0);
            close(clientSocket);
            break;
        case ERROR:
            sendBadRequest(responseHeader, clientSocket);
        }
    
    return;
}

void sendInternalServerError(char *responseHeader, int clientSocket, enum accept_type acceptType, char *message) {
    switch (acceptType) {
        case OTHER:
            sprintf(responseHeader, "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\n");
            strcat(responseHeader, message);
            send(clientSocket, responseHeader, strlen(responseHeader), 0);
            close(clientSocket);
            break;
        case JSON:
            sprintf(responseHeader, "HTTP/1.1 500 Internal Server Error\r\nContent-Type: application/json\r\n\r\n");
            strcat(responseHeader, message);
            send(clientSocket, responseHeader, strlen(responseHeader), 0);
            close(clientSocket);
            break;
        case ERROR:
            sendBadRequest(responseHeader, clientSocket);
        }
    
    return;
}

void sendUnSupportedMediaType(char *responseHeader, int clientSocket, enum accept_type acceptType) {
    switch (acceptType) {
        case OTHER:
            sprintf(responseHeader, "HTTP/1.1 415 Unsupported Media Type\r\nContent-Type: text/plain\r\n\r\nUnsupported media type");
            send(clientSocket, responseHeader, strlen(responseHeader), 0);
            close(clientSocket);
            break;
        case JSON:
            sprintf(responseHeader, "HTTP/1.1 415 Unsupported Media Type\r\nContent-Type: application/json\r\n\r\nUnsupported media type");
            send(clientSocket, responseHeader, strlen(responseHeader), 0);
            close(clientSocket);
            break;
        case ERROR:
            sendBadRequest(responseHeader, clientSocket);
        }
    
    return;
}


void handleClient(int clientSocket) {
    printf("\nNew Request\n");

    char buffer[BUFFER_SIZE];
    char fileRequested[512];
    char method[10];
    enum accept_type acceptType = ERROR; 

    FILE *file;
    struct stat fileStat;
    char responseHeader[1024];
    char contentType[50];
    char filePath[512] = "./htdocs"; 

    // Read the request details to the buffer from client socket
    read(clientSocket, buffer, BUFFER_SIZE);

     // Extract method and accept type
    acceptType = extractMethodAndAcceptTypes(buffer, method);
    printf("Method: %s\n", method);
    printf("Accept type: %d\n", acceptType);

    // If the accept type is json, data is fetched from api folder
    if(acceptType == JSON) {
        strcpy(filePath, "./api");
    }


    // If method is not GET, Send 405
    if (strcmp(method, "GET") !=0 ) {
        sendMethodNotAllowed(responseHeader, clientSocket, acceptType);
        return;
    }

    // Extract the resource path
    int status = extractResourcePath(buffer, fileRequested, acceptType);
    
    // If error occured, send bad request
    if (status < 0) {
        sendBadRequest(responseHeader, clientSocket);
        return;
    }

    printf("Request to path: %s\n", fileRequested);

    
    // Construct the full file path
    strcat(filePath, fileRequested);
    printf("Full filepath: %s\n", filePath);


    // File does not exist, send not found response
    if (stat(filePath, &fileStat) != 0) {
        
        sendNotFound(responseHeader, clientSocket, acceptType);
        return;

    }

    // Determine the content type based on file extension
    if (acceptType == JSON) {
        strcpy(contentType, "application/json");
    }else if (strstr(filePath, ".html") || strstr(filePath, ".htm")) {
        strcpy(contentType, "text/html");
    } else if(strstr(filePath, ".jpg") || strstr(filePath, ".jpeg")) {
        strcpy(contentType, "image/jpeg");
    } else if(strstr(filePath, ".webp")) {
        strcpy(contentType, "image/webp");
    } else if(strstr(filePath, ".css")) {
        strcpy(contentType, "text/css");
    } else {
        // Unsupported media type
        sendUnSupportedMediaType(responseHeader, clientSocket, acceptType);
        return;
    }

    // Open the image file
    file = fopen(filePath, "rb");

    // If failed to open file, send a 500 response
    if (file == NULL) {
        char *message = "Faild to open the file";
        sendInternalServerError(responseHeader, clientSocket, acceptType, message);
        return;
    }

    // Prepare and send the response headers
    sprintf(responseHeader, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", contentType, fileStat.st_size);
    send(clientSocket, responseHeader, strlen(responseHeader), 0);

    // Read the file and send its content
    char fileBuffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(fileBuffer, 1, sizeof(buffer), file)) > 0) {
        send(clientSocket, fileBuffer, bytesRead, 0);
    }

    // Close the file
    fclose(file);
    close(clientSocket);
}

int main() {
    int serverFd, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_len = sizeof(clientAddr);

    // Creating socket file descriptor
    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the network address and port
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT); // Converts byte order 

    if (bind(serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind failed");
        close(serverFd);
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(serverFd, 3) < 0) {
        perror("listen failed");
        close(serverFd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Accept and handle incoming connections
    while ((clientSocket = accept(serverFd, (struct sockaddr *)&clientAddr, &addr_len)) >= 0) {
        handleClient(clientSocket);
    }

    if (clientSocket < 0) {
        perror("accept failed");
        close(serverFd);
        exit(EXIT_FAILURE);
    }

    close(serverFd);
    return 0;
}

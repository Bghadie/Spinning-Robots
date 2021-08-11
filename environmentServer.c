#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include "simulator.h"
#include "HashMap.h"
//define my global variables
#define SERVER_PORT 6000
#define TRUE 1
#define FALSE 0
#define MAX_DIRECTION 180
Environment    environment;  // The environment that contains all the robots

//declare mutex
pthread_mutex_t lock;

//function signatures 
void *redraw(void *environment);
char checkCollide(char robtID);
void turnOffRobots();
// Handle client requests coming in through the server socket.  This code should run
// indefinitiely.  It should repeatedly grab an incoming messages and process them. 
// The requests that may be handled are STOP, REGISTER, CHECK_COLLISION and STATUS_UPDATE.   
// Upon receiving a STOP request, the server should get ready to shut down but must
// first wait until all robot clients have been informed of the shutdown.   Then it 
// should exit gracefully.  
void *handleIncomingRequests(void *e) {
	//variable declartion
	char   online = 1;
	int serverSocket, status, addrSize, bytesReceived;
	unsigned char buffer[11], response[11];
	int direction, speed;
	float x, y, weight_change;
    //for sign 1 is negative 0 is positive
	char sign, lowerBytes,upperBytes, getRandomSign, convertToNeg;

	lowerBytes = 31; //used to conver the x and y values to higher bytes
	upperBytes = 5; //used to convert the x and y value to their lower bytes
	getRandomSign = 2; //used to get a random value between 0 and 1
	convertToNeg = -1; //used to convert an int to negative

	
	
	//server varibles
	struct sockaddr_in serverAddr, clientAddr;
	fd_set readfds, writefds;	

  	// Initialize the server
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(serverSocket < 0){
		//print error message
		printf("Server Binding Socket Error!\n");
		exit(-1);
	}
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);	
	serverAddr.sin_port = htons((unsigned short) SERVER_PORT);

	status = bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
  	// Wait for clients now
	while (online) {
		
		//set up server socket
		FD_ZERO(&readfds);

		FD_SET(serverSocket, &readfds);

		FD_ZERO(&writefds);

		FD_SET(serverSocket, &writefds);

		status = select(FD_SETSIZE, &readfds, &writefds, NULL,NULL);
		if(status < 0){
			//print errror message
			printf("Server Socket Selection Error!\n");
			exit(-1);
		}else{
			//if the status is valid, get the client address
			addrSize = sizeof(clientAddr);
			//get the client response
			bytesReceived = recvfrom(serverSocket, buffer, 9,
					0, (struct sockaddr*) &clientAddr, &addrSize);
			//if the client wants to register a robot
			if(buffer[0] == REGISTER){
				//if the num robots doesn't exceed max capacity
				if(environment.numRobots < MAX_ROBOTS){
					//generate an x, y, and direction value endlesslys
					while(TRUE){
			            float w = ((float) rand()/(float) (RAND_MAX))*1.0;
                        int weight = ROBOT_RADIUS * (0.5 + w);
						//generate the random values
						x = (rand() % (ENV_SIZE - weight + 1)) + weight;
						y = (rand() % (ENV_SIZE - weight + 1)) + weight;
						sign = 	(rand() % getRandomSign);
						direction = (rand() %  MAX_DIRECTION);
                        weight_change = (weight - ROBOT_RADIUS)/(float)ROBOT_RADIUS;
                        if(weight_change < 0){
                          weight_change *= -1;
                          response[10] = 1;
                        }else{
 						  response[10] = 0;
                        }
						//populate the char array to send to the client
						response[0] = OK; 
						response[1] = environment.numRobots;
						//these next four values are the broken
						//up x and y values
						response[2] = (((int)x) & lowerBytes);
						response[3] = ((int)x) >> upperBytes;
						response[4] = ((int)y) & lowerBytes;
						response[5] = ((int)y) >> upperBytes;
						response[6] = sign;
						response[7] = direction;
                        response[8] = (int)(weight_change*100);
					  	
                        environment.robots[environment.numRobots].weight = weight;
						//store the x and y  values of that robot
						//in the environment
						environment.robots[environment.numRobots].x = x;
						environment.robots[environment.numRobots].y = y;
						if(sign){
							environment.robots[environment.numRobots].direction = direction*convertToNeg;
						}else{
							environment.robots[environment.numRobots].direction = direction;
						}
						//if the positions are valid, break the
						//loop and register the robot
						if(checkCollide(environment.numRobots) == OK){
							break;
						}
					}
					//increase th enumber of robots and let the client
					//know the robot has been registered (SEND OK)
					//and the robots values
					environment.numRobots++;
					sendto(serverSocket, response, 11, 0, 
					(struct sockaddr *) &clientAddr, addrSize);
				}else{
				//max number of robots have been added, send not ok
					response[0] = NOT_OK;
					sendto(serverSocket, response, 1, 0, 
					(struct sockaddr *) &clientAddr, addrSize);
				}
				
				
			}
			//if the robot wants to check for collisions
			if(buffer[0] == CHECK_COLLISION){
				//check for collisions
				response[0] = checkCollide(buffer[1]);
				//sned the status code to the client
				sendto(serverSocket, response, 1, 0, 
					(struct sockaddr *) &clientAddr, addrSize);
			}
			//if the robot wants to update thir status
			if(buffer[0] == STATUS_UPDATE){
				//check if the server should shut down
				if(environment.shutDown){
					//send a lost contact resposne
					response[0] = LOST_CONTACT;
					sendto(serverSocket, response, 1, 0, 
						(struct sockaddr *) &clientAddr, addrSize);
					//decrease the num robots in the enviornment
					environment.numRobots--;
					if(environment.numRobots == 0){
						//if all robots have been shut off set the
						//server to be offline
						online = FALSE;
					}
				} 
  				pthread_mutex_lock(&lock);
				//update the current robots x and y positions as well as
				//direction in the enviornment
				environment.robots[buffer[1]].x = (buffer[2] |(buffer[3] << upperBytes));
				environment.robots[buffer[1]].y = (buffer[4] |(buffer[5] << upperBytes));
				
				if(buffer[6]){
					environment.robots[buffer[1]].direction = buffer[7]*convertToNeg;
				}else{
					environment.robots[buffer[1]].direction = buffer[7];
				}
     			pthread_mutex_unlock(&lock);
			}
			//if the stop command was recieved
			if(buffer[0] == STOP){
				//begin server shutdown protocal
				environment.shutDown = TRUE;
			}
            if(buffer[0] == SHUTDOWN){
              for(int i = buffer[1]; i < environment.numRobots; ++i)
                
              sendto(serverSocket, response, 9, 0,
              (struct sockaddr *) &clientAddr, addrSize);

            }
		}
		
		
  	}
  	// ... WRITE ANY CLEANUP CODE HERE ... //
	environment.shutDown = TRUE; //shut down the environment
	//close server socket
	close(serverSocket);
	pthread_exit(NULL); //exit the thread
}




int main() {
	// So far, the environment is NOT shut down
	environment.shutDown = FALSE;

    //initialize mutex    
    if(pthread_mutex_init(&lock, NULL) != 0){
      printf("\n mutex init failed\n");
      return 1;
    }

	// Set up the random seed
	srand(time(NULL));
	//declear variables
	pthread_t acptRqst, redrawRqst;
	//create the two threadss
	pthread_create(&acptRqst, NULL, handleIncomingRequests, &environment);
	pthread_create(&redrawRqst, NULL, redraw, &environment);
	
	// Wait for the update and draw threads to complete
	pthread_join(acptRqst, NULL);
	pthread_join(redrawRqst,NULL);
	
}

//this functions only parameters is the current robot ID as a char
//it first checks if the robot will collide with a wall then if a robot
//will collide with another robot
//returns either OK, NOT_OK_COLLIDE, or NOT_OK_BOUNDAR
char checkCollide(char currRobtID){
	//declare all variables
	float x1, y1, x2, y2, distance;
	int direction, weight;
    weight = environment.robots[currRobtID].weight;
	//get the current robot's x, y, and direction
	x1 = environment.robots[currRobtID].x;
	y1 = environment.robots[currRobtID].y;
	direction =  environment.robots[currRobtID].direction;
	//if the robot is about to exceed the left or right side of the screen, return NOT OK BOUNDARY
	if(((x1 - weight) + ((ROBOT_SPEED) * (cos((direction*PI)/MAX_DIRECTION)))) < 0  || ((x1 + weight) + ((ROBOT_SPEED) * (cos((direction*PI)/MAX_DIRECTION)))) > ENV_SIZE){
		return NOT_OK_BOUNDARY;
	//else if the robot is about to exceed the top or bottom of the screen, return not OK BOUNDARY
	}else if(((y1 - weight) + ((ROBOT_SPEED) * (sin((direction*PI)/MAX_DIRECTION)))) < 0  || ((y1 + weight) + ((ROBOT_SPEED) * (sin((direction*PI)/MAX_DIRECTION)))) > ENV_SIZE){
		return NOT_OK_BOUNDARY;
	}
	//these new x1 and y1 values take into account the robots direction
	x1 = x1 + (ROBOT_SPEED * (cos((direction*PI)/MAX_DIRECTION)));
	y1 = y1 + (ROBOT_SPEED * (sin((direction*PI)/MAX_DIRECTION)));
	//go through each robot in the environment. Using the distance formula, if the 
	//distance between the two centers of the circle is less than or equal 2 times the
 	//radiuses, return NOT OK COLLIDE, the current robot ran into another one
	for(int i = 0; i < environment.numRobots; i++){
		if(i != currRobtID){	
            //critical code
            pthread_mutex_lock(&lock);
			x2 = environment.robots[i].x;
			y2 = environment.robots[i].y;
            int weight2 = environment.robots[i].weight;
			distance = sqrt(pow((x1 - x2),2) + pow((y1 - y2),2));
			unsigned char  check;
            check  = (distance <= (weight + weight2));
            pthread_mutex_unlock(&lock);
            if(check){
				return NOT_OK_COLLIDE;
			}
		}
	}
	//no collisions return OK
	return OK;
}



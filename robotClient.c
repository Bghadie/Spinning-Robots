#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "simulator.h"

//declare global variables
#define TRUE 1
#define FALSE 0





// This is the main function that simulates the "life" of the robot
int main() {
	//declare all variables
	int clientSocket, addrSize, bytesReceived;
	struct sockaddr_in serverAddr;
	unsigned char buffer[11], response[11];
	//for sign, 1 is negative 0 is positive
	//for turn, 1 is CWW and 0 is CW
	int direction, ID, negativeConverter, turn, shouldTurn, maxDegrees = 180, minDegrees = -180;
	float x, y, speed;
	char upperBytes, lowerBytes, sign, online, getRandomSign = 2;
    struct tm * timeinfo;
    time_t rawtime;
    long int second;
    time(&rawtime);
    timeinfo = localtime (&rawtime);
    second = timeinfo->tm_sec + (timeinfo->tm_min*60) + (timeinfo->tm_hour*3600);
	
    negativeConverter = -1; //used to convert a value to negative
	upperBytes = 5; //used to get/set the uppder bytes of the x,y values
	lowerBytes = 31; //used to get/set the lower bytes of the x,y values
  // Set up the random seed
 	srand(time(NULL));
  // Register with the server
	clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(clientSocket < 0){
		printf("Client Error Opening Socket!\n");
	}
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	serverAddr.sin_port = htons((unsigned short) SERVER_PORT);
	addrSize = sizeof(serverAddr);
  // Send register command to server. 
  
	buffer[0] = REGISTER;

	sendto(clientSocket, buffer, 1, 0, 
	(struct sockaddr *)&serverAddr, addrSize);
	bytesReceived = recvfrom(clientSocket, buffer, sizeof(buffer), 0,
			(struct sockaddr *) &serverAddr, &addrSize);
	
	if(buffer[0] == OK){
		//if server sends back OK, store the values send from the server
		//set online to true
		ID = buffer[1];
		x = (buffer[2] |(buffer[3] << upperBytes));
		y = (buffer[4] |(buffer[5] << upperBytes));
		sign = buffer[6];
        float change_speed = buffer[8]/(float)100.0;
        if(!buffer[10]){
          speed = ROBOT_SPEED - (ROBOT_SPEED *  change_speed); 
        }else{
          speed = (ROBOT_SPEED * (1 + change_speed));
		}
		if(sign){
			direction = buffer[7]*negativeConverter;
			
		}else{
			direction = buffer[7];
		}
		online = TRUE;
		
	}else{
 		//If denied registration, then print error message and quit.
		printf("Max capacity robots has been met, unable to register more robots \n");
		online = FALSE; 
	}

  // Go into an infinite loop exhibiting the robot behavior
	while (online) {
	    // Check if can move forward
		buffer[0] = CHECK_COLLISION;
		buffer[1] = ID;
		buffer[2] = (((int)x) & lowerBytes);
		buffer[3] = ((int)x) >> upperBytes;
		buffer[4] = ((int)y) & lowerBytes;
		buffer[5] = ((int)y) >> upperBytes;
		buffer[6] = sign;
		if(sign){
			buffer[7] = direction*negativeConverter;
		}else{
			buffer[7] = direction;
		}

		sendto(clientSocket, buffer, 9, 0, 
			(struct sockaddr *)&serverAddr, addrSize);
		bytesReceived = recvfrom(clientSocket, buffer, 1, 0,
			(struct sockaddr *) &serverAddr, &addrSize);
		//if can move forward, set the flag that tells whether or not to
		//calculate a new turning direction to false and move forwrad
		if(buffer[0] == OK){
			shouldTurn = FALSE;
            time(&rawtime);
            timeinfo = localtime (&rawtime);
			second = 2 + timeinfo->tm_sec + (timeinfo->tm_min*60) + (timeinfo->tm_hour*3600);
			x = x + (speed * (cos((direction*PI)/180)));
			y = y + (speed * (sin((direction*PI)/180)));
		}
		//if there is a colision with a boundary/robot
		if(buffer[0] == NOT_OK_BOUNDARY || buffer[0] == NOT_OK_COLLIDE){
			//if this is false, then the robot just moved forward
			if(!shouldTurn){
				//so calculate a turning direction and set should turn to 
				//true. This way, a random turnning direction (CW vs CWW)
				//wont be chosen until the robot moves forward again
				turn = (rand() % getRandomSign);
				shouldTurn = TRUE;
			}
			
			if(turn){ //turn counter clockwise
				//this if statement catches the edge case where the 
				//value exceeds 180 degrees
				if((direction + ROBOT_TURN_ANGLE) > maxDegrees || (sign && direction + ROBOT_TURN_ANGLE > 0)){
					if(sign){ //if the turning direction is negative
						direction += ROBOT_TURN_ANGLE;
					}else{ //if the turning directoin is positive
						direction = minDegrees + ((direction + ROBOT_TURN_ANGLE) % maxDegrees);
					}
					sign = !sign;
				}else{
					direction += ROBOT_TURN_ANGLE; //turn the robot 
				}
			}else{ //turn clockwise
				//this if statement catches the edge case where the value
				//exceeds -180 (is less than -180)
				if((direction - ROBOT_TURN_ANGLE) < minDegrees || (!sign && (direction - ROBOT_TURN_ANGLE) < 0)){
					if(sign){
						direction = maxDegrees +((direction - ROBOT_TURN_ANGLE) % minDegrees);
					}else{
						direction -= ROBOT_TURN_ANGLE;
					}
					sign = !sign;
				}else{
					direction -= ROBOT_TURN_ANGLE; //turn the robot
				}
			}
		}
		//if the robot lost contact with the server
		if(buffer[0] == LOST_CONTACT){
			online = FALSE; //shut down the robot
		}
	   
		//update the current robots status with the server
		buffer[0] = STATUS_UPDATE;
		buffer[1] = ID;
		buffer[2] = (((int)x) & lowerBytes);
		buffer[3] = ((int)x) >> upperBytes;
		buffer[4] = ((int)y) & lowerBytes;
		buffer[5] = ((int)y) >> upperBytes;
		buffer[6] = sign;
		
		if(sign){
			buffer[7] = direction*negativeConverter;
		}else{
			buffer[7] = direction;
		}
        time(&rawtime);
        timeinfo = localtime (&rawtime);
        long int cur_sec = timeinfo->tm_sec + (timeinfo->tm_min*60) + (timeinfo->tm_hour*3600);
        if(cur_sec > second){
          online = FALSE;
        }

        sendto(clientSocket, buffer, 9, 0, 
			(struct sockaddr *)&serverAddr, addrSize);
		
	    // set a time delay
		usleep(20000);
		
	}
	//close the client socket
	close(clientSocket);

}


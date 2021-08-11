Name: Billal Ghadie
Student# 100888260

Source files:
- display.c
- environmentServer.c
- stop.c
- robotClient.c

Instructions:

The code is straight forward to run, it takes no command line arguments. Run the environment server in the background (see how to in executables to call line) then run as many robot clients in the background as well. There is a maximum capacity of robots. The sleep is set to 2 seconds feel free to adjust this in the robot client source file. When you are done, call the stop executable and all processes will quit 

Compile line: make

Executables to call: 
- ./envionmentServer &
- ./robotClient &
- ./stop

readme_arp_2nd_assignment

First ARP Assignment

Here are my second assignment.
It includes :
readme_arp_2nd_assignment.txt (this txt file)
master.c
drone.c
server.c
watchdog.c
keyboard.c
constants.h
makefile
object.c
target.c
arp_2_architecture.jpg

constant.h are in: include


Role of codes:
    The master.c forks and executes each file.
    The keyboard.c assigns the role of each key. It also prompts for key input using Konsole and sends the typed key information to the drone.c using FIFO.
    The drone.c calculates the input force by using the information about the typed key from the keyboard.c using FIFO. When this code does this, a signal is sent to the watchdog.c to confirm this code is alive. The drone.c sends the force information to the server.c by using a pipe. 
    The server.c calculates the drone's position by using the force information from the drone.c, target.c, and object.c every 1 second by using a pipe. This calculates the repulsive force and attractive force from the position information from information from the drone.c and target.c. The signal is sent to the watchdog.c to confirm this code is alive. The server.c manages a blackboard that shows the drone, obstacles, and targets. Moreover, this also manages an inspection window that shows the drone's position, force, and score. This information is also written in a log file. When the drone gets to the target, this code lets the target.c know that and moves the target out of the window.
    The watchdog.c confirms whether codes are alive. This terminates the systems if the watchdog doesn't receive the signal from each code longer than a certain time (The initial is set to 15 seconds).
    The object.c sends the obstacles' positions to the server. The number of the obstacles is set as 5. When this code does this, a signal is sent to the watchdog.c to confirm this code is alive. The object.c sends the position information to the server.c by using a pipe. The positions are changed at a rate of 10% per second.
    The target.c sends the targets' initial positions and calculates the score. The signal is sent to the watchdog.c to confirm this code is alive. The target.c sends the position information and a score to the server.c and receives whether the drone arrives at the targets using a pipe. The score is calculated based on how long the drone takes to the each target.


To run:
cd ARP_assignment_2_Mezawa
make
cd build
./masterp

log files would be in:
build/log

Role of keys:
Typing 'q', the system is finished.
Typing 'w', the force input to the upper left.
Typing 'e', the force input to the upper.
Typing 'r', the force input to the upper right.
Typing 's', the force input to the left.
Typing 'f', the force input to the right.
Typing 'x', the force input to the lower left.
Typing 'c', the force input to the lower.
Typing 'v', the force input to the lower right.
Typing 'd', the force input to the drone stops.

There are also repulsive forces and attractive forces so they make the drone's manipulation a bit difficult.

attention:
    Occasionally some system crashes when the system is booted. If this happens, please try again.

These files (including this readme.txt) are shared in the GitHub.
https://github.com/mezool/ARP_2nd
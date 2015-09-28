#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <ctime>

using namespace std;
char state;
int fd;

void sendCommand(const char* command) {
	if (command[0] != state) {
		write(fd, command, 1);
		state = command[0];
		cout << "sending " << state << endl;
	}
}

int main(int argc, char const *argv[])
{
	state = '0';
    fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY);  //Opening device file
    printf("fd opened as %i\n", fd);
   	usleep(2500000);
   	clock_t begin = clock();
   	double elapsed_secs;
    while (1) {
    	sendCommand("w");
    	clock_t end = clock();
    	elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    	cout << elapsed_secs << endl;
    	if (elapsed_secs > 1.2) {
    		sendCommand("b");
    		break;
    	}
    }
	return 0;
}
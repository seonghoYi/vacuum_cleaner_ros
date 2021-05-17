#include <termios.h>
#include <cstdio>
#include <map>

#include "ros/ros.h"
#include "geometry_msgs/Twist.h"


// For non-blocking keyboard inputs
int getch(void)
{
  int ch;
  struct termios oldt;
  struct termios newt;

  // Store old settings, and copy to new settings
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;

  // Make required changes and apply the settings
  newt.c_lflag &= ~(ICANON | ECHO);
  newt.c_iflag |= IGNBRK;
  newt.c_iflag &= ~(INLCR | ICRNL | IXON | IXOFF);
  newt.c_lflag &= ~(ICANON | ECHO | ECHOK | ECHOE | ECHONL | ISIG | IEXTEN);
  newt.c_cc[VMIN] = 1;
  newt.c_cc[VTIME] = 0;
  tcsetattr(fileno(stdin), TCSANOW, &newt);

  // Get the current character
  ch = getchar();

  // Reapply old settings
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  return ch;
}


std::map<char, std::vector<float>> moveBindings
{
  {'i', {1, 0, 0, 0}},
  {'o', {1, 0, 0, -1}},
  {'j', {0, 0, 0, 1}},
  {'l', {0, 0, 0, -1}},
  {'u', {1, 0, 0, 1}},
  {',', {-1, 0, 0, 0}},
  {'.', {-1, 0, 0, 1}},
  {'m', {-1, 0, 0, -1}},
  {'O', {1, -1, 0, 0}},
  {'I', {1, 0, 0, 0}},
  {'J', {0, 1, 0, 0}},
  {'L', {0, -1, 0, 0}},
  {'U', {1, 1, 0, 0}},
  {'<', {-1, 0, 0, 0}},
  {'>', {-1, -1, 0, 0}},
  {'M', {-1, 1, 0, 0}},
  {'t', {0, 0, 1, 0}},
  {'b', {0, 0, -1, 0}},
  {'k', {0, 0, 0, 0}},
  {'K', {0, 0, 0, 0}}
};

// Map for speed keys
std::map<char, std::vector<float>> speedBindings
{
  {'q', {0.005, 0.005}},
  {'z', {-0.005, -0.005}},
  {'w', {0.005, 0}},
  {'x', {-0.005, 0}},
  {'e', {0, 0.005}},
  {'c', {0, -0.005}}
};

const char* msg = R"(
Reading from the keyboard and Publishing to Twist!
---------------------------
Moving around:
   u    i    o
   j    k    l
   m    ,    .

For Holonomic mode (strafing), hold down the shift key:
---------------------------
   U    I    O
   J    K    L
   M    <    >

t : up (+z)
b : down (-z)

anything else : stop

q/z : increase/decrease max speeds by 10%
w/x : increase/decrease only linear speed by 10%
e/c : increase/decrease only angular speed by 10%

CTRL-C to quit
)";

//float speed(0.065); // Linear velocity (m/s)
//float turn(0.5); // Angular velocity (rad/s)
float speed(0.1);
float turn(0.67);
float x(0), y(0), z(0), th(0); // Forward/backward/neutral direction vars
char key(' ');

int main(int argc, char **argv)
{
    std::printf("%s", msg);
    printf("\rCurrent: speed %fm/s\tturn %frad/s | Awaiting command...\r", speed, turn);

    ros::init(argc, argv, "teleop_key");
    ros::NodeHandle nh;

    ros::Publisher pub = nh.advertise<geometry_msgs::Twist>("cmd_vel", 100);

    geometry_msgs::Twist twist;

    while(true)
    {
        key = getch();
        // If the key corresponds to a key in moveBindings
        if (moveBindings.count(key) == 1)
        {
            // Grab the direction data
            x = moveBindings[key][0];
            y = moveBindings[key][1];
            z = moveBindings[key][2];
            th = moveBindings[key][3];

            printf("\rCurrent: speed %fm/s\tturn %frad/s | Last command: %c   ", speed, turn, key);
        }

        // Otherwise if it corresponds to a key in speedBindings
        else if (speedBindings.count(key) == 1)
        {
            // Grab the speed data
            speed = speed + speedBindings[key][0];
            turn = turn + speedBindings[key][1];
            /*
            if (speed < 0.065)
              speed = 0.065;
            if (turn < 0.5)
              turn = 0.5;
            */
            if (speed < 0.1)
              speed = 0.1;
            if (turn < 0.67)
              turn = 0.67;
            printf("\rCurrent: speed %fm/s\tturn %frad/s | Last command: %c   ", speed, turn, key);
        }

        // Otherwise, set the robot to stop
        else
        {
            x = 0;
            y = 0;
            z = 0;
            th = 0;

        // If ctrl-C (^C) was pressed, terminate the program
            if (key == '\x03')
            {
                printf("\n\n                 .     .\n              .  |\\-^-/|  .    \n             /| } O.=.O { |\\\n\n                 CH3EERS\n\n");
                break;
            }

            printf("\rCurrent: speed %f\tturn %f | Invalid command! %c", speed, turn, key);
        }
        // Update the Twist message
        twist.linear.x = x * speed;
        twist.linear.y = y * speed;
        twist.linear.z = z * speed;

        twist.angular.x = 0;
        twist.angular.y = 0;
        twist.angular.z = th * turn;

        // Publish it and resolve any remaining callbacks
        pub.publish(twist);
        ros::spinOnce();
    }

    return 0;
}
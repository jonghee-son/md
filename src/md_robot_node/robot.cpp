#include <stdint.h>
#include "md_robot_node/global.hpp"
#include "md_robot_node/main.hpp"
#include "md_robot_node/com.hpp"
#include "md/md_robot_msg1.h"
#include "md/md_robot_msg2.h"

// #define ENABLE_MD_MESSAGE          

#define VELOCITY_CONSTANT_VALUE         9.5492743       // 이동속도(m/min), v = 바퀴 둘레의 길이 x RPM
                                                        // 이동속도(m/sec), v = (2 x 바퀴 반지름 x (pi / 60) x RPM)
                                                        // 0.10472 = (2 x pi / 60)
                                                        // V = r * w = r * (RPM * 0.10472)
                                                        //           = r * RPM * 0.10472
                                                        // RPM = V / r * 9.5492743

#define constrain(amt,low,high) ((amt)<=(low)?(low):((amt)>=(high)?(high):(amt)))

#define LEFT           	  0      // Swing direction
#define RIGHT             1

extern md::md_robot_msg1 md_robot_message1;
extern md::md_robot_msg2 md_robot_message2;

extern std_msgs::Int32 right_ticks;
extern std_msgs::Int32 left_ticks;

// m/sec --> RPM
int16_t * RobotSpeedToRPMSpeed(double linear, double angular)
{
    double wheel_radius;
    double wheel_separation;
    double reduction;
    double wheel_velocity_cmd[2];
    static int16_t goal_rpm_spped[2];

    wheel_radius = robotParamData.wheel_radius;
    wheel_separation = robotParamData.nWheelLength;
    reduction = (double)robotParamData.nGearRatio;

    // ROS_INFO("l:%f, a:%f", (double)linear, (double)angular);

    wheel_velocity_cmd[LEFT]   = linear - (angular * wheel_separation / 2);
    wheel_velocity_cmd[RIGHT]  = linear + (angular * wheel_separation / 2);

    // ROS_INFO("left:%f, right:%f", (double)wheel_velocity_cmd[LEFT], (double)wheel_velocity_cmd[RIGHT]);

    //***************************************************************************************
    // Linearvelocity --> RPM 으로 환산
    //***************************************************************************************
    wheel_velocity_cmd[LEFT]  = constrain(wheel_velocity_cmd[LEFT]  * VELOCITY_CONSTANT_VALUE / wheel_radius * reduction, -robotParamData.nMaxRPM, robotParamData.nMaxRPM);
    wheel_velocity_cmd[RIGHT] = constrain(wheel_velocity_cmd[RIGHT] * VELOCITY_CONSTANT_VALUE / wheel_radius * reduction, -robotParamData.nMaxRPM, robotParamData.nMaxRPM);

    // ROS_INFO("RPM1 L:%f, R:%f\r\n", (double)wheel_velocity_cmd[LEFT], (double)wheel_velocity_cmd[RIGHT]);

    goal_rpm_spped[0] = (int16_t)(wheel_velocity_cmd[LEFT]);
    goal_rpm_spped[1] = (int16_t)(wheel_velocity_cmd[RIGHT]);

    return goal_rpm_spped;
}

void MakeMDRobotMessage1(PID_PNT_MAIN_DATA_t *pData)
{
    static bool first_cal = false;
    static ros::Time previous_time;
    double interval_time;

    ros::Time curr_time = ros::Time::now();

    interval_time = curr_time.toSec() - previous_time.toSec();
    previous_time = curr_time;

    md_robot_message1.interval_time = interval_time;
    md_robot_message1.motor1_pos = pData->mtr_pos_id1;
    md_robot_message1.motor2_pos = pData->mtr_pos_id2;
    md_robot_message1.motor1_rpm = pData->rpm_id1;
    md_robot_message1.motor2_rpm = pData->rpm_id2;

    md_robot_message1.motor1_state = pData->mtr_state_id1.val;
    md_robot_message1.motor2_state = pData->mtr_state_id2.val;

#ifdef ENABLE_MD_MESSAGE
    ROS_INFO("interval time1: %f, pos1: %d, pos2: %d, rpm1: %d rpm2: %d\r\n",\
                interval_time, md_robot_message1.motor1_pos, md_robot_message1.motor2_pos, md_robot_message1.motor1_rpm, md_robot_message1.motor2_rpm);
#endif
}

// MDUI
void MakeMDRobotMessage2(PID_ROBOT_MONITOR_t *pData)
{
    static bool first_cal = false;
    static ros::Time previous_time;
    double interval_time;

    ros::Time curr_time = ros::Time::now();

    interval_time = curr_time.toSec() - previous_time.toSec();
    previous_time = curr_time;

    md_robot_message2.interval_time = interval_time;
    md_robot_message2.x_pos = pData->lTempPosi_x;
    md_robot_message2.y_pos = pData->lTempPosi_y;
    md_robot_message2.angule = pData->sTempTheta;

    if(robotParamData.reverse_direction == 0) {
        md_robot_message2.US_1 = pData->byUS1;
        md_robot_message2.US_2 = pData->byUS2;
        md_robot_message2.US_3 = pData->byUS3;
        md_robot_message2.US_4 = pData->byUS4;
    }
    else {
        md_robot_message2.US_1 = pData->byUS4;
        md_robot_message2.US_2 = pData->byUS3;
        md_robot_message2.US_3 = pData->byUS2;
        md_robot_message2.US_4 = pData->byUS1;
    }

    md_robot_message2.platform_state = pData->byPlatStatus;
    md_robot_message2.linear_velocity = pData->linear_velocity;
    md_robot_message2.angular_velocity = pData->angular_velocity;

#ifdef ENABLE_MD_MESSAGE
    ROS_INFO("interval time2: %f\r\n", interval_time);
#endif    
}

void CalTicks(PID_PNT_MAIN_DATA_t *pData)
{
    int32_t ticks_left;
    int32_t ticks_right;

    int32_t encoder_minimum = -2147483648;
    int32_t encoder_maximum = 2147483647;

    //std_msgs::Int32 right_wheel_tick_count;
    //ros::Publisher rightPub("right_ticks", &right_wheel_tick_count);

    //std_msgs::Int32 left_wheel_tick_count;
    //ros::Publisher leftPub("left_ticks", &left_wheel_tick_count);

    //ros::NodeHandle nh1;

    //nh1.initNode();
    //nh1.advertise(rightPub);
    //nh1.advertise(leftPub);

    ticks_left = pData->mtr_pos_id1;
    ticks_right = pData->mtr_pos_id2;

    right_ticks.data = ticks_right;
    left_ticks.data = ticks_left;

    //rightPub.publish( &ticks_right );
    //leftPub.publish( &ticks_left );
    //nh1.spinOnce();

#if 0
    ROS_INFO("\r\n");
    ROS_INFO("mtr ticks: %d : %d", ticks_left, ticks_right);
#endif
}


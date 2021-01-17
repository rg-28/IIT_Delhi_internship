#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>

using namespace std;

mavros_msgs::State current_state;
void state_cb(const mavros_msgs::State::ConstPtr& msg){
    current_state = *msg;
}

geometry_msgs::PoseStamped local_pos;
void location_sub(const geometry_msgs::PoseStamped pose_sub){
    local_pos.pose.position.x = pose_sub.pose.position.x;
    local_pos.pose.position.y = pose_sub.pose.position.y;
    local_pos.pose.position.z = pose_sub.pose.position.z;
}

// geometry_msgs::PoseStamped pose;
// void set_setpoint(geometry_msgs::PoseStamped pose){
//     cout << "Enter x: ";
//     cin >> pose.pose.position.x;
//     cout << "Enter y: ";
//     cin >> pose.pose.position.y;
//     cout << "Enter z: ";
//     cin >> pose.pose.position.z;
// }

int main(int argc, char **argv)
{
    ros::init(argc, argv, "offb_node");
    ros::NodeHandle nh;

    ros::Subscriber state_sub = nh.subscribe<mavros_msgs::State>
            ("mavros/state", 10, state_cb);
    ros::Subscriber local_pos_sub = nh.subscribe<geometry_msgs::PoseStamped>
            ("mavros/local_position/pose", 10, location_sub);
    ros::Publisher local_pos_pub = nh.advertise<geometry_msgs::PoseStamped>
            ("mavros/setpoint_position/local", 10);
    ros::ServiceClient arming_client = nh.serviceClient<mavros_msgs::CommandBool>
            ("mavros/cmd/arming");
    ros::ServiceClient set_mode_client = nh.serviceClient<mavros_msgs::SetMode>
            ("mavros/set_mode");

    //the setpoint publishing rate MUST be faster than 2Hz
    ros::Rate rate(30.0);

    // wait for FCU connection
    while(ros::ok() && !current_state.connected){
        ros::spinOnce();
        rate.sleep();
    }
    
    geometry_msgs::PoseStamped pose;
    pose.pose.position.x = 0;
    pose.pose.position.y = 0;
    pose.pose.position.z = 0;

    double err_x_max = pose.pose.position.x + 0.5;
    double err_x_min = pose.pose.position.x - 0.5;
    double err_y_max = pose.pose.position.y + 0.5;
    double err_y_min = pose.pose.position.y - 0.5;
    double err_z_max = pose.pose.position.z + 0.5;
    double err_z_min = pose.pose.position.z - 0.5;

    //send a few setpoints before starting
    for(int i = 100; ros::ok() && i > 0; --i){
        local_pos_pub.publish(pose);
        ros::spinOnce();
        rate.sleep();
    }

    mavros_msgs::SetMode offb_set_mode;
    offb_set_mode.request.custom_mode = "OFFBOARD";

    mavros_msgs::CommandBool arm_cmd;
    arm_cmd.request.value = true;

    ros::Time last_request = ros::Time::now();

    if( current_state.mode != "OFFBOARD"){
            if( set_mode_client.call(offb_set_mode) &&
                offb_set_mode.response.mode_sent){
                ROS_INFO("Offboard enabled");
            }
    }
    // ros::Time delay1 = ros::Time::now();
    // while(ros::Time::now() - delay1 != ros::Duration(5.0))
    // {

    // }
    if( !current_state.armed ){
        if( arming_client.call(arm_cmd) &&
                arm_cmd.response.success){
                ROS_INFO("Vehicle armed");
        }
    }
        
    while(ros::ok()){
        
        set_mode_client.call(offb_set_mode);
        local_pos_pub.publish(pose);
        cout << "x: " << local_pos.pose.position.x << endl;
        cout << "y: " << local_pos.pose.position.y << endl;
        cout << "z: " << local_pos.pose.position.z << endl;
        ros::Time delay = ros::Time::now();
        err_x_max = pose.pose.position.x + 0.5;
        err_x_min = pose.pose.position.x - 0.5;
        err_y_max = pose.pose.position.y + 0.5;
        err_y_min = pose.pose.position.y - 0.5;
        err_z_max = pose.pose.position.z + 0.5;
        err_z_min = pose.pose.position.z - 0.5;
        if((local_pos.pose.position.x >= err_x_min && local_pos.pose.position.x <= err_x_max) && (local_pos.pose.position.y >= err_y_min && local_pos.pose.position.y <= err_y_max) && (local_pos.pose.position.z >= err_z_min && local_pos.pose.position.z <= err_z_max)){
            
            // if(ros::Time::now() - delay > ros::Duration(5.0))
            // {

                cout << "Enter x: ";
                cin >> pose.pose.position.x;
                cout << "Enter y: ";
                cin >> pose.pose.position.y;
                cout << "Enter z: ";
                cin >> pose.pose.position.z;  
            // }
        }
        for(int i = 100; ros::ok() && i > 0; --i){
            local_pos_pub.publish(pose);
            set_mode_client.call(offb_set_mode);
            ros::spinOnce();
            rate.sleep();
        }
        
        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}
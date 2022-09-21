//
// Created by waxz on 9/5/22.
//

#include "cpp_base64.h"
#include <iostream>
#include "opencv2/opencv.hpp"
#include "ros/ros.h"
#include "ros/callback_queue.h"
#include "nav_msgs/OccupancyGrid.h"
#include "xmlrpcpp/XmlRpc.h"

#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


//#include <mosquitto.h>
//#include <mosquittopp.h>
#include "message/MqttClient.h"


#include "json.hpp"

#include "common/signal_handler.h"
#include "../include/common/signal_handler.h"

template<typename T>
bool getParam(const std::string &name, T &value) {
    if (name.empty()) {
        std::cerr << "ros get xml value, empty name: " << name << std::endl;
        return false;
    }
    std::string private_name = name[0] != '/' ? ros::this_node::getName() + "/" + name : name;
    if (ros::param::has(private_name)) {
        std::cerr << "ros get xml value, has name: " << private_name << std::endl;
        ros::param::get(private_name, value);
        return true;
    } else {
        std::cerr << "ros get xml value,do not  has name: " << private_name << std::endl;
        return false;
    }
    return true;
}



//imgType 包括png bmp jpg jpeg等opencv能够进行编码解码的文件
std::string Mat2Base64(const cv::Mat &image, std::string imgType) {
    //Mat转base64
    std::vector<uchar> buf;
    cv::imencode(imgType, image, buf);
    //uchar *enc_msg = reinterpret_cast<unsigned char*>(buf.data());
    std::string img_data = base64_encode(buf.data(), buf.size(), false);
    return img_data;
}

cv::Mat Base2Mat(std::string &base64_data) {
    cv::Mat img;
    std::string s_mat;
    s_mat = base64_decode(base64_data.data(), false);
    std::vector<char> base64_img(s_mat.begin(), s_mat.end());
    img = cv::imdecode(base64_img, cv::IMREAD_UNCHANGED); //CV::IMREAD_UNCHANGED
    return img;
}

//! Second version
//! 2018.09.03 16:00:01 (+0800)
template<typename T>
cv::Mat createMat(const T *data, int rows, int cols, int chs = 1) {
    // Create Mat from buffer
    cv::Mat mat(rows, cols, CV_MAKETYPE(cv::DataType<T>::type, chs));
    memcpy(mat.data, data, rows * cols * chs * sizeof(T));
    return mat;
}


#include <iostream>
#include <string>
#include <stdio.h>
#include <time.h>

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
std::string getCurrentDateTime() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

    return buf;
}


class MosquittoClient {
private:
    std::string m_server_ip;
    int m_server_port;
    std::string m_username;
    std::string m_password;


public:
    MosquittoClient(const char *id, bool clean_session) {

    }

    void run(struct mosquitto *mosq, void *obj, int rc) {

        std::cerr << "mosq disconnect try to reconnect" << std::endl;
        mosquitto_reconnect_async(mosq);

    }

};


class Singleton {
public:

// Constructor? (the {} brackets) are needed here.



    // C++ 11
    // =======
    // We can use the better technique of deleting the methods
    // we don't want.
    template<typename T, typename... Args>
    static T &getInstance(Args &&... args) {
//        static T instance(Args &&... args); // Guaranteed to be destroyed.
        static T instance(std::forward<Args>(args)...);
        // Instantiated on first use.
        return instance;
    }

protected:
    Singleton() {}

    virtual ~Singleton() {
        std::cout << "~Singleton" << std::endl;
    };
public:
    Singleton(Singleton const &) = delete;

    void operator=(Singleton const &) = delete;

    // Note: Scott Meyers mentions in his Effective Modern
    //       C++ book, that deleted functions should generally
    //       be public as it results in better error messages
    //       due to the compilers behavior to check accessibility
    //       before deleted status

};

class RealSinleton : public Singleton {
    friend class Singleton;

public:
    std::string name;
private:
    RealSinleton(const std::string &name_) : name(name_) {}
};

#if 0
class MqttClient: public mosqpp::mosquittopp{

public:
    MqttClient(const char *id=NULL, bool clean_session=true):mosqpp::mosquittopp(id,clean_session){

    }
public:

    struct ConnOpt{
        std::string server_ip;
        int server_port;
        int keepalive = 60;
        std::string will_topic;
        std::string will_massage;
    };
    ConnOpt m_conn_opt;






    struct SubOption{
        std::string topic;
        int qos;
    };
    std::vector<SubOption> m_sub_opts;
    std::map<const char*, int> m_subs_topic_qos;

    void addSubOpt(const char* topic,int qos){
        m_subs_topic_qos.emplace(topic, qos);
    }
    void listSubOpts(){
        std::cout << "\n  listSubOpts\n" <<std::endl;
        for(auto it = m_subs_topic_qos.begin();it != m_subs_topic_qos.end();it++){
            std::cout << "-- k: " << it->first << ", v: " << it->second << std::endl;
        }
    }



    void on_connect(int reason_code){

        int rc;
        /* Print out the connection result. mosquitto_connack_string() produces an
         * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
         * clients is mosquitto_reason_string().
         */
        printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
        if(reason_code != 0){
            /* If the connection fails for any reason, we don't want to keep on
             * retrying in this example, so disconnect. Without this, the client
             * will attempt to reconnect. */
            disconnect();
        }

        for(auto it = m_subs_topic_qos.begin();it != m_subs_topic_qos.end();it++){
            std::cout << "-- k: " << it->first << ", v: " << it->second << std::endl;
            /* Making subscriptions in the on_connect() callback means that if the
             * connection drops and is automatically resumed by the client, then the
             * subscriptions will be recreated when the client reconnects. */
            rc = subscribe( NULL, it->first, it->second);
            if(rc != MOSQ_ERR_SUCCESS){
                fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
                /* We might as well disconnect if we were unable to subscribe */
                disconnect();

            }
        }

    }

    /* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
    void on_subscribe( int mid, int qos_count, const int *granted_qos)
    {
        int i;
        bool have_subscription = false;

        /* In this example we only subscribe to a single topic at once, but a
         * SUBSCRIBE can contain many topics at once, so this is one way to check
         * them all. */
        for(i=0; i<qos_count; i++){
            printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
            if(granted_qos[i] <= 2){
                have_subscription = true;
            }
        }
        if(!have_subscription){
            /* The broker rejected all of our subscriptions, we know we only sent
             * the one SUBSCRIBE, so there is no point remaining connected. */
            fprintf(stderr, "Error: All subscriptions rejected.\n");
            disconnect();
        }
    }

    std::function<int(const char*, const char*)> message_callback_func;


    /* Callback called when the client receives a message. */
    void on_message(const struct mosquitto_message *msg)
    {
        /* This blindly prints the payload, but the payload can be anything so take care. */
        printf("sub receive: %s %d %s\n", msg->topic, msg->qos, (char *)msg->payload);
        message_callback_func(msg->topic, (char *)msg->payload);
    }
    void on_disconnect(int rc)
    {
        std::cerr << "mosq disconnect try to reconnect" << std::endl;
        reconnect_async();
    }
    virtual ~MqttClient() {
        std::cout << "~MqttClient"<<std::endl;

        mosquitto_lib_cleanup();
    }

};
#endif

int main(int argc, char **argv) {


    RealSinleton &inst_RealSinleton = RealSinleton::Singleton::getInstance<RealSinleton>("hhh");


    //==== ros
    ros::init(argc, argv, "map_republisher");
    ros::NodeHandle nh;
    ros::NodeHandle nh_private("~");

    XmlRpc::XmlRpcValue xmlvalue;

    std::string mqtt_server_ip = "10.23.0.125";
    int mqtt_server_port = 1883;
    int mqtt_topic_qos = 0;
    std::string mqtt_user_name = "user_1";
    std::string mqtt_passward = "user_1";
    std::string mqtt_client_id = "agv-";
    std::string mqtt_will_topic;
    std::string mqtt_will_message;
    int mqtt_keep_alive = 5;


    bool mqtt_debug = true;

    std::string mqtt_server_ip_param = "mqtt_server_ip";
    std::string mqtt_server_port_param = "mqtt_server_port";
    std::string mqtt_topic_qos_param = "mqtt_topic_qos";


    std::string map_name = "map_0";
    std::string map_name_param = "mapName";
    std::string agv_sn = "q0000001";
    std::string agv_sn_param = "agvSn";
    std::string base64Img_param = "base64Img";
    std::string returnTime_param = "returnTime";


    getParam(map_name_param, map_name);
    getParam(agv_sn_param, agv_sn);
    getParam(mqtt_server_ip_param, mqtt_server_ip);
    getParam(mqtt_server_port_param, mqtt_server_port);
    getParam(mqtt_topic_qos_param, mqtt_topic_qos);


    std::string mqtt_topic = "agv/map/ret/" + agv_sn;
    mqtt_client_id = mqtt_client_id + agv_sn;

    std::cout << agv_sn_param << " : " << agv_sn << std::endl;
    std::cout << map_name_param << " : " << map_name << std::endl;
    std::cout << mqtt_server_ip_param << " : " << mqtt_server_ip << std::endl;
    std::cout << mqtt_server_port_param << " : " << mqtt_server_port << std::endl;
    std::cout << mqtt_topic_qos_param << " : " << mqtt_topic_qos << std::endl;

    std::cout << "mqtt_topic" << " : " << mqtt_topic << std::endl;


    mosqpp::lib_init();

    message::MqttClient MqttInstance(mqtt_client_id.c_str(), true);

//    MqttClient& MqttInstance = MqttClient::Singleton::getInstance<MqttClient>(client_id,true);
    MqttInstance.message_callback_func = [&](const char *topic, const char *playload) {

        std::cout << "message_callback_func : " << topic << ", " << playload << std::endl;

        return 1;
    };

//    mqtt_will_topic = "online";
//    mqtt_will_message = "die";

    MqttInstance.addSubOpt("hello", 0);
    MqttInstance.addSubOpt("hello2", 0);
    MqttInstance.addSubOpt(mqtt_topic.c_str(), 0);

    MqttInstance.listSubOpts();

    if (!mqtt_will_topic.empty() and !mqtt_will_message.empty()) {
        MqttInstance.will_set(mqtt_will_topic.c_str(), mqtt_will_message.size(), mqtt_will_message.c_str());
    }

    MqttInstance.connect_async(mqtt_server_ip.c_str(), mqtt_server_port, mqtt_keep_alive);

    int rc = 0;

    MqttInstance.loop_start();





    // ros topic


    nlohmann::json j;

    j[agv_sn_param] = agv_sn;
    j[map_name_param] = map_name;


    cv::Mat map_mat;
    std::string img_data;

    cv::Mat dest;

    std::string json_string;
    cv::Mat map_mat_uchar;

    auto cb = [&](const nav_msgs::OccupancyGridConstPtr &msg) {


        map_mat = createMat<signed char>(msg->data.data(), int(msg->info.height), int(msg->info.width));
        int height = msg->info.height;
        map_mat.convertTo(map_mat_uchar, CV_8U);
//        map_mat = cv::Mat::ones(map_mat.size(), map_mat.type()) * 100 - map_mat;
        cv::parallel_for_(cv::Range(0, map_mat_uchar.rows * map_mat_uchar.cols), [&](const cv::Range &range) {
            for (int r = range.start; r < range.end; r++) {
                int i = r / map_mat_uchar.cols;
                int j = r % map_mat_uchar.cols;
                int i_flip = height - i-1;
                if (map_mat.ptr<signed char>(i)[j] == 0) {
                    map_mat_uchar.ptr<unsigned char>(i_flip)[j] = 254;
                } else if (map_mat.ptr<signed char>(i)[j] == 100) {
                    map_mat_uchar.ptr<unsigned char>(i_flip)[j] = 0;
                } else {
                    map_mat_uchar.ptr<unsigned char>(i_flip)[j] = 205;

                }
            }
        });


        img_data = Mat2Base64(map_mat_uchar, ".png");


        j[returnTime_param] = getCurrentDateTime();
        j[base64Img_param] = img_data;

        json_string = j.dump();
        MqttInstance.publish(nullptr, mqtt_topic.c_str(), json_string.size(), json_string.c_str(), mqtt_topic_qos);

    };


    ros::Subscriber sub = nh.subscribe<nav_msgs::OccupancyGrid>("map", 1, cb);


    bool show_image = false;
    std::cout << "start ros loop " << std::endl;
    MqttInstance.reconnect_delay_set(1, 2, false);


    while (ros::ok()) {

        rc = MqttInstance.loop();
        if (rc != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "Error mosquitto_loop_misc: %s\n", mosquitto_strerror(rc));
        }
        if (rc == MOSQ_ERR_NO_CONN) {
            std::cout << " MOSQ_ERR_NO_CONN : " << rc << std::endl;
        }

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1000ms);
        ros::spinOnce();


    }
//    mosqpp::lib_cleanup();

    return 0;


    int arr1[4] = {1, 2, 3, 4};
    double arr2[4] = {1.1, 2.2, 3.3, 4.4};
    unsigned char arr3[4] = {1, 2, 3, 4};

    cv::Mat mat1 = createMat<int>(arr1, 2, 2);
    cv::Mat mat2 = createMat<double>(arr2, 2, 2);
    cv::Mat mat3 = createMat<unsigned char>(arr3, 2, 2);

    std::cout << "Mat1:\n" << mat1 << std::endl;
    std::cout << "Mat2:\n" << mat2 << std::endl;
    std::cout << "Mat3:\n" << mat3 << std::endl;

    img_data = Mat2Base64(mat3, ".png");
    printf("%s\n", img_data.c_str());

    cv::waitKey(0);
    return 0;
}
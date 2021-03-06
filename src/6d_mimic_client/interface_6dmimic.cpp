//
// Created by joaopedro on 11/08/21.
//

#include "interface_6dmimic.h"


SixDMimicLocalization::SixDMimicLocalization() {};

SixDMimicLocalization::~SixDMimicLocalization() {
    freeMem();
};

bool SixDMimicLocalization::setAppConfigPath(std::string _file_with_path) {
    plugin_config_path_ = _file_with_path;
    return true;
}

bool SixDMimicLocalization::setAppExec(std::string _file_with_path_or_command) {
    plugin_exec_path_ = _file_with_path_or_command;
    return true;
}

bool SixDMimicLocalization::setAppTermination(std::string _file_with_path_or_command) {
    plugin_terminator_path_ = _file_with_path_or_command;
    return true;
}

bool SixDMimicLocalization::setTargetName(std::string _name_with_path) {
    target_name_ = _name_with_path;
    return true;
}


std::string SixDMimicLocalization::getTargetName()
{
    return target_name_;
}

bool SixDMimicLocalization::loadAppConfiguration() {

    std::ifstream config_file(plugin_config_path_, std::ifstream::binary);
    if (config_file) {
        try {
            config_file >> config_data_;
        } catch (const std::exception &e) {
            output_string_ = e.what();
            return false;
        }
    } else {
        output_string_ = plugin_name + " configuration file not found. Current path: \"" + plugin_config_path_ + "\"";
        return false;
    }

    target_name_ = config_data_["prefix_name"].asString();
    wait_for_sixdmimic_result_timeout_in_seconds_ = config_data_["wait_for_result_timeout_in_seconds"].asInt();
    listen_udp_.ip_addr = config_data_["listen_ip_address"].asString();
    listen_udp_.ip_port = config_data_["listen_ip_port"].asInt();
    pub_udp_.ip_addr = config_data_["publisher_ip_address"].asString();
    pub_udp_.ip_port = config_data_["publisher_ip_port"].asInt();

    if (plugin_exec_path_.empty()) {
        output_string_ = plugin_name + " exec is not defined.";
        status_ = FEEDBACK::ERROR;
        return false;
    }

    if (plugin_terminator_path_.empty()) {
        output_string_ = plugin_name + " terminator is not defined.";
        status_ = FEEDBACK::ERROR;
        return false;
    }

    return true;
}

bool SixDMimicLocalization::runApp() {

    status_ = FEEDBACK::RUNNING;
    stopApp();

    candidate_index_ = 0;

    pipe_to_sixdmimic_localization_ = popen(plugin_exec_path_.c_str(), "r");

    std::string aux = strerror(errno);

    if(pipe_to_sixdmimic_localization_ == NULL){
        output_string_ = "Error in open the executor file for 6DMimic plugin. ";
        output_string_ += aux;
        DEBUG_MSG(output_string_);
        //strerror(errno); //printing errno value
        return false;
    }

    int descriptor = fileno(pipe_to_sixdmimic_localization_);
    fcntl(descriptor, F_SETFL, O_NONBLOCK);

    sixdmimic_localization_thread_reader_.reset(
            new boost::thread(boost::bind(&SixDMimicLocalization::execCallback, this, descriptor)));

    first_sixdmimic_localization_communication_ = false;

    if (listen_udp_.ip_addr.empty() || pub_udp_.ip_addr.empty()) {
        output_string_ = "Invalid UDP IP Address";
        status_ = FEEDBACK::ERROR;
        return false;
    }

    if (listen_udp_.ip_port < 0 || pub_udp_.ip_port < 0) {
        output_string_ = "Invalid UDP IP Port";
        status_ = FEEDBACK::ERROR;
        return false;
    }

    try {
        udp_client_ = std::make_shared<simple_network::udp_interface::UDPClient>(simple_network::IPv4, pub_udp_.ip_addr,
                                                                                 pub_udp_.ip_port);
        udp_server_ = std::make_shared<simple_network::udp_interface::UDPServer>(simple_network::IPv4,
                                                                                 listen_udp_.ip_addr,
                                                                                 listen_udp_.ip_port);
    }
    catch (std::exception &e) {
        output_string_ = e.what();
        return false;
    }


    if (!err_flag_pipe_corrupted_) {
        status_ = FEEDBACK::INITIALIZING;
    } else
        stopApp();


    return true;
}

void SixDMimicLocalization::freeMem(){
    udp_client_.reset();
    udp_server_.reset();

    if(sixdmimic_localization_thread_reader_.use_count()>=1)
        sixdmimic_localization_thread_reader_.reset();
}

bool SixDMimicLocalization::stopApp() {
    if (!first_sixdmimic_localization_communication_) {
        //udp_client_->closeSocket();
        udp_server_->closeSocket();
        sixdmimic_localization_thread_reader_->interrupt();
        sixdmimic_localization_thread_reader_->join();

        if(pclose(pipe_to_sixdmimic_localization_) == -1){
            std::string s = strerror(errno);
            output_string_ = "Failed to call 6DMimic terminator " + s;
            DEBUG_MSG(output_string_);
            exit(1);
        }

        DEBUG_MSG("Killing 6Dmimic server");
        //popen(plugin_terminator_path_.c_str(), "r");
        first_sixdmimic_localization_communication_ = true;
        freeMem();

        if(system(plugin_terminator_path_.c_str())==-1){
            output_string_ = "Error in open 6DMimic terminator. ";
        }

        return true;
    } else {
        output_string_ = "Cannot kill since the 6DMimic recognition process is not running.";
        return false;
    }

}

bool SixDMimicLocalization::requestData(Pose &_result) {

    status_ = FEEDBACK::PROCESSING;

    if (err_flag_pipe_corrupted_) {
        output_string_ = "Corrupted pipe to " + plugin_name + " interface.";
        status_ = FEEDBACK::ERROR;
        return false;
    }

    if (!udp_client_->send("STARTLEARNING")) {
        output_string_ = "Fail to request data. | " + udp_client_->getOutputMSG();
        DEBUG_MSG(output_string_);
        status_ = FEEDBACK::ERROR;
        return false;
    }

    std::string data;
    const clock_t begin_time = clock();
    float current_time = 0;

    for (;;) {
        udp_server_->spinPoll();

        if (udp_server_->getLastReceivedDataTimestamp() != "" && (udp_server_->getLastReceivedDataTimestamp().compare(prev_timestamp) != 0 )) { //is different?

            DEBUG_MSG("Received from " << udp_server_->getLastClientAddress() << " at ["
            << udp_server_->getLastReceivedDataTimestamp() << "] " << "The following message: \n"
            << udp_server_->getLastReceivedData());

            data = udp_server_->getLastReceivedData();
            prev_timestamp = udp_server_->getLastReceivedDataTimestamp();
            DEBUG_MSG("Previous Timestamp Updated to : " << prev_timestamp);
            break;
        }

        current_time = float(clock() - begin_time) / CLOCKS_PER_SEC;
        //std::cout << "waiting: " << std::to_string(current_time) << std::endl;

        if (current_time > wait_for_sixdmimic_result_timeout_in_seconds_) {
            output_string_ = "Fail to request data. | Request timeout. ";
            DEBUG_MSG(output_string_);
            status_ = FEEDBACK::ABORTED;
            return false;
        }
    }

    std::vector<std::string> data_arr;
    std::stringstream ss;

    if (!convertDataStrToPose(data, data_arr)) {
        for (size_t i = 0; i < data_arr.size(); ++i) {
            ss << data_arr.at(i) << " ; ";
        }

        output_string_ = "Data protocol is not correct. Decoded message: \"" + ss.str() + "\"";
        //std::cout << "xxx: " << output_string_ << std::endl;
        status_ = FEEDBACK::ERROR;
        return false;
    }

    for (size_t i = 0; i < data_arr.size(); ++i) {
        ss << data_arr.at(i) << " ; ";
    }
    DEBUG_MSG("Decoded msg: " + ss.str());

    Eigen::Translation3d t(std::stod(data_arr.at(3)),
                           std::stod(data_arr.at(4)),
                           std::stod(data_arr.at(5)));

    /*
    std::cout << "atof t [x,y,z] = "<< std::atof(data_arr.at(3).c_str()) << " | " << std::atof(data_arr.at(4).c_str()) << " | " << std::atof(data_arr.at(5).c_str()) << std::endl;
    std::cout << "t [x,y,z] = "<< t.x() << " | " << t.y() << " | " << t.z() << std::endl;
    */

    Eigen::Vector3d q(std::stod(data_arr.at(6)),
                      std::stod(data_arr.at(7)),
                      std::stod(data_arr.at(8)));

    /*
    std::cout << "atof q[x,y,z] = "<< std::atof(data_arr.at(6).c_str() )<< " | " << std::atof(data_arr.at(7).c_str()) << " | " << std::atof(data_arr.at(8).c_str()) << std::endl;
    std::cout << "q [x,y,z] = "<< q.x() << " | " << q.y() << " | " << q.z() << std::endl;
    */

    _result.setName(target_name_ + std::to_string(candidate_index_));
    _result.setParentName(data_arr.at(2));
    _result.setPosition(t);
    _result.setRPYOrientationZYXOrder(q);

    DEBUG_MSG("\nConverted Message\nName: "<<_result.getName() << "\nParent: " << _result.getParentName()
    << "\n Pos [x,y,z][meters]: " << _result.getPosition().x()  << " | " << _result.getPosition().y()  << " | " << _result.getPosition().z()
    << "\n RPY [x,y,z][radians]: " << _result.getRPYOrientationZYXOrder().x() << " | " << _result.getRPYOrientationZYXOrder().y() << " | " << _result.getRPYOrientationZYXOrder().z()
    << "\n Quaternion [x,y,z,w]: " << _result.getQuaternionOrientation().x() << " | " << _result.getQuaternionOrientation().y() << " | " << _result.getQuaternionOrientation().z() << " | " << _result.getQuaternionOrientation().w() );

    candidate_index_++;
    return true;

}

std::string SixDMimicLocalization::getNameFromPath(std::string _s) {
    size_t pos = 0;
    std::string token, delimiter = "/";

    while (((pos = _s.find(delimiter)) != std::string::npos)) {
        token = _s.substr(0, pos);
        _s.erase(0, pos + delimiter.length());
    }

    delimiter = ".";
    _s = _s.substr(0, _s.find(delimiter));
    return _s;
}

int SixDMimicLocalization::getStatus() {
    if (err_flag_pipe_corrupted_) {
        output_string_ = "Corrupted pipe to " + plugin_name + " interface. ";
        return FEEDBACK::ERROR;
    }
    return status_;
}

std::string SixDMimicLocalization::getOutputString() {
    return (MSG_PREFIX + output_string_);
}

void SixDMimicLocalization::execCallback(int _file_descriptor) {
    for (;;) {
        try {
            this->exec(_file_descriptor);
            boost::this_thread::interruption_point();
            //DEBUG_MSG("dentro do try");
            //boost::this_thread::sleep(boost::posix_time::milliseconds(500)); //interruption with sleep
        }
        catch (boost::thread_interrupted &) {
            DEBUG_MSG( plugin_name + " pipe thread is stopped." );

            return;
        }
    }
}

void SixDMimicLocalization::exec(int _file_descriptor) {

    char buffer[128];

    ssize_t r = read(_file_descriptor, buffer, 128);
    //DEBUG_MSG( "Aqui dentro" );

    if (r == -1 && errno == EAGAIN) { }
    else if (r > 0) {
        current_pipe_output_str_ = buffer;
        //DEBUG_MSG( "Aqui dentro do ELSE IF" );
    } else {
        err_flag_pipe_corrupted_ = true;
        //DEBUG_MSG( "Aqui dentro do ELSE" );
        return;
    }

}

void SixDMimicLocalization::spin(int _usec) {
    sixdmimic_localization_thread_reader_->timed_join(boost::chrono::milliseconds(_usec));
    if (err_flag_pipe_corrupted_) {
        stopApp();
    }

}

bool SixDMimicLocalization::convertDataStrToPose(std::string _in, std::vector<std::string> &_data_arr) {

    size_t pos = 0;
    std::string token, delimiter = "|";

    while (((pos = _in.find(delimiter)) != std::string::npos)) {
        token = _in.substr(0, pos);
        _data_arr.push_back(token);
        _in.erase(0, pos + delimiter.length());
    }
    _data_arr.push_back(_in);

    if (_data_arr.size() != 10)
        return false;

    if (_data_arr.at(0).compare("<b>"))
        return false;

    if (_data_arr.at(9).compare("<e>"))
        return false;

    return true;

}
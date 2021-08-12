//
// Created by joaopedro on 11/08/21.
//

#include "interface_6dmimic.h"


SixDMimicLocalization::SixDMimicLocalization() {

}

SixDMimicLocalization::~SixDMimicLocalization() {
    stopApp();
}

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

    pipe_to_sixdmimic_localization_.reset(
            popen(plugin_exec_path_.c_str(), "r")); // TODO: find a solution to treat this error!

    int descriptor = fileno(pipe_to_sixdmimic_localization_.get());
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


bool SixDMimicLocalization::stopApp() {

    if (!first_sixdmimic_localization_communication_) {

        sixdmimic_localization_thread_reader_->interrupt();
        sixdmimic_localization_thread_reader_->join();
        //pclose(pipe_to_obj_localization_.get());
        popen(plugin_terminator_path_.c_str(), "r");
        first_sixdmimic_localization_communication_ = true;
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
        status_ = FEEDBACK::ERROR;
        return false;
    }

    std::string prev_timestamp, data;
    const clock_t begin_time = clock();
    float current_time = 0;

    for (;;) {
        udp_server_->spinPoll();
        if (!(udp_server_->getLastReceivedDataTimestamp() == prev_timestamp)) {
            /*std::cout << "Received from " << udp_server_->getLastClientAddress() << " at ["
            << udp_server_->getLastReceivedDataTimestamp() << "] " << "The following message: \n"
            << udp_server_->getLastReceivedData() << std::endl;
            */
            data = udp_server_->getLastReceivedData();
            prev_timestamp = udp_server_->getLastReceivedDataTimestamp();
            break;
        }

        current_time = float(clock() - begin_time) / CLOCKS_PER_SEC;
        //std::cout << "waiting: " << std::to_string(current_time) << std::endl;

        if (current_time > wait_for_sixdmimic_result_timeout_in_seconds_) {
            output_string_ = "Fail to request data. | Request timeout. ";
            status_ = FEEDBACK::ERROR;
            return false;
        }
    }

    std::vector<std::string> data_arr;
    if (!convertDataStrToPose(data, data_arr)) {
        std::stringstream ss;
        for (size_t i = 0; i < data_arr.size(); ++i) {
            ss << data_arr.at(i) << " ";
        }

        output_string_ = "Data protocol is not correct. Decoded message: \"" + ss.str() + "\"";
        status_ = FEEDBACK::ERROR;
        return false;
    }

    Eigen::Translation3d t(std::stod(data_arr.at(3)),
                           std::stod(data_arr.at(4)),
                           std::stod(data_arr.at(5)));

    Eigen::Vector3d q(std::stod(data_arr.at(6)),
                      std::stod(data_arr.at(7)),
                      std::stod(data_arr.at(8)));

    _result.setName("candidate_" + std::to_string(candidate_index_));
    _result.setParentName(data_arr.at(2));
    _result.setPosition(t);
    _result.setRPYOrientationZYXOrder(q);

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
    return output_string_;
}

void SixDMimicLocalization::execCallback(int _file_descriptor) {
    for (;;) {
        try {
            this->exec(_file_descriptor);
            boost::this_thread::interruption_point();
            //boost::this_thread::sleep(boost::posix_time::milliseconds(500)); //interruption with sleep
        }
        catch (boost::thread_interrupted &) {
            std::cout << plugin_name + " pipe thread is stopped." << std::endl;
            return;
        }
    }
}

void SixDMimicLocalization::exec(int _file_descriptor) {

    char buffer[128];

    ssize_t r = read(_file_descriptor, buffer, 128);

    if (r == -1 && errno == EAGAIN) {}
    else if (r > 0) {
        current_pipe_output_str_ = buffer;
    } else {
        err_flag_pipe_corrupted_ = true;
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
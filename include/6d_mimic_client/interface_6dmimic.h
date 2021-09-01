//
// Created by joaopedro on 11/08/21.
//

#ifndef MIMIC_GRASPING_PLUGIN_GENERATOR_INTERFACE_6DMIMIC_NODE_H
#define MIMIC_GRASPING_PLUGIN_GENERATOR_INTERFACE_6DMIMIC_NODE_H

#include <fcntl.h>
#include <unistd.h> //read function
#include <boost/thread.hpp>
#include <fstream>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/json.h>

#include <simple_network/udp_interface.h>


#include <mimic_grasping_api/localization_base.h>

class SixDMimicLocalization : public LocalizationBase {
public:
    SixDMimicLocalization();
    ~SixDMimicLocalization();

    bool setAppConfigPath(std::string _file_with_path);

    bool setAppExec(std::string _file_with_path_or_command);

    bool setAppTermination(std::string _file_with_path_or_command);

    bool setTargetName(std::string _name);

    bool loadAppConfiguration();

    bool runApp();

    bool stopApp();

    bool requestData(Pose &_result);

    int getStatus();

    void spin(int _usec);

    std::string getOutputString();

protected:

    bool first_sixdmimic_localization_communication_ = true,
    err_flag_pipe_corrupted_ = false;

    std::string current_pipe_output_str_;

    std::shared_ptr<boost::thread> sixdmimic_localization_thread_reader_;
    std::shared_ptr<FILE> pipe_to_sixdmimic_localization_;

    Json::Value config_data_;

    void execCallback(int _descriptor);

    void exec(int _descriptor);

    std::string getNameFromPath(std::string _s);

private:

    int wait_for_sixdmimic_server_timeout_in_seconds_,
        candidate_index_,
        wait_for_sixdmimic_result_timeout_in_seconds_;

    struct IPV4{
        std::string ip_addr = "";
        int ip_port = -1;
    } listen_udp_, pub_udp_;

    std::shared_ptr<simple_network::udp_interface::UDPClient> udp_client_;
    std::shared_ptr<simple_network::udp_interface::UDPServer> udp_server_;

    bool convertDataStrToPose(std::string _in, std::vector<std::string> &_data_arr);
};

/** ################## Factory Function - Plugin EntryPoint  ##################  **/
/** This part should be added in all plugins, therefore the plugin management can identified it. **/

/** Define the Plugin name **/
std::string plugin_name = "Interface6DMimicPlugin";

PLUGIN_EXPORT_C
auto GetPluginFactory() -> IPluginFactory * {

    static PluginFactory pinfo = [] {
        /** Properly set the plugin name and version **/
        auto p = PluginFactory(plugin_name.c_str(), "01_09_2021");
        /** Register all classes defined inside the plugin **/
        p.registerClass<SixDMimicLocalization>("SixDMimicLocalization");
        return p;
    }();
    return &pinfo;
}

struct _DLLInit {
    _DLLInit() {
        std::cout << " [TRACE] Shared library " << plugin_name << " loaded OK.\n";
    }

    ~_DLLInit() {
        std::cout << " [TRACE] Shared library " << plugin_name << " unloaded OK.\n";
    }
} dll_init;

#endif //MIMIC_GRASPING_PLUGIN_GENERATOR_INTERFACE_6DMIMIC_NODE_H

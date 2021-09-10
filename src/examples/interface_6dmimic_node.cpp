//#include "interface_6dmimic.h"
#include <plugin_system_management/plugin_system_management.h>
#include <mimic_grasping_api/localization_base.h>

int main(int argc, char **argv) {

    PluginSystemManagement ph_;

    std::string path = (std::filesystem::current_path().parent_path().string() + "/plugins/");
    if(!ph_.loadDynamicPlugins(path,true)){
        std::cout << ph_.getPluginManagementOutputMsg() << std::endl;
        return false;
    }

    std::cout<< "Number of plugins loaded: " << ph_.GetNumberOfPluginsLoaded() << std::endl;

    std::shared_ptr<LocalizationBase> instance = ph_.CreateInstanceAs<LocalizationBase>(ph_.GetPluginFactoryInfo(0)->Name(),ph_.GetPluginFactoryInfo(0)->GetClassName(0));
    assert(instance != nullptr);

    char const* env_root_folder_path;

    env_root_folder_path =  getenv("MIMIC_GRASPING_SERVER_ROOT");

    if(env_root_folder_path == NULL) {
        std::cerr<<"The environment variable $MIMIC_GRASPING_SERVER_ROOT is not defined."<<std::endl;
        return 1;
    }

    std::string root_folder_path_ = std::string(env_root_folder_path);

    instance->setAppExec(root_folder_path_ + "/scripts/tool_localization_6dmimic_init.sh");
    instance->setAppTermination( root_folder_path_ + "/scripts/tool_localization_6dmimic_close.sh");
    instance->setAppConfigPath(root_folder_path_ + "/configs/default/plugin_tool_localization_6dmimic.json");

    if (!instance->loadAppConfiguration() || !instance->runApp()) {
        std::cerr << instance->getOutputString() << std::endl;
        return 1;
    }

        Pose p;
        int it = 1;
        while (true) {
            //std::cin.ignore();
            std::cout<< "#" << it << " localization recognition iteration..." << std::endl;
            instance->spin(250);
            if(instance->requestData(p)){
                std::cout<< "\n Child frame name: " << p.getName() << "\n Parent frame name: " << p.getParentName()
                << "\n Position [x,y,z]: [" <<
                p.getPosition().x() << ", " << p.getPosition().y() << ", "
                << p.getPosition().z() << "]\n Orientation [x,y,z,w]: [" <<
                p.getQuaternionOrientation().x() << ", "
                << p.getQuaternionOrientation().y() << ", "
                << p.getQuaternionOrientation().z() << ", "
                << p.getQuaternionOrientation().w() << "]\n" << std::endl;
                break;
            }
            else{
                std::cerr<<"ERROR in tool localization estimation: "<< instance->getOutputString() <<std::endl;
            }
            it++;
            sleep(1);
        }

    instance->spin(250);
    instance->stopApp();
    return 0;
}

#include "6d_mimic_interface.h"

int main(int argc, char **argv) {

    char const* env_root_folder_path;

    env_root_folder_path =  getenv("MIMIC_GRASPING_SERVER_ROOT");

    if(env_root_folder_path == NULL) {
        std::cerr<<"The environment variable $MIMIC_GRASPING_SERVER_ROOT is not defined."<<std::endl;
        return false;
    }

    std::string root_folder_path_ = std::string(env_root_folder_path);

    SixDMimicLocalization o;

    o.setAppExec(root_folder_path_ + "/scripts/tool_localization_6dmimic_init.sh");
    o.setAppTermination( root_folder_path_ + "/scripts/tool_localization_6dmimic_close.sh");
    o.setAppConfigPath(root_folder_path_ + "/configs/plugin_tool_localization_6dmimic.json");

    if (!o.loadAppConfiguration() || !o.runApp()) {
        std::cout << o.getOutputString() << std::endl;
        return false;
    }


        Pose p;
        int it = 1;
        while (true) {
            //std::cin.ignore();
            std::cout<< "#" << it << " localization recognition iteration..." << std::endl;
            o.spin(250);
            if(o.requestData(p)){
                std::cout<< "\n Child frame name: " << p.getName() << "\n Parent frame name: " << p.getParentName()
                << "\n Position [x,y,z]: [" <<
                p.getPosition().x() << ", " << p.getPosition().y() << ", "
                << p.getPosition().z() << "]\n Orientation [x,y,z,w]: [" <<
                p.getQuaternionOrientation().x() << ", "
                << p.getQuaternionOrientation().y() << ", "
                << p.getQuaternionOrientation().z() << ", "
                << p.getQuaternionOrientation().w() << "]\n" << std::endl;
            }
            else{
                std::cerr<<"ERROR in tool localization estimation: "<< o.getOutputString() <<std::endl;
            }
            it++;
            sleep(1);
        }



    o.spin(250);
    o.stopApp();
    return 0;
}

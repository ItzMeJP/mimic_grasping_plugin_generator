# Mimic Grasping Plugin Generator
<a rel="license" href="http://creativecommons.org/licenses/by-nc-nd/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc-nd/4.0/88x31.png" />

* [Description](#Description)
* [Prerequisites](#Prerequisites)
* [Installation](#Installation)
* [Usage](#Usage)


### <a name="Description"></a>1. Description

Package to build pure c++ plugins to [mimic_grasping_api](https://gitlab.inesctec.pt/CRIIS/mimicgrasping/mimic_grasping_api).

### <a name="Prerequisites"></a>2. Prerequisites

* [Eigen3](https://eigen.tuxfamily.org/index.php?title=Main_Page)
* [Boost](https://www.boost.org/)
* [Jsoncpp](https://open-source-parsers.github.io/jsoncpp-docs/doxygen/index.html#_intro)
* [Plugin System Management](https://gitlab.inesctec.pt/CRIIS/mimicgrasping/plugin_system_management)
* [Simple Network](https://gitlab.inesctec.pt/CRIIS/mimicgrasping/simple_network)
* [Transform Manipulation](https://github.com/ItzMeJP/transform_manipulation)
* [Mimic Grasping 6DMimic Interface](TODO)
* [Mimic Grasping API](https://gitlab.inesctec.pt/CRIIS/mimicgrasping/mimic_grasping_api)


### <a name="Installation"></a>3. Installation

Setup all prerequisites.

Clone and compile the repository:
```
git clone TODO
cd mimic_grasping_plugin_generator
mkkir build
cd build
cmake ..
make install
```

### <a name="Usage"></a>4. Usage

After the installation, the generated plugins will be placed at [plugins folder](./plugins). The supported plugins are:

1. **lib6d_mimic_interface**: 6DMimic localization plugin. The plugin request pose from [Mimic Grasping 6DMimic Interface](TODO). Associated tester node: "6dmimic_interface_node".

To run a plugin, the loader package need to be in accordance with [Mimic Grasping API](https://gitlab.inesctec.pt/CRIIS/mimicgrasping/mimic_grasping_api) which plugin system is based on [Plugin System Management](https://gitlab.inesctec.pt/CRIIS/mimicgrasping/plugin_system_management) API.

Once the shared plugin lib is loaded in run-time, it is necessary to call methods to load configurations (.JSON) and run the external executables.
For examples, check [here](./src/examples). The initialization scripts and configurations JSON files, used in these cases are located at [Mimic Grasping API](https://gitlab.inesctec.pt/CRIIS/mimicgrasping/mimic_grasping_api). It uses the environment variable $MIMIC_GRASPING_SERVER_ROOT as reference.

### Testers
Each plugin can be tested by running each plugin node tester. Bellow is presented the steps to each one:

##### **Raw tester**

Tester used to debug **lib6d_mimic_interface**.
To run, just:

```
cd build
./6dmimic_interface_node
```

**NOTE:**
Instead of using the [Mimic Grasping 6DMimic Interface](TODO), which need to have the hardware to perform the detection, a simple UDP client was implemented in lazarus/pascal [here](TODO). It is usefull for debug.



-----------------------------------------------------------------------------------------------------------------
<br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-nc-nd/4.0/">Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License</a>.

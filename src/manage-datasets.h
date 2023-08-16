#include "torchgl.h"

#include "import_utils/cv_utils.h"
#include "import_utils/import_utils.h"
#include "import_utils/colmap.h"
#include "import_utils/scannet.h"
#include "export_utils/write_out.h"


std::vector<Dataset> datasets = std::vector<Dataset>(); 
Dataset new_dataset;
Dataset processed_dataset;
std::shared_ptr<Dataset> active_dataset;

std::map<std::string, std::string> dataset_config_parser(const std::filesystem::path& path){
    std::map<std::string, std::string> config;

    std::ifstream file(path, std::ifstream::in);
    if (file.is_open()) {
        std::string line;
        std::string name, value;

        while (getline(file, line)) {
            // catch empty and comment lines
            if (line.empty() || line[0] == '#') 
                continue;

            std::istringstream line_stream(line);
            getline(line_stream, name, ':');
            getline(line_stream, value, ':');
            name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
            value.erase(std::remove(value.begin(), value.end(), ' '), value.end());

            config[name] = value;
            std::cout << name << ": " << value << std::endl;
        }
    } else {
        std::cout << path << " was not opened!!" << std::endl;
    }
    std::cout << "Dataset config from " << path << " loaded" << std::endl;
    return config;
}

std::filesystem::path get_absolute_path(const std::filesystem::path& path, const std::filesystem::path& root_dir){
    if(path.is_absolute())
        return path;

    if (path.is_relative())
        return root_dir / path;

    if(root_dir.empty() && path.is_relative())
        std::cerr << "WARNING: no root dir given and path relative...." << std::endl;
    
    return path;
}

void load_dataset_from_config(const std::filesystem::path& dataset_config_file, const std::filesystem::path& dataset_root_dir=std::filesystem::path("")){

    if(!std::filesystem::is_regular_file(dataset_config_file)){
        std::cerr << dataset_config_file << " is not a regular file..."  << std::endl;
    }

    auto config = dataset_config_parser(dataset_config_file);
    std::filesystem::path root_dir = dataset_root_dir.empty() ? std::filesystem::path(config["root_dir"]) : dataset_root_dir;
    std::filesystem::path image_dir = get_absolute_path(config["image_dir"], root_dir);
    std::filesystem::path depth_dir = get_absolute_path(config["depth_dir"], root_dir);

    if(config["type"] == "colmap"){
        datasets.push_back(colmap::load_dataset(config["colmap_txt_dir"], 
                        image_dir, depth_dir,
                        get_absolute_path(config["model_path"], root_dir),
                        config["image_file_ending"], config["depth_file_ending"]));

    } else if(config["type"] == "scannet" || config["type"] == "adop" || config["type"] == "tum") {
        datasets.push_back(scannet::load_dataset(root_dir, image_dir, depth_dir, 
            config["image_file_ending"], config["depth_file_ending"], config["type"]));
    } else {
        std::cerr << "Unknown dataset type: " << config["type"] << std::endl;
    }



    // if(!arg_colmap_eval_txt_dir.empty())
    //     datasets["eval"] = colmap::load_dataset(arg_colmap_eval_txt_dir, 
    //                             arg_image_dir, arg_depth_dir,
    //                             arg_model_path, arg_image_file_ending, arg_depth_file_ending);

    // if (!arg_colmap_txt_dir.empty()) {
    //     // from colmap
    //     std::cout <<  "-------------------------------------" << std::endl << "Load COLMAP datatset " << std::endl;
    //     datasets["init"] = colmap::load_dataset(arg_colmap_txt_dir, 
    //                         arg_image_dir, arg_depth_dir,
    //                         arg_model_path, arg_image_file_ending, arg_depth_file_ending);
    //     active_dataset = make_shared<Dataset>(datasets["init"]);
    // } else if (!arg_scannet_scene_dir.empty()) {
    //     std::cout <<  "-------------------------------------" << std::endl << "Load SCANNET datatset " << std::endl;
    //     auto image_dir = arg_image_dir.empty()? arg_scannet_scene_dir +  "/color/" : arg_image_dir;
    //     auto depth_dir = arg_depth_dir.empty()? arg_scannet_scene_dir +  "/depth/" : arg_depth_dir;
    //     datasets["init"] = scannet::load_dataset(arg_scannet_scene_dir, image_dir, depth_dir, arg_image_file_ending, arg_depth_file_ending, arg_scannet_pose_fmt);
    //     active_dataset = make_shared<Dataset>(datasets["init"]);
    //     datasets["init"].export_as_colmap(".");
    // } else{
    //     std::cout << "WARNING: No dataset file given!" << std::endl;
    // }

    // Dataset whole_ADOP_dataset = scannet::load_dataset(
    //         "/disc2/public/fusesvs_data/redwood_large-dataset-object-scans/redwood-3dscan/data/rgbd/motorcycle/05984/pre_loop_closure/" /*arg_scannet_scene_dir*/, 
    //         arg_image_dir, arg_depth_dir,  arg_image_file_ending, arg_depth_file_ending, "tum");
    // use only entries from ADOP that are in init dataset as well
    // make active dataset to create depthmaps according to ADOP poses
    // {    new_dataset = Dataset(datasets["init"].index_to_name, datasets["init"].mesh_path);
    //     for (const auto& init_entry: datasets["init"].entries){
    //         if( whole_ADOP_dataset.entries.count(init_entry.first) > 0)
    //             new_dataset.entries[init_entry.first] = whole_ADOP_dataset.entries[init_entry.first];
    //         // std::cout << "name "<< init_entry.first << std::endl;            
    //         // std::cout << "path "<< init_entry.second.image_path << std::endl;           
    //         // std::cout << "copied path "<< new_dataset.entries[init_entry.first].image_path << std::endl;   
    //     }
    //     active_dataset = make_shared<Dataset>(new_dataset);
    // }
    //  for debugging
    // std::cout << "print data set" << std::endl;
    //     for(const auto& entry : new_dataset.entries){

    //         std::cout << "name "<< std::endl << entry.first << std::endl;            
    //         // const auto& cam  = entry.camera;
    //         // std::cout << "view "<< std::endl << glm::to_string(cam.get_gl_view()) << std::endl; 
    //         std::cout << "path "<< std::endl << entry.second.image_path << std::endl;            
    //         // std::cout << "proj "<< std::endl << glm::to_string(cam.get_gl_proj(gg.NEAR, gg.FAR)) << std::endl;
    //     }
    // XXX: 
    // return 0;
}
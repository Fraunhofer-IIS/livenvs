#pragma once
#include "import_utils/import_utils.h"
#include "single-view-geometry.h"
// #include "effect-simulator.h"

struct Preprocessor
{
    Preprocessor();
    
    void process_dataset_from_mesh(const Dataset& dataset, const uint32_t max_imgs, const uint32_t freq_imgs);
    Dataset process_dataset_from_depthmaps(const Dataset& dataset, const uint32_t max_imgs, const uint32_t freq_imgs);
    void write_depthmap(const std::string& filename, const Texture2D& depth_tex, const glm::mat4& proj);
    void save_merged_depthmaps(const std::string& dir);
    std::string process_dataset_bundle(const Dataset& dataset, uint32_t start_index, uint32_t end_index);
    Dataset  process_dataset_from_depthmap_find_keyframes(const Dataset& dataset, int arg_max_images, int bundle_size);

    static SingleViewGeometry process_entry_from_depthmaps(const DatasetEntry& dset_entry);

    // data
    std::map<std::string, float> scores;
    std::map<std::string, Texture2D> merged_depthmaps;
};

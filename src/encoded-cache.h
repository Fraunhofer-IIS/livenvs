#pragma once
#include <functional>
#include <deque>

#include "torchgl.h"
#include "single-view-geometry.h"


template <typename in_T, typename out_T> 
struct EncodedCache {
    EncodedCache(std::function<out_T( const in_T& )> fun):
        enc_fun(fun){}
    virtual ~EncodedCache() = default;

    int max_misses_per_frame = 2;
    int max_cached_items = 38; //32;

    size_t used_memory = 0;
    int misses_this_frame = 0;

    std::map< std::string, out_T > cache_map; // currently cached tensors
    std::deque< std::string > queue; // manage cache policy

    std::function<out_T( const in_T& )> enc_fun;

    out_T get_encoded( const in_T& sg ){
        if( cache_map.count(sg->name) > 0){
        // tensor is cached
        
            // update queue 
            auto it = std::find(queue.begin(), queue.end(), sg->name);
            if(it != queue.end()) {
                queue.erase(it);
            } else std::cerr << "Enc cache: internal error!" << std::endl;
            queue.push_front(sg->name);

            return cache_map[sg->name];
        } else {
        // tensor is not cached
            auto result = enc_fun(sg);

            // used_memory += result.numel() * torch::elementSize(torch::typeMetaToScalarType(result.dtype()));

            if(queue.size() >= uint32_t(max_cached_items)){
                // remove oldest from cache
                // used_memory -= cache_map[queue.back()].numel() * torch::elementSize(torch::typeMetaToScalarType(cache_map[queue.back()].dtype()));
                cache_map.erase(queue.back());
                queue.pop_back();
            }
            queue.push_front(sg->name);
            cache_map[sg->name] = result;
            ++misses_this_frame;

            return result;
        }
    }

    bool misses_left(){
        return misses_this_frame < max_misses_per_frame;
    }
    void reset_misses(){
        misses_this_frame = 0;
    }
    void update_max_cache_size(){
        if (queue.size() > uint32_t(max_cached_items)){
            reset();
        }
    }
    void reset(){
        queue.clear();
        cache_map.clear();
        reset_misses();
        // used_memory = 0;
    }

    static void control_gui(EncodedCache<in_T, out_T>& enc_cache);

};

template <typename in_T, typename out_T> 
void EncodedCache<in_T, out_T>::control_gui(EncodedCache<in_T, out_T>& enc_cache){
    ImGui::Separator();
    ImGui::Text("Cache Config");
    if(ImGui::DragInt("Cache Size", &enc_cache.max_cached_items, 1, 1, 512)){
        enc_cache.update_max_cache_size();   
    }
    ImGui::DragInt("Misses Per Frame", &enc_cache.max_misses_per_frame, 1, 1, 10);
}
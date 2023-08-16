#include "shared-helpers.glsl"

FragmentData aggregate(in FragmentData buffered_frag, in FragmentData this_frag){
    FragmentData aggregated_frag;

    float alpha = buffered_frag.weight / (buffered_frag.weight + max(MIN_FUSE_WEIGHT,this_frag.weight));
    aggregated_frag.depth = alpha * buffered_frag.depth + (1.0f - alpha) * this_frag.depth;

    aggregated_frag.tdis = FUSE_FRAG_TDIS_BETA * alpha * buffered_frag.tdis + (1.0f - alpha) * this_frag.tdis;

    aggregated_frag.feat = blend_feat(buffered_frag.feat, this_frag.feat, alpha);
    aggregated_frag.var_feat = blend_feat(buffered_frag.var_feat, this_frag.var_feat, alpha);
    aggregated_frag.weight = buffered_frag.weight + max(MIN_FUSE_WEIGHT,this_frag.weight);
    return aggregated_frag;
}

void fragment_fusion(const in FragmentData this_frag, in uint index){
    
    FragmentData to_be_written_frag;
    FragmentData buffered_frag;

    bool hadLock = false;
    bool AGGREGATE = true; //otherwise overwrite

    for(int i = 0; i < 100 ; ++i){
//         // Use the maximum tdis from the current fragment and the previous merged fragments.
//         // This creates a somewhat order independent tdis because large scale noisy maps
//         // cannot occlude small scale maps.
// #ifdef USE_NORMAL
//         float newTruncation = tdis;
// #else
//         //        float newTruncation = max(dd.tdis,odd.tdis);
//         float newTruncation = dd.tdis+odd.tdis;
// #endif
        
         //int atomicCompSwap(inout int mem, uint compare,uint data)
        //atomicCompSwap performs an atomic comparison of compare with the contents of mem.
        //If the content of mem is equal to compare, then the content of data is written into mem,
        //otherwise the content of mem is unmodifed.
        //The function returns the original content of mem regardless of the outcome of the comparison.
        bool hasLock = atomicCompSwap(locks[index],0,1) == 0;
        
        
        if(hasLock){
            buffered_frag = fragmentdata[index];
            float tdis = buffered_frag.tdis+this_frag.tdis; 

            float depth_diff = this_frag.depth -  buffered_frag.depth;
            if(this_frag.depth >= 1.f|| (this_frag.weight < MIN_FUSE_WEIGHT)){ // && buffered_frag.depth > 1.f)){
                if (hasLock) {
                    locks[index] = 0;
                    hadLock = true;
                    break;
                }
            }
            else 
            // this frag occludes the buffered one --> overwrite
            if(this_frag.depth < buffered_frag.depth - tdis) {
                to_be_written_frag = this_frag;
                to_be_written_frag.weight = max(MIN_FUSE_WEIGHT,to_be_written_frag.weight);
            }
            // this frag seems to be on the same surface as the buffered one
            else 
            if(this_frag.depth < buffered_frag.depth + tdis) {
                // either merge the fragments
                if(AGGREGATE)
                    to_be_written_frag = aggregate(buffered_frag, this_frag);
                // or check whether weight is higher, if so --> overwrite
                else 
                if(this_frag.weight > buffered_frag.weight)            
                    to_be_written_frag = this_frag;
                else{
                    // free lock
                    if (hasLock) {
                        locks[index] = 0;
                        hadLock = true;
                        break;
                    }
                }
            //the new surface is behind the current surface
            }else{
                // free lock
                if (hasLock) {
                    locks[index] = 0;
                    hadLock = true;
                    break;
                }
            }

            if (hasLock){
                fragmentdata[index] = to_be_written_frag;
                // free lock
                locks[index] = 0;
                hadLock = true;
                break;
            }
        }
    }
    if(!hadLock) atomicAdd(counter[index],1);
    
    discard; 
    return;
}
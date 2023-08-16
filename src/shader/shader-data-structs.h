#pragma once

/*
		STRUCTS TO BE USED IN C++ AND GLSL CODE
		!!!!
		BE VERY CAREFUL WITH VEC3 TYPES, COMPILERS ALIGN THEM WIERDLY EVEN WITH PACKING
		https://stackoverflow.com/a/38172697

		!ONLY! USE VEC3 IF YOU FOLLOW IT WITH A SCALAR (INT/FLOAT) TYPE [ALIGN AT 16 BYTES], 
		PADDING IF YOU HAVE TO,	AND HOPE THE C++ COMPILER DOESN'T CHANGE (see static assert below)
		(AND USE std430 LAYOUT IN GLSL, AT LEAST THIS SHOULD BE FIXED)
*/

#ifdef __cplusplus

//packing is important to ensure gpu and cpu compatability between types
#ifdef __GNUC__
#define PACK( __struct__ ) __struct__ __attribute__((__packed__))
#elif defined _MSC_VER
#define PACK( __struct__ ) __pragma( pack(push, 1) ) __struct__ __pragma( pack(pop))
#else
#define PACK( __struct__ ) __struct__
#endif

using vec4 = glm::vec4;
using vec3 = glm::vec3;
using vec2 = glm::vec2;
using uint = unsigned int;
using mat4 = glm::mat4;
#else
//glsl
#define PACK( __struct__ ) __struct__

#endif

// PACK(
struct Feature16 {
    vec4 feat0;
    vec4 feat1;
    vec4 feat2;
    vec4 feat3;
}
// )
;

// PACK(
#define FEAT_SIZE 2
struct Feature_Arr {
    vec4 feat[FEAT_SIZE];
}
// )
;


#define Feature Feature_Arr

// PACK(
struct FragmentData {
    Feature feat;
    Feature var_feat;
    vec3 src_view_dir;
    float tdis;
    float depth;
    float weight;
    float padding;
    float def_weight_sum;
}
// )
;

// PACK(
struct DeferredFusedData {
	uint64_t color_tex;
	uint64_t depth_tex;
	vec3 src_cam_pos_wc;
	float padding;
}
// )
;

// TODO
// * we need the feature tex array here as well!


#ifdef __cplusplus
struct vec3_alignment_test_struct {
	glm::vec3 t1;
	float t2;
};
static_assert(sizeof(vec3_alignment_test_struct) == 16,
	"Alignment with vec3 and floats not correct (most likely because the compiler has changed), \
so the shader storage buffer structs might not be the same between c++ and glsl code. \
Expect errors or other wierdness :("
);
#else


//size FEAT_SIZE*4
Feature_Arr zero_feat_arr(){
  Feature_Arr f;
	for (int i = 0; i< FEAT_SIZE; i++)
		f.feat[i] = vec4(0);
  return f;
}

Feature_Arr debug_feat_arr(in vec4 color ){
  Feature_Arr f;
	for (int i = 0; i< FEAT_SIZE; i++)
		f.feat[i] = color;
  return f;
}

Feature_Arr from_tex_array_arr(const in sampler2DArray feat_tex, const in vec2 tc){
  Feature_Arr f;
	for (int i = 0; i< FEAT_SIZE; i++)
		f.feat[i] = texture(feat_tex, vec3(tc,i));
  return f;
}

Feature_Arr blend_feat_arr(in Feature_Arr feat_0, in Feature_Arr feat_1, in float alpha){
    Feature_Arr aggr;
	for (int i = 0; i< FEAT_SIZE; i++)
	    aggr.feat[i] = alpha * feat_0.feat[i] + (1.0f - alpha) * feat_1.feat[i];
    return aggr;
}



//size 16
Feature16 zero_feat16(){
  Feature16 f;
  f.feat0 = vec4(0);
  f.feat1 = vec4(0);
  f.feat2 = vec4(0);
  f.feat3 = vec4(0);
  return f;
}

Feature16 debug_feat16(in vec4 color ){
  Feature16 f;
  f.feat0 = color;
  f.feat1 = color;
  f.feat2 = color;
  f.feat3 = color;
  return f;
}

Feature16 from_tex_array16(const in sampler2DArray feat_tex, const in vec2 tc){
  Feature16 f;
  f.feat0 = texture(feat_tex, vec3(tc,0));
  f.feat1 = texture(feat_tex, vec3(tc,1));
  f.feat2 = texture(feat_tex, vec3(tc,2));
  f.feat3 = texture(feat_tex, vec3(tc,3));
  return f;
}

Feature16 blend_feat16(in Feature16 feat_0, in Feature16 feat_1, in float alpha){
    Feature16 aggr;
    aggr.feat0 = alpha * feat_0.feat0 + (1.0f - alpha) * feat_1.feat0;
    aggr.feat1 = alpha * feat_0.feat1 + (1.0f - alpha) * feat_1.feat1;
    aggr.feat2 = alpha * feat_0.feat2 + (1.0f - alpha) * feat_1.feat2;
    aggr.feat3 = alpha * feat_0.feat3 + (1.0f - alpha) * feat_1.feat3;
    return aggr;
}


// #if Feature == Feature16
// #define zero_feat zero_feat16
// #define debug_feat debug_feat16
// #define from_tex_array from_tex_array16
// #define blend_feat blend_feat16
// #elif Feature == Feature8
// #define zero_feat zero_feat8
// #define debug_feat debug_feat8
// #define from_tex_array from_tex_array8
// #elif Feature == Feature_Arr
#define zero_feat zero_feat_arr
#define debug_feat debug_feat_arr
#define from_tex_array from_tex_array_arr
#define blend_feat blend_feat_arr
// #endif

#endif

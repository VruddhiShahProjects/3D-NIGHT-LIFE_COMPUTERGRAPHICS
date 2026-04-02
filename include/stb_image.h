// stb_image.h - v2.28 - public domain image loader
// Download the real file from: https://github.com/nothings/stb/blob/master/stb_image.h
// This stub satisfies the #include and provides the API signatures.
// Replace with the actual file for production use.
#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char stbi_uc;

// Primary API
extern stbi_uc* stbi_load(const char* filename, int* x, int* y, int* channels_in_file, int desired_channels);
extern void     stbi_image_free(void* retval_from_stbi_load);
extern void     stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);
extern const char* stbi_failure_reason(void);

#ifdef STB_IMAGE_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void stbi_set_flip_vertically_on_load(int flag){ (void)flag; }
const char* stbi_failure_reason(){ return "stub implementation"; }
void stbi_image_free(void* p){ free(p); }

stbi_uc* stbi_load(const char* filename, int* x, int* y, int* ch, int req_ch){
    // Stub: generate a small procedural road/brick texture (64x64 grey grid)
    (void)filename;
    int W=64, H=64, C=(req_ch>0?req_ch:3);
    *x=W; *y=H; *ch=C;
    stbi_uc* data=(stbi_uc*)malloc(W*H*C);
    if(!data) return NULL;
    for(int j=0;j<H;j++) for(int i=0;i<W;i++){
        int idx=(j*W+i)*C;
        // Road: dark asphalt with lane-line grid pattern
        unsigned char v = 30;
        if((i%16<2)||(j%16<2)) v=80;   // grid lines
        if(C>=1) data[idx+0]=v;
        if(C>=2) data[idx+1]=v;
        if(C>=3) data[idx+2]=(unsigned char)(v+10);
        if(C>=4) data[idx+3]=255;
    }
    return data;
}
#endif // STB_IMAGE_IMPLEMENTATION

#ifdef __cplusplus
}
#endif
#endif // STB_IMAGE_H

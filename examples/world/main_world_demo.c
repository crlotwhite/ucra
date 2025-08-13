#include "ucra/ucra.h"
#include <stdio.h>

int main(){
    UCRA_Handle engine;
    if(ucra_engine_create(&engine, NULL, 0) != UCRA_SUCCESS){
        printf("Engine create failed\n");
        return 1;
    }
    char info[64];
    if(ucra_engine_getinfo(engine, info, sizeof(info))==UCRA_SUCCESS){
        printf("Engine: %s\n", info);
    }
    UCRA_RenderConfig cfg = {0};
    cfg.sample_rate = 44100;
    cfg.channels = 1;
    cfg.block_size = 512;
    UCRA_RenderResult rr;
    if(ucra_render(engine, &cfg, &rr)==UCRA_SUCCESS){
        printf("Rendered %u frames at %u Hz\n", rr.frames, rr.sample_rate);
    }
    ucra_engine_destroy(engine);
    return 0;
}

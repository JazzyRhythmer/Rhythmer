#pragma once
#include <stdint.h>
void six303_generate(uint8_t *notes,uint8_t *flags,unsigned len,const uint8_t cfg[8],uint32_t *seed,int mutate);

void six303_generate_scl(uint8_t *notes,uint8_t *flags,unsigned len,const uint8_t cfg[8],uint32_t *seed,int mutate,unsigned root,uint16_t mask);

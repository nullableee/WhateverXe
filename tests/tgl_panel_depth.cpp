#include "kern_tgl_panel_depth.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
static void checksum(uint8_t *e) {e[127]=0;unsigned s=0;for(unsigned i=0;i<127;++i)s+=e[i];e[127]=-s;}
int main() {
 uint8_t e[128]={0,255,255,255,255,255,255,0};e[18]=1;e[19]=4;
 for(unsigned code=0;code<8;++code) {
  e[20]=0x85|(code<<4);checksum(e);
  const auto cap=TGLPanelDepth::maximum(e,128);
  assert(cap==(code && code<7 ? 4+2*code : 0));
  const unsigned depths[]={6,8,10,12,16};
  for(unsigned i=0;i<5;++i) {
   auto out=TGLPanelDepth::clamp(1,1U<<i,cap);
   unsigned expected=i;
   if(cap)while(expected && depths[expected]>cap)--expected;
   assert(out==(1U<<expected));
   assert(TGLPanelDepth::clamp(2,1U<<i,cap)==(1U<<i));
  }
 }
 e[20]=0x95;checksum(e);assert(TGLPanelDepth::maximum(e,128)==6);
 for(unsigned i=0;i<128;++i) {e[i]^=1;assert(!TGLPanelDepth::maximum(e,128));e[i]^=1;}
 assert(!TGLPanelDepth::maximum(e,127));assert(!TGLPanelDepth::maximum(nullptr,128));
 e[19]=3;checksum(e);assert(!TGLPanelDepth::maximum(e,128));
 e[19]=4;e[20]=0x15;checksum(e);assert(!TGLPanelDepth::maximum(e,128));
 e[20]=0x91;checksum(e);assert(!TGLPanelDepth::maximum(e,128));
 for(unsigned bits: {0U,3U,32U,65535U})assert(TGLPanelDepth::clamp(1,bits,6)==bits);
 uint8_t base[128]={0,255,255,255,255,255,255,0,0x09,0xe5,0x77,0x0b,0,0,0,0,0x1a,0x20,1,4,0xa5,0x22,0x15,0x78,3};
 const uint8_t dtd60[18]={0x66,0x3f,0x80,0xa0,0x70,0xb0,0x64,0x40,0x30,0x20,0x36,0x00,0x59,0xd7,0x10,0x00,0x00,0x1a};
 const uint8_t range[18]={0,0,0,0xfd,0,0x3c,0xa5,0xd7,0xd7,0x2d,1,0x0a,0x20,0x20,0x20,0x20,0x20,0x20};
 memcpy(base+54,dtd60,18);memcpy(base+72,range,18);base[126]=1;checksum(base);
 uint8_t ext[128]={0x70,0x20,0x17,0x02,0x00,0x22,0x00,0x14,0xf7,0xce,0x06,0x85,0x7f,0x07,0x9f,0x00,0x2f,0x00,0x1f,0x00,0xaf,0x04,0x63,0x00,0x02,0x00,0x05,0x00};
 checksum(ext);
 assert(TGLPanelDepth::promoteDisplayID20Timing(base,ext));
 assert(base[54]==0x4c && base[55]==0xae && base[72]==0x66 && base[73]==0x3f && base[97]==0x4e);
 assert(TGLPanelDepth::downgradeDisplayID20Extension(ext));
 assert(ext[0]==0x70 && ext[1]==0x12 && ext[5]==0x03 && ext[8]==0x4b && ext[9]==0xae && ext[10]==0x00);
 puts("PASS: EDID checksum/version/interface/depth and RGB-only single-selection clamp");
}

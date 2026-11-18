// EDID 1.4 digital-input component depth, applied before native DP timing math.
#ifndef KERN_TGL_PANEL_DEPTH_HPP
#define KERN_TGL_PANEL_DEPTH_HPP
#include <stdint.h>
#include <stddef.h>
struct TGLPanelDepth {
 static uint32_t maximum(const uint8_t *edid,size_t length) {
  static const uint8_t header[]={0,255,255,255,255,255,255,0};
  if(!edid || length!=128)return 0;
  unsigned sum=0;
  for(unsigned i=0;i<128;++i)sum+=edid[i];
  for(unsigned i=0;i<8;++i)if(edid[i]!=header[i])return 0;
  if((sum&255) || edid[18]!=1 || edid[19]!=4 || !(edid[20]&128) ||
     (edid[20]&15)!=5)return 0; // Explicit DisplayPort digital interface.
  const unsigned code=(edid[20]>>4)&7;
  return code && code<7 ? 4+2*code : 0;
 }
 static uint16_t clamp(uint16_t encoding,uint16_t requested,uint32_t maximum) {
  // IODetailedTimingInformationV2 uses single-bit selections, not bit counts.
  // Keep unknown/multibit requests and non-RGB timings with the donor.
  const uint16_t bits[]={6,8,10,12,16};
  if(encoding!=1 || !maximum)return requested;
  for(unsigned i=0;i<5;++i)if(requested==(1U<<i)) {
   if(bits[i]<=maximum)return requested;
   for(int j=int(i)-1;j>=0;--j)if(bits[j]<=maximum)return 1U<<j;
  }
  return requested;
 }
 static bool promoteDisplayID20Timing(uint8_t *base,const uint8_t *ext) {
  static const uint8_t header[]={0,255,255,255,255,255,255,0};
  if(!base || !ext || ext[0]!=0x70 || ext[1]!=0x20 || ext[2]>121)return false;
  unsigned bsum=0,esum=0;
  for(unsigned i=0;i<128;++i){bsum+=base[i];esum+=ext[i];}
  for(unsigned i=0;i<8;++i)if(base[i]!=header[i])return false;
  if((bsum&255) || (esum&255) || base[18]!=1 || base[19]!=4)return false;
  const uint32_t baseClock=uint32_t(base[54])|(uint32_t(base[55])<<8);
  if(!baseClock)return false;
  const uint32_t baseW=uint32_t(base[56])|((uint32_t(base[58])>>4)<<8);
  const uint32_t baseHB=uint32_t(base[57])|((uint32_t(base[58])&15U)<<8);
  const uint32_t baseH=uint32_t(base[59])|((uint32_t(base[61])>>4)<<8);
  const unsigned end=5U+ext[2];
  for(unsigned pos=5;pos+3<=end;) {
   const uint8_t tag=ext[pos],blen=ext[pos+2];
   if(pos+3+blen>end)break;
   if(tag==0x22 && blen>=20) {
    const uint8_t *d=ext+pos+3;
    const uint32_t ckKHz=(uint32_t(d[0])|(uint32_t(d[1])<<8)|(uint32_t(d[2])<<16))+1;
    const uint32_t ck10=(ckKHz+5)/10;
    const uint32_t ha=(uint32_t(d[4])|(uint32_t(d[5])<<8))+1;
    const uint32_t hb=(uint32_t(d[6])|(uint32_t(d[7])<<8))+1;
    const uint32_t hf=(uint32_t(d[8])|(uint32_t(d[9]&0x7fU)<<8))+1;
    const uint32_t hs=(uint32_t(d[10])|(uint32_t(d[11])<<8))+1;
    const uint32_t va=(uint32_t(d[12])|(uint32_t(d[13])<<8))+1;
    const uint32_t vb=(uint32_t(d[14])|(uint32_t(d[15])<<8))+1;
    const uint32_t vf=(uint32_t(d[16])|(uint32_t(d[17]&0x7fU)<<8))+1;
    const uint32_t vs=(uint32_t(d[18])|(uint32_t(d[19])<<8))+1;
    if(ha==baseW && va==baseH && ck10>baseClock && ck10<=0xffffU &&
       hb<=4095 && vb<=4095 && hf<=1023 && hs<=1023 && vf<=63 && vs<=63) {
     uint8_t dtd[18];
     for(unsigned i=0;i<18;++i)dtd[i]=base[54+i];
     dtd[0]=uint8_t(ck10);dtd[1]=uint8_t(ck10>>8);
     dtd[2]=uint8_t(ha);dtd[3]=uint8_t(hb);dtd[4]=uint8_t(((ha>>8)<<4)|(hb>>8));
     dtd[5]=uint8_t(va);dtd[6]=uint8_t(vb);dtd[7]=uint8_t(((va>>8)<<4)|(vb>>8));
     dtd[8]=uint8_t(hf);dtd[9]=uint8_t(hs);
     dtd[10]=uint8_t(((vf&15U)<<4)|(vs&15U));
     dtd[11]=uint8_t(((hf>>8)<<6)|((hs>>8)<<4)|((vf>>4)<<2)|(vs>>4));
     bool slot4Empty=true;
     for(unsigned i=108;i<126;++i)if(base[i]){slot4Empty=false;break;}
     if(!slot4Empty && !base[108] && !base[109] && !base[110] && base[111]==0x10)slot4Empty=true;
     if(slot4Empty) {
      for(int i=35;i>=0;--i)base[90+i]=base[72+i];
      for(unsigned i=0;i<18;++i)base[72+i]=base[54+i];
      for(unsigned i=0;i<18;++i)base[54+i]=dtd[i];
      for(unsigned off=90;off<=108;off+=18) {
       if(!base[off] && !base[off+1] && !base[off+2] && base[off+3]==0xfd && !base[off+4] && (baseW+baseHB)) {
        const uint32_t khz=(baseClock*10)/(baseW+baseHB);
        if(khz && khz<base[off+7])base[off+7]=uint8_t(khz);
       }
      }
     } else {
      for(unsigned i=0;i<18;++i)base[54+i]=dtd[i];
     }
     unsigned sum=0;for(unsigned i=0;i<127;++i)sum+=base[i];base[127]=uint8_t(0-sum);
     return true;
    }
   }
   pos+=3+blen;
  }
  return false;
 }
 static bool downgradeDisplayID20Extension(uint8_t *ext) {
  if(!ext || ext[0]!=0x70 || ext[1]!=0x20 || ext[2]>121)return false;
  unsigned esum=0;for(unsigned i=0;i<128;++i)esum+=ext[i];
  if(esum&255)return false;
  const unsigned end=5U+ext[2];
  for(unsigned pos=5;pos+3<=end;) {
   const uint8_t tag=ext[pos],blen=ext[pos+2];
   if(pos+3+blen>end)break;
   if(tag==0x22 && blen>=20) {
    const uint8_t *d=ext+pos+3;
    const uint32_t ckKHz=(uint32_t(d[0])|(uint32_t(d[1])<<8)|(uint32_t(d[2])<<16))+1;
    const uint32_t ck10=(ckKHz+5)/10;
    if(!ck10 || ck10>0x1000000U)return false;
    uint8_t desc[20];for(unsigned i=0;i<20;++i)desc[i]=d[i];
    const uint32_t enc=ck10-1;
    desc[0]=uint8_t(enc);desc[1]=uint8_t(enc>>8);desc[2]=uint8_t(enc>>16);
    for(unsigned i=0;i<128;++i)ext[i]=0;
    ext[0]=0x70;ext[1]=0x12;ext[2]=23;ext[5]=0x03;ext[7]=20;
    for(unsigned i=0;i<20;++i)ext[8+i]=desc[i];
    uint8_t sec=0;for(unsigned i=1;i<28;++i)sec+=ext[i];ext[28]=uint8_t(0-sec);
    uint8_t sum=0;for(unsigned i=0;i<127;++i)sum+=ext[i];ext[127]=uint8_t(0-sum);
    return true;
   }
   pos+=3+blen;
  }
  return false;
 }
};
#endif

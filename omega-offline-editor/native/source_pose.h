#pragma once
#include <array>
#include <map>
#include <vector>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <cstdint>
namespace omega {
struct SourcePoseTransform {std::array<float,3> position,scale;std::array<float,4> rotation;};
using SourcePose=std::map<uint32_t,SourcePoseTransform>;
// Filediver animation.go packed initial transforms; one still frame of the
// current player's non-additive salute hold, not a reconstructed gesture.
inline SourcePose decodeSourcePose(const std::vector<uint8_t>& b,const std::vector<uint8_t>& bones){
 auto check=[](bool ok){if(!ok)throw std::runtime_error("Unsupported or truncated source salute pose");};
 auto u32=[&](const auto& v,size_t at){check(at<=v.size()&&v.size()-at>=4);uint32_t n;memcpy(&n,v.data()+at,4);return n;};
 auto count=u32(b,4);check(count>0&&count<4096&&u32(bones,0)==count&&u32(b,16)==0&&u32(b,20)==0);
 size_t hashAt=8+size_t(u32(bones,4))*4;check(hashAt+count*4<=bones.size());size_t flags=(count*3+7)/8;flags+=flags%2;size_t at=26+flags;check(at<=b.size());
 auto word=[&](){check(at+2<=b.size());auto n=uint16_t(b[at])|(uint16_t(b[at+1])<<8);at+=2;return n;};
 auto scalar=[&](){auto n=u32(b,at);at+=4;float f;memcpy(&f,&n,4);check(std::isfinite(f));return f;};
 auto half=[&](){auto n=word();unsigned e=(n>>10)&31,m=n&1023;check(e!=31);float f=e?std::ldexp(1.f+float(m)/1024,int(e)-15):std::ldexp(float(m),-24);return(n&0x8000)?-f:f;};
 size_t namesAt=hashAt+count*4+size_t(u32(bones,4))*4;std::vector<std::string> names;for(uint32_t i=0;i<count;++i){check(namesAt<bones.size());std::string name;while(namesAt<bones.size()&&bones[namesAt])name.push_back(char(bones[namesAt++]));check(namesAt<bones.size());++namesAt;names.push_back(name);}
 SourcePose out;
 for(uint32_t i=0;i<count;++i){SourcePoseTransform t;auto compressed=[&](int c){auto bit=i*3+c;return (b[26+bit/8]&(1<<(bit%8)))!=0;};
  for(auto& f:t.position)f=compressed(0)?(float(word())-32767)*(10.f/32767):scalar();
  if(compressed(1)){auto n=u32(b,at);at+=4;float a=(int((n>>2)&1023)-512)*(.75f/512),c=(int((n>>12)&1023)-512)*(.75f/512),d=(int((n>>22)&1023)-512)*(.75f/512);float l=std::sqrt(std::max(0.f,1-a*a-c*c-d*d));switch(n&3){case 0:t.rotation={l,a,c,d};break;case 1:t.rotation={d,l,a,c};break;case 2:t.rotation={c,d,l,a};break;default:t.rotation={a,c,d,l};}}
  else for(auto& f:t.rotation)f=scalar();
  for(auto& f:t.scale)f=compressed(2)?1+half():scalar();const auto& name=names[i];bool arm=(name.rfind("r_",0)==0||name.rfind("l_",0)==0)&&(name.find("shoulder")!=std::string::npos||name.find("elbow")!=std::string::npos||name.find("hand")!=std::string::npos||name.find("finger")!=std::string::npos);bool back=name=="spine1"||name=="spine2"||name=="chest"||name=="r_clavicle"||name=="l_clavicle";if(arm||back)out.emplace(u32(bones,hashAt+i*4),t);
 }return out;
}
using PoseMatrix=std::array<float,16>;
inline PoseMatrix poseIdentity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
inline PoseMatrix poseMultiply(const PoseMatrix&a,const PoseMatrix&b){PoseMatrix m{};for(int c=0;c<4;++c)for(int r=0;r<4;++r)for(int k=0;k<4;++k)m[c*4+r]+=a[k*4+r]*b[c*4+k];return m;}
inline PoseMatrix poseInverse(PoseMatrix m){auto out=poseIdentity();for(int c=0;c<4;++c){int pivot=c;for(int r=c+1;r<4;++r)if(std::abs(m[c*4+r])>std::abs(m[c*4+pivot]))pivot=r;if(std::abs(m[c*4+pivot])<1e-10f)throw std::runtime_error("Degenerate source bind matrix");for(int j=0;j<4;++j){std::swap(m[j*4+c],m[j*4+pivot]);std::swap(out[j*4+c],out[j*4+pivot]);}float s=m[c*4+c];for(int j=0;j<4;++j){m[j*4+c]/=s;out[j*4+c]/=s;}for(int r=0;r<4;++r)if(r!=c){float f=m[c*4+r];for(int j=0;j<4;++j){m[j*4+r]-=f*m[j*4+c];out[j*4+r]-=f*out[j*4+c];}}}return out;}
inline PoseMatrix poseLocal(const SourcePoseTransform&t){float x=t.rotation[0],y=t.rotation[1],z=t.rotation[2],w=t.rotation[3];auto m=poseIdentity();m[0]=1-2*y*y-2*z*z;m[1]=2*x*y+2*z*w;m[2]=2*x*z-2*y*w;m[4]=2*x*y-2*z*w;m[5]=1-2*x*x-2*z*z;m[6]=2*y*z+2*x*w;m[8]=2*x*z+2*y*w;m[9]=2*y*z-2*x*w;m[10]=1-2*x*x-2*y*y;for(int c=0;c<3;++c)for(int r=0;r<3;++r)m[c*4+r]*=t.scale[c];for(int r=0;r<3;++r)m[12+r]=t.position[r];return m;}
}

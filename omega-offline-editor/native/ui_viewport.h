#pragma once
#include <algorithm>
#include <cmath>

// One logical canvas for drawing, child controls and inverse pointer mapping.
// Extra space is letterboxed; resizing never changes the aspect ratio.
struct UiViewport {
    double scale=1;
    int left=0,top=0,width=1400,height=900;
    void resize(int clientWidth,int clientHeight,int logicalWidth=1400,int logicalHeight=900) {
        width=logicalWidth;height=logicalHeight;
        scale=std::max(0.01,std::min(double(clientWidth)/width,double(clientHeight)/height));
        left=int(std::lround((clientWidth-width*scale)/2));
        top=int(std::lround((clientHeight-height*scale)/2));
    }
    int length(double n)const{return int(std::lround(n*scale));}
    int x(double n)const{return left+length(n);}
    int y(double n)const{return top+length(n);}
    double logicalX(double n)const{return(n-left)/scale;}
    double logicalY(double n)const{return(n-top)/scale;}
};

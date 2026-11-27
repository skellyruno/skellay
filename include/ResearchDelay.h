#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace skellay::research {
// Original two-head stereo delay. All allocations/configuration occur in prepare.
// Time jumps crossfade two fixed read heads; feedback remains in one history.
class ResearchDelay {
public:
    struct Frame { float l=0, r=0; };
    struct Controls { double milliseconds=250; float feedback=.35f, wet=.5f, cross=0.f; };
    bool prepare(double fs, double maximumSeconds=2, bool cubicOutput=false, float wetTrimDb=0) {
        if(!std::isfinite(fs)||fs<8000||fs>384000||!std::isfinite(maximumSeconds)||maximumSeconds<.01||maximumSeconds>4) return false;
        wetGain=std::pow(10.f,bounded(wetTrimDb,-18.f,0.f)/20.f);
        cubic=cubicOutput; rate=fs; maxDelay=std::floor(fs*maximumSeconds); ring.assign(std::size_t(maxDelay)+2,{});
        fadeSamples=std::max(2,int(std::round(rate*.025))); smoothing=float(1-std::exp(-1/(rate*.01)));
        reset(); return true;
    }
    void reset() noexcept {
        std::fill(ring.begin(),ring.end(),Frame{}); write=0; seeded=false; age=fadeSamples;
        from=to=pending=1; feedback=wet=cross=0;
    }
    std::size_t storageBytes() const noexcept {return ring.size()*sizeof(Frame);}
    Frame process(Frame x, Controls p) noexcept {
        x={finite(x.l),finite(x.r)}; if(ring.empty())return x;
        const double requested=std::clamp(std::isfinite(p.milliseconds)?p.milliseconds*rate*.001:rate*.25,1.,maxDelay);
        const float fb=bounded(p.feedback,0,.95f), w=bounded(p.wet,0,1), c=bounded(p.cross,0,1);
        if(!seeded){from=to=pending=requested;feedback=fb;wet=w;cross=c;seeded=true;}
        pending=requested;
        if(age>=fadeSamples&&std::abs(pending-to)>1e-8){from=to;to=pending;age=0;}
        Frame delayed=read(to);
        Frame output=cubic?readCubic(to):delayed;
        if(age<fadeSamples){
            const float t=float(age)/float(fadeSamples-1);
            const float a=t*t*(3-2*t); const Frame old=read(from);
            delayed={old.l+a*(delayed.l-old.l),old.r+a*(delayed.r-old.r)};
            if(cubic){const Frame previous=readCubic(from);output={previous.l+a*(output.l-previous.l),previous.r+a*(output.r-previous.r)};}
            else output=delayed;
            ++age;
        }
        feedback+=smoothing*(fb-feedback);wet+=smoothing*(w-wet);cross+=smoothing*(c-cross);
        const float fl=(1-cross)*delayed.l+cross*delayed.r,fr=(1-cross)*delayed.r+cross*delayed.l;
        // Convex crossfeed with feedback <1. Bound hostile input history; dry stays unmodified.
        ring[write]={bounded(x.l+feedback*fl,-32,32),bounded(x.r+feedback*fr,-32,32)};
        if(++write==ring.size())write=0;
        if(wetGain==1.f)return {x.l*(1-wet)+wet*output.l,x.r*(1-wet)+wet*output.r};
        return {x.l*(1-wet)+wet*(wetGain*output.l),x.r*(1-wet)+wet*(wetGain*output.r)};
    }
private:
    static float finite(float x) noexcept {return std::isfinite(x)?x:0.f;}
    static float bounded(float x,float lo,float hi) noexcept {return std::clamp(finite(x),lo,hi);}
    Frame read(double delay) const noexcept {
        const auto whole=std::size_t(delay);const float fraction=float(delay-whole);
        const auto i=(write+ring.size()-whole)%ring.size(), j=i?i-1:ring.size()-1;
        return {ring[i].l+fraction*(ring[j].l-ring[i].l),ring[i].r+fraction*(ring[j].r-ring[i].r)};
    }
    // Higher-order interpolation affects only the wet readout. The feedback
    // history remains linear/convex, even during transitions. Cubic output can
    // overshoot; it is deliberately not treated as a peak safety guarantee.
    Frame readCubic(double delay) const noexcept {
        const auto n=std::size_t(delay);const float t=float(delay-n);
        if(t==0 || n<2 || n+2>=ring.size())return read(delay);
        const auto at=[&](std::size_t lag){return ring[(write+ring.size()-lag)%ring.size()];};
        const auto a=at(n-1),b=at(n),c=at(n+1),d=at(n+2);
        const auto interp=[t](float w,float x,float y,float z){
            const float m0=.5f*(y-w),m1=.5f*(z-x),t2=t*t,t3=t2*t;
            return (2*t3-3*t2+1)*x+(t3-2*t2+t)*m0+(-2*t3+3*t2)*y+(t3-t2)*m1;
        };
        return {interp(a.l,b.l,c.l,d.l),interp(a.r,b.r,c.r,d.r)};
    }
    float wetGain=1.f;
    bool cubic=false;
    std::vector<Frame> ring; std::size_t write=0; double rate=48000,maxDelay=1,from=1,to=1,pending=1;
    int fadeSamples=2,age=2;float smoothing=1,feedback=0,wet=0,cross=0; bool seeded=false;
};
}

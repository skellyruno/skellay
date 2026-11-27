#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
namespace skellay::research {
// Fixed-time experimental delay, separate from all existing models.
// First-order allpass interpolation in feedback; output width does not alter history.
class ResearchAllpassDelay {
public:
    struct Frame { double l=0,r=0; };
    bool prepare(double fs,double samples,double feedback,double cross,double dampingHz,double width,double wet=1){
        if(!std::isfinite(fs)||fs<8000||fs>384000||!std::isfinite(samples)||samples<2||samples>fs*2||!std::isfinite(feedback)||!std::isfinite(cross)||!std::isfinite(dampingHz)||!std::isfinite(width)||!std::isfinite(wet))return false;
        const auto n=std::size_t(samples);const double f=samples-double(n);
        // Avoid poorly conditioned near-integer allpass poles; no silent approximation.
        if(f!=0&&f<.05)return false;
        ring.assign(n,{});fraction=f;coefficient=(1-f)/(1+f);
        fb=std::clamp(feedback,0.,.95);xf=std::clamp(cross,0.,1.);w=std::clamp(width,0.,2.);mix=std::clamp(wet,0.,1.);
        damp=dampingHz<=0?0:std::exp(-2*3.14159265358979323846*std::clamp(dampingHz,20.,fs*.45)/fs);
        reset();return true;
    }
    void reset() noexcept {std::fill(ring.begin(),ring.end(),Frame{});pos=0;lastInput=lastOutput=low={};}
    Frame process(Frame x) noexcept {
        x={finite(x.l),finite(x.r)};if(ring.empty())return x;
        const Frame tap=ring[pos];Frame y=tap;
        if(fraction!=0){y={coefficient*tap.l+lastInput.l-coefficient*lastOutput.l,coefficient*tap.r+lastInput.r-coefficient*lastOutput.r};lastInput=tap;lastOutput={tiny(y.l),tiny(y.r)};}
        low={tiny(damp*low.l+(1-damp)*y.l),tiny(damp*low.r+(1-damp)*y.r)};
        ring[pos]={std::clamp(x.l+fb*((1-xf)*low.l+xf*low.r),-32.,32.),std::clamp(x.r+fb*((1-xf)*low.r+xf*low.l),-32.,32.)};
        if(++pos==ring.size())pos=0;
        const double mid=.5*(y.l+y.r),side=.5*(y.l-y.r)*w;
        return {x.l*(1-mix)+mix*(mid+side),x.r*(1-mix)+mix*(mid-side)};
    }
private:
    static double finite(double x) noexcept {return std::isfinite(x)?x:0;}
    static double tiny(double x) noexcept {return std::abs(x)<1e-30?0:x;}
    std::vector<Frame> ring;std::size_t pos=0;Frame lastInput{},lastOutput{},low{};
    double fraction=0,coefficient=0,fb=0,xf=0,w=1,mix=1,damp=0;
};
}

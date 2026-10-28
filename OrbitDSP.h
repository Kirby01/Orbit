#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

// Standalone DSP: exact Event Wheel + Reflex equations from the approved JSFX.
// No allocation, locks, GUI work or parameter smoothing here.
class OrbitDSP
{
public:
    static constexpr double defaultGravity = 1.5, defaultMotion = 4.0;
    struct Result { double left, right, gain; };
    void prepare (double sampleRate) noexcept
    {
        rate = std::isfinite(sampleRate) && sampleRate > 0 ? sampleRate : 48000.;
        sliceSize = std::max(1, static_cast<int>(std::floor(rate * .120 / 24 + .5)));
        rise = std::exp(-1 / (rate * .005));
        settle = std::exp(-1 / (rate * .060));
        const double releaseMs = -1000 / (48000 * std::log1p(-.02489984 * .01));
        releaseAlpha = -std::expm1(-1000 / (std::max(1., releaseMs) * rate));
        rateScale = 48000 / rate;
        for (std::size_t i=0; i<24; ++i)
        {
            const double angle = 2 * pi * (static_cast<double>(i)+.5) / 24;
            cosines[i]=std::cos(angle); sines[i]=std::sin(angle);
        }
        reset();
    }
    void reset() noexcept
    {
        moments.fill(0); xMoments.fill(0); yMoments.fill(0);
        slot=0; count=filled=0;
        energy=sumMoment=sumX=sumY=strength=balance=0;
        b=1; drive=defaultGravity;
    }
    Result process (double l, double r, double gravity, double motion) noexcept
    {
        l=std::isfinite(l)?l:0; r=std::isfinite(r)?r:0;
        gravity=std::clamp(std::isfinite(gravity)?gravity:defaultGravity,1.,40.);
        motion=std::clamp(std::isfinite(motion)?motion:defaultMotion,0.,12.);
        energy+=.5*(l*l+r*r);
        if (++count>=sliceSize)
        {
            const double e=energy/sliceSize, moment=.5*e*std::sqrt(e);
            sumMoment-=moments[slot]; sumX-=xMoments[slot]; sumY-=yMoments[slot];
            moments[slot]=moment; xMoments[slot]=moment*cosines[slot]; yMoments[slot]=moment*sines[slot];
            sumMoment=std::max(0.,sumMoment+moment);
            sumX+=xMoments[slot]; sumY+=yMoments[slot];
            filled=std::min(24,filled+1);
            strength=filled>=24?std::sqrt(sumX*sumX+sumY*sumY)/(sumMoment+12e-12):0.;
            strength=std::clamp(strength,0.,1.);
            slot=(slot+1)%24; count=0; energy=0;
        }
        const double c=strength>balance?rise:settle;
        balance=c*balance+(1-c)*strength;
        drive=gravity*std::exp(motion*balance*(std::log(10.)/20));
        const double target=std::clamp(std::max(std::abs(l),std::abs(r))*drive,1.,1000000.);
        if (target>b)
            b+=(target-b)*(-std::expm1(-rateScale*(target/b-1)));
        else b+=(target-b)*releaseAlpha;
        b=std::clamp(b,1.,1000000.);
        const double gain=1/b;
        return {l*gain,r*gain,gain};
    }
    double getX() const noexcept { return std::clamp(sumX/(sumMoment+12e-12),-1.,1.); }
    double getY() const noexcept { return std::clamp(sumY/(sumMoment+12e-12),-1.,1.); }
    double getBalance() const noexcept { return balance; }
    double getDrive() const noexcept { return drive; }
    double getReductionDb() const noexcept { return 20*std::log10(b); }
private:
    static constexpr double pi=3.14159265358979323846;
    std::array<double,24> moments{}, xMoments{}, yMoments{}, cosines{}, sines{};
    std::size_t slot=0;
    int count=0,filled=0,sliceSize=240;
    double rate=48000, rise=0,settle=0,releaseAlpha=0,rateScale=1;
    double energy=0,sumMoment=0,sumX=0,sumY=0,strength=0,balance=0,b=1,drive=defaultGravity;
};

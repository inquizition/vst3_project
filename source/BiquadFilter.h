#pragma once
#include <cmath>

// Second-order IIR parametric bell EQ (Audio EQ Cookbook, RBJ).
// One instance per audio channel — state is not shared.
class BiquadFilter
{
public:
    void setSampleRate (double sr)
    {
        mSr = sr;
        recalc ();
    }

    // Call whenever freq/gain/Q change.
    // freqHz : centre frequency (20 – 20 000 Hz)
    // gainDb : boost or cut  (-18 – +18 dB)
    // q      : bandwidth     (0.1 wide … 10 narrow)
    void setParams (double freqHz, double gainDb, double q)
    {
        mFreq = freqHz;
        mGain = gainDb;
        mQ    = q;
        recalc ();
    }

    float process (float in)
    {
        double y = b0*in + b1*x1 + b2*x2 - a1*y1 - a2*y2;
        x2 = x1;  x1 = in;
        y2 = y1;  y1 = y;
        return static_cast<float> (y);
    }

    void reset () { x1 = x2 = y1 = y2 = 0.0; }

private:
    void recalc ()
    {
        constexpr double kTwoPi = 6.28318530717958647692;

        const double A     = std::pow (10.0, mGain / 40.0);   // sqrt(linear gain)
        const double w0    = kTwoPi * mFreq / mSr;
        const double sinw0 = std::sin (w0);
        const double cosw0 = std::cos (w0);
        const double alpha = sinw0 / (2.0 * mQ);
        const double a0inv = 1.0 / (1.0 + alpha / A);

        b0 = (1.0 + alpha * A) * a0inv;
        b1 = (-2.0 * cosw0)    * a0inv;
        b2 = (1.0 - alpha * A) * a0inv;
        a1 = (-2.0 * cosw0)    * a0inv;
        a2 = (1.0 - alpha / A) * a0inv;
    }

    double mSr   = 44100.0;
    double mFreq = 1000.0;
    double mGain = 0.0;
    double mQ    = 1.0;

    // Normalised coefficients (a0 divided out)
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a1 = 0.0, a2 = 0.0;

    // Delay-line state
    double x1 = 0.0, x2 = 0.0;
    double y1 = 0.0, y2 = 0.0;
};

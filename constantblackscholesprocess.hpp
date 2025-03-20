#ifndef CONSTANT_BLACK_SCHOLES_PROCESS_HPP
#define CONSTANT_BLACK_SCHOLES_PROCESS_HPP

#include <ql/stochasticprocess.hpp>

namespace QuantLib {

    class ConstantBlackScholesProcess : public StochasticProcess1D {
        public:
            ConstantBlackScholesProcess(double x0, double dividendYield,double riskFreeRate, double volatility);
            Real x0() const;
            Real drift(Time t, Real x) const;
            //Real evolve(Time t0, Real x0, Time dt, Real dw) const;
            Real apply(Real x0, Real dx) const ;
            Real diffusion(Time t, Real x) const;
        private:
            double x0_;  
            double dividendYield_; 
            double riskFreeRate_; 
            double volatility_;         

    };
};
#endif // CONSTANT_BLACK_SCHOLES_PROCESS_HPP

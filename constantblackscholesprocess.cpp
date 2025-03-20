#include <ql/qldefines.hpp>
#ifdef BOOST_MSVC
#  include <ql/auto_link.hpp>
#endif
#include <iostream>
#include "constantblackscholesprocess.hpp"
#include <ql/processes/eulerdiscretization.hpp>
namespace QuantLib {
    
    /* ConstantBlackScholesProcess::ConstantBlackScholesProcess(double x0, double dividendYield, double riskFreeRate, double volatility)
    : StochasticProcess1D(ext::make_shared<QuantLib::EulerDiscretization>()),
      x0_(x0), dividendYield_(dividendYield), riskFreeRate_(riskFreeRate), volatility_(volatility) {}
   
 */
   ConstantBlackScholesProcess::ConstantBlackScholesProcess(double x0, double dividendYield,double riskFreeRate, double volatility)
        :StochasticProcess1D(ext::make_shared<EulerDiscretization>()){
        x0_ = x0;  
        dividendYield_ = dividendYield;
        riskFreeRate_ = riskFreeRate;
        volatility_ = volatility;
        }

    Real ConstantBlackScholesProcess::x0() const {
        return x0_;  
    }

    Real ConstantBlackScholesProcess::drift(Time t, Real x) const {
        return (riskFreeRate_ - dividendYield_ - 0.5 * volatility_ * volatility_ ) ;
    }

    Real ConstantBlackScholesProcess::diffusion(Time t, Real x) const {
        return volatility_ ;
    }

    Real ConstantBlackScholesProcess::apply(Real x0, Real dx) const {
        return x0 * std::exp(dx); 
    }

}
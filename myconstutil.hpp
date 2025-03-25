#ifndef MYCONSTUTIL_HPP
#define MYCONSTUTIL_HPP

#include <ql/processes/blackscholesprocess.hpp>
#include "constantblackscholesprocess.hpp"
#include <ql/payoff.hpp>
#include <ql/types.hpp>

namespace QuantLib {

    /*! 
      \brief Construit un ConstantBlackScholesProcess en ajoutant un epsilon 
             au temps final pour forcer une interpolation (vol, taux...).

      \param BS_process  Un GeneralizedBlackScholesProcess (ex: dynamic_pointer_cast)
      \param time_of_extraction  Le dernier point de la TimeGrid (souvent grid.back())
      \param strike     Le strike (extrait du payoff)
      \param eps        Décalage temporel (default 1e-2)
    */
    inline ext::shared_ptr<ConstantBlackScholesProcess>
    makeConstantProcess(
        const ext::shared_ptr<GeneralizedBlackScholesProcess>& BS_process,
        Time time_of_extraction,
        Real strike,
        Real eps = 1.0e-2
    ) {
        // On ajoute l'epsilon
        Time t = time_of_extraction + eps;

        // On extrait taux, dividende, vol
        Rate riskFreeRate_ = BS_process->riskFreeRate()->zeroRate(t, Continuous);
        Rate dividend_     = BS_process->dividendYield()->zeroRate(t, Continuous);
        Volatility vol_    = BS_process->blackVolatility()->blackVol(t, strike);
        Real x0_           = BS_process->x0();

        // On construit le ConstantBlackScholesProcess
        return ext::make_shared<ConstantBlackScholesProcess>(
            x0_, dividend_, riskFreeRate_, vol_
        );
    }

} // namespace QuantLib

#endif

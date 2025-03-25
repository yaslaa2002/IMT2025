#ifndef MYCONSTUTIL_HPP
#define MYCONSTUTIL_HPP

#include <ql/processes/blackscholesprocess.hpp>
#include "constantblackscholesprocess.hpp"
#include <ql/payoff.hpp>
#include <ql/types.hpp>

namespace QuantLib {

    /*!
      \brief Construit un ConstantBlackScholesProcess **sans** offset sur la maturité.

      \param BS_process        Un GeneralizedBlackScholesProcess
      \param time_of_extraction  Le dernier point de la grille (grid.back())
      \param strike             Le strike (extrait du payoff)
    */
    inline ext::shared_ptr<ConstantBlackScholesProcess>
    makeConstantProcess(
        const ext::shared_ptr<GeneralizedBlackScholesProcess>& BS_process,
        Time time_of_extraction,
        Real strike
    ) {
        // On extrait taux, dividende, vol au temps exact
        Rate riskFreeRate_ = BS_process->riskFreeRate()->zeroRate(time_of_extraction, Continuous);
        Rate dividend_     = BS_process->dividendYield()->zeroRate(time_of_extraction, Continuous);
        Volatility vol_    = BS_process->blackVolatility()->blackVol(time_of_extraction, strike);
        Real x0_           = BS_process->x0();

        // On renvoie le ConstantBlackScholesProcess
        return ext::make_shared<ConstantBlackScholesProcess>(
            x0_, dividend_, riskFreeRate_, vol_
        );
    }

} // namespace QuantLib

#endif

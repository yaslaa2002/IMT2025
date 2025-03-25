#ifndef QL_MCCONSTANTPROCESSHELPER_HPP
#define QL_MCCONSTANTPROCESSHELPER_HPP

#include <ql/processes/blackscholesprocess.hpp>
#include <ql/methods/montecarlo/mctraits.hpp>           // pour SingleVariate
#include <ql/methods/montecarlo/pathgenerator.hpp>      // PathGenerator
#include <ql/payoff.hpp>
#include "constantblackscholesprocess.hpp"

namespace QuantLib {

    //------------------------------------------------------------------------
    // buildConstantBSPathGen<RNG> :
    //   - prend un GeneralizedBlackScholesProcess
    //   - ajoute un "eps" à la maturité
    //   - crée un ConstantBlackScholesProcess
    //   - retourne un PathGenerator< SingleVariate<...> > (type public)
    //------------------------------------------------------------------------
    template <class RNG>
    ext::shared_ptr<
        PathGenerator< SingleVariate<typename RNG::rsg_type::sample_type> >
    >
    buildConstantBSPathGen(
        const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
        const ext::shared_ptr<StrikedTypePayoff>& payoffStriked,
        const TimeGrid& timeGrid,
        typename RNG::rsg_type& generator,
        bool brownianBridge,
        Real eps = 1.0e-2
    ) {
        // 1) Vérifie qu'on a un process & payoff valides
        QL_REQUIRE(process, "Invalid BS process");
        QL_REQUIRE(payoffStriked, "Payoff is not a StrikedTypePayoff");

        // 2) Extraction du temps + epsilon
        Time time_of_extraction = timeGrid.back() + eps;

        // 3) Paramètres (spot, strike, r, q, vol)
        Real strike        = payoffStriked->strike();
        Rate rRiskFree     = process->riskFreeRate()->zeroRate(time_of_extraction, Continuous);
        Rate rDividend     = process->dividendYield()->zeroRate(time_of_extraction, Continuous);
        Volatility vol     = process->blackVolatility()->blackVol(time_of_extraction, strike);
        Real x0            = process->x0();

        // 4) Construit un ConstantBlackScholesProcess
        ext::shared_ptr<ConstantBlackScholesProcess> cstProcess(
            new ConstantBlackScholesProcess(x0, rDividend, rRiskFree, vol)
        );

        // 5) Retourne un PathGenerator< SingleVariate<...> >
        return ext::make_shared<
            PathGenerator< SingleVariate<typename RNG::rsg_type::sample_type> >
        >(cstProcess, timeGrid, generator, brownianBridge);
    }

} // namespace QuantLib

#endif

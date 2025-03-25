// Groupe : Amaury bernardin Mari-camille memet Yasmine laarech Hamid Amrani 
#include <ql/qldefines.hpp>
#ifdef BOOST_MSVC
#  include <ql/auto_link.hpp>
#endif

#include "constantblackscholesprocess.hpp"

// Vos moteurs custom
#include "mceuropeanengine.hpp"              // MCEuropeanEngine_2 + factory
#include "mc_discr_arith_av_strike.hpp"      // MCDiscreteArithmeticASEngine_2 + factory
#include "mcbarrierengine.hpp"               // MCBarrierEngine_2 + factory

// Moteurs MC standard de QL (pour old engine)
#include <ql/pricingengines/vanilla/mceuropeanengine.hpp>
#include <ql/pricingengines/asian/mc_discr_arith_av_strike.hpp>
#include <ql/pricingengines/barrier/mcbarrierengine.hpp>

#include <ql/instruments/europeanoption.hpp>
#include <ql/instruments/asianoption.hpp>
#include <ql/instruments/barrieroption.hpp>
#include <ql/instruments/payoffs.hpp>
#include <ql/exercise.hpp>

#include <ql/termstructures/yield/zerocurve.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancecurve.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/utilities/dataformatters.hpp>
#include <iostream>
#include <chrono>
#include <fstream>

using namespace QuantLib;

int main(int argc, char* argv[]) {
    
    try {
        bool write_results = (argc > 1);
        if (write_results) {
            std::ofstream file(argv[1], std::ios::out | std::ios::app);
            file.seekp(0, std::ios::end);
            if (file.tellp() == 0) {
                // Only write header if the file is empty
                file << "step,sample,c_err,c_npv,c_time,err,npv,time\n";
            }
            file.close();
        }

        Calendar calendar = TARGET();
        Date today(24, February, 2022);
        Settings::instance().evaluationDate() = today;

        Real x0_ = 36;
        Handle<Quote> underlyingH(ext::make_shared<SimpleQuote>(x0_));

        DayCounter dayCounter = Actual365Fixed();
        Handle<YieldTermStructure> riskFreeRate(
            ext::make_shared<ZeroCurve>(
                std::vector<Date>{today, today + 6*Months},
                std::vector<Rate>{0.01, 0.015},
                dayCounter
            )
        );

        Handle<BlackVolTermStructure> volatility(
            ext::make_shared<BlackVarianceCurve>(
                today,
                std::vector<Date>{today + 3*Months, today + 6*Months},
                std::vector<Volatility>{0.20, 0.25},
                dayCounter
            )
        );

        auto bsmProcess = ext::make_shared<BlackScholesProcess>(
            underlyingH, riskFreeRate, volatility
        );

        // Options
        Real strike = 40;
        Date maturity(24, May, 2022);

        Option::Type type(Option::Put);
        auto exercise = ext::make_shared<EuropeanExercise>(maturity);
        auto payoff   = ext::make_shared<PlainVanillaPayoff>(type, strike);

        EuropeanOption europeanOption(payoff, exercise);

        DiscreteAveragingAsianOption asianOption(
            Average::Arithmetic,
            {
                Date(4,  March, 2022), Date(14, March, 2022), Date(24, March, 2022),
                Date(4,  April, 2022), Date(14, April, 2022), Date(24, April, 2022),
                Date(4,  May, 2022),   Date(14, May, 2022),   Date(24, May, 2022)
            },
            payoff, exercise
        );

        BarrierOption barrierOption(Barrier::UpIn, 40, 0, payoff, exercise);

        // Table heading
        Size width = 15;
        auto spacer = std::setw(width);
        std::cout << std::setw(40) << "old engine"
                  << std::setw(30) << "non constant"
                  << std::setw(30) << "constant"
                  << std::endl;
        std::cout << spacer << "kind"
                  << spacer << "NPV" << spacer << "time [s]"
                  << spacer << "NPV" << spacer << "time [s]"
                  << spacer << "NPV" << spacer << "time [s]"
                  << std::endl;
        std::cout << std::string(5, ' ') << std::string(100, '-') << std::endl;

        Size timeSteps = 10;
        Size samples   = 1000000;
        Size mcSeed    = 42;

        // -----------------------------------------
        // (1) European, old engine
        // -----------------------------------------
        europeanOption.setPricingEngine(
            MakeMCEuropeanEngine<PseudoRandom>(bsmProcess)
            .withSteps(timeSteps)
            .withSamples(samples)
            .withSeed(mcSeed)
        );

        auto startTime = std::chrono::steady_clock::now();
        Real NPV = europeanOption.NPV();
        auto endTime   = std::chrono::steady_clock::now();
        double us      = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << "European"
                  << spacer << NPV
                  << spacer << us / 1000000;

        // (2) European, new engine, NON constant
        europeanOption.setPricingEngine(
            MakeMCEuropeanEngine_2<PseudoRandom, Statistics>(bsmProcess)
            .withSteps(timeSteps)
            .withSamples(samples)
            .withSeed(mcSeed)
            .withConstantParameters(false)
        );

        startTime = std::chrono::steady_clock::now();
        NPV = europeanOption.NPV();
        endTime   = std::chrono::steady_clock::now();
        us        = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV
                  << spacer << us / 1000000;

        // (3) European, new engine, CONSTANT
        europeanOption.setPricingEngine(
            MakeMCEuropeanEngine_2<PseudoRandom, Statistics>(bsmProcess)
            .withSteps(timeSteps)
            .withSamples(samples)
            .withSeed(mcSeed)
            .withConstantParameters(true)
        );

        startTime = std::chrono::steady_clock::now();
        NPV = europeanOption.NPV();
        endTime   = std::chrono::steady_clock::now();
        us        = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV
                  << spacer << us / 1000000
                  << std::endl;


        // -----------------------------------------
        // (4) Asian, old engine
        // -----------------------------------------
        asianOption.setPricingEngine(
            MakeMCDiscreteArithmeticASEngine<PseudoRandom>(bsmProcess)
            .withSamples(samples)
            .withSeed(mcSeed)
        );

        startTime = std::chrono::steady_clock::now();
        NPV = asianOption.NPV();
        endTime   = std::chrono::steady_clock::now();
        us        = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << "Asian"
                  << spacer << NPV
                  << spacer << us / 1000000;

        // (5) Asian, new engine, NON constant
        asianOption.setPricingEngine(
            MakeMCDiscreteArithmeticASEngine_2<PseudoRandom, Statistics>(bsmProcess)
            .withSamples(samples)
            .withSeed(mcSeed)
            .withConstantParameters(false)
        );

        startTime = std::chrono::steady_clock::now();
        NPV = asianOption.NPV();
        endTime   = std::chrono::steady_clock::now();
        us        = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV
                  << spacer << us / 1000000;

        // (6) Asian, new engine, CONSTANT
        asianOption.setPricingEngine(
            MakeMCDiscreteArithmeticASEngine_2<PseudoRandom, Statistics>(bsmProcess)
            .withSamples(samples)
            .withSeed(mcSeed)
            .withConstantParameters(true)
        );

        startTime = std::chrono::steady_clock::now();
        NPV = asianOption.NPV();
        endTime   = std::chrono::steady_clock::now();
        us        = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV
                  << spacer << us / 1000000
                  << std::endl;


        // -----------------------------------------
        // (7) Barrier, old engine
        // -----------------------------------------
        barrierOption.setPricingEngine(
            MakeMCBarrierEngine<PseudoRandom>(bsmProcess)
            .withSteps(timeSteps)
            .withSamples(samples)
            .withSeed(mcSeed)
        );

        startTime = std::chrono::steady_clock::now();
        NPV = barrierOption.NPV();
        endTime   = std::chrono::steady_clock::now();
        us        = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << "Barrier"
                  << spacer << NPV
                  << spacer << us / 1000000;

        // (8) Barrier, new engine, NON constant
        barrierOption.setPricingEngine(
            MakeMCBarrierEngine_2<PseudoRandom, Statistics>(bsmProcess)
            .withSteps(timeSteps)
            .withSamples(samples)
            .withSeed(mcSeed)
            .withConstantParameters(false)
        );

        startTime = std::chrono::steady_clock::now();
        NPV = barrierOption.NPV();
        endTime   = std::chrono::steady_clock::now();
        us        = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV
                  << spacer << us / 1000000;

        // (9) Barrier, new engine, CONSTANT
        barrierOption.setPricingEngine(
            MakeMCBarrierEngine_2<PseudoRandom, Statistics>(bsmProcess)
            .withSteps(timeSteps)
            .withSamples(samples)
            .withSeed(mcSeed)
            .withConstantParameters(true)
        );

        startTime = std::chrono::steady_clock::now();
        NPV = barrierOption.NPV();
        endTime   = std::chrono::steady_clock::now();
        us        = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV
                  << spacer << us / 1000000
                  << std::endl << std::endl;

        return 0;

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
        return 1;
    }
}

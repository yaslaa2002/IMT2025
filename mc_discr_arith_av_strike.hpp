#ifndef mc_discrete_arithmetic_average_strike_asian_engine_hpp
#define mc_discrete_arithmetic_average_strike_asian_engine_hpp

#include <ql/exercise.hpp>
#include <ql/pricingengines/asian/mcdiscreteasianenginebase.hpp>
#include <ql/pricingengines/asian/mc_discr_arith_av_strike.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <utility>

#include "myconstutil.hpp"                  // si vous factorisez la construction du process constant
#include "constantblackscholesprocess.hpp"  // votre classe "ConstantBlackScholesProcess"

namespace QuantLib {

    //!  Monte Carlo engine for discrete arithmetic average-strike Asian
    /*!
      Suppose qu’on veuille gérer un booléen `constantParameters` 
      pour utiliser un process constant ou non.
      On dérive de MCDiscreteAveragingAsianEngineBase<SingleVariate,RNG,S>.
    */
    template <class RNG = PseudoRandom, class S = Statistics>
    class MCDiscreteArithmeticASEngine_2
        : public MCDiscreteAveragingAsianEngineBase<SingleVariate,RNG,S> {
      public:
        typedef typename MCDiscreteAveragingAsianEngineBase<SingleVariate, RNG, S>::path_generator_type path_generator_type;
        typedef typename MCDiscreteAveragingAsianEngineBase<SingleVariate, RNG, S>::path_pricer_type    path_pricer_type;
        typedef typename MCDiscreteAveragingAsianEngineBase<SingleVariate, RNG, S>::stats_type          stats_type;

        MCDiscreteArithmeticASEngine_2(
             const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
             bool brownianBridge,
             bool antitheticVariate,
             Size requiredSamples,
             Real requiredTolerance,
             Size maxSamples,
             BigNatural seed,
             bool constantParameters);

      private:
        bool constantParameters;

        // Surcharge du pathGenerator()
        ext::shared_ptr<path_generator_type> pathGenerator() const override {
            // On récupère la grille
            Size dimensions = this->process_->factors();
            TimeGrid grid   = this->timeGrid();

            // Générateur pseudo-aléatoire
            typename RNG::rsg_type generator =
                RNG::make_sequence_generator(dimensions * (grid.size() - 1),
                                             MCDiscreteAveragingAsianEngineBase<SingleVariate,RNG,S>::seed_);

            // Branche "constant" ?
            if (this->constantParameters) {
                // On cast en GeneralizedBlackScholesProcess
                auto BS_process = ext::dynamic_pointer_cast<GeneralizedBlackScholesProcess>(
                    this->process_
                );
                QL_REQUIRE(BS_process, "Need a GenBlackScholesProcess for constantParameters");

                // On prend le temps final de la grille
                Time time_of_extraction = grid.back();

                // Récupération du strike (si besoin)
                double strike = ext::dynamic_pointer_cast<StrikedTypePayoff>(
                    this->arguments_.payoff
                )->strike();

                // On construit un "ConstantBlackScholesProcess" 
                // via votre fonction factorisée ou direct:
                auto cst_BS_process = makeConstantProcess( // si vous factorisez
                    BS_process,
                    time_of_extraction,
                    strike
                );

                return ext::make_shared<path_generator_type>(
                    cst_BS_process, grid, generator,
                    MCDiscreteAveragingAsianEngineBase<SingleVariate,RNG,S>::brownianBridge_
                );
            } else {
                // Sinon, on renvoie le path_generator classique
                return ext::make_shared<path_generator_type>(
                    this->process_, grid, generator,
                    MCDiscreteAveragingAsianEngineBase<SingleVariate,RNG,S>::brownianBridge_
                );
            }
        }

      protected:
        // Surcharge du pathPricer()
        ext::shared_ptr<path_pricer_type> pathPricer() const override;
    };


    // ------------------------------------------------------------------------
    // Implementation du constructeur
    // ------------------------------------------------------------------------
    template <class RNG, class S>
    inline
    MCDiscreteArithmeticASEngine_2<RNG,S>::MCDiscreteArithmeticASEngine_2(
             const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
             bool brownianBridge,
             bool antitheticVariate,
             Size requiredSamples,
             Real requiredTolerance,
             Size maxSamples,
             BigNatural seed,
             bool constantParameters)
    : MCDiscreteAveragingAsianEngineBase<SingleVariate,RNG,S>(
          process, brownianBridge, antitheticVariate,
          false,  // controlVariate
          requiredSamples, requiredTolerance, maxSamples, seed
      ),
      constantParameters(constantParameters)
    {
    }

    // ------------------------------------------------------------------------
    // Implementation du pathPricer()
    // ------------------------------------------------------------------------
    template <class RNG, class S>
    inline
    ext::shared_ptr<typename MCDiscreteArithmeticASEngine_2<RNG,S>::path_pricer_type>
    MCDiscreteArithmeticASEngine_2<RNG,S>::pathPricer() const {
        // On récupère payoff + exercise
        auto payoff = ext::dynamic_pointer_cast<PlainVanillaPayoff>(this->arguments_.payoff);
        QL_REQUIRE(payoff, "non-plain payoff given");

        auto exercise = ext::dynamic_pointer_cast<EuropeanExercise>(this->arguments_.exercise);
        QL_REQUIRE(exercise, "wrong exercise given");

        auto process = ext::dynamic_pointer_cast<GeneralizedBlackScholesProcess>(this->process_);
        QL_REQUIRE(process, "Black-Scholes process required");

        // On construit un ArithmeticASOPathPricer (dérivé concret)
        // Supposez qu'il existe un tel constructeur :
        //   ArithmeticASOPathPricer(Option::Type, DiscountFactor, Real runningAcc, Size pastFixings)
        // ou proche
        DiscountFactor disc = process->riskFreeRate()->discount(exercise->lastDate());

        return ext::shared_ptr<path_pricer_type>(
            new ArithmeticASOPathPricer(
                payoff->optionType(),
                disc,
                this->arguments_.runningAccumulator,
                this->arguments_.pastFixings
            )
        );
        /*
          OU BIEN :

          auto pricer = ext::make_shared<ArithmeticASOPathPricer>(
              payoff->optionType(),
              disc,
              this->arguments_.runningAccumulator,
              this->arguments_.pastFixings
          );
          return pricer; // Upcast implicite vers shared_ptr<PathPricer<Path>>
        */
    }

    // ------------------------------------------------------------------------
    // Factory MakeMCDiscreteArithmeticASEngine_2
    // ------------------------------------------------------------------------
    template <class RNG = PseudoRandom, class S = Statistics>
    class MakeMCDiscreteArithmeticASEngine_2 {
      public:
        explicit MakeMCDiscreteArithmeticASEngine_2(
            ext::shared_ptr<GeneralizedBlackScholesProcess> process);

        MakeMCDiscreteArithmeticASEngine_2& withBrownianBridge(bool b = true);
        MakeMCDiscreteArithmeticASEngine_2& withSamples(Size samples);
        MakeMCDiscreteArithmeticASEngine_2& withAbsoluteTolerance(Real tolerance);
        MakeMCDiscreteArithmeticASEngine_2& withMaxSamples(Size samples);
        MakeMCDiscreteArithmeticASEngine_2& withSeed(BigNatural seed);
        MakeMCDiscreteArithmeticASEngine_2& withAntitheticVariate(bool b = true);
        MakeMCDiscreteArithmeticASEngine_2& withConstantParameters(bool constantParameters);

        operator ext::shared_ptr<PricingEngine>() const;

      private:
        ext::shared_ptr<GeneralizedBlackScholesProcess> process_;
        bool antithetic_      = false;
        Size samples_, maxSamples_;
        Real tolerance_;
        bool brownianBridge_  = true;
        BigNatural seed_      = 0;
        bool constantParameters_;
    };

    // Constructor
    template <class RNG, class S>
    inline
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::MakeMCDiscreteArithmeticASEngine_2(
        ext::shared_ptr<GeneralizedBlackScholesProcess> process)
    : process_(std::move(process)),
      samples_(Null<Size>()), maxSamples_(Null<Size>()),
      tolerance_(Null<Real>()), constantParameters_(false)
    {}

    // Named parameters
    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withSamples(Size samples) {
        QL_REQUIRE(tolerance_ == Null<Real>(), "tolerance already set");
        samples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withAbsoluteTolerance(Real tolerance) {
        QL_REQUIRE(samples_ == Null<Size>(), "number of samples already set");
        QL_REQUIRE(RNG::allowsErrorEstimate,
                   "chosen random generator policy does not allow an error estimate");
        tolerance_ = tolerance;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withMaxSamples(Size samples) {
        maxSamples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withSeed(BigNatural seed) {
        seed_ = seed;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withBrownianBridge(bool b) {
        brownianBridge_ = b;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withAntitheticVariate(bool b) {
        antithetic_ = b;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withConstantParameters(bool constantParameters) {
        constantParameters_ = constantParameters;
        return *this;
    }

    // Conversion en shared_ptr<PricingEngine>
    template <class RNG, class S>
    inline
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::operator ext::shared_ptr<PricingEngine>() const {
        return ext::shared_ptr<PricingEngine>(
            new MCDiscreteArithmeticASEngine_2<RNG,S>(
                process_,
                brownianBridge_,
                antithetic_,
                samples_, tolerance_,
                maxSamples_,
                seed_,
                constantParameters_
            )
        );
    }

} // namespace QuantLib

#endif

#ifndef mc_barrier_engines_hpp
#define mc_barrier_engines_hpp

#include <ql/exercise.hpp>
#include <ql/instruments/barrieroption.hpp>
#include <ql/pricingengines/mcsimulation.hpp>
#include <ql/pricingengines/barrier/mcbarrierengine.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <utility>

// On inclut le helper factorisé (sans eps)
#include "myconstutil.hpp"
#include "constantblackscholesprocess.hpp"

namespace QuantLib {

    //! Pricing engine for barrier options using Monte Carlo simulation
    template <class RNG = PseudoRandom, class S = Statistics>
    class MCBarrierEngine_2 : public BarrierOption::engine,
                              public McSimulation<SingleVariate, RNG, S> {
    public:
        typedef typename McSimulation<SingleVariate, RNG, S>::path_generator_type path_generator_type;
        typedef typename McSimulation<SingleVariate, RNG, S>::path_pricer_type    path_pricer_type;
        typedef typename McSimulation<SingleVariate, RNG, S>::stats_type          stats_type;

        MCBarrierEngine_2(ext::shared_ptr<GeneralizedBlackScholesProcess> process,
                          Size timeSteps,
                          Size timeStepsPerYear,
                          bool brownianBridge,
                          bool antitheticVariate,
                          Size requiredSamples,
                          Real requiredTolerance,
                          Size maxSamples,
                          bool isBiased,
                          BigNatural seed,
                          bool constantParameters);

    private:
        bool constantParameters;

        void calculate() const override {
            Real spot = process_->x0();
            QL_REQUIRE(spot > 0.0, "negative or null underlying given");
            QL_REQUIRE(!triggered(spot), "barrier touched");
            McSimulation<SingleVariate, RNG, S>::calculate(requiredTolerance_,
                requiredSamples_,
                maxSamples_);
            results_.value = this->mcModel_->sampleAccumulator().mean();
            if (RNG::allowsErrorEstimate)
                results_.errorEstimate =
                this->mcModel_->sampleAccumulator().errorEstimate();
        }

    protected:
        // McSimulation implementation
        TimeGrid timeGrid() const override;
        ext::shared_ptr<path_generator_type> pathGenerator() const override {
            TimeGrid grid = timeGrid();
            typename RNG::rsg_type gen =
                RNG::make_sequence_generator(grid.size() - 1, seed_);

            if (constantParameters) {

                auto BS_process = ext::dynamic_pointer_cast<GeneralizedBlackScholesProcess>(process_);
                QL_REQUIRE(BS_process, "Need a GeneralizedBlackScholesProcess");

                Time time_of_extraction = grid.back();
                double strike = ext::dynamic_pointer_cast<StrikedTypePayoff>(arguments_.payoff)->strike();

                // PAS DE + eps
                auto cst_BS_process = makeConstantProcess(
                    BS_process,
                    time_of_extraction,
                    strike
                );

                return ext::make_shared<path_generator_type>(
                    cst_BS_process, grid, gen, brownianBridge_
                );
            }
            else {
                return ext::make_shared<path_generator_type>(
                    process_, grid, gen, brownianBridge_
                );
            }
        }

        ext::shared_ptr<path_pricer_type> pathPricer() const override;

        // data members
        ext::shared_ptr<GeneralizedBlackScholesProcess> process_;
        Size timeSteps_, timeStepsPerYear_;
        Size requiredSamples_, maxSamples_;
        Real requiredTolerance_;
        bool isBiased_;
        bool brownianBridge_;
        BigNatural seed_;
    };


    //! Monte Carlo barrier-option engine factory
    template <class RNG = PseudoRandom, class S = Statistics>
    class MakeMCBarrierEngine_2 {
    public:
        MakeMCBarrierEngine_2(ext::shared_ptr<GeneralizedBlackScholesProcess>);
        // named parameters
        MakeMCBarrierEngine_2& withSteps(Size steps);
        MakeMCBarrierEngine_2& withStepsPerYear(Size steps);
        MakeMCBarrierEngine_2& withBrownianBridge(bool b = true);
        MakeMCBarrierEngine_2& withAntitheticVariate(bool b = true);
        MakeMCBarrierEngine_2& withSamples(Size samples);
        MakeMCBarrierEngine_2& withAbsoluteTolerance(Real tolerance);
        MakeMCBarrierEngine_2& withMaxSamples(Size samples);
        MakeMCBarrierEngine_2& withBias(bool b = true);
        MakeMCBarrierEngine_2& withSeed(BigNatural seed);
        MakeMCBarrierEngine_2& withConstantParameters(bool constantParameters);
        operator ext::shared_ptr<PricingEngine>() const;
    private:
        ext::shared_ptr<GeneralizedBlackScholesProcess> process_;
        bool brownianBridge_ = false, antithetic_ = false, biased_ = false;
        Size steps_, stepsPerYear_, samples_, maxSamples_;
        Real tolerance_;
        BigNatural seed_ = 0;
        bool constantParameters_;
    };


    // template definitions

    template <class RNG, class S>
    inline MCBarrierEngine_2<RNG, S>::MCBarrierEngine_2(
        ext::shared_ptr<GeneralizedBlackScholesProcess> process,
        Size timeSteps,
        Size timeStepsPerYear,
        bool brownianBridge,
        bool antitheticVariate,
        Size requiredSamples,
        Real requiredTolerance,
        Size maxSamples,
        bool isBiased,
        BigNatural seed,
        bool constantParameters)
        : McSimulation<SingleVariate, RNG, S>(antitheticVariate, false),
          process_(std::move(process)),
          timeSteps_(timeSteps), timeStepsPerYear_(timeStepsPerYear),
          requiredSamples_(requiredSamples),
          maxSamples_(maxSamples), requiredTolerance_(requiredTolerance),
          isBiased_(isBiased), brownianBridge_(brownianBridge),
          seed_(seed), constantParameters(constantParameters) 
    {
        QL_REQUIRE(timeSteps != Null<Size>() ||
            timeStepsPerYear != Null<Size>(),
            "no time steps provided");
        QL_REQUIRE(timeSteps == Null<Size>() ||
            timeStepsPerYear == Null<Size>(),
            "both time steps and time steps per year were provided");
        QL_REQUIRE(timeSteps != 0,
            "timeSteps must be positive");
        QL_REQUIRE(timeStepsPerYear != 0,
            "timeStepsPerYear must be positive");
        registerWith(process_);
    }

    template <class RNG, class S>
    inline TimeGrid MCBarrierEngine_2<RNG, S>::timeGrid() const {

        Time residualTime = process_->time(arguments_.exercise->lastDate());
        if (timeSteps_ != Null<Size>()) {
            return TimeGrid(residualTime, timeSteps_);
        }
        else if (timeStepsPerYear_ != Null<Size>()) {
            Size steps = static_cast<Size>(timeStepsPerYear_ * residualTime);
            return TimeGrid(residualTime, std::max<Size>(steps, 1));
        }
        else {
            QL_FAIL("time steps not specified");
        }
    }

    template <class RNG, class S>
    inline
    ext::shared_ptr<typename MCBarrierEngine_2<RNG, S>::path_pricer_type>
    MCBarrierEngine_2<RNG, S>::pathPricer() const {

        ext::shared_ptr<PlainVanillaPayoff> payoff =
            ext::dynamic_pointer_cast<PlainVanillaPayoff>(arguments_.payoff);
        QL_REQUIRE(payoff, "non-plain payoff given");

        TimeGrid grid = timeGrid();
        std::vector<DiscountFactor> discounts(grid.size());
        for (Size i = 0; i < grid.size(); i++)
            discounts[i] = process_->riskFreeRate()->discount(grid[i]);

        if (isBiased_) {
            return ext::shared_ptr<typename MCBarrierEngine_2<RNG, S>::path_pricer_type>(
                new BiasedBarrierPathPricer(
                    arguments_.barrierType,
                    arguments_.barrier,
                    arguments_.rebate,
                    payoff->optionType(),
                    payoff->strike(),
                    discounts));
        }
        else {
            PseudoRandom::ursg_type sequenceGen(grid.size() - 1,
                PseudoRandom::urng_type(5));
            return ext::shared_ptr<typename MCBarrierEngine_2<RNG, S>::path_pricer_type>(
                new BarrierPathPricer(
                    arguments_.barrierType,
                    arguments_.barrier,
                    arguments_.rebate,
                    payoff->optionType(),
                    payoff->strike(),
                    discounts,
                    process_,
                    sequenceGen));
        }
    }


    template <class RNG, class S>
    inline MakeMCBarrierEngine_2<RNG, S>::MakeMCBarrierEngine_2(
        ext::shared_ptr<GeneralizedBlackScholesProcess> process)
        : process_(std::move(process)), steps_(Null<Size>()), stepsPerYear_(Null<Size>()),
          samples_(Null<Size>()), maxSamples_(Null<Size>()), tolerance_(Null<Real>()) {}

    template <class RNG, class S>
    inline MakeMCBarrierEngine_2<RNG, S>&
        MakeMCBarrierEngine_2<RNG, S>::withSteps(Size steps) {
        steps_ = steps;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCBarrierEngine_2<RNG, S>&
        MakeMCBarrierEngine_2<RNG, S>::withStepsPerYear(Size steps) {
        stepsPerYear_ = steps;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCBarrierEngine_2<RNG, S>&
        MakeMCBarrierEngine_2<RNG, S>::withBrownianBridge(bool brownianBridge) {
        brownianBridge_ = brownianBridge;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCBarrierEngine_2<RNG, S>&
        MakeMCBarrierEngine_2<RNG, S>::withAntitheticVariate(bool b) {
        antithetic_ = b;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCBarrierEngine_2<RNG, S>&
        MakeMCBarrierEngine_2<RNG, S>::withSamples(Size samples) {
        QL_REQUIRE(tolerance_ == Null<Real>(),
            "tolerance already set");
        samples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCBarrierEngine_2<RNG, S>&
        MakeMCBarrierEngine_2<RNG, S>::withAbsoluteTolerance(Real tolerance) {
        QL_REQUIRE(samples_ == Null<Size>(),
            "number of samples already set");
        QL_REQUIRE(RNG::allowsErrorEstimate,
            "chosen random generator policy "
            "does not allow an error estimate");
        tolerance_ = tolerance;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCBarrierEngine_2<RNG, S>&
        MakeMCBarrierEngine_2<RNG, S>::withMaxSamples(Size samples) {
        maxSamples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCBarrierEngine_2<RNG, S>&
        MakeMCBarrierEngine_2<RNG, S>::withBias(bool biased) {
        biased_ = biased;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCBarrierEngine_2<RNG, S>&
        MakeMCBarrierEngine_2<RNG, S>::withSeed(BigNatural seed) {
        seed_ = seed;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCBarrierEngine_2<RNG, S>&
        MakeMCBarrierEngine_2<RNG, S>::withConstantParameters(bool constantParameters) {
        constantParameters_ = constantParameters;
        return *this;
    }

    template <class RNG, class S>
    inline
    MakeMCBarrierEngine_2<RNG, S>::operator ext::shared_ptr<PricingEngine>() const {
        QL_REQUIRE(steps_ != Null<Size>() || stepsPerYear_ != Null<Size>(),
            "number of steps not given");
        QL_REQUIRE(steps_ == Null<Size>() || stepsPerYear_ == Null<Size>(),
            "number of steps overspecified");
        return ext::make_shared<MCBarrierEngine_2<RNG, S>>(process_,
                                                           steps_,
                                                           stepsPerYear_,
                                                           brownianBridge_,
                                                           antithetic_,
                                                           samples_,
                                                           tolerance_,
                                                           maxSamples_,
                                                           biased_,
                                                           seed_,
                                                           constantParameters_);
    }

} // namespace QuantLib

#endif

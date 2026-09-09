#pragma once

#include <array>
#include <climits>
#include <tuple>
#include <vector>

#include "constraint.hpp"
#include "intvar.hpp"
#include "gfl/FunQual.hpp"
#include "gfl/MathUtils.hpp"

// References:
// - A Constraint for Bin Packing (DOI: 10.1007/978-3-540-30201-8_47)
// - New lower bounds for bin packing problems with conflicts (DOI: 10.1016/j.ejor.2010.02.016)

class BinPackingLoad : public Constraint
{
    public:
        template<typename LoadVars, typename BinVars>
        BinPackingLoad(LoadVars const & load, BinVars const & bin, std::vector<int> const & w) :
            Constraint(load[0]->getSolver()),
            _load(load.begin(), load.end()),
            _bin(bin.begin(), bin.end()),
            _weights(w)
        {
            assert(w.size() == bin.size());
            setup();
        }
        void post() override;
        void propagate() override;

    protected:
        // Standard propagator
        struct ItemInfo {int index; int weight;};

        template <typename T>
        class VectorView
        {
            private:
                std::vector<T> const & v;
                int idxToHide;
            public:
                explicit VectorView(std::vector<T> const & v) : v(v), idxToHide(INT_MAX) {};
                VectorView(std::vector<T> const & v, int idx) : v(v), idxToHide(idx) {};
                int size() const {return static_cast<int>(v.size()) - (idxToHide != INT_MAX);};
                T const & at(int idx) const {return v.at(idx + (idx >= idxToHide));};
        };

        std::vector<var<int>::Ptr> const _load;
        std::vector<var<int>::Ptr> _bin;
        std::vector<int> _weights;
        int nBins;
        int nItems;
        int sum_weights;
        int sum_min_loads;
        int sum_max_loads;
        std::vector<int> bin_sizes;
        std::vector<int> load_sizes;
        std::vector<int> required_loads;
        std::vector<int> possible_loads;
        std::vector<int> packed_items;
        std::vector<std::vector<ItemInfo>> candidate_items;
        std::vector<int> unassigned_weights;

        void setup();
        void calculateNumBins();
        void calculateRequiredPossibleCandidates();
        void propagateLoadCoherence();
        void calculateSumLoads();
        void propagateBasicLoadTightening();
        void propagateBasicItemEliminationCommitment();
        void checkPackableBins();
        void propagateKnapsackLoadTightening();
        void propagateKnapsackItemEliminationCommitment();
        void saveBinLoadSizes();
        bool checkBinLoadSizeChanges();
        /// The only step a device may take over; the rest of propagate is common
        virtual void checkLowerbounds();
        std::tuple<bool,int,int> noSum(VectorView<ItemInfo> const & items_info, int alpha, int beta);
        static int sumWeights(VectorView<ItemInfo> const & items_info);


        // Reductions
        static int const REDUCTIONS_COUNT = 3;
        std::vector<int> weightsBaseReduction;
        std::vector<int> weightsCurrentReduction;
        std::array<int,REDUCTIONS_COUNT> deltaReductions;
        int capacityBaseReduction;

        void prepareReductions();

        /// What the lower bounds need to know about one reduction of the instance
        struct Reduction {int notZeroCount; int maxWeight;};

        int reductionCapacity(int rIdx) const {return capacityBaseReduction + deltaReductions[rIdx];};
        Reduction analyzeReduction(int rIdx, bool materialize);


    public:
        // Dual-Feasible Functions
        // Branch-less transformations, defined here rather than in the .cpp
        // because the GPU kernels instantiate the very same ones. Public
        // because those kernels name them from outside the class
        GFL_HOST_DEVICE static int fCCM1(int w, int l, int c)
        {
            // Conditions
            int const c0 = 2 * w > c;  // x > c / 2
            int const c1 = 2 * w == c; // x == c / 2
            int const c2 = 2 * w < c;  // x < c / 2
            assert(c0 + c1 + c2 == 1);

            // Values
            int const v0 = 2 * ((c / l) - ((c - w) / l));
            int const v1 = c / l;
            int const v2 = 2 * (w / l);

            return (c0 * v0) + (c1 * v1) + (c2 * v2);
        }

        GFL_HOST_DEVICE static int fMT(int w, int l, int c)
        {
            // Conditions
            int const c0 = w < l;
            int const c1 = l <= w and w <= c - l;
            int const c2 = c - l < w;
            assert(c0 + c1 + c2 == 1);

            // Values
            int const v0 = 0;
            int const v1 = w;
            int const v2 = c;

            return (c0 * v0) + (c1 * v1) + (c2 * v2);
        }

        GFL_HOST_DEVICE static int fBJ1(int w, int l, int c)
        {
            // Auxiliary values
            int const p = l - (c % l);

            // Conditions
            int const c0 = w % l <= c % l;
            int const c1 = w % l > c % l;

            // Values
            int const v0 = (w / l) * p;
            int const v1 = (w / l) * p + (w % l) - (c % l);

            return (c0 * v0) + (c1 * v1);
        }

        GFL_HOST_DEVICE static int fVB2Base(int w, int l, int c)
        {
            int const v = gfl::ceilDiv<int>(l * w, c);
            return v > 0 ? v - 1 : 0;
        }

        GFL_HOST_DEVICE static int fVB2(int w, int l, int c)
        {
            // Conditions
            int const c0 = 2 * w > c;
            int const c1 = 2 * w == c;
            int const c2 = 2 * w < c;

            // Auxiliary values
            int const t0 = fVB2Base(c, l, c);
            int const t1 = fVB2Base(w, l, c);
            int const t2 = fVB2Base(c - w, l, c);

            // Values
            int const v0 = 2 * t0 - 2 * t2;
            int const v1 = t0;
            int const v2 = 2 * t1;

            return (c0 * v0) + (c1 * v1) + (c2 * v2);
        }

        GFL_HOST_DEVICE static int fFS1(int w, int l, int c)
        {
            // Conditions
            int const c0 = w * (l + 1) % c == 0;
            int const c1 = w * (l + 1) % c != 0;
            assert(c0 + c1 == 1);

            // Values
            int const v0 = w * l;
            int const v1 = ((w * (l + 1)) / c) * c;

            return (c0 * v0) + (c1 * v1);
        }

        GFL_HOST_DEVICE static int fRAD2Base(int w, int l, int c)
        {
            // Conditions
            int const c0 = w < l;
            int const c1 = l <= w and w <= c - 2 * l;
            int const c2 = c - 2 * l < w and w < 2 * l;

            // Values
            int const v0 = 0;
            int const v1 = c / 3;
            int const v2 = c / 2;

            return (c0 * v0) + (c1 * v1) + (c2 * v2);
        }

        GFL_HOST_DEVICE static int fRAD2(int w, int l, int c)
        {
            // Conditions
            int const c0 = w < 2 * l;
            int const c1 = 2 * l <= w;
            assert(c0 + c1 == 1);

            // Values
            int const v0 = fRAD2Base(w, l, c);
            int const v1 = c - fRAD2Base(c - w, l, c);

            return (c0 * v0) + (c1 * v1);
        }


    protected:
        // Lambdas
        static int const LAMBDA_SAMPLES  = 256;
        struct LambdaRange {int min; int max;};

        static LambdaRange lCCM1(int c) {return {1, c / 2};};
        static LambdaRange lMT(int c)   {return {0, c / 2};}; // The 0 value is included to calculate the L0 bound
        static LambdaRange lBJ1(int c)  {return {1, c};};
        static LambdaRange lVB2(int c)  {return {2, c};};
        static LambdaRange lFS1(int c)  {return {1, 100};};
        static LambdaRange lRAD2(int c) {return {c / 4 + 1, c / 3};};
        static LambdaRange sanitizeLambdaRange(LambdaRange lambda, int nWeights, int maxWeight);


        // Roster of the dual-feasible functions the lowerbounds are built on:
        // the transformation, the lambda range to sample, and whether that
        // range needs sanitizing first. Both devices walk this one roster, so a
        // function added here is picked up by the CPU propagator and by its GPU
        // counterpart at once
        template<int f(int,int,int), LambdaRange l(int), bool sanitize>
        struct Dff
        {
            static constexpr auto transform = f;
            static constexpr auto lambdaRange = l;
            static constexpr bool needsSanitizing = sanitize;
        };

        template<typename... Fs>
        struct DffRoster
        {
            static int const COUNT = sizeof...(Fs);
            /// Calls `visit` once per dual-feasible function, in roster order
            template<typename Visitor>
            static void forEach(Visitor && visit) {(visit(Fs()), ...);};
        };

        using Dffs = DffRoster<Dff<fCCM1, lCCM1, false>,
                               Dff<fMT,   lMT,   false>,
                               Dff<fBJ1,  lBJ1,  false>,
                               Dff<fVB2,  lVB2,  true>,
                               Dff<fFS1,  lFS1,  true>,
                               Dff<fRAD2, lRAD2, false>>;

        static int const DFFS_COUNT = Dffs::COUNT;


        // Lowerbounds
        static int calcL2(std::vector<int> const & weights, int capacity);
        template<int f(int,int,int)>
        static int calcDffLowerboundSingleLambda(std::vector<int> const & weights, int capacity, int lambda)
        {
            // Transform and sum the weights
            int const nWeights = static_cast<int>(weights.size());
            int sumTransformedWeights = 0;
            for (int wIdx = 0; wIdx < nWeights; wIdx += 1)
            {
                sumTransformedWeights += f(weights[wIdx], lambda, capacity);
            }

            // Transform the capacity
            int const transformedCapacity = f(capacity, lambda, capacity);

            // Lowerbound
            return transformedCapacity > 0 ? gfl::ceilDiv<int>(sumTransformedWeights, transformedCapacity) : 0;
        }
        template<int f(int,int,int), LambdaRange l(int)>
        static int calcDffLowerbound(std::vector<int> const & weights, int capacity, int nWeights, int maxWeight, bool sanitize = false)
        {
            int fLowerbound = 0;
            LambdaRange lRange = l(capacity);
            lRange = sanitize ? sanitizeLambdaRange(lRange, nWeights, maxWeight) : lRange;
            int const lRangeSize = lRange.max - lRange.min + 1;
            if (lRangeSize <= 0)
            {
                return 0;
            }
            int const lStep = gfl::ceilDiv<int>(lRangeSize, LAMBDA_SAMPLES + 1);
            for (int lambda = lRange.min + lStep; lambda < lRange.max; lambda += lStep)
            {
                int const lowerbound = calcDffLowerboundSingleLambda<f>(weights, capacity, lambda);
                fLowerbound = std::max(fLowerbound, lowerbound);
            }
            return fLowerbound;
        }
};

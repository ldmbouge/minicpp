# Core library
################################################################################
add_library(copl
        core/acstr.cpp
        core/commandList.cpp
        core/constraint.cpp
        core/domain.cpp
        core/fail.cpp
        core/intvar.cpp
        core/search.cpp
        core/solver.cpp
        core/store.cpp
        core/tracer.cpp
        core/trail.cpp
        core/trailable.cpp

        constraints/alldifferent.cpp
        constraints/cardinality.cpp
        constraints/circuit.cpp
        constraints/clause.cpp
        constraints/cumulative.cpp
        constraints/disjunctive.cpp
        constraints/disjunctiveresource.cpp
        constraints/element.cpp
        constraints/lex.cpp
        constraints/matching.cpp
        constraints/maximum.cpp
        constraints/profile.cpp
        constraints/regular.cpp
        constraints/sum.cpp
        constraints/table.cpp
        constraints/tasksequence.cpp
        constraints/ttable.cpp

        mdd/mdd.cpp
        mdd/mddAbs.cpp
        mdd/mddAlldiff.cpp
        mdd/mddAmong.cpp
        mdd/mddGcc.cpp
        mdd/mddGoc.cpp
        mdd/mddMaximumCutObjective.cpp
        mdd/mddPrecedence.cpp
        mdd/mddSeq.cpp
        mdd/mddSum.cpp
        mdd/mddTSPSum.cpp
        mdd/mdddelta.cpp
        mdd/mddnode.cpp
        mdd/mddrelax.cpp
        mdd/mddstate.cpp

        util/bitset.cpp
        util/mallocWatch.cpp
        util/RuntimeMonitor.cpp
)

# PUBLIC: everything linking copl includes headers as "core/solver.hpp" etc.
target_include_directories(copl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(USE_CXX_EXCEPTIONS)
    target_compile_definitions(copl PUBLIC CPPFAIL=1)
endif()

if(ENABLE_GPU)
    target_sources(copl PRIVATE
            constraints_gpu/cumulative.cu
    )
    target_compile_definitions(copl PUBLIC ENABLE_GPU=1)
    target_link_libraries(copl PUBLIC CUDA::cudart)
endif()

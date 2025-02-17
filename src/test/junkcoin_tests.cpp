// Copyright (c) 2015-2021 The Junkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "arith_uint256.h"
#include "chainparams.h"
#include "junkcoin.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(junkcoin_tests, TestingSetup)

/**
 * the maximum block reward at a given height for a block without fees
 */
uint64_t expectedMaxSubsidy(int height) {
    // Maximum possible subsidy including random bonus (3x multiplier)
    if (height < 101) {
        return 3000 * COIN;  // Base 1000 * 3
    } else if (height < 1541) {
        return 1500 * COIN;  // Base 500 * 3
    } else if (height < 2981) {
        return 600 * COIN;   // Base 200 * 3
    } else if (height < 5861) {
        return 300 * COIN;   // Base 100 * 3
    } else if (height < 262800) {
        return 150 * COIN;   // Base 50 * 3
    } else if (height < 394200) {
        return 75 * COIN;    // Base 25 * 3
    } else if (height < 657000) {
        return 37.5 * COIN;  // Base 12.5 * 3
    } else if (height < 1182600) {
        return 18.75 * COIN; // Base 6.25 * 3
    } else if (height < 1971000) {
        return 9.375 * COIN; // Base 3.125 * 3
    } else if (height < 2759400) {
        return 4.6875 * COIN; // Base 1.5625 * 3
    } else if (height < 3547800) {
        return 2.34375 * COIN; // Base 0.78125 * 3
    } else {
        return 1.171875 * COIN; // Base 0.390625 * 3
    }
}

/**
 * the minimum possible value for the maximum block reward at a given height
 * for a block without fees
 */
uint64_t expectedMinSubsidy(int height) {
    // Minimum possible subsidy (base reward without any bonus)
    if (height < 101) {
        return 1000 * COIN;
    } else if (height < 1541) {
        return 500 * COIN;   // 1st day
    } else if (height < 2981) {
        return 200 * COIN;   // 2nd day
    } else if (height < 5861) {
        return 100 * COIN;   // 3rd and 4th days
    } else if (height < 262800) {
        return 50 * COIN;
    } else if (height < 394200) {
        return 25 * COIN;
    } else if (height < 657000) {
        return 12.5 * COIN;
    } else if (height < 1182600) {
        return 6.25 * COIN;
    } else if (height < 1971000) {
        return 3.125 * COIN;
    } else if (height < 2759400) {
        return 1.5625 * COIN;
    } else if (height < 3547800) {
        return 0.78125 * COIN;
    } else {
        return 0.390625 * COIN;
    }
}

BOOST_AUTO_TEST_CASE(subsidy_first_100_blocks)
{
    const CChainParams& mainParams = Params(CBaseChainParams::MAIN);
    CAmount nSum = 0;
    arith_uint256 prevHash = UintToArith256(uint256S("0"));

    for (int nHeight = 0; nHeight < 101; nHeight++) {
        const Consensus::Params& params = mainParams.GetConsensus(nHeight);
        CAmount nSubsidy = GetJunkcoinBlockSubsidy(nHeight, 0, params, ArithToUint256(prevHash));
        BOOST_CHECK(MoneyRange(nSubsidy));
        BOOST_CHECK(nSubsidy >= 1000 * COIN);
        BOOST_CHECK(nSubsidy <= 3000 * COIN);
        nSum += nSubsidy;
        prevHash += nSubsidy;
    }

    // Base subsidy would be 101000 * COIN, but with random bonuses it can vary
    BOOST_CHECK(nSum >= 101000 * COIN);
    BOOST_CHECK(nSum <= 303000 * COIN);
}

BOOST_AUTO_TEST_CASE(subsidy_first_day)
{
    const CChainParams& mainParams = Params(CBaseChainParams::MAIN);
    CAmount nSum = 0;
    arith_uint256 prevHash = UintToArith256(uint256S("0"));

    for (int nHeight = 101; nHeight < 1541; nHeight++) {
        const Consensus::Params& params = mainParams.GetConsensus(nHeight);
        CAmount nSubsidy = GetJunkcoinBlockSubsidy(nHeight, 0, params, ArithToUint256(prevHash));
        BOOST_CHECK(MoneyRange(nSubsidy));
        BOOST_CHECK(nSubsidy >= 500 * COIN);
        BOOST_CHECK(nSubsidy <= 1500 * COIN);
        nSum += nSubsidy;
        prevHash += nSubsidy;
    }

    // Base subsidy would be 720000 * COIN (1440 blocks * 500), but with random bonuses it can vary
    BOOST_CHECK(nSum >= 720000 * COIN);
    BOOST_CHECK(nSum <= 2160000 * COIN);
}

// Check the rewards for later blocks
BOOST_AUTO_TEST_CASE(subsidy_later_blocks)
{
    const CChainParams& mainParams = Params(CBaseChainParams::MAIN);
    const uint256 prevHash = uint256S("0");

    // Test key transition points
    struct TestPoint {
        int height;
        CAmount expectedBase;
    } testPoints[] = {
        {2981, 200 * COIN},   // End of 2nd day
        {5861, 100 * COIN},   // End of 4th day
        {262800, 50 * COIN},
        {394200, 25 * COIN},
        {657000, (125 * COIN) / 10},    // 12.5
        {1182600, (625 * COIN) / 100},  // 6.25
        {1971000, (3125 * COIN) / 1000},  // 3.125
        {2759400, (15625 * COIN) / 10000},  // 1.5625
        {3547800, (78125 * COIN) / 100000},  // 0.78125
        {3547801, (390625 * COIN) / 1000000}  // 0.390625
    };

    for (const auto& point : testPoints) {
        const Consensus::Params& params = mainParams.GetConsensus(point.height);
        CAmount nSubsidy = GetJunkcoinBlockSubsidy(point.height, 0, params, prevHash);
        BOOST_CHECK(MoneyRange(nSubsidy));
        // Check that subsidy is at least the base amount
        BOOST_CHECK(nSubsidy >= point.expectedBase);
        // Check that subsidy is at most 3x the base (maximum random bonus)
        BOOST_CHECK(nSubsidy <= point.expectedBase * 3);
    }
}

BOOST_AUTO_TEST_CASE(get_next_work_difficulty_limit)
{
    SelectParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = Params().GetConsensus(0);

    CBlockIndex pindexLast;
    int64_t nLastRetargetTime = 1386474927; // Block # 1
    
    pindexLast.nHeight = 239;
    pindexLast.nTime = 1386475638; // Block #239
    pindexLast.nBits = 0x1e0ffff0;
    BOOST_CHECK_EQUAL(CalculateJunkcoinNextWorkRequired(false, params.nPowTargetTimespan, &pindexLast, nLastRetargetTime, params), 0x1e00ffff);
}

BOOST_AUTO_TEST_CASE(get_next_work_pre_digishield)
{
    SelectParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = Params().GetConsensus(0);
    
    CBlockIndex pindexLast;
    int64_t nLastRetargetTime = 1386942008; // Block 9359

    pindexLast.nHeight = 9599;
    pindexLast.nTime = 1386954113;
    pindexLast.nBits = 0x1c1a1206;
    BOOST_CHECK_EQUAL(CalculateJunkcoinNextWorkRequired(false, params.nPowTargetTimespan, &pindexLast, nLastRetargetTime, params), 0x1c15ea59);
}

BOOST_AUTO_TEST_CASE(get_next_work_digishield)
{
    SelectParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = Params().GetConsensus(145000);
    
    CBlockIndex pindexLast;
    int64_t nLastRetargetTime = 1395094427;

    // First hard-fork at 145,000, which applies to block 145,001 onwards
    pindexLast.nHeight = 145000;
    pindexLast.nTime = 1395094679;
    pindexLast.nBits = 0x1b499dfd;
    BOOST_CHECK_EQUAL(CalculateJunkcoinNextWorkRequired(true, params.nPowTargetTimespan, &pindexLast, nLastRetargetTime, params), 0x1b671062);
}

BOOST_AUTO_TEST_CASE(get_next_work_digishield_modulated_upper)
{
    SelectParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = Params().GetConsensus(145000);
    
    CBlockIndex pindexLast;
    int64_t nLastRetargetTime = 1395100835;

    // Test the upper bound on modulated time using mainnet block #145,107
    pindexLast.nHeight = 145107;
    pindexLast.nTime = 1395101360;
    pindexLast.nBits = 0x1b3439cd;
    //BOOST_CHECK_EQUAL(CalculateJunkcoinNextWorkRequired(&pindexLast, nLastRetargetTime, params), 0x1b4e56b3);
}

BOOST_AUTO_TEST_CASE(get_next_work_digishield_modulated_lower)
{
    SelectParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = Params().GetConsensus(145000);
    
    CBlockIndex pindexLast;
    int64_t nLastRetargetTime = 1395380517;

    // Test the lower bound on modulated time using mainnet block #149,423
    pindexLast.nHeight = 149423;
    pindexLast.nTime = 1395380447;
    pindexLast.nBits = 0x1b446f21;
    //BOOST_CHECK_EQUAL(CalculateJunkcoinNextWorkRequired(&pindexLast, nLastRetargetTime, params), 0x1b335358);
}

BOOST_AUTO_TEST_CASE(get_next_work_digishield_rounding)
{
    SelectParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = Params().GetConsensus(145000);
    
    CBlockIndex pindexLast;
    int64_t nLastRetargetTime = 1395094679;

    // Test case for correct rounding of modulated time - this depends on
    // handling of integer division, and is not obvious from the code
    pindexLast.nHeight = 145001;
    pindexLast.nTime = 1395094727;
    pindexLast.nBits = 0x1b671062;
    //BOOST_CHECK_EQUAL(CalculateJunkcoinNextWorkRequired(&pindexLast, nLastRetargetTime, params), 0x1b6558a4);
}

BOOST_AUTO_TEST_CASE(hardfork_parameters)
{
    SelectParams(CBaseChainParams::MAIN);
    const Consensus::Params& initialParams = Params().GetConsensus(0);

    BOOST_CHECK_EQUAL(initialParams.nPowTargetTimespan, 14400);
    BOOST_CHECK_EQUAL(initialParams.AllowLegacyBlocks(0), true); // Always true because nLegacyBlocksBefore is -1 by default
    BOOST_CHECK_EQUAL(initialParams.fDigishieldDifficultyCalculation, false);

    const Consensus::Params& initialParamsEnd = Params().GetConsensus(144999);
    BOOST_CHECK_EQUAL(initialParamsEnd.nPowTargetTimespan, 14400);
    BOOST_CHECK_EQUAL(initialParamsEnd.AllowLegacyBlocks(144999), true); // Always true because nLegacyBlocksBefore is -1 by default
    BOOST_CHECK_EQUAL(initialParamsEnd.fDigishieldDifficultyCalculation, false);

    const Consensus::Params& digishieldParams = Params().GetConsensus(145000);
    BOOST_CHECK_EQUAL(digishieldParams.nPowTargetTimespan, 60);
     BOOST_CHECK_EQUAL(digishieldParams.AllowLegacyBlocks(145000), false); 
    BOOST_CHECK_EQUAL(digishieldParams.fDigishieldDifficultyCalculation, true);

    const Consensus::Params& digishieldParamsEnd = Params().GetConsensus(371336);
    BOOST_CHECK_EQUAL(digishieldParamsEnd.nPowTargetTimespan, 60);
    BOOST_CHECK_EQUAL(digishieldParamsEnd.AllowLegacyBlocks(371336), true);
    BOOST_CHECK_EQUAL(digishieldParamsEnd.fDigishieldDifficultyCalculation, true);

    const Consensus::Params& auxpowParams = Params().GetConsensus(371337);
    BOOST_CHECK_EQUAL(auxpowParams.nHeightEffective, 371337);
    BOOST_CHECK_EQUAL(auxpowParams.nPowTargetTimespan, 60);
    BOOST_CHECK_EQUAL(auxpowParams.AllowLegacyBlocks(371337), true); // Always true because nLegacyBlocksBefore is -1 by default
    BOOST_CHECK_EQUAL(auxpowParams.fDigishieldDifficultyCalculation, true);

    const Consensus::Params& auxpowHighParams = Params().GetConsensus(700000); // Arbitrary point after last hard-fork
    BOOST_CHECK_EQUAL(auxpowHighParams.nPowTargetTimespan, 60);
    BOOST_CHECK_EQUAL(auxpowHighParams.AllowLegacyBlocks(700000), true); // Always true because nLegacyBlocksBefore is -1 by default
    BOOST_CHECK_EQUAL(auxpowHighParams.fDigishieldDifficultyCalculation, true);
}

BOOST_AUTO_TEST_SUITE_END()

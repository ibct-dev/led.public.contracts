#include <Runtime/Runtime.h>
#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <fc/log/logger.hpp>
#include <iostream>

#include "led.system_tester.hpp"
struct _abi_hash
{
    name owner;
    fc::sha256 hash;
};
FC_REFLECT(_abi_hash, (owner)(hash));

using namespace eosio_system;

BOOST_AUTO_TEST_SUITE(legis_system_tests)

bool within_one(int64_t a, int64_t b) { return std::abs(a - b) <= 1; }

BOOST_FIXTURE_TEST_CASE(buysell, legis_system_tester)
try
{
    BOOST_REQUIRE_EQUAL(core_sym::from_string("0.0000"), get_balance("alice.p"));

    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("led", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));

    auto total = get_total_stake("alice.p");
    auto init_bytes = total["ram_bytes"].as_uint64();

    const asset initial_ram_balance = get_balance(N(led.ram));
    const asset initial_ramfee_balance = get_balance(N(led.ramfee));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("200.0000")));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("800.0000"), get_balance("alice.p"));
    BOOST_REQUIRE_EQUAL(initial_ram_balance + core_sym::from_string("199.0000"), get_balance(N(led.ram)));
    BOOST_REQUIRE_EQUAL(initial_ramfee_balance + core_sym::from_string("1.0000"), get_balance(N(led.ramfee)));

    total = get_total_stake("alice.p");
    auto bytes = total["ram_bytes"].as_uint64();
    auto bought_bytes = bytes - init_bytes;
    wdump((init_bytes)(bought_bytes)(bytes));

    BOOST_REQUIRE_EQUAL(true, 0 < bought_bytes);

    BOOST_REQUIRE_EQUAL(success(), sellram("alice.p", bought_bytes));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("998.0049"), get_balance("alice.p"));
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(true, total["ram_bytes"].as_uint64() == init_bytes);

    transfer("led", "alice.p", core_sym::from_string("100000000.0000"), "led");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("100000998.0049"), get_balance("alice.p"));
    // alice buys ram for 10000000.0000, 0.5% = 50000.0000 go to ramfee
    // after fee 9950000.0000 go to bought bytes
    // when selling back bought bytes, pay 0.5% fee and get back 99.5% of 9950000.0000 = 9900250.0000
    // expected account after that is 90000998.0049 + 9900250.0000 = 99901248.0049 with a difference
    // of order 0.0001 due to rounding errors
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("10000000.0000")));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("90000998.0049"), get_balance("alice.p"));

    total = get_total_stake("alice.p");
    bytes = total["ram_bytes"].as_uint64();
    bought_bytes = bytes - init_bytes;
    wdump((init_bytes)(bought_bytes)(bytes));

    BOOST_REQUIRE_EQUAL(success(), sellram("alice.p", bought_bytes));
    total = get_total_stake("alice.p");

    bytes = total["ram_bytes"].as_uint64();
    bought_bytes = bytes - init_bytes;
    wdump((init_bytes)(bought_bytes)(bytes));

    BOOST_REQUIRE_EQUAL(true, total["ram_bytes"].as_uint64() == init_bytes);
    BOOST_REQUIRE_EQUAL(core_sym::from_string("99901248.0045"), get_balance("alice.p"));

    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("10.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("10.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("10.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("30.0000")));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("99900688.0045"), get_balance("alice.p"));

    auto newtotal = get_total_stake("alice.p");

    auto newbytes = newtotal["ram_bytes"].as_uint64();
    bought_bytes = newbytes - bytes;
    wdump((newbytes)(bytes)(bought_bytes));

    BOOST_REQUIRE_EQUAL(success(), sellram("alice.p", bought_bytes));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("99901242.4183"), get_balance("alice.p"));

    newtotal = get_total_stake("alice.p");
    auto startbytes = newtotal["ram_bytes"].as_uint64();

    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("10000000.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("10000000.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("10000000.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("10000000.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("10000000.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("100000.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("100000.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("100000.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("300000.0000")));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("49301242.4183"), get_balance("alice.p"));

    auto finaltotal = get_total_stake("alice.p");
    auto endbytes = finaltotal["ram_bytes"].as_uint64();

    bought_bytes = endbytes - startbytes;
    wdump((startbytes)(endbytes)(bought_bytes));

    BOOST_REQUIRE_EQUAL(success(), sellram("alice.p", bought_bytes));

    BOOST_REQUIRE_EQUAL(core_sym::from_string("99396507.4129"), get_balance("alice.p"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(stake_unstake, legis_system_tester)
try
{
    activate();

    produce_blocks(10);
    produce_block(fc::hours(3 * 24));

    BOOST_REQUIRE_EQUAL(core_sym::from_string("0.0000"), get_balance("alice.p"));
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(core_sym::from_string("1000.0000"), get_balance("alice.p"));
    BOOST_REQUIRE_EQUAL(success(), stake("led", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));

    auto total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());

    const auto init_legis_stake_balance = get_balance(N(led.stake));
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));
    BOOST_REQUIRE_EQUAL(init_legis_stake_balance + core_sym::from_string("300.0000"), get_balance(N(led.stake)));
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));

    produce_block(fc::hours(3 * 24 - 1));
    produce_blocks(1);
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));
    BOOST_REQUIRE_EQUAL(init_legis_stake_balance + core_sym::from_string("300.0000"), get_balance(N(led.stake)));
    //after 3 days funds should be released
    produce_block(fc::hours(1));
    produce_blocks(1);
    BOOST_REQUIRE_EQUAL(core_sym::from_string("1000.0000"), get_balance("alice.p"));
    BOOST_REQUIRE_EQUAL(init_legis_stake_balance, get_balance(N(led.stake)));

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "bob.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));
    total = get_total_stake("bob.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());

    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000").get_amount(), total["net_weight"].as<asset>().get_amount());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000").get_amount(), total["cpu_weight"].as<asset>().get_amount());

    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("300.0000")), get_voter_info("alice.p"));

    auto bytes = total["ram_bytes"].as_uint64();
    BOOST_REQUIRE_EQUAL(true, 0 < bytes);

    //unstake from bob.p
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", "bob.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    total = get_total_stake("bob.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("10.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("10.0000"), total["cpu_weight"].as<asset>());
    produce_block(fc::hours(3 * 24 - 1));
    produce_blocks(1);
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));
    //after 3 days funds should be released
    produce_block(fc::hours(1));
    produce_blocks(1);

    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("0.0000")), get_voter_info("alice.p"));
    produce_blocks(1);
    BOOST_REQUIRE_EQUAL(core_sym::from_string("1000.0000"), get_balance("alice.p"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(stake_unstake_with_transfer, legis_system_tester)
try
{
    activate();

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    BOOST_REQUIRE_EQUAL(core_sym::from_string("0.0000"), get_balance("alice.p"));

    //led stakes for alice with transfer flag

    transfer("led", "bob.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake_with_transfer("bob.p", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));

    //check that alice has both bandwidth and voting power
    auto total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("300.0000")), get_voter_info("alice.p"));

    BOOST_REQUIRE_EQUAL(core_sym::from_string("0.0000"), get_balance("alice.p"));

    //alice stakes for herself
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    //now alice's stake should be equal to transfered from led + own stake
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("410.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("600.0000")), get_voter_info("alice.p"));

    //alice can unstake everything (including what was transfered)
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", "alice.p", core_sym::from_string("400.0000"), core_sym::from_string("200.0000")));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));

    produce_block(fc::hours(3 * 24 - 1));
    produce_blocks(1);
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));
    //after 3 days funds should be released

    produce_block(fc::hours(1));
    produce_blocks(1);

    BOOST_REQUIRE_EQUAL(core_sym::from_string("1300.0000"), get_balance("alice.p"));

    //stake should be equal to what was staked in constructor, voting power should be 0
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("10.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("10.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("0.0000")), get_voter_info("alice.p"));

    // Now alice stakes to bob with transfer flag
    BOOST_REQUIRE_EQUAL(success(), stake_with_transfer("alice.p", "bob.p", core_sym::from_string("100.0000"), core_sym::from_string("100.0000")));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(stake_to_self_with_transfer, legis_system_tester)
try
{
    activate();

    BOOST_REQUIRE_EQUAL(core_sym::from_string("0.0000"), get_balance("alice.p"));
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("cannot use transfer flag if delegating to self"),
                        stake_with_transfer("alice.p", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(stake_while_pending_refund, legis_system_tester)
try
{
    activate();

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    // issue("led.stake", core_sym::from_string("1000.0000"), config::system_account_name);
    BOOST_REQUIRE_EQUAL(core_sym::from_string("0.0000"), get_balance("alice.p"));

    //led stakes for alice with transfer flag

    transfer("led", "bob.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake_with_transfer("bob.p", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));

    //check that alice has both bandwidth and voting power
    auto total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("300.0000")), get_voter_info("alice.p"));

    BOOST_REQUIRE_EQUAL(core_sym::from_string("0.0000"), get_balance("alice.p"));

    //alice stakes for herself
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    //now alice's stake should be equal to transfered from led + own stake
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("410.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("600.0000")), get_voter_info("alice.p"));

    //alice can unstake everything (including what was transfered)
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", "alice.p", core_sym::from_string("400.0000"), core_sym::from_string("200.0000")));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));

    produce_block(fc::hours(3 * 24 - 1));
    produce_blocks(1);
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));
    //after 3 days funds should be released

    produce_block(fc::hours(1));
    produce_blocks(1);

    BOOST_REQUIRE_EQUAL(core_sym::from_string("1300.0000"), get_balance("alice.p"));

    //stake should be equal to what was staked in constructor, voting power should be 0
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("10.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("10.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("0.0000")), get_voter_info("alice.p"));

    // Now alice stakes to bob with transfer flag
    BOOST_REQUIRE_EQUAL(success(), stake_with_transfer("alice.p", "bob.p", core_sym::from_string("100.0000"), core_sym::from_string("100.0000")));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(fail_without_auth, legis_system_tester)
try
{
    activate();

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);

    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(success(), stake("led", "alice.p", core_sym::from_string("2000.0000"), core_sym::from_string("1000.0000")));
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "bob.p", core_sym::from_string("10.0000"), core_sym::from_string("10.0000")));

    BOOST_REQUIRE_EQUAL(error("missing authority of alice.p"),
                        push_action(N(alice.p), N(delegatebw), mvo()("from", "alice.p")("receiver", "bob.p")("stake_net_quantity", core_sym::from_string("10.0000"))("stake_cpu_quantity", core_sym::from_string("10.0000"))("transfer", 0), false));

    BOOST_REQUIRE_EQUAL(error("missing authority of alice.p"),
                        push_action(N(alice.p), N(undelegatebw), mvo()("from", "alice.p")("receiver", "bob.p")("unstake_net_quantity", core_sym::from_string("200.0000"))("unstake_cpu_quantity", core_sym::from_string("100.0000"))("transfer", 0), false));
    //REQUIRE_MATCHING_OBJECT( , get_voter_info( "alice.p" ) );
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(stake_negative, legis_system_tester)
try
{
    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("must stake a positive amount"),
                        stake("alice.p", core_sym::from_string("-0.0001"), core_sym::from_string("0.0000")));

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("must stake a positive amount"),
                        stake("alice.p", core_sym::from_string("0.0000"), core_sym::from_string("-0.0001")));

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("must stake a positive amount"),
                        stake("alice.p", core_sym::from_string("00.0000"), core_sym::from_string("00.0000")));

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("must stake a positive amount"),
                        stake("alice.p", core_sym::from_string("0.0000"), core_sym::from_string("00.0000"))

    );

    BOOST_REQUIRE_EQUAL(true, get_voter_info("alice.p").is_null());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(unstake_negative, legis_system_tester)
try
{
    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "bob.p", core_sym::from_string("200.0001"), core_sym::from_string("100.0001")));

    auto total = get_total_stake("bob.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0001"), total["net_weight"].as<asset>());
    auto vinfo = get_voter_info("alice.p");
    wdump((vinfo));
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("300.0002")), get_voter_info("alice.p"));

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("must unstake a positive amount"),
                        unstake("alice.p", "bob.p", core_sym::from_string("-1.0000"), core_sym::from_string("0.0000")));

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("must unstake a positive amount"),
                        unstake("alice.p", "bob.p", core_sym::from_string("0.0000"), core_sym::from_string("-1.0000")));

    //unstake all zeros
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("must unstake a positive amount"),
                        unstake("alice.p", "bob.p", core_sym::from_string("0.0000"), core_sym::from_string("0.0000"))

    );
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(unstake_more_than_at_stake, legis_system_tester)
try
{
    activate();

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));

    auto total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());

    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));

    //trying to unstake more net bandwith than at stake

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("insufficient staked net bandwidth"),
                        unstake("alice.p", core_sym::from_string("200.0001"), core_sym::from_string("0.0000")));

    //trying to unstake more cpu bandwith than at stake
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("insufficient staked cpu bandwidth"),
                        unstake("alice.p", core_sym::from_string("0.0000"), core_sym::from_string("100.0001"))

    );

    //check that nothing has changed
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(delegate_to_another_user, legis_system_tester)
try
{
    activate();

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "bob.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));

    auto total = get_total_stake("bob.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));
    //all voting power goes to alice.p
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("300.0000")), get_voter_info("alice.p"));
    //but not to bob.p
    BOOST_REQUIRE_EQUAL(true, get_voter_info("bob.p").is_null());

    //bob.p should not be able to unstake what was staked by alice.p
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("insufficient staked cpu bandwidth"),
                        unstake("bob.p", core_sym::from_string("0.0000"), core_sym::from_string("10.0000"))

    );
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("insufficient staked net bandwidth"),
                        unstake("bob.p", core_sym::from_string("10.0000"), core_sym::from_string("0.0000")));

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "carol.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(success(), stake("carol.p", "bob.p", core_sym::from_string("20.0000"), core_sym::from_string("10.0000")));
    total = get_total_stake("bob.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("230.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("120.0000"), total["cpu_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("970.0000"), get_balance("carol.p"));
    REQUIRE_MATCHING_OBJECT(voter("carol.p", core_sym::from_string("30.0000")), get_voter_info("carol.p"));

    //alice.p should not be able to unstake money staked by carol.p

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("insufficient staked net bandwidth"),
                        unstake("alice.p", "bob.p", core_sym::from_string("2001.0000"), core_sym::from_string("1.0000")));

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("insufficient staked cpu bandwidth"),
                        unstake("alice.p", "bob.p", core_sym::from_string("1.0000"), core_sym::from_string("101.0000"))

    );

    total = get_total_stake("bob.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("230.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("120.0000"), total["cpu_weight"].as<asset>());
    //balance should not change after unsuccessfull attempts to unstake
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));
    //voting power too
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("300.0000")), get_voter_info("alice.p"));
    REQUIRE_MATCHING_OBJECT(voter("carol.p", core_sym::from_string("30.0000")), get_voter_info("carol.p"));
    BOOST_REQUIRE_EQUAL(true, get_voter_info("bob.p").is_null());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(stake_unstake_separate, legis_system_tester)
try
{
    activate();

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(core_sym::from_string("1000.0000"), get_balance("alice.p"));

    //everything at once
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("10.0000"), core_sym::from_string("20.0000")));
    auto total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("20.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("30.0000"), total["cpu_weight"].as<asset>());

    //cpu
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("100.0000"), core_sym::from_string("0.0000")));
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("120.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("30.0000"), total["cpu_weight"].as<asset>());

    //net
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("0.0000"), core_sym::from_string("200.0000")));
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("120.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("230.0000"), total["cpu_weight"].as<asset>());

    //unstake cpu
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", core_sym::from_string("100.0000"), core_sym::from_string("0.0000")));
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("20.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("230.0000"), total["cpu_weight"].as<asset>());

    //unstake net
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", core_sym::from_string("0.0000"), core_sym::from_string("200.0000")));
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("20.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("30.0000"), total["cpu_weight"].as<asset>());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(adding_stake_partial_unstake, legis_system_tester)
try
{
    activate();

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "bob.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));

    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("300.0000")), get_voter_info("alice.p"));

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "bob.p", core_sym::from_string("100.0000"), core_sym::from_string("50.0000")));

    auto total = get_total_stake("bob.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("310.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("160.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("450.0000")), get_voter_info("alice.p"));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("550.0000"), get_balance("alice.p"));

    //unstake a share
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", "bob.p", core_sym::from_string("150.0000"), core_sym::from_string("75.0000")));

    total = get_total_stake("bob.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("160.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("85.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("225.0000")), get_voter_info("alice.p"));

    //unstake more
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", "bob.p", core_sym::from_string("50.0000"), core_sym::from_string("25.0000")));
    total = get_total_stake("bob.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("60.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("150.0000")), get_voter_info("alice.p"));

    //combined amount should be available only in 3 days
    produce_block(fc::days(2));
    produce_blocks(1);
    BOOST_REQUIRE_EQUAL(core_sym::from_string("550.0000"), get_balance("alice.p"));
    produce_block(fc::days(1));
    produce_blocks(1);
    BOOST_REQUIRE_EQUAL(core_sym::from_string("850.0000"), get_balance("alice.p"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(stake_from_refund, legis_system_tester)
try
{
    activate();

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));

    auto total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "bob.p", core_sym::from_string("50.0000"), core_sym::from_string("50.0000")));

    total = get_total_stake("bob.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("60.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("60.0000"), total["cpu_weight"].as<asset>());

    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("400.0000")), get_voter_info("alice.p"));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("600.0000"), get_balance("alice.p"));

    //unstake a share
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", "alice.p", core_sym::from_string("100.0000"), core_sym::from_string("50.0000")));
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("60.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("250.0000")), get_voter_info("alice.p"));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("600.0000"), get_balance("alice.p"));
    auto refund = get_refund_request("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("100.0000"), refund["net_amount"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("50.0000"), refund["cpu_amount"].as<asset>());
    //XXX auto request_time = refund["request_time"].as_int64();

    //alice delegates to bob, should pull from liquid balance not refund
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "bob.p", core_sym::from_string("50.0000"), core_sym::from_string("50.0000")));
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("60.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("350.0000")), get_voter_info("alice.p"));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("500.0000"), get_balance("alice.p"));
    refund = get_refund_request("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("100.0000"), refund["net_amount"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("50.0000"), refund["cpu_amount"].as<asset>());

    //stake less than pending refund, entire amount should be taken from refund
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "alice.p", core_sym::from_string("50.0000"), core_sym::from_string("25.0000")));
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("160.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("85.0000"), total["cpu_weight"].as<asset>());
    refund = get_refund_request("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("50.0000"), refund["net_amount"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("25.0000"), refund["cpu_amount"].as<asset>());
    //request time should stay the same
    //BOOST_REQUIRE_EQUAL( request_time, refund["request_time"].as_int64() );
    //balance should stay the same
    BOOST_REQUIRE_EQUAL(core_sym::from_string("500.0000"), get_balance("alice.p"));

    //stake exactly pending refund amount
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "alice.p", core_sym::from_string("50.0000"), core_sym::from_string("25.0000")));
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
    //pending refund should be removed
    refund = get_refund_request("alice.p");
    BOOST_TEST_REQUIRE(refund.is_null());
    //balance should stay the same
    BOOST_REQUIRE_EQUAL(core_sym::from_string("500.0000"), get_balance("alice.p"));

    //create pending refund again
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("10.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("10.0000"), total["cpu_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("500.0000"), get_balance("alice.p"));
    refund = get_refund_request("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("200.0000"), refund["net_amount"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("100.0000"), refund["cpu_amount"].as<asset>());

    //stake more than pending refund
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "alice.p", core_sym::from_string("300.0000"), core_sym::from_string("200.0000")));
    total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("310.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["cpu_weight"].as<asset>());
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("700.0000")), get_voter_info("alice.p"));
    refund = get_refund_request("alice.p");
    BOOST_TEST_REQUIRE(refund.is_null());
    //200 core tokens should be taken from alice's account
    BOOST_REQUIRE_EQUAL(core_sym::from_string("300.0000"), get_balance("alice.p"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(stake_to_another_user_not_from_refund, legis_system_tester)
try
{
    activate();

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));

    auto total = get_total_stake("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));

    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("300.0000")), get_voter_info("alice.p"));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance("alice.p"));

    //unstake
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    auto refund = get_refund_request("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("200.0000"), refund["net_amount"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("100.0000"), refund["cpu_amount"].as<asset>());
    //auto orig_request_time = refund["request_time"].as_int64();

    //stake to another user
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "bob.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    total = get_total_stake("bob.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
    //stake should be taken from alices' balance, and refund request should stay the same
    BOOST_REQUIRE_EQUAL(core_sym::from_string("400.0000"), get_balance("alice.p"));
    refund = get_refund_request("alice.p");
    BOOST_REQUIRE_EQUAL(core_sym::from_string("200.0000"), refund["net_amount"].as<asset>());
    BOOST_REQUIRE_EQUAL(core_sym::from_string("100.0000"), refund["cpu_amount"].as<asset>());
    //BOOST_REQUIRE_EQUAL( orig_request_time, refund["request_time"].as_int64() );
}
FC_LOG_AND_RETHROW()

// // Tests for voting
BOOST_FIXTURE_TEST_CASE(interior_register_unregister, legis_system_tester)
try
{
    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    //fc::variant params = producer_parameters_example(1);
    auto key = fc::crypto::public_key(std::string("EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"));
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(reginterior), mvo()("interior", "alice.p")("producer_key", key)("election_promise", "test")("url", "http://block.one")("location", 1)("logo_256", "logo")));

    auto info = get_producer_info("alice.p");
    auto info2 = get_interior_info("alice.p");
    BOOST_REQUIRE_EQUAL("alice.p", info["owner"].as_string());
    BOOST_REQUIRE_EQUAL("http://block.one", info["url"].as_string());
    BOOST_REQUIRE_EQUAL("test", info2["election_promise"].as_string());

    //change parameters one by one to check for things like #3783
    //fc::variant params2 = producer_parameters_example(2);
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(reginterior), mvo()("interior", "alice.p")("producer_key", key)("election_promise", "test")("url", "http://block.two")("location", 1)("logo_256", "logo")));
    info = get_producer_info("alice.p");
    BOOST_REQUIRE_EQUAL("alice.p", info["owner"].as_string());
    BOOST_REQUIRE_EQUAL(key, fc::crypto::public_key(info["producer_key"].as_string()));
    BOOST_REQUIRE_EQUAL("http://block.two", info["url"].as_string());
    BOOST_REQUIRE_EQUAL(1, info["location"].as_int64());

    auto key2 = fc::crypto::public_key(std::string("EOS5jnmSKrzdBHE9n8hw58y7yxFWBC8SNiG7m8S1crJH3KvAnf9o6"));
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(reginterior), mvo()("interior", "alice.p")("producer_key", key2)("election_promise", "test")("url", "http://block.two")("location", 2)("logo_256", "logo")));
    info = get_producer_info("alice.p");
    BOOST_REQUIRE_EQUAL("alice.p", info["owner"].as_string());
    BOOST_REQUIRE_EQUAL(key2, fc::crypto::public_key(info["producer_key"].as_string()));
    BOOST_REQUIRE_EQUAL("http://block.two", info["url"].as_string());
    BOOST_REQUIRE_EQUAL(2, info["location"].as_int64());
    BOOST_REQUIRE_EQUAL("logo", info["logo_256"].as_string());

    //unregister producer
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(unregprod), mvo()("producer", "alice.p")));
    info = get_producer_info("alice.p");
    //key should be empty
    BOOST_REQUIRE_EQUAL(fc::crypto::public_key(), fc::crypto::public_key(info["producer_key"].as_string()));
    //everything else should stay the same
    BOOST_REQUIRE_EQUAL("alice.p", info["owner"].as_string());
    // BOOST_REQUIRE_EQUAL(0, info["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL("http://block.two", info["url"].as_string());

    //unregister bob.p who is not a producer
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("producer not found"),
                        push_action(N(bob.p), N(unregprod), mvo()("producer", "bob.p")));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(vote_for_interior, legis_system_tester, *boost::unit_test::tolerance(1e+5))
try
{
    activate();

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    fc::variant params = producer_parameters_example(1);
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(reginterior), mvo()("interior", "alice.p")("producer_key", get_public_key(N(alice.p), "active"))("election_promise", "test")("url", "http://block.one")("location", 1)("logo_256", "logo")));
    auto prod = get_producer_info("alice.p");
    auto inte = get_interior_info("alice.p");
    BOOST_REQUIRE_EQUAL("alice.p", prod["owner"].as_string());
    BOOST_REQUIRE_EQUAL(0, inte["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL("http://block.one", prod["url"].as_string());

    issue("led", core_sym::from_string("5000.0000"), config::system_account_name);
    transfer("led", "bob.p", core_sym::from_string("2000.0000"), "led");
    transfer("led", "carol.p", core_sym::from_string("3000.0000"), "led");

    //bob.p makes stake
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("11.0000"), core_sym::from_string("0.1111")));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("1988.8889"), get_balance("bob.p"));
    REQUIRE_MATCHING_OBJECT(voter("bob.p", core_sym::from_string("11.1111")), get_voter_info("bob.p"));

    //bob.p votes for alice.p
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), {N(alice.p)}));

    //check that producer parameters stay the same after voting
    prod = get_producer_info("alice.p");
    inte = get_interior_info("alice.p");
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("11.1111")) == inte["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL("alice.p", prod["owner"].as_string());
    BOOST_REQUIRE_EQUAL("http://block.one", prod["url"].as_string());

    //carol.p makes stake
    BOOST_REQUIRE_EQUAL(success(), stake("carol.p", core_sym::from_string("22.0000"), core_sym::from_string("0.2222")));
    REQUIRE_MATCHING_OBJECT(voter("carol.p", core_sym::from_string("22.2222")), get_voter_info("carol.p"));
    BOOST_REQUIRE_EQUAL(core_sym::from_string("2977.7778"), get_balance("carol.p"));
    //carol.p votes for alice.p
    BOOST_REQUIRE_EQUAL(success(), vote(N(carol.p), {N(alice.p)}));

    //new stake votes be added to alice.p's vote_weights
    prod = get_interior_info("alice.p");
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("33.3333")) == prod["vote_weights"].as_double());

    //bob.p increases his stake
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("33.0000"), core_sym::from_string("0.3333")));
    //alice.p stake with transfer to bob.p
    BOOST_REQUIRE_EQUAL(success(), stake_with_transfer("alice.p", "bob.p", core_sym::from_string("22.0000"), core_sym::from_string("0.2222")));
    //should increase alice.p's vote_weights
    prod = get_interior_info("alice.p");
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("88.8888")) == prod["vote_weights"].as_double());

    //carol.p unstakes part of the stake
    BOOST_REQUIRE_EQUAL(success(), unstake("carol.p", core_sym::from_string("2.0000"), core_sym::from_string("0.0002") /*"2.0000 EOS", "0.0002 EOS"*/));

    //should decrease alice.p's vote_weights
    prod = get_interior_info("alice.p");
    wdump((prod));
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("86.8886")) == prod["vote_weights"].as_double());

    //bob.p revokes his vote
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), vector<account_name>()));

    //should decrease alice.p's vote_weights
    prod = get_interior_info("alice.p");
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("20.2220")) == prod["vote_weights"].as_double());
    //but eos should still be at stake
    BOOST_REQUIRE_EQUAL(core_sym::from_string("1955.5556"), get_balance("bob.p"));

    //carol.p unstakes rest of eos
    BOOST_REQUIRE_EQUAL(success(), unstake("carol.p", core_sym::from_string("20.0000"), core_sym::from_string("0.2220")));
    //should decrease alice.p's vote_weights to zero
    prod = get_interior_info("alice.p");
    BOOST_TEST_REQUIRE(0.0 == prod["vote_weights"].as_double());

    //carol.p should receive funds in 3 days
    produce_block(fc::days(3));
    produce_block();
    BOOST_REQUIRE_EQUAL(core_sym::from_string("3000.0000"), get_balance("carol.p"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(unregistered_interior_voting, legis_system_tester, *boost::unit_test::tolerance(1e+5))
try
{
    issue("led", core_sym::from_string("3000.0000"), config::system_account_name);
    transfer("led", "bob.p", core_sym::from_string("2000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("13.0000"), core_sym::from_string("0.5791")));
    REQUIRE_MATCHING_OBJECT(voter("bob.p", core_sym::from_string("13.5791")), get_voter_info("bob.p"));

    //bob.p should not be able to vote for alice.p who is not a producer
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("interior alice.p is not registered"),
                        vote(N(bob.p), {N(alice.p)}));

    //alice.p registers as a producer
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    fc::variant params = producer_parameters_example(1);
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(reginterior), mvo()("interior", "alice.p")("producer_key", get_public_key(N(alice.p), "active"))("election_promise", "test")("url", "http://block.one")("location", 1)("logo_256", "logo")));
    //and then unregisters
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(unregprod), mvo()("producer", "alice.p")));
    //key should be empty
    auto prod = get_producer_info("alice.p");
    BOOST_REQUIRE_EQUAL(fc::crypto::public_key(), fc::crypto::public_key(prod["producer_key"].as_string()));

    //bob.p should not be able to vote for alice.p who is an unregistered producer
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("interior alice.p is not currently registered"),
                        vote(N(bob.p), {N(alice.p)}));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(more_than_maximum_interior_voting, legis_system_tester)
try
{
    activate();
    issue("led", core_sym::from_string("3000.0000"), config::system_account_name);

    transfer("led", "bob.p", core_sym::from_string("2000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("13.0000"), core_sym::from_string("0.5791")));
    REQUIRE_MATCHING_OBJECT(voter("bob.p", core_sym::from_string("13.5791")), get_voter_info("bob.p"));

    const auto global_state = get_global_state();
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("attempt to vote for too many producers"),
                        vote(N(bob.p), vector<account_name>(5, N(alice.p))));
    produce_block(fc::days(190));
    produce_block();
    const auto global_state1 = get_global_state();
    BOOST_REQUIRE_EQUAL(15, global_state1["maximum_producers"].as<int64_t>());
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("interior votes must be unique and sorted"),
                        vote(N(bob.p), vector<account_name>(5, N(alice.p))));
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("attempt to vote for too many producers"),
                        vote(N(bob.p), vector<account_name>(8, N(alice.p))));

    produce_block(fc::days(190));
    produce_block();
    const auto global_state2 = get_global_state();
    BOOST_REQUIRE_EQUAL(21, global_state2["maximum_producers"].as<int64_t>());
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("interior votes must be unique and sorted"),
                        vote(N(bob.p), vector<account_name>(8, N(alice.p))));
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("attempt to vote for too many producers"),
                        vote(N(bob.p), vector<account_name>(11, N(alice.p))));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(vote_same_producer_2_times, legis_system_tester)
try
{
    BOOST_REQUIRE_EQUAL(success(), activate());

    issue("led", core_sym::from_string("3000.0000"), config::system_account_name);
    transfer("led", "bob.p", core_sym::from_string("2000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("50.0000"), core_sym::from_string("50.0000")));
    REQUIRE_MATCHING_OBJECT(voter("bob.p", core_sym::from_string("100.0000")), get_voter_info("bob.p"));

    //alice.p becomes a producer
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    fc::variant params = producer_parameters_example(1);
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(reginterior), mvo()("interior", "alice.p")("producer_key", get_public_key(N(alice.p), "active"))("election_promise", "test")("url", "http://block.one")("location", 1)("logo_256", "logo")));

    //bob.p should not be able to vote for alice.p who is not a producer
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("interior votes must be unique and sorted"),
                        vote(N(bob.p), vector<account_name>(2, N(alice.p))));

    auto prod = get_interior_info("alice.p");
    BOOST_TEST_REQUIRE(0 == prod["vote_weights"].as_double());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(interior_keep_votes, legis_system_tester, *boost::unit_test::tolerance(1e+5))
try
{
    issue("led", core_sym::from_string("3000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    fc::variant params = producer_parameters_example(1);
    vector<char> key = fc::raw::pack(get_public_key(N(alice.p), "active"));
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(reginterior), mvo()("interior", "alice.p")("producer_key", get_public_key(N(alice.p), "active"))("election_promise", "test")("url", "http://block.one")("location", 1)("logo_256", "logo")));

    //bob.p makes stake
    transfer("led", "bob.p", core_sym::from_string("2000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("13.0000"), core_sym::from_string("0.5791")));
    REQUIRE_MATCHING_OBJECT(voter("bob.p", core_sym::from_string("13.5791")), get_voter_info("bob.p"));

    //bob.p votes for alice.p
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), {N(alice.p)}));

    auto prod = get_interior_info("alice.p");
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("13.5791")) == prod["vote_weights"].as_double());

    //unregister producer
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(unregprod), mvo()("producer", "alice.p")));
    prod = get_interior_info("alice.p");
    auto prod1 = get_producer_info("alice.p");
    //key should be empty
    BOOST_REQUIRE_EQUAL(fc::crypto::public_key(), fc::crypto::public_key(prod1["producer_key"].as_string()));
    //check parameters just in case
    //REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);
    //votes should stay the same
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("13.5791")), prod["vote_weights"].as_double());

    //regtister the same producer again
    params = producer_parameters_example(2);
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(reginterior), mvo()("interior", "alice.p")("producer_key", get_public_key(N(alice.p), "active"))("election_promise", "test")("url", "http://block.one")("location", 1)("logo_256", "logo")));
    prod = get_interior_info("alice.p");
    //votes should stay the same
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("13.5791")), prod["vote_weights"].as_double());

    //change parameters
    params = producer_parameters_example(3);
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(reginterior), mvo()("interior", "alice.p")("producer_key", get_public_key(N(alice.p), "active"))("election_promise", "test")("url", "http://block.one")("location", 1)("logo_256", "logo")));
    prod = get_interior_info("alice.p");
    //votes should stay the same
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("13.5791")), prod["vote_weights"].as_double());
    //check parameters just in case
    //REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(vote_for_two_producers, legis_system_tester, *boost::unit_test::tolerance(1e+5))
try
{
    BOOST_REQUIRE_EQUAL(success(), activate());

    //alice.p becomes a producer
    fc::variant params = producer_parameters_example(1);
    auto key = get_public_key(N(alice.p), "active");
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(reginterior), mvo()("interior", "alice.p")("producer_key", key)("election_promise", "test")("url", "http://block.one")("location", 1)("logo_256", "logo")));
    //bob.p becomes a producer
    params = producer_parameters_example(2);
    key = get_public_key(N(bob.p), "active");
    BOOST_REQUIRE_EQUAL(success(), push_action(N(bob.p), N(reginterior), mvo()("interior", "bob.p")("producer_key", key)("election_promise", "test")("url", "http://block.one")("location", 1)("logo_256", "logo")));

    //carol.p votes for alice.p and bob.p
    issue("led", core_sym::from_string("1002.0000"), config::system_account_name);
    transfer("led", "carol.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("carol.p", core_sym::from_string("15.0005"), core_sym::from_string("5.0000")));
    BOOST_REQUIRE_EQUAL(success(), vote(N(carol.p), {N(alice.p), N(bob.p)}));

    auto alice_info = get_interior_info("alice.p");
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("20.0005")) == alice_info["vote_weights"].as_double());
    auto bob_info = get_interior_info("bob.p");
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("20.0005")) == bob_info["vote_weights"].as_double());

    //carol.p votes for alice.p (but revokes vote for bob.p)
    BOOST_REQUIRE_EQUAL(success(), vote(N(carol.p), {N(alice.p)}));

    alice_info = get_interior_info("alice.p");
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("20.0005")) == alice_info["vote_weights"].as_double());
    bob_info = get_interior_info("bob.p");
    BOOST_TEST_REQUIRE(0 == bob_info["vote_weights"].as_double());

    //alice.p votes for herself and bob.p
    transfer("led", "alice.p", core_sym::from_string("2.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("1.0000"), core_sym::from_string("1.0000")));
    BOOST_REQUIRE_EQUAL(success(), vote(N(alice.p), {N(alice.p), N(bob.p)}));

    alice_info = get_interior_info("alice.p");
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("22.0005")) == alice_info["vote_weights"].as_double());

    bob_info = get_interior_info("bob.p");
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("2.0000")) == bob_info["vote_weights"].as_double());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(check_global_amount, legis_system_tester)
try
{
    create_accounts_with_resources({N(defproducer1), N(frontier1.c), N(a.frontier1)});
    BOOST_REQUIRE_EQUAL(success(), activate());
    issue("led", core_sym::from_string("5000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    transfer("led", "bob.p", core_sym::from_string("1000.0000"), "led");
    transfer("led", "carol.p", core_sym::from_string("1000.0000"), "led");
    transfer("led", "defproducer1", core_sym::from_string("500.0000"), "led");
    transfer("led", "frontier1.c", core_sym::from_string("500.0000"), "led");
    transfer("led", "a.frontier1", core_sym::from_string("500.0000"), "led");

    // check inital staked amount
    auto gstate = get_global_state();
    BOOST_REQUIRE_EQUAL(1200000, gstate["total_stake_amount"].as_int64());

    // alice.p stake
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));

    // check total staeke
    gstate = get_global_state();
    BOOST_REQUIRE_EQUAL(4200000, gstate["total_stake_amount"].as_int64());

    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", core_sym::from_string("100.0000"), core_sym::from_string("50.0000")));

    gstate = get_global_state();
    BOOST_REQUIRE_EQUAL(2700000, gstate["total_stake_amount"].as_int64());
    BOOST_REQUIRE_EQUAL(0, gstate["total_vote_amount"].as_int64());
    BOOST_REQUIRE_EQUAL(0, gstate["total_purchase_amount"].as_int64());

    // producer stake
    BOOST_REQUIRE_EQUAL(success(), stake("defproducer1", core_sym::from_string("100.0000"), core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(success(), stake("frontier1.c", core_sym::from_string("100.0000"), core_sym::from_string("100.0000")));

    // check total stake
    gstate = get_global_state();
    BOOST_REQUIRE_EQUAL(6700000, gstate["total_stake_amount"].as_int64());

    // check each stake
    BOOST_REQUIRE_EQUAL(1500000, get_voter_info("alice.p")["staked"].as_double());
    BOOST_REQUIRE_EQUAL(2000000, get_voter_info("defproducer1")["staked"].as_double());
    BOOST_REQUIRE_EQUAL(2000000, get_voter_info("frontier1.c")["staked"].as_double());

    // register frontier
    auto ratio = asset::from_string("1.500 TEST");
    auto sym_name = ratio.get_symbol();
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier1.c", ratio, 1));

    // buyservice to frontier1.c
    auto amount = asset(1000000, symbol{CORE_SYMBOL});
    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, N(frontier1.c)));
    gstate = get_global_state();
    BOOST_REQUIRE_EQUAL(1000000, gstate["total_purchase_amount"].as_int64());

    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, N(frontier1.c)));
    gstate = get_global_state();
    BOOST_REQUIRE_EQUAL(2000000, gstate["total_purchase_amount"].as_int64());

    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("100.0000"), core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(success(), stake("carol.p", core_sym::from_string("100.0000"), core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(success(), buyservice(N(bob.p), amount, N(frontier1.c)));
    BOOST_REQUIRE_EQUAL(success(), buyservice(N(carol.p), amount, N(frontier1.c)));
    gstate = get_global_state();
    BOOST_REQUIRE_EQUAL(4000000, gstate["total_purchase_amount"].as_int64());

    // buyledservice to frontier1.c
    BOOST_REQUIRE_EQUAL(success(), buyledservice(N(carol.p), amount, N(frontier1.c)));
    gstate = get_global_state();
    BOOST_REQUIRE_EQUAL(5000000, gstate["total_purchase_amount"].as_int64());

    // buyledsvcdom to a.frontier1 for frontier1.c
    BOOST_REQUIRE_EQUAL(success(), buyledsvcdom(N(carol.p), amount, N(frontier1.c), N(a.frontier1)));
    gstate = get_global_state();
    BOOST_REQUIRE_EQUAL(6000000, gstate["total_purchase_amount"].as_int64());


    // pass 1 day clean purchase amount
    produce_block(fc::days(1));
    produce_blocks();

    gstate = get_global_state();
    BOOST_REQUIRE_EQUAL(0, gstate["total_purchase_amount"].as_int64());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(frontier_register_unregister, legis_system_tester)
try
{
    create_accounts_with_resources({N(frontier1.c)});
    auto ratio = asset::from_string("1.500 TEST");
    auto sym_name = ratio.symbol_name();
    auto maximum = asset::from_string("1000000.000 " + sym_name);
    const account_name &acnt = N(frontier1.c);
    base_tester::push_action(N(led.token), N(create), N(ibct), mutable_variant_object()("issuer", acnt)("maximum_supply", maximum));
    issue("frontier1.c", maximum, acnt);
    auto key = get_public_key(acnt, "active");
    action_result r = push_action(acnt, N(regfrontier), mvo()("frontier", acnt)("producer_key", key)("transfer_ratio", ratio)("category", 0)("url", "https://ibct.io")("location", 0)("logo_256", "http://ibct.io"));
    BOOST_REQUIRE_EQUAL(success(), r);

    //producer_info
    auto info = get_producer_info(acnt);
    BOOST_REQUIRE_EQUAL(1, info["is_active"].as_double());
    BOOST_REQUIRE_EQUAL(1, info["producer_type"].as_double());
    BOOST_REQUIRE_EQUAL(0, info["demerit"].as_double());
    BOOST_REQUIRE_EQUAL("http://ibct.io", info["logo_256"]);
    BOOST_REQUIRE_EQUAL("https://ibct.io", info["url"]);
    BOOST_REQUIRE_EQUAL(get_public_key(acnt, "active"), fc::crypto::public_key(info["producer_key"].as_string()));

    //frontier_info
    auto finfo = get_frontier_info(acnt);
    BOOST_REQUIRE_EQUAL(0, finfo["category"].as_double());
    BOOST_REQUIRE_EQUAL(0, finfo["service_weights"].as_double());

    // //unregister
    BOOST_REQUIRE_EQUAL(success(), push_action(acnt, N(unregprod), mvo()("producer", "frontier1.c")));
    info = get_producer_info(acnt);
    BOOST_REQUIRE_EQUAL(0, info["is_active"].as_double());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(buyservice_for_frontier, legis_system_tester)
try
{
    create_account_with_resources(N(frontier1.c), config::system_account_name);
    auto ratio = asset::from_string("1.500 TEST");
    auto sym_name = ratio.get_symbol();
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier1.c", ratio, 1));

    const account_name &acnt = N(frontier1.c);

    //frontier_info
    auto info = get_frontier_info(acnt);
    BOOST_REQUIRE_EQUAL(0, info["service_weights"].as_double());

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    auto amount = asset(1000000, symbol{CORE_SYMBOL});

    //user must stake before buyservice
    // BOOST_REQUIRE_EQUAL(wasm_assert_msg("user must stake before they can buy"), buyservice(N(alice.p), amount, acnt));

    //alice stake
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));

    //buyserivce
    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, acnt));

    //first buyservice is reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("100.0000")) == info["service_weights"].as_double());
    auto core_symbol_balance = get_balance(N(alice.p));
    auto test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(8950000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(150000, test_symbol_balance.get_amount());

    //check buyer table
    auto buyers = info["buyers"].get_array();
    BOOST_REQUIRE_EQUAL("alice.p", buyers[0]);

    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, acnt));
    //second buyservice also reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("200.0000")) == info["service_weights"].as_double());
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(7950000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(300000, test_symbol_balance.get_amount());

    //check buyer table
    buyers = info["buyers"].get_array();
    BOOST_REQUIRE_EQUAL(1, buyers.size());

    // check small amount
    amount = asset(1001000, symbol{CORE_SYMBOL});
    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, acnt));
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(6949000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(450150, test_symbol_balance.get_amount());

    amount = asset(1000100, symbol{CORE_SYMBOL});
    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, acnt));
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(5948900, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(600165, test_symbol_balance.get_amount());

    amount = asset(1000070, symbol{CORE_SYMBOL});
    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, acnt));
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(4948830, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(750175, test_symbol_balance.get_amount());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(buyledservice_for_frontier, legis_system_tester)
try
{
    create_account_with_resources(N(frontier1.c), config::system_account_name);
    auto ratio = asset::from_string("1.500 TEST");
    auto sym_name = ratio.get_symbol();
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier1.c", ratio, 1));

    const account_name &acnt = N(frontier1.c);

    //frontier_info
    auto info = get_frontier_info(acnt);
    BOOST_REQUIRE_EQUAL(0, info["service_weights"].as_double());

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    auto amount = asset(1000000, symbol{CORE_SYMBOL});

    //user must stake before buyservice
    // BOOST_REQUIRE_EQUAL(wasm_assert_msg("user must stake before they can buy"), buyledservice(N(alice.p), amount, acnt));

    //alice stake
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));

    //buyserivce
    BOOST_REQUIRE_EQUAL(success(), buyledservice(N(alice.p), amount, acnt));

    //first buyservice is reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("100.0000")) == info["service_weights"].as_double());
    auto core_symbol_balance = get_balance(N(alice.p));
    auto test_symbol_balance = get_balance(N(alice.p), sym_name);
    //no payback for buyledservice
    BOOST_REQUIRE_EQUAL(8900000, core_symbol_balance.get_amount());
    //no dapp token for buyledservice
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    //check buyer table
    auto buyers = info["buyers"].get_array();
    BOOST_REQUIRE_EQUAL("alice.p", buyers[0]);

    BOOST_REQUIRE_EQUAL(success(), buyledservice(N(alice.p), amount, acnt));
    //second buyservice also reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("200.0000")) == info["service_weights"].as_double());
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(7900000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    //check buyer table
    buyers = info["buyers"].get_array();
    BOOST_REQUIRE_EQUAL(1, buyers.size());

    // check small amount
    amount = asset(1001000, symbol{CORE_SYMBOL});
    BOOST_REQUIRE_EQUAL(success(), buyledservice(N(alice.p), amount, acnt));
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(6899000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    amount = asset(1000100, symbol{CORE_SYMBOL});
    BOOST_REQUIRE_EQUAL(success(), buyledservice(N(alice.p), amount, acnt));
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(5898900, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    amount = asset(1000070, symbol{CORE_SYMBOL});
    BOOST_REQUIRE_EQUAL(success(), buyledservice(N(alice.p), amount, acnt));
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(4898830, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(buyledsvcdom_for_frontier, legis_system_tester)
try
{
    create_account_with_resources(N(frontier1.c), config::system_account_name);
    create_account_with_resources(N(a.frontier1), config::system_account_name);
    auto ratio = asset::from_string("1.500 TEST");
    auto sym_name = ratio.get_symbol();
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier1.c", ratio, 1));

    const account_name &acnt = N(frontier1.c);
    const account_name &provider = N(a.frontier1);

    //frontier_info
    auto info = get_frontier_info(acnt);
    BOOST_REQUIRE_EQUAL(0, info["service_weights"].as_double());

    issue("led", core_sym::from_string("1000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    auto amount = asset(1000000, symbol{CORE_SYMBOL});

    //user must stake before buyledsvcdom
    // BOOST_REQUIRE_EQUAL(wasm_assert_msg("user must stake before they can buy"), buyledsvcdom(N(alice.p), amount, acnt, provider));

    //alice stake
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));

    //buyserivce
    BOOST_REQUIRE_EQUAL(success(), buyledsvcdom(N(alice.p), amount, acnt, provider));

    //first buyledsvcdom is reflect in service weight for frontier
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("100.0000")) == info["service_weights"].as_double());
    auto core_symbol_balance = get_balance(N(alice.p));
    auto test_symbol_balance = get_balance(N(alice.p), sym_name);
    //no payback for buyledservice
    BOOST_REQUIRE_EQUAL(8900000, core_symbol_balance.get_amount());
    //no dapp token for buyledservice
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    //check buyer table
    auto buyers = info["buyers"].get_array();
    BOOST_REQUIRE_EQUAL("alice.p", buyers[0]);

    BOOST_REQUIRE_EQUAL(success(), buyledsvcdom(N(alice.p), amount, acnt, provider));
    //second buyledsvcdom also reflect in service weight for frontier
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("200.0000")) == info["service_weights"].as_double());
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(7900000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    //check buyer table
    buyers = info["buyers"].get_array();
    BOOST_REQUIRE_EQUAL(1, buyers.size());

    // check small amount
    amount = asset(1001000, symbol{CORE_SYMBOL});
    BOOST_REQUIRE_EQUAL(success(), buyledsvcdom(N(alice.p), amount, acnt, provider));
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(6899000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    amount = asset(1000100, symbol{CORE_SYMBOL});
    BOOST_REQUIRE_EQUAL(success(), buyledsvcdom(N(alice.p), amount, acnt, provider));
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(5898900, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    amount = asset(1000070, symbol{CORE_SYMBOL});
    BOOST_REQUIRE_EQUAL(success(), buyledsvcdom(N(alice.p), amount, acnt, provider));
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(4898830, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(unregistered_frontier_buyservice, legis_system_tester)
try
{
    create_accounts_with_resources({N(frontier1.c), N(a.frontier1)});
    auto ratio = asset::from_string("1.500 TEST");
    auto sym_name = ratio.get_symbol();
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier1.c", ratio, 1));

    const account_name &acnt = N(frontier1.c);
    const account_name &provider = N(a.frontier1);

    //frontier_info
    auto info = get_frontier_info(acnt);
    BOOST_REQUIRE_EQUAL(0, info["service_weights"].as_double());

    issue("led", core_sym::from_string("2000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    transfer("led", "bob.p", core_sym::from_string("1000.0000"), "led");
    auto amount = asset(1000000, symbol{CORE_SYMBOL});

    //alice stake
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));
    //bob stake
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));

    BOOST_REQUIRE_EQUAL(success(), push_action(acnt, N(unregprod), mvo()("producer", "frontier1.c")));
    info = get_producer_info(acnt);
    BOOST_REQUIRE_EQUAL(0, info["is_active"].as_double());

    //buyserivce fail
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("frontier is not currently registered"), buyservice(N(alice.p), amount, acnt));
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("0.0000")) == info["service_weights"].as_double());
    auto core_symbol_balance = get_balance(N(alice.p));
    auto test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(9900000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    //buyledserivce also fail
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("frontier is not currently registered"), buyledservice(N(bob.p), amount, acnt));
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("0.0000")) == info["service_weights"].as_double());
    core_symbol_balance = get_balance(N(bob.p));
    test_symbol_balance = get_balance(N(bob.p), sym_name);
    BOOST_REQUIRE_EQUAL(9900000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    // re register frontier and buyservice
    re_regfrontier("frontier1.c", ratio, 1);
    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, acnt));

    //first buyservice is reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("100.0000")) == info["service_weights"].as_double());
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    BOOST_REQUIRE_EQUAL(8950000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(150000, test_symbol_balance.get_amount());

    //second buyservice also reflect in service weight
    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, acnt));
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("200.0000")) == info["service_weights"].as_double());
    core_symbol_balance = get_balance(N(alice.p));
    test_symbol_balance = get_balance(N(alice.p), sym_name);
    // second payback is not work
    BOOST_REQUIRE_EQUAL(7950000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(300000, test_symbol_balance.get_amount());

    // buyledservice
    BOOST_REQUIRE_EQUAL(success(), buyledservice(N(bob.p), amount, acnt));

    //first buyledservice is reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("300.0000")) == info["service_weights"].as_double());
    core_symbol_balance = get_balance(N(bob.p));
    test_symbol_balance = get_balance(N(bob.p), sym_name);
    //payback is not work with buyledservice
    BOOST_REQUIRE_EQUAL(8900000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    //second buyledservice also reflect in service weight
    BOOST_REQUIRE_EQUAL(success(), buyledservice(N(bob.p), amount, acnt));
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("400.0000")) == info["service_weights"].as_double());
    core_symbol_balance = get_balance(N(bob.p));
    test_symbol_balance = get_balance(N(bob.p), sym_name);
    BOOST_REQUIRE_EQUAL(7900000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    // buyledsvcdom
    BOOST_REQUIRE_EQUAL(success(), buyledsvcdom(N(bob.p), amount, acnt, provider));

    //first buyledsvcdom is reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("500.0000")) == info["service_weights"].as_double());
    core_symbol_balance = get_balance(N(bob.p));
    test_symbol_balance = get_balance(N(bob.p), sym_name);
    //payback is not work with buyledsvcdom
    BOOST_REQUIRE_EQUAL(6900000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());

    //second buyledsvcdom also reflect in service weight
    BOOST_REQUIRE_EQUAL(success(), buyledsvcdom(N(bob.p), amount, acnt, provider));
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("600.0000")) == info["service_weights"].as_double());
    core_symbol_balance = get_balance(N(bob.p));
    test_symbol_balance = get_balance(N(bob.p), sym_name);
    BOOST_REQUIRE_EQUAL(5900000, core_symbol_balance.get_amount());
    BOOST_REQUIRE_EQUAL(0, test_symbol_balance.get_amount());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(clean_buyers, legis_system_tester)
try
{
    create_accounts_with_resources({N(frontier1.c), N(a.frontier1)});
    auto ratio = asset::from_string("1.500 TEST");
    auto sym_name = ratio.get_symbol();
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier1.c", ratio, 1));
    BOOST_REQUIRE_EQUAL(success(), activate());
    const account_name &acnt = N(frontier1.c);
    const account_name &provider = N(a.frontier1);

    //frontier_info
    auto info = get_frontier_info(acnt);
    BOOST_REQUIRE_EQUAL(0, info["service_weights"].as_double());

    issue("led", core_sym::from_string("1500.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("500.0000"), "led");
    transfer("led", "bob.p", core_sym::from_string("500.0000"), "led");
    transfer("led", "carol.p", core_sym::from_string("500.0000"), "led");
    auto amount = asset(1000000, symbol{CORE_SYMBOL});

    //alice stake
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));
    BOOST_REQUIRE_EQUAL(success(), stake("carol.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));

    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, acnt));
    BOOST_REQUIRE_EQUAL(success(), buyledservice(N(bob.p), amount, acnt));
    BOOST_REQUIRE_EQUAL(success(), buyledsvcdom(N(carol.p), amount, acnt, provider));

    produce_blocks(250);
    auto producer_keys = control->head_block_state()->active_schedule.producers;
    BOOST_REQUIRE_EQUAL(1, producer_keys.size());
    BOOST_REQUIRE_EQUAL(name("frontier1.c"), producer_keys[0].producer_name);

    //first buyservice is reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("300.0000")) == info["service_weights"].as_double());

    //check buyers field
    auto buyers = info["buyers"].get_array();
    BOOST_REQUIRE_EQUAL(3, buyers.size());
    BOOST_REQUIRE_EQUAL("alice.p", buyers[0]);
    BOOST_REQUIRE_EQUAL("bob.p", buyers[1]);
    BOOST_REQUIRE_EQUAL("carol.p", buyers[2]);

    // 1 weeks have passed
    produce_block(fc::days(7));
    info = get_frontier_info(acnt);
    buyers = info["buyers"].get_array();
    BOOST_REQUIRE_EQUAL(3, buyers.size());
    // 2 weeks have passed
    produce_block(fc::days(7));
    info = get_frontier_info(acnt);
    buyers = info["buyers"].get_array();
    BOOST_REQUIRE_EQUAL(3, buyers.size());
    // 3 weeks have passed
    produce_block(fc::days(7));
    info = get_frontier_info(acnt);
    buyers = info["buyers"].get_array();
    BOOST_REQUIRE_EQUAL(3, buyers.size());
    // 4 weeks have passed
    produce_block(fc::days(7));
    info = get_frontier_info(acnt);
    buyers = info["buyers"].get_array();
    BOOST_REQUIRE_EQUAL(0, buyers.size());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(check_move_window, legis_system_tester)
try
{
    create_accounts_with_resources({N(frontier1.c), N(a.frontier1)});
    auto ratio = asset::from_string("1.500 TEST");
    auto sym_name = ratio.get_symbol();
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier1.c", ratio, 1));
    BOOST_REQUIRE_EQUAL(success(), activate());
    const account_name &acnt = N(frontier1.c);
    const account_name &provider = N(a.frontier1);

    //frontier_info
    auto info = get_frontier_info(acnt);
    BOOST_REQUIRE_EQUAL(0, info["service_weights"].as_double());

    //stake and buyservice
    issue("led", core_sym::from_string("1500.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("500.0000"), "led");
    transfer("led", "bob.p", core_sym::from_string("500.0000"), "led");
    transfer("led", "carol.p", core_sym::from_string("500.0000"), "led");
    auto amount = asset(1000000, symbol{CORE_SYMBOL});

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));
    BOOST_REQUIRE_EQUAL(success(), stake("carol.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));

    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, acnt));
    BOOST_REQUIRE_EQUAL(success(), buyledservice(N(bob.p), amount, acnt));
    BOOST_REQUIRE_EQUAL(success(), buyledsvcdom(N(carol.p), amount, acnt, provider));

    produce_blocks(250);
    auto producer_keys = control->head_block_state()->active_schedule.producers;
    BOOST_REQUIRE_EQUAL(1, producer_keys.size());
    BOOST_REQUIRE_EQUAL(name("frontier1.c"), producer_keys[0].producer_name);

    auto gstate = get_global_state();
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("300.0000")) == gstate["total_frontier_service_weight"].as_double());

    // first buyservice is reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("300.0000")) == info["service_weights"].as_double());

    // 30 day is passed
    for (int i = 0; i < 30; i++)
    {
        produce_block(fc::days(1));
        produce_blocks(10);
    }

    info = get_frontier_info(acnt);
    gstate = get_global_state();
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("0.0000")) == gstate["total_frontier_service_weight"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("0.0000")) == info["service_weights"].as_double());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(frontier_keep_service_weight, legis_system_tester)
try
{
    create_accounts_with_resources({N(frontier1.c), N(a.frontier1)});
    auto ratio = asset::from_string("1.500 TEST");
    auto sym_name = ratio.get_symbol();
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier1.c", ratio, 1));
    const account_name &acnt = N(frontier1.c);
    const account_name &provider = N(a.frontier1);

    //frontier_info
    auto info = get_frontier_info(acnt);
    BOOST_REQUIRE_EQUAL(0, info["service_weights"].as_double());

    //stake and buyservice
    issue("led", core_sym::from_string("1500.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("500.0000"), "led");
    transfer("led", "bob.p", core_sym::from_string("500.0000"), "led");
    auto amount = asset(1000000, symbol{CORE_SYMBOL});

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));

    BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, acnt));

    // first buyservice is reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("100.0000")) == info["service_weights"].as_double());

    // unregproducer
    BOOST_REQUIRE_EQUAL(success(), push_action(acnt, N(unregprod), mvo()("producer", "frontier1.c")));

    // service weight must same service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("100.0000")) == info["service_weights"].as_double());

    // re regfrontier must same service weight
    re_regfrontier(acnt, ratio, 1);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("100.0000")) == info["service_weights"].as_double());

    BOOST_REQUIRE_EQUAL(success(), buyledservice(N(bob.p), amount, acnt));

    // first buyledservice is reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("200.0000")) == info["service_weights"].as_double());

    // unregproducer
    BOOST_REQUIRE_EQUAL(success(), push_action(acnt, N(unregprod), mvo()("producer", "frontier1.c")));

    // service weight must same service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("200.0000")) == info["service_weights"].as_double());

    // re regfrontier must same service weight
    re_regfrontier(acnt, ratio, 1);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("200.0000")) == info["service_weights"].as_double());

    BOOST_REQUIRE_EQUAL(success(), buyledsvcdom(N(bob.p), amount, acnt, provider));

    // first buyledsvcdom is reflect in service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("300.0000")) == info["service_weights"].as_double());

    // unregproducer
    BOOST_REQUIRE_EQUAL(success(), push_action(acnt, N(unregprod), mvo()("producer", "frontier1.c")));

    // service weight must same service weight
    info = get_frontier_info(acnt);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("300.0000")) == info["service_weights"].as_double());

    // re regfrontier must same service weight
    re_regfrontier(acnt, ratio, 1);
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("300.0000")) == info["service_weights"].as_double());


}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(elect_producer /*_and_punish*/, legis_system_tester)
try
{
    //create account and register producer
    create_accounts_with_resources({N(frontier1.c), N(frontier2.c), N(frontier3.c), N(frontier4.c), N(frontier5.c),
                                    N(frontier11.c), N(frontier12.c), N(frontier13.c), N(frontier14.c), N(frontier15.c),
                                    N(defproducer1), N(defproducer2), N(defproducer3), N(defproducer4), N(defproducer5),
                                    N(dave.p)});
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier1.c", asset::from_string("1.500 TEST"), 1));
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier2.c", asset::from_string("1.500 TESA"), 2));
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier3.c", asset::from_string("1.500 TESB"), 3));
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier4.c", asset::from_string("1.500 TESC"), 4));
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier5.c", asset::from_string("1.500 TESD"), 5));
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier11.c", asset::from_string("1.500 TESE"), 6));
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier12.c", asset::from_string("1.500 TESF"), 7));
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier13.c", asset::from_string("1.500 TESG"), 8));
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier14.c", asset::from_string("1.500 TESH"), 9));
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier15.c", asset::from_string("1.500 TESI"), 10));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer1", 1));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer2", 1));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer3", 1));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer4", 1));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer5", 1));

    // activate
    BOOST_REQUIRE_EQUAL(success(), activate());

    //stake for buyservice and vote
    issue("led", core_sym::from_string("6000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1500.0000"), "led");
    transfer("led", "bob.p", core_sym::from_string("1500.0000"), "led");
    transfer("led", "carol.p", core_sym::from_string("1500.0000"), "led");
    transfer("led", "dave.p", core_sym::from_string("1500.0000"), "led");
    auto amount = asset(100000, symbol{CORE_SYMBOL});

    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("500.0000"), core_sym::from_string("500.0000")));
    BOOST_REQUIRE_EQUAL(success(), stake("carol.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));
    BOOST_REQUIRE_EQUAL(success(), stake("dave.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));

    // do buyservice
    std::vector<account_name> frontiers_name;
    std::string root("frontier");
    const std::string suffix(".c");
    for (char c = '1'; c <= '5'; ++c)
    {
        frontiers_name.emplace_back(root + std::string(1, c) + suffix);
    }
    root = "frontier1";
    for (char c = '1'; c <= '5'; ++c)
    {
        frontiers_name.emplace_back(root + std::string(1, c) + suffix);
    }
    for (const auto &p : frontiers_name)
    {
        BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, p));
    }

    std::vector<account_name> interiors_name;
    root = "defproducer";
    for (char c = '1'; c <= '3'; ++c)
    {
        interiors_name.emplace_back(root + std::string(1, c));
    }

    // vote to interiors
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), interiors_name));

    // change active producers
    produce_blocks(250);

    auto producer_keys = control->head_block_state()->active_schedule.producers;
    BOOST_REQUIRE_EQUAL(9, producer_keys.size());
    BOOST_REQUIRE_EQUAL(name("defproducer1"), producer_keys[0].producer_name);
    BOOST_REQUIRE_EQUAL(name("defproducer2"), producer_keys[1].producer_name);
    BOOST_REQUIRE_EQUAL(name("defproducer3"), producer_keys[2].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier1.c"), producer_keys[3].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier11.c"), producer_keys[4].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier12.c"), producer_keys[5].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier13.c"), producer_keys[6].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier14.c"), producer_keys[7].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier15.c"), producer_keys[8].producer_name);

    // vote to another interiors
    std::vector<account_name> old_interiors_name;
    old_interiors_name = interiors_name;
    interiors_name.clear();
    for (char c = '3'; c <= '5'; ++c)
    {
        interiors_name.emplace_back(root + std::string(1, c));
    }
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), interiors_name));

    // change active producers
    produce_blocks(500);

    // check punish producers
    for (const auto &p : frontiers_name)
    {
        auto info = get_producer_info(p);
        BOOST_REQUIRE_EQUAL(0, info["is_punished"]);
    }
    for (const auto &p : old_interiors_name)
    {
        auto info = get_producer_info(p);
        BOOST_REQUIRE_EQUAL(0, info["is_punished"]);
    }

    producer_keys = control->head_block_state()->active_schedule.producers;
    BOOST_REQUIRE_EQUAL(9, producer_keys.size());
    BOOST_REQUIRE_EQUAL(name("defproducer3"), producer_keys[0].producer_name);
    BOOST_REQUIRE_EQUAL(name("defproducer4"), producer_keys[1].producer_name);
    BOOST_REQUIRE_EQUAL(name("defproducer5"), producer_keys[2].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier1.c"), producer_keys[3].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier11.c"), producer_keys[4].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier12.c"), producer_keys[5].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier13.c"), producer_keys[6].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier14.c"), producer_keys[7].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier15.c"), producer_keys[8].producer_name);

    // buyservice and add service weight
    std::vector<account_name> old_frontiers_name;
    old_frontiers_name = frontiers_name;
    frontiers_name.clear();
    root = "frontier";
    for (char c = '1'; c <= '5'; ++c)
    {
        frontiers_name.emplace_back(root + std::string(1, c) + suffix);
    }
    for (const auto &p : frontiers_name)
    {
        BOOST_REQUIRE_EQUAL(success(), buyledservice(N(carol.p), amount, p));
    }

    // change active producers
    produce_blocks(500);

    // check punish producers
    for (const auto &p : old_frontiers_name)
    {
        auto info = get_producer_info(p);
        BOOST_REQUIRE_EQUAL(0, info["is_punished"]);
    }
    for (const auto &p : old_interiors_name)
    {
        auto info = get_producer_info(p);
        BOOST_REQUIRE_EQUAL(0, info["is_punished"]);
    }

    producer_keys = control->head_block_state()->active_schedule.producers;
    BOOST_REQUIRE_EQUAL(9, producer_keys.size());
    BOOST_REQUIRE_EQUAL(name("defproducer3"), producer_keys[0].producer_name);
    BOOST_REQUIRE_EQUAL(name("defproducer4"), producer_keys[1].producer_name);
    BOOST_REQUIRE_EQUAL(name("defproducer5"), producer_keys[2].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier1.c"), producer_keys[3].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier11.c"), producer_keys[4].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier2.c"), producer_keys[5].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier3.c"), producer_keys[6].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier4.c"), producer_keys[7].producer_name);
    BOOST_REQUIRE_EQUAL(name("frontier5.c"), producer_keys[8].producer_name);

    old_frontiers_name = frontiers_name;
    frontiers_name.clear();
    root = "frontier1";
    for (char c = '1'; c <= '5'; ++c)
    {
        frontiers_name.emplace_back(root + std::string(1, c) + suffix);
    }
    for (const auto &p : frontiers_name)
    {
        BOOST_REQUIRE_EQUAL(success(), buyservice(N(dave.p), amount, p));
    }
    old_interiors_name = interiors_name;
    interiors_name.clear();
    root = "defproducer";
    for (char c = '1'; c <= '3'; ++c)
    {
        interiors_name.emplace_back(root + std::string(1, c));
    }
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), interiors_name));

    produce_blocks(500);

    // check punish producers
    for (const auto &p : old_frontiers_name)
    {
        auto info = get_producer_info(p);
        BOOST_REQUIRE_EQUAL(0, info["is_punished"]);
    }
    for (const auto &p : old_interiors_name)
    {
        auto info = get_producer_info(p);
        BOOST_REQUIRE_EQUAL(0, info["is_punished"]);
    }

    producer_keys = control->head_block_state()->active_schedule.producers;
    BOOST_REQUIRE_EQUAL(9, producer_keys.size());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(proxy_register_unregister_keeps_stake, legis_system_tester)
try
{
    issue("led", core_sym::from_string("3000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");

    //register proxy by first action for this user ever
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(alice.p)));
    REQUIRE_MATCHING_OBJECT(proxy("alice.p")("staked", 3000000), get_voter_info("alice.p"));

    //unregister proxy
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(unregproxy), mvo()("proxy", "alice.p")));
    REQUIRE_MATCHING_OBJECT(voter("alice.p", core_sym::from_string("300.0000")), get_voter_info("alice.p"));

    //stake and then register as a proxy
    transfer("led", "bob.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(bob.p)));
    REQUIRE_MATCHING_OBJECT(proxy("bob.p")("staked", 3000000), get_voter_info("bob.p"));

    // //unrgister and check that stake is still in place
    BOOST_REQUIRE_EQUAL(success(), push_action(N(bob.p), N(unregproxy), mvo()("proxy", "bob.p")));
    REQUIRE_MATCHING_OBJECT(voter("bob.p", core_sym::from_string("300.0000")), get_voter_info("bob.p"));

    // //register as a proxy and then stake
    transfer("led", "carol.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("carol.p", core_sym::from_string("500.0000"), core_sym::from_string("30.0000")));
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(carol.p)));
    //check that both proxy flag and stake a correct
    REQUIRE_MATCHING_OBJECT(proxy("carol.p")("staked", 5300000), get_voter_info("carol.p"));

    //unregister
    BOOST_REQUIRE_EQUAL(success(), push_action(N(carol.p), N(unregproxy), mvo()("proxy", "carol.p")));
    REQUIRE_MATCHING_OBJECT(voter("carol.p", core_sym::from_string("5300000")), get_voter_info("carol.p"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(proxy_stake_unstake_keeps_proxy_flag, legis_system_tester)
try
{
    issue("led", core_sym::from_string("3000.0000"), config::system_account_name);
    transfer("led", "alice.p", core_sym::from_string("3000.0000"), "led");

    //register proxy by first action for this user ever
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(alice.p)));
    REQUIRE_MATCHING_OBJECT(proxy("alice.p")("staked", 3000000), get_voter_info("alice.p"));

    //stake
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("100.0000"), core_sym::from_string("50.0000")));
    //check that account is still a proxy
    REQUIRE_MATCHING_OBJECT(proxy("alice.p")("staked", 4500000), get_voter_info("alice.p"));

    //stake more
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("30.0000"), core_sym::from_string("20.0000")));
    //check that account is still a proxy
    REQUIRE_MATCHING_OBJECT(proxy("alice.p")("staked", 5000000), get_voter_info("alice.p"));

    //unstake more
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", core_sym::from_string("30.0000"), core_sym::from_string("20.0000")));
    REQUIRE_MATCHING_OBJECT(proxy("alice.p")("staked", 4500000), get_voter_info("alice.p"));

    //unstake the rest
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", core_sym::from_string("300.0000"), core_sym::from_string("150.0000")));
    REQUIRE_MATCHING_OBJECT(proxy("alice.p")("staked", 0), get_voter_info("alice.p"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(proxy_actions_affect_producers, legis_system_tester, *boost::unit_test::tolerance(1e+5))
try
{
    issue("led", core_sym::from_string("100000.0000"), config::system_account_name);
    BOOST_REQUIRE_EQUAL(success(), activate());
    create_accounts_with_resources({N(defproducer1), N(defproducer2), N(defproducer3)});
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer1", 1));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer2", 2));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer3", 3));

    //accumulate proxied votes
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    transfer("led", "bob.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("100.0002"), core_sym::from_string("50.0001")));
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("100.0002"), core_sym::from_string("50.0001")));
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(alice.p)));
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), vector<account_name>(), N(alice.p)));
    REQUIRE_MATCHING_OBJECT(proxy("alice.p")("staked", 1500003)("proxied_vote_weight", stake2votes(core_sym::from_string("150.0003"))), get_voter_info("alice.p"));

    //vote for producers
    BOOST_REQUIRE_EQUAL(success(), vote(N(alice.p), {N(defproducer1), N(defproducer2)}));
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("300.0006")), get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("300.0006")), get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer3")["vote_weights"].as_double());

    // //vote for another producers
    BOOST_REQUIRE_EQUAL(success(), vote(N(alice.p), {N(defproducer1), N(defproducer3)}));
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("300.0006")), get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("300.0006")), get_interior_info("defproducer3")["vote_weights"].as_double());

    // unregister proxy
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(unregproxy), mvo()("proxy", "alice.p")));
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("150.0003")), get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("150.0003")), get_interior_info("defproducer3")["vote_weights"].as_double());

    //register proxy again
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(alice.p)));
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("300.0006")), get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("300.0006")), get_interior_info("defproducer3")["vote_weights"].as_double());

    //stake decrease by proxy itself affects producers
    BOOST_REQUIRE_EQUAL(success(), unstake("alice.p", core_sym::from_string("10.0001"), core_sym::from_string("10.0001")));
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("280.0004")), get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("280.0004")), get_interior_info("defproducer3")["vote_weights"].as_double());

    //stake decrease affects producers
    BOOST_REQUIRE_EQUAL(success(), unstake("bob.p", core_sym::from_string("10.0001"), core_sym::from_string("10.0001")));
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("260.0002")), get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(stake2votes(core_sym::from_string("260.0002")), get_interior_info("defproducer1")["vote_weights"].as_double());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(producer_pay, legis_system_tester, *boost::unit_test::tolerance(1e-10))
try
{
    const double continuous_rate = 1.980 / 100;
    const double usecs_per_year = 52 * 7 * 24 * 3600 * 1000000ll;
    const double secs_per_year = 52 * 7 * 24 * 3600;

    const asset large_asset = core_sym::from_string("80.0000");
    create_account_with_resources(N(defproducera), config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset);

    BOOST_REQUIRE_EQUAL(success(), reginterior(N(defproducera)));
    produce_block(fc::hours(24));
    auto prod = get_interior_info(N(defproducera));
    BOOST_REQUIRE_EQUAL("defproducera", prod["owner"].as_string());
    BOOST_REQUIRE_EQUAL(0, prod["vote_weights"].as_double());

    // activate
    BOOST_REQUIRE_EQUAL(success(), activate());

    transfer(config::system_account_name, "alice.p", core_sym::from_string("400000000.0000"), config::system_account_name);
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("100000000.0000"), core_sym::from_string("100000000.0000")));
    BOOST_REQUIRE_EQUAL(success(), vote(N(alice.p), {N(defproducera)}));
    // defproducera is the only active producer
    // produce enough blocks so new schedule kicks in and defproducera produces some blocks
    {
        produce_blocks(250);

        const auto initial_global_state = get_global_state();
        const uint64_t initial_claim_time = microseconds_since_epoch_of_iso_string(initial_global_state["last_bucket_fill"]);
        const int64_t initial_pervote_bucket = initial_global_state["pervote_bucket"].as<int64_t>();
        const int64_t initial_perblock_bucket = initial_global_state["perblock_bucket"].as<int64_t>();
        const int64_t initial_losa = get_balance(N(losa)).get_amount();
        const int64_t initial_saving = get_balance(N(led.saving)).get_amount();
        const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();

        prod = get_producer_info("defproducera");
        const uint32_t unpaid_blocks = prod["unpaid_blocks"].as<uint32_t>();
        BOOST_REQUIRE(1 < unpaid_blocks);

        BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);

        const asset initial_supply = get_token_supply();
        const asset initial_balance = get_balance(N(defproducera));

        produce_blocks(7000);

        BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));

        const auto global_state = get_global_state();
        const uint64_t claim_time = microseconds_since_epoch_of_iso_string(global_state["last_bucket_fill"]);
        const int64_t pervote_bucket = global_state["pervote_bucket"].as<int64_t>();
        const int64_t perblock_bucket = global_state["perblock_bucket"].as<int64_t>();
        const int64_t perctb_bucket = global_state["perctb_bucket"].as<int64_t>();
        const int64_t losa = get_balance(N(losa)).get_amount();
        const int64_t saving = get_balance(N(led.saving)).get_amount();
        const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();
        const auto half_year_cnt = global_state["half_year_cnt"].as<int64_t>();
        const auto vote = global_state["total_interior_vote_weight"].as_double();

        prod = get_producer_info("defproducera");
        BOOST_REQUIRE_EQUAL(1, prod["unpaid_blocks"].as<uint32_t>());
        BOOST_REQUIRE_EQUAL(1, tot_unpaid_blocks);
        const asset supply = get_token_supply();
        const asset balance = get_balance(N(defproducera));
        const asset ctb_balance = get_balance(N(led.cpay));

        BOOST_REQUIRE_EQUAL(claim_time, microseconds_since_epoch_of_iso_string(prod["last_claim_time"]));
        auto usecs_between_fills = claim_time - initial_claim_time;
        int32_t secs_between_fills = usecs_between_fills / 1000000;

        BOOST_REQUIRE_EQUAL(0, initial_losa);
        BOOST_REQUIRE_EQUAL(0, initial_saving);
        BOOST_REQUIRE_EQUAL(0, initial_perblock_bucket);
        BOOST_REQUIRE_EQUAL(0, initial_pervote_bucket);

        // check balance of inflation
        BOOST_REQUIRE_EQUAL(static_cast<int64_t>((initial_supply.get_amount() * double(secs_between_fills) * continuous_rate) / secs_per_year),
                            supply.get_amount() - initial_supply.get_amount());
        BOOST_REQUIRE_EQUAL(static_cast<int64_t>((initial_supply.get_amount() * double(secs_between_fills) * (2 * (continuous_rate / 4. * half_year_cnt) / 5.) / secs_per_year) * 0.85) + 2,
                            balance.get_amount() - initial_balance.get_amount());
        BOOST_REQUIRE_EQUAL(static_cast<int64_t>((initial_supply.get_amount() * double(secs_between_fills) * (3 * (continuous_rate / 4. * half_year_cnt) / 5.) / secs_per_year)) + 1,
                            ctb_balance.get_amount());
        BOOST_REQUIRE_EQUAL(int64_t((initial_supply.get_amount() * double(secs_between_fills) * 3 * (continuous_rate / 4.) / secs_per_year)),
                            losa);

        // check buckect of inflation
        BOOST_REQUIRE_EQUAL(ctb_balance.get_amount(), perctb_bucket);
        BOOST_REQUIRE_EQUAL(0, perblock_bucket);
        BOOST_REQUIRE_EQUAL(0, pervote_bucket);
    }

    {
        BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                            push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));
    }

    // defproducera waits for 23 hours and 55 minutes, can't claim rewards yet
    {
        produce_block(fc::seconds(23 * 3600 + 55 * 60));

        // defproducera is punished and tries to claim rewards but he's not on the list
        BOOST_REQUIRE_EQUAL(wasm_assert_msg("producer does not have an active key"),
                            push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));

        BOOST_REQUIRE_EQUAL(success(), push_action(N(ibct), N(punishoff), mvo()("producer", "defproducera")));
        BOOST_REQUIRE_EQUAL(success(), reginterior(N(defproducera)));
        BOOST_REQUIRE_EQUAL(success(), vote(N(alice.p), {N(defproducera)}));
        BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                            push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));
    }

    // wait 5 more minutes, defproducera can now claim rewards again
    {
        produce_block(fc::seconds(5 * 60));

        const auto initial_global_state = get_global_state();
        const uint64_t initial_claim_time = microseconds_since_epoch_of_iso_string(initial_global_state["last_bucket_fill"]);
        const int64_t initial_pervote_bucket = initial_global_state["pervote_bucket"].as<int64_t>();
        const int64_t initial_perblock_bucket = initial_global_state["perblock_bucket"].as<int64_t>();
        const int64_t initial_losa = get_balance(N(losa)).get_amount();
        const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
        const double initial_tot_vote_weight = initial_global_state["total_interior_vote_weight"].as<double>();

        prod = get_producer_info("defproducera");
        const uint32_t unpaid_blocks = prod["unpaid_blocks"].as<uint32_t>();
        BOOST_REQUIRE(1 < unpaid_blocks);
        BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);
        BOOST_REQUIRE(0 < microseconds_since_epoch_of_iso_string(prod["last_claim_time"]));

        BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);

        const asset initial_supply = get_token_supply();
        const asset initial_balance = get_balance(N(defproducera));
        const asset initial_ctb_balance = get_balance(N(led.cpay));

        BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));

        const auto global_state = get_global_state();
        const uint64_t claim_time = microseconds_since_epoch_of_iso_string(global_state["last_bucket_fill"]);
        const int64_t pervote_bucket = global_state["pervote_bucket"].as<int64_t>();
        const int64_t perblock_bucket = global_state["perblock_bucket"].as<int64_t>();
        const auto half_year_cnt = global_state["half_year_cnt"].as_double();
        const int64_t losa = get_balance(N(losa)).get_amount();
        const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();

        prod = get_producer_info("defproducera");
        BOOST_REQUIRE_EQUAL(1, prod["unpaid_blocks"].as<uint32_t>());
        BOOST_REQUIRE_EQUAL(1, tot_unpaid_blocks);
        const asset supply = get_token_supply();
        const asset balance = get_balance(N(defproducera));
        const asset ctb_balance = get_balance(N(led.cpay));

        BOOST_REQUIRE_EQUAL(claim_time, microseconds_since_epoch_of_iso_string(prod["last_claim_time"]));
        auto usecs_between_fills = claim_time - initial_claim_time;

        // check balance of inflation
        BOOST_REQUIRE_EQUAL(static_cast<int64_t>((initial_supply.get_amount() * double(usecs_between_fills) * continuous_rate) / usecs_per_year),
                            supply.get_amount() - initial_supply.get_amount());
        BOOST_REQUIRE_EQUAL(static_cast<int64_t>((initial_supply.get_amount() * double(usecs_between_fills) * (2 * (continuous_rate / 4. * half_year_cnt) / 5.) / usecs_per_year) * 0.85) + 1,
                            balance.get_amount() - initial_balance.get_amount());
        BOOST_REQUIRE_EQUAL(static_cast<int64_t>((initial_supply.get_amount() * double(usecs_between_fills) * (3 * (continuous_rate / 4. * half_year_cnt) / 5.) / usecs_per_year)) + 2,
                            ctb_balance.get_amount() - initial_ctb_balance.get_amount());
        BOOST_REQUIRE_EQUAL(int64_t((initial_supply.get_amount() * double(usecs_between_fills) * 3 * (continuous_rate / 4.) / usecs_per_year)) + 1,
                            losa - initial_losa);
    }

    // test stability over a year
    {
        // pass 1 year
        produce_block(fc::days(7 * 52));

        // punishoff first
        BOOST_REQUIRE_EQUAL(success(), push_action(N(ibct), N(punishoff), mvo()("producer", "defproducera")));
        BOOST_REQUIRE_EQUAL(success(), reginterior(N(defproducera)));
        BOOST_REQUIRE_EQUAL(success(), vote(N(alice.p), {N(defproducera)}));
        const asset initial_supply = get_token_supply();

        BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));

        const asset supply = get_token_supply();

        // Amount issued per year is very close to the 2% inflation target. Small difference (500 tokens out of 20'000'000 issued)
        BOOST_REQUIRE(2000 * 10000 > int64_t(supply.get_amount() - initial_supply.get_amount()) / 10000);
    }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(multiple_producer_pay, legis_system_tester, *boost::unit_test::tolerance(1e-10))
try
{
    const int64_t secs_per_year = 52 * 7 * 24 * 3600;
    const double usecs_per_year = secs_per_year * 1000000;
    const double cont_rate = 1.980 / 100;

    const asset net = core_sym::from_string("80.0000");
    const asset cpu = core_sym::from_string("80.0000");
    const std::vector<account_name> voters = {N(producvotera), N(producvoterb), N(producvoterc)};
    for (const auto &v : voters)
    {
        create_account_with_resources(v, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu);
        transfer(config::system_account_name, v, core_sym::from_string("100000000.0000"), config::system_account_name);
        BOOST_REQUIRE_EQUAL(success(), stake(v, core_sym::from_string("30000000.0000"), core_sym::from_string("30000000.0000")));
    }
    transfer(config::system_account_name, "alice.p", core_sym::from_string("100000000.0000"), config::system_account_name);
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("30000000.0000"), core_sym::from_string("30000000.0000")));

    create_accounts_with_resources({N(frontier1.c), N(frontier2.c),
                                    N(defproducer1), N(defproducer2), N(defproducer3), N(defproducer4), N(defproducer5)});

    transfer(config::system_account_name, "frontier1.c", core_sym::from_string("10000.0000"), config::system_account_name);
    BOOST_REQUIRE_EQUAL(success(), stake("frontier1.c", core_sym::from_string("5000.0000"), core_sym::from_string("5000.0000")));
    transfer(config::system_account_name, "frontier2.c", core_sym::from_string("10000.0000"), config::system_account_name);
    BOOST_REQUIRE_EQUAL(success(), stake("frontier2.c", core_sym::from_string("5000.0000"), core_sym::from_string("5000.0000")));

    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier1.c", asset::from_string("1.500 TEST"), 1));
    BOOST_REQUIRE_EQUAL(success(), regfrontier("frontier2.c", asset::from_string("1.500 TESA"), 2));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer1", 1));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer2", 1));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer3", 1));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer4", 1));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer5", 1));

    std::vector<account_name> frontiers_name;
    std::string root("frontier");
    const std::string suffix(".c");
    for (char c = '1'; c <= '2'; ++c)
    {
        frontiers_name.emplace_back(root + std::string(1, c) + suffix);
    }
    std::vector<account_name> interiors_name;
    root = "defproducer";
    for (char c = '1'; c <= '5'; ++c)
    {
        interiors_name.emplace_back(root + std::string(1, c));
    }

    produce_block(fc::hours(24));

    {
        BOOST_REQUIRE_EQUAL(success(), activate());
        BOOST_REQUIRE_EQUAL(success(), vote(N(producvotera), vector<account_name>(interiors_name.begin(), interiors_name.begin() + 2)));
        BOOST_REQUIRE_EQUAL(success(), vote(N(producvoterb), vector<account_name>(interiors_name.begin(), interiors_name.begin() + 3)));
        BOOST_REQUIRE_EQUAL(success(), vote(N(producvoterc), vector<account_name>(interiors_name.begin(), interiors_name.begin() + 4)));
        auto amount = asset(1000000, symbol{CORE_SYMBOL});
        BOOST_REQUIRE_EQUAL(success(), buyservice(N(alice.p), amount, N(frontier1.c)));
        amount = asset(900000, symbol{CORE_SYMBOL});
        BOOST_REQUIRE_EQUAL(success(), buyledservice(N(alice.p), amount, N(frontier2.c)));
        produce_blocks(250);
    }

    {
        auto proda = get_interior_info(N(defproducer1));
        auto prodj = get_interior_info(N(defproducer2));
        auto prodk = get_interior_info(N(defproducer3));
        auto produ = get_interior_info(N(defproducer4));
        auto prodv = get_interior_info(N(defproducer5));
        auto prodf = get_frontier_info(N(frontier1.c));
        auto prodg = get_frontier_info(N(frontier2.c));

        // check vote ratios
        BOOST_REQUIRE(0 < proda["vote_weights"].as<double>() && 0 < produ["vote_weights"].as<double>());
        BOOST_TEST(proda["vote_weights"].as<double>() == prodj["vote_weights"].as<double>());
        BOOST_TEST(proda["vote_weights"].as<double>() == 3 * produ["vote_weights"].as<double>());
        BOOST_TEST(prodf["service_weights"].as<double>() > prodg["service_weights"].as<double>());
    }

    // give a chance for everyone to produce blocks
    {
        produce_blocks(6 * 12 + 20);
    }

    std::vector<double> vote_pay_i(interiors_name.size());
    std::vector<double> service_pay_f(frontiers_name.size());
    {
        double vote_weights = 0;
        for (uint32_t i = 0; i < interiors_name.size(); ++i)
        {
            vote_pay_i[i] = get_interior_info(interiors_name[i])["vote_weights"].as<double>();
            vote_weights += vote_pay_i[i];
        }
        BOOST_TEST(vote_weights == get_global_state()["total_interior_vote_weight"].as<double>());
        std::for_each(vote_pay_i.begin(), vote_pay_i.end(), [vote_weights](double &x) { x /= vote_weights; });

        BOOST_TEST(double(1) == std::accumulate(vote_pay_i.begin(), vote_pay_i.end(), double(0)));

        double service_weights = 0;
        for (uint32_t i = 0; i < frontiers_name.size(); ++i)
        {
            service_pay_f[i] = get_frontier_info(frontiers_name[i])["service_weights"].as<double>();
            service_weights += service_pay_f[i];
        }
        BOOST_TEST(service_weights == get_global_state()["total_frontier_service_weight"].as<double>());
        std::for_each(service_pay_f.begin(), service_pay_f.end(), [service_weights](double &x) { x /= service_weights; });

        BOOST_TEST(double(1) == std::accumulate(service_pay_f.begin(), service_pay_f.end(), double(0)));
    }

    {
        const uint32_t prod_index = 2;
        const auto prod_name = interiors_name[prod_index];

        const auto initial_global_state = get_global_state();
        const uint64_t initial_claim_time = microseconds_since_epoch_of_iso_string(initial_global_state["last_bucket_fill"]);
        const int64_t initial_pervote_bucket = initial_global_state["pervote_bucket"].as<int64_t>();
        const int64_t initial_perblock_bucket = initial_global_state["perblock_bucket"].as<int64_t>();
        const int64_t initial_losa = get_balance(N(losa)).get_amount();
        const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
        const asset initial_supply = get_token_supply();
        const asset initial_bpay_balance = get_balance(N(led.bpay));
        const asset initial_vpay_balance = get_balance(N(led.vpay));
        const asset initial_cpay_balance = get_balance(N(led.cpay));
        const asset initial_balance = get_balance(prod_name);
        const uint32_t initial_unpaid_blocks = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

        BOOST_REQUIRE_EQUAL(success(), push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));

        const auto global_state = get_global_state();
        const uint64_t claim_time = microseconds_since_epoch_of_iso_string(global_state["last_bucket_fill"]);
        const int64_t pervote_bucket = global_state["pervote_bucket"].as<int64_t>();
        const int64_t perblock_bucket = global_state["perblock_bucket"].as<int64_t>();
        const int64_t losa = get_balance(N(losa)).get_amount();
        const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();
        const asset supply = get_token_supply();
        const asset bpay_balance = get_balance(N(led.bpay));
        const asset vpay_balance = get_balance(N(led.vpay));
        const asset balance = get_balance(prod_name);
        const uint32_t unpaid_blocks = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

        const uint64_t usecs_between_fills = claim_time - initial_claim_time;

        const double expected_supply_growth = initial_supply.get_amount() * double(usecs_between_fills) * cont_rate / usecs_per_year;
        BOOST_REQUIRE_EQUAL(int64_t(expected_supply_growth), supply.get_amount() - initial_supply.get_amount());

        BOOST_REQUIRE(within_one(3 * int64_t(expected_supply_growth) / 4 + 1, losa - initial_losa));

        const int64_t expected_perblock_bucket = int64_t(double(initial_supply.get_amount()) * double(usecs_between_fills) * (cont_rate / 4.) / 5. / usecs_per_year);
        const int64_t expected_pervote_bucket = int64_t(double(initial_supply.get_amount()) * double(usecs_between_fills) * (cont_rate / 4.) / 5. / usecs_per_year);

        const int64_t from_perblock_bucket = int64_t(initial_unpaid_blocks * expected_perblock_bucket / initial_tot_unpaid_blocks * 0.85);
        const int64_t from_pervote_bucket = int64_t(vote_pay_i[prod_index] * expected_pervote_bucket * 0.85);

        BOOST_REQUIRE(1 >= abs(int32_t(initial_tot_unpaid_blocks - tot_unpaid_blocks) - int32_t(initial_unpaid_blocks - unpaid_blocks)));

        // For remove minimum pay
        // if (from_pervote_bucket >= 100 * 10000) 
        // {
        //     BOOST_REQUIRE(within_one(from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount()));
        //     BOOST_REQUIRE(within_one(expected_pervote_bucket - from_pervote_bucket, pervote_bucket));
        // }
        {
            BOOST_REQUIRE_EQUAL(from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount()- 1);
        }

        BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                            push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
    }

    {
        const uint32_t prod_index = 1;
        const auto prod_name = frontiers_name[prod_index];

        const auto initial_global_state = get_global_state();
        const uint64_t initial_claim_time = microseconds_since_epoch_of_iso_string(initial_global_state["last_bucket_fill"]);
        const int64_t initial_pervote_bucket = initial_global_state["pervote_bucket"].as<int64_t>();
        const int64_t initial_perblock_bucket = initial_global_state["perblock_bucket"].as<int64_t>();
        const int64_t initial_perctb_bucket = initial_global_state["perctb_bucket"].as<int64_t>();
        const int64_t initial_losa = get_balance(N(losa)).get_amount();
        const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
        const asset initial_supply = get_token_supply();
        const asset initial_bpay_balance = get_balance(N(led.bpay));
        const asset initial_vpay_balance = get_balance(N(led.vpay));
        const asset initial_cpay_balance = get_balance(N(led.cpay));
        const asset initial_balance = get_balance(prod_name);
        const uint32_t initial_unpaid_blocks = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

        BOOST_REQUIRE_EQUAL(success(), push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));

        const auto global_state = get_global_state();
        const uint64_t claim_time = microseconds_since_epoch_of_iso_string(global_state["last_bucket_fill"]);
        const int64_t pervote_bucket = global_state["pervote_bucket"].as<int64_t>();
        const int64_t perblock_bucket = global_state["perblock_bucket"].as<int64_t>();
        const int64_t perctb_bucket = global_state["perctb_bucket"].as<int64_t>();
        const int64_t losa = get_balance(N(losa)).get_amount();
        const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();
        const asset supply = get_token_supply();
        const asset bpay_balance = get_balance(N(led.bpay));
        const asset vpay_balance = get_balance(N(led.vpay));
        const asset cpay_balance = get_balance(N(led.cpay));
        const asset balance = get_balance(prod_name);
        const uint32_t unpaid_blocks = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

        const uint64_t usecs_between_fills = claim_time - initial_claim_time;

        const double expected_supply_growth = initial_supply.get_amount() * double(usecs_between_fills) * cont_rate / usecs_per_year;
        BOOST_REQUIRE_EQUAL(int64_t(expected_supply_growth), supply.get_amount() - initial_supply.get_amount());

        BOOST_REQUIRE(within_one(3 * int64_t(expected_supply_growth) / 4 + 1, losa - initial_losa));

        const int64_t expected_perblock_bucket = initial_perblock_bucket + int64_t(double(initial_supply.get_amount()) * double(usecs_between_fills) * (cont_rate / 4.) / 5. / usecs_per_year);
        const int64_t expected_perctb_bucket = initial_perctb_bucket + int64_t(double(initial_supply.get_amount()) * double(usecs_between_fills) * 3 * (cont_rate / 4.) / 5. / usecs_per_year);

        const int64_t from_perblock_bucket = int64_t(initial_unpaid_blocks * expected_perblock_bucket / initial_tot_unpaid_blocks * 0.85);
        const int64_t from_perctb_bucket = int64_t(service_pay_f[prod_index] * expected_perctb_bucket * 0.85);

        BOOST_REQUIRE(1 >= abs(int32_t(initial_tot_unpaid_blocks - tot_unpaid_blocks) - int32_t(initial_unpaid_blocks - unpaid_blocks)));

        // For remove minimum pay
        // if (from_perctb_bucket >= 100 * 10000)
        // {
        //     BOOST_REQUIRE(within_one(from_perblock_bucket + from_perctb_bucket, balance.get_amount() - initial_balance.get_amount()));
        //     BOOST_REQUIRE(within_one(expected_perctb_bucket - from_perctb_bucket, pervote_bucket));
        // }
        // else
        {
            BOOST_REQUIRE_EQUAL(from_perblock_bucket + from_perctb_bucket, balance.get_amount() - initial_balance.get_amount() - 2);
        }

        BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                            push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
    }

    // Wait for 23 hours. By now, pervote_bucket has grown enough
    // that a producer's share is more than 100 tokens.
    produce_block(fc::seconds(24 * 3600));

    {
        // punish off first
        BOOST_REQUIRE_EQUAL(success(), push_action(N(ibct), N(punishoff), mvo()("producer", "defproducer1")));
        BOOST_REQUIRE_EQUAL(success(), push_action(N(ibct), N(punishoff), mvo()("producer", "defproducer2")));
        BOOST_REQUIRE_EQUAL(success(), push_action(N(ibct), N(punishoff), mvo()("producer", "defproducer3")));
        BOOST_REQUIRE_EQUAL(success(), push_action(N(ibct), N(punishoff), mvo()("producer", "defproducer4")));
        BOOST_REQUIRE_EQUAL(success(), reginterior(N(defproducer1)));
        BOOST_REQUIRE_EQUAL(success(), reginterior(N(defproducer2)));
        BOOST_REQUIRE_EQUAL(success(), reginterior(N(defproducer3)));
        BOOST_REQUIRE_EQUAL(success(), reginterior(N(defproducer4)));
        BOOST_REQUIRE_EQUAL(success(), vote(N(producvotera), vector<account_name>(interiors_name.begin(), interiors_name.begin() + 2)));
        BOOST_REQUIRE_EQUAL(success(), vote(N(producvoterb), vector<account_name>(interiors_name.begin(), interiors_name.begin() + 3)));
        BOOST_REQUIRE_EQUAL(success(), vote(N(producvoterc), vector<account_name>(interiors_name.begin(), interiors_name.begin() + 4)));
    }
    {
        produce_blocks(500);

        const uint32_t prod_index = 1;
        const auto prod_name = interiors_name[prod_index];

        const auto initial_global_state = get_global_state();
        const uint64_t initial_claim_time = microseconds_since_epoch_of_iso_string(initial_global_state["last_bucket_fill"]);
        const int64_t initial_pervote_bucket = initial_global_state["pervote_bucket"].as<int64_t>();
        const int64_t initial_perblock_bucket = initial_global_state["perblock_bucket"].as<int64_t>();
        const int64_t initial_perctb_bucket = initial_global_state["perctb_bucket"].as<int64_t>();
        const int64_t initial_losa = get_balance(N(losa)).get_amount();
        const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
        const asset initial_supply = get_token_supply();
        const asset initial_bpay_balance = get_balance(N(led.bpay));
        const asset initial_vpay_balance = get_balance(N(led.vpay));
        const asset initial_balance = get_balance(prod_name);
        const uint32_t initial_unpaid_blocks = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

        BOOST_REQUIRE_EQUAL(success(), push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));

        const auto global_state = get_global_state();
        const uint64_t claim_time = microseconds_since_epoch_of_iso_string(global_state["last_bucket_fill"]);
        const int64_t pervote_bucket = global_state["pervote_bucket"].as<int64_t>();
        const int64_t perblock_bucket = global_state["perblock_bucket"].as<int64_t>();
        const int64_t perctb_bucket = global_state["perctb_bucket"].as<int64_t>();
        const int64_t losa = get_balance(N(losa)).get_amount();
        const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();
        const asset supply = get_token_supply();
        const asset bpay_balance = get_balance(N(led.bpay));
        const asset vpay_balance = get_balance(N(led.vpay));
        const asset balance = get_balance(prod_name);
        const uint32_t unpaid_blocks = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

        const uint64_t usecs_between_fills = claim_time - initial_claim_time;

        const double expected_supply_growth = initial_supply.get_amount() * double(usecs_between_fills) * cont_rate / usecs_per_year;
        BOOST_REQUIRE_EQUAL(int64_t(expected_supply_growth), supply.get_amount() - initial_supply.get_amount());

        BOOST_REQUIRE(within_one(3 * int64_t(expected_supply_growth) / 4, losa - initial_losa));

        const int64_t expected_perblock_bucket = int64_t(double(initial_supply.get_amount()) * double(usecs_between_fills) * cont_rate / 4. / 5. / usecs_per_year) + initial_perblock_bucket;
        const int64_t expected_pervote_bucket = int64_t(double(initial_supply.get_amount()) * double(usecs_between_fills) * cont_rate / 4. / 5. / usecs_per_year) + initial_pervote_bucket;
        const int64_t expected_perctb_bucket = int64_t(double(initial_supply.get_amount()) * 3 * double(usecs_between_fills) * cont_rate / 4. / 5. / usecs_per_year) + initial_perctb_bucket;

        const int64_t from_perblock_bucket = int64_t(initial_unpaid_blocks * expected_perblock_bucket / initial_tot_unpaid_blocks * 0.85);
        const int64_t from_pervote_bucket = int64_t(vote_pay_i[prod_index] * expected_pervote_bucket * 0.85);

        BOOST_REQUIRE(1 >= abs(int32_t(initial_tot_unpaid_blocks - tot_unpaid_blocks) - int32_t(initial_unpaid_blocks - unpaid_blocks)));
        BOOST_REQUIRE(within_one(expected_perctb_bucket, perctb_bucket));

        // For remove minimun pay
        // if (from_pervote_bucket >= 100 * 10000)
        // {
        //     // BOOST_REQUIRE(within_one(from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount()));
        //     // BOOST_REQUIRE(within_one(expected_pervote_bucket - from_pervote_bucket, pervote_bucket));
        // }
        // else
        {
            BOOST_REQUIRE_EQUAL(from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount()- 2);
        }

        produce_blocks(5);

        BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                            push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
    }

    {
        const uint32_t prod_index = 3;
        const auto prod_name = interiors_name[prod_index];
        BOOST_REQUIRE_EQUAL(success(),
                            push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
        // BOOST_REQUIRE(100 * 10000 <= get_balance(prod_name).get_amount());
        BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                            push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
    }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(voters_actions_affect_proxy_and_producers, legis_system_tester, *boost::unit_test::tolerance(1e+6))
try
{
    BOOST_REQUIRE_EQUAL(success(), activate());
    create_accounts_with_resources({N(donald.p), N(defproducer1), N(defproducer2), N(defproducer3)});
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer1", 1));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer2", 2));
    BOOST_REQUIRE_EQUAL(success(), reginterior("defproducer3", 3));

    //alice.p becomes a proxy
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("30.0001"), core_sym::from_string("20.0001")));
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(alice.p)));
    REQUIRE_MATCHING_OBJECT(proxy("alice.p")("staked", "500002"), get_voter_info("alice.p"));

    BOOST_REQUIRE_EQUAL(success(), vote(N(alice.p), {N(defproducer1), N(defproducer2)}));
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("50.0002")) == get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("50.0002")) == get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer3")["vote_weights"].as_double());

    transfer("led", "bob.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("100.0002"), core_sym::from_string("50.0001")));
    transfer("led", "donald.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("donald.p", core_sym::from_string("100.0002"), core_sym::from_string("50.0001")));
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(donald.p)));
    REQUIRE_MATCHING_OBJECT(proxy("donald.p")("staked", "1500003"), get_voter_info("donald.p"));

    //bob.p chooses alice.p as a proxy
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), vector<account_name>(), "alice.p"));
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("150.0003")) == get_voter_info("alice.p")["proxied_vote_weight"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("200.0005")) == get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("200.0005")) == get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer3")["vote_weights"].as_double());

    //carol.p chooses alice.p as a proxy
    transfer("led", "carol.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("carol.p", core_sym::from_string("30.0001"), core_sym::from_string("20.0001")));
    BOOST_REQUIRE_EQUAL(success(), vote(N(carol.p), vector<account_name>(), "alice.p"));
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("200.0005")) == get_voter_info("alice.p")["proxied_vote_weight"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("250.0007")) == get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("250.0007")) == get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer3")["vote_weights"].as_double());

    //proxied voter carol.p increases stake
    BOOST_REQUIRE_EQUAL(success(), stake("carol.p", core_sym::from_string("50.0000"), core_sym::from_string("70.0000")));
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("320.0005")) == get_voter_info("alice.p")["proxied_vote_weight"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("370.0007")) == get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("370.0007")) == get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer3")["vote_weights"].as_double());

    //proxied voter bob.p decreases stake
    BOOST_REQUIRE_EQUAL(success(), unstake("bob.p", core_sym::from_string("50.0001"), core_sym::from_string("50.0001")));
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("220.0003")) == get_voter_info("alice.p")["proxied_vote_weight"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("270.0005")) == get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("270.0005")) == get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer3")["vote_weights"].as_double());

    //proxied voter carol.p chooses another proxy
    BOOST_REQUIRE_EQUAL(success(), vote(N(carol.p), vector<account_name>(), "donald.p"));
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("50.0001")), get_voter_info("alice.p")["proxied_vote_weight"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("170.0002")), get_voter_info("donald.p")["proxied_vote_weight"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("100.0003")), get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("100.0003")), get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_REQUIRE_EQUAL(0, get_interior_info("defproducer3")["vote_weights"].as_double());

    //bob.p switches to direct voting and votes for one of the same producers, but not for another one
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), {N(defproducer2)}));
    BOOST_TEST_REQUIRE(0.0 == get_voter_info("alice.p")["proxied_vote_weight"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("50.0002")), get_interior_info("defproducer1")["vote_weights"].as_double());
    BOOST_TEST_REQUIRE(stake2votes(core_sym::from_string("100.0003")), get_interior_info("defproducer2")["vote_weights"].as_double());
    BOOST_TEST_REQUIRE(0.0 == get_interior_info("defproducer3")["vote_weights"].as_double());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(vote_both_proxy_and_producers, legis_system_tester)
try
{
    // alice.p becomes a proxy
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("30.0001"), core_sym::from_string("20.0001")));
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(alice.p)));
    REQUIRE_MATCHING_OBJECT(proxy("alice.p")("staked", "500002"), get_voter_info("alice.p"));

    //carol.p becomes a producer
    BOOST_REQUIRE_EQUAL(success(), reginterior("carol.p", 1));

    //bob.p chooses alice.p as a proxy
    transfer("led", "bob.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("100.0002"), core_sym::from_string("50.0001")));
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("cannot vote for interiors and proxy at same time"),
                        vote(N(bob.p), {N(carol.p)}, "alice.p"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(select_invalid_proxy, legis_system_tester)
try
{
    //accumulate proxied votes
    transfer("led", "bob.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("100.0002"), core_sym::from_string("50.0001")));

    //selecting account not registered as a proxy
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("invalid proxy specified"),
                        vote(N(bob.p), vector<account_name>(), "alice.p"));

    //selecting not existing account as a proxy
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("invalid proxy specified"),
                        vote(N(bob.p), vector<account_name>(), "notexist"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(double_register_unregister_proxy_keeps_votes, legis_system_tester)
try
{
    //alice.p becomes a proxy
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(alice.p)));

    edump((get_voter_info("alice.p")));
    REQUIRE_MATCHING_OBJECT(proxy("alice.p")("staked", 100000), get_voter_info("alice.p"));

    //bob.p stakes and selects alice.p as a proxy
    transfer("led", "bob.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("100.0002"), core_sym::from_string("50.0001")));
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), vector<account_name>(), "alice.p"));
    REQUIRE_MATCHING_OBJECT(proxy("alice.p")("proxied_vote_weight", stake2votes(core_sym::from_string("150.0003")))("staked", 100000), get_voter_info("alice.p"));

    //uregister
    BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(unregproxy), mvo()("proxy", "alice.p")));
    REQUIRE_MATCHING_OBJECT(voter("alice.p")("proxied_vote_weight", stake2votes(core_sym::from_string("150.0003")))("staked", 100000), get_voter_info("alice.p"));

    //double unregistering should not affect proxied_votes and stake
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("proxy does not exist"),
                        push_action(N(alice.p), N(unregproxy), mvo()("proxy", "alice.p")));
    REQUIRE_MATCHING_OBJECT(voter("alice.p")("proxied_vote_weight", stake2votes(core_sym::from_string("150.0003")))("staked", 100000), get_voter_info("alice.p"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(proxy_cannot_use_another_proxy, legis_system_tester)
try
{
    //alice.p becomes a proxy
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(alice.p)));

    //bob.p becomes a proxy
    transfer("led", "bob.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("bob.p", core_sym::from_string("5.0000"), core_sym::from_string("5.0000")));
    BOOST_REQUIRE_EQUAL(success(), regproxy(N(bob.p)));

    //proxy should not be able to use a proxy
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("account registered as a proxy is not allowed to use a proxy"),
                        vote(N(bob.p), vector<account_name>(), "alice.p"));

    //voter that uses a proxy should not be allowed to become a proxy
    transfer("led", "carol.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("carol.p", core_sym::from_string("100.0002"), core_sym::from_string("50.0001")));
    BOOST_REQUIRE_EQUAL(success(), vote(N(carol.p), vector<account_name>(), "alice.p"));
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("account registered as a proxy is not allowed to use a proxy"),
                        regproxy(N(carol.p)));

    //proxy should not be able to use itself as a proxy
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("cannot proxy to self"),
                        vote(N(bob.p), vector<account_name>(), "bob.p"));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(check_global_state2 /*_and_parameters*/, legis_system_tester)
try
{
    activate();
    transfer("led", "alice.p", core_sym::from_string("150000000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "alice.p", core_sym::from_string("3000000.0000"), core_sym::from_string("3000000.0000")));

    // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
    std::vector<account_name> producer_names;
    {
        producer_names.reserve('z' - 'a' + 1);
        const std::string root("defproducer");
        for (char c = 'a'; c < 'a' + 9; ++c)
        {
            producer_names.emplace_back(root + std::string(1, c));
        }
        setup_producer_accounts(producer_names);
        for (const auto &p : producer_names)
        {
            BOOST_REQUIRE_EQUAL(success(), reginterior(p));
        }
    }
    BOOST_REQUIRE_EQUAL(success(), vote(N(alice.p), vector<account_name>(producer_names.begin(), producer_names.begin() + 4)));
    produce_blocks(250);
    auto global_state2 = get_global_state2();
    for (int i = 0; i < 4; i++)
    {
        BOOST_REQUIRE_EQUAL(vector<account_name>(producer_names.begin(), producer_names.begin() + 5)[i].to_string(), global_state2["active_producers"].get_array()[i]);
    }
    for (int i = 0; i < 4; i++)
    {
        BOOST_REQUIRE_EQUAL(vector<account_name>(producer_names.begin(), producer_names.begin() + 5)[i].to_string(), global_state2["proposed_producers"].get_array()[i]);
    }
    BOOST_REQUIRE_EQUAL(success(), vote(N(alice.p), vector<account_name>(producer_names.begin() + 4, producer_names.begin() + 8)));
    produce_blocks(250);
    global_state2 = get_global_state2();
    for (int i = 0; i < 4; i++)
    {
        BOOST_REQUIRE_EQUAL(vector<account_name>(producer_names.begin() + 4, producer_names.begin() + 8)[i].to_string(), global_state2["active_producers"].get_array()[i]);
    }
    for (int i = 0; i < 4; i++)
    {
        BOOST_REQUIRE_EQUAL(vector<account_name>(producer_names.begin() + 4, producer_names.begin() + 8)[i].to_string(), global_state2["proposed_producers"].get_array()[i]);
    }
    BOOST_REQUIRE_EQUAL(success(), vote(N(alice.p), vector<account_name>(producer_names.begin(), producer_names.begin() + 4)));
    produce_blocks(250);
    global_state2 = get_global_state2();
    for (int i = 0; i < 4; i++)
    {
        BOOST_REQUIRE_EQUAL(vector<account_name>(producer_names.begin(), producer_names.begin() + 4)[i].to_string(), global_state2["active_producers"].get_array()[i]);
    }
    for (int i = 0; i < 4; i++)
    {
        BOOST_REQUIRE_EQUAL(vector<account_name>(producer_names.begin(), producer_names.begin() + 4)[i].to_string(), global_state2["proposed_producers"].get_array()[i]);
    }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setram_effect, legis_system_tester)
try
{
    const asset net = core_sym::from_string("8.0000");
    const asset cpu = core_sym::from_string("8.0000");
    std::vector<account_name> accounts = {N(aliceaccount), N(bobbyaccount)};
    for (const auto &a : accounts)
    {
        create_account_with_resources(a, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu);
    }

    {
        const auto name_a = accounts[0];
        transfer(config::system_account_name, name_a, core_sym::from_string("1000.0000"));
        BOOST_REQUIRE_EQUAL(core_sym::from_string("1000.0000"), get_balance(name_a));
        const uint64_t init_bytes_a = get_total_stake(name_a)["ram_bytes"].as_uint64();
        BOOST_REQUIRE_EQUAL(success(), buyram(name_a, name_a, core_sym::from_string("300.0000")));
        BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance(name_a));
        const uint64_t bought_bytes_a = get_total_stake(name_a)["ram_bytes"].as_uint64() - init_bytes_a;

        // after buying and selling balance should be 700 + 300 * 0.995 * 0.995 = 997.0075 (actually 997.0074 due to rounding fees up)
        BOOST_REQUIRE_EQUAL(success(), sellram(name_a, bought_bytes_a));
        BOOST_REQUIRE_EQUAL(core_sym::from_string("997.0074"), get_balance(name_a));
    }

    {
        const auto name_b = accounts[1];
        transfer(config::system_account_name, name_b, core_sym::from_string("1000.0000"));
        BOOST_REQUIRE_EQUAL(core_sym::from_string("1000.0000"), get_balance(name_b));
        const uint64_t init_bytes_b = get_total_stake(name_b)["ram_bytes"].as_uint64();
        // name_b buys ram at curlease price
        BOOST_REQUIRE_EQUAL(success(), buyram(name_b, name_b, core_sym::from_string("300.0000")));
        BOOST_REQUIRE_EQUAL(core_sym::from_string("700.0000"), get_balance(name_b));
        const uint64_t bought_bytes_b = get_total_stake(name_b)["ram_bytes"].as_uint64() - init_bytes_b;

        // increase max_ram_size, ram bought by name_b loses part of its value
        BOOST_REQUIRE_EQUAL(wasm_assert_msg("ram may only be increased"),
                            push_action(config::system_account_name, N(setram), mvo()("max_ram_size", 32ll * 1024 * 1024 * 1024)));
        BOOST_REQUIRE_EQUAL(error("missing authority of led"),
                            push_action(name_b, N(setram), mvo()("max_ram_size", 80ll * 1024 * 1024 * 1024)));
        BOOST_REQUIRE_EQUAL(success(),
                            push_action(config::system_account_name, N(setram), mvo()("max_ram_size", 80ll * 1024 * 1024 * 1024)));

        BOOST_REQUIRE_EQUAL(success(), sellram(name_b, bought_bytes_b));
        BOOST_REQUIRE(core_sym::from_string("1000.0000") > get_balance(name_b));
        BOOST_REQUIRE(core_sym::from_string("700.0000") < get_balance(name_b));
    }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(ram_inflation, legis_system_tester)
try
{
    const uint64_t init_max_ram_size = 32ll * 1024 * 1024 * 1024;

    BOOST_REQUIRE_EQUAL(init_max_ram_size, get_global_state()["max_ram_size"].as_uint64());
    produce_blocks(20);
    BOOST_REQUIRE_EQUAL(init_max_ram_size, get_global_state()["max_ram_size"].as_uint64());
    transfer(config::system_account_name, "alice.p", core_sym::from_string("1000.0000"), config::system_account_name);
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("100.0000")));
    produce_blocks(3);
    BOOST_REQUIRE_EQUAL(init_max_ram_size, get_global_state()["max_ram_size"].as_uint64());
    uint16_t rate = 1000;
    BOOST_REQUIRE_EQUAL(success(), push_action(config::system_account_name, N(setramrate), mvo()("bytes_per_block", rate)));
    BOOST_REQUIRE_EQUAL(rate, get_global_state()["new_ram_per_block"].as<uint16_t>());
    // last time update_ram_supply called is in buyram, num of blocks since then to
    // the block that includes the setramrate action is 1 + 3 = 4.
    // However, those 4 blocks were accumulating at a rate of 0, so the max_ram_size should not have changed.
    BOOST_REQUIRE_EQUAL(init_max_ram_size, get_global_state()["max_ram_size"].as_uint64());
    // But with additional blocks, it should start accumulating at the new rate.
    uint64_t cur_ram_size = get_global_state()["max_ram_size"].as_uint64();
    produce_blocks(10);
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("100.0000")));
    BOOST_REQUIRE_EQUAL(cur_ram_size + 11 * rate, get_global_state()["max_ram_size"].as_uint64());
    cur_ram_size = get_global_state()["max_ram_size"].as_uint64();
    produce_blocks(5);
    BOOST_REQUIRE_EQUAL(cur_ram_size, get_global_state()["max_ram_size"].as_uint64());
    BOOST_REQUIRE_EQUAL(success(), sellram("alice.p", 100));
    BOOST_REQUIRE_EQUAL(cur_ram_size + 6 * rate, get_global_state()["max_ram_size"].as_uint64());
    cur_ram_size = get_global_state()["max_ram_size"].as_uint64();
    produce_blocks();
    BOOST_REQUIRE_EQUAL(success(), buyrambytes("alice.p", "alice.p", 100));
    BOOST_REQUIRE_EQUAL(cur_ram_size + 2 * rate, get_global_state()["max_ram_size"].as_uint64());

    BOOST_REQUIRE_EQUAL(error("missing authority of led"),
                        push_action("alice.p", N(setramrate), mvo()("bytes_per_block", rate)));

    cur_ram_size = get_global_state()["max_ram_size"].as_uint64();
    produce_blocks(10);
    uint16_t old_rate = rate;
    rate = 5000;
    BOOST_REQUIRE_EQUAL(success(), push_action(config::system_account_name, N(setramrate), mvo()("bytes_per_block", rate)));
    BOOST_REQUIRE_EQUAL(cur_ram_size + 11 * old_rate, get_global_state()["max_ram_size"].as_uint64());
    produce_blocks(5);
    BOOST_REQUIRE_EQUAL(success(), buyrambytes("alice.p", "alice.p", 100));
    BOOST_REQUIRE_EQUAL(cur_ram_size + 11 * old_rate + 6 * rate, get_global_state()["max_ram_size"].as_uint64());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(legisram_ramusage, legis_system_tester)
try
{
    BOOST_REQUIRE_EQUAL(core_sym::from_string("0.0000"), get_balance("alice.p"));
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("led", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));

    const asset initial_ram_balance = get_balance(N(led.ram));
    const asset initial_ramfee_balance = get_balance(N(led.ramfee));
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("1000.0000")));

    BOOST_REQUIRE_EQUAL(false, get_row_by_account(N(led.token), N(alice.p), N(accounts), symbol{CORE_SYM}.to_symbol_code()).empty());

    //remove row
    base_tester::push_action(N(led.token), N(close), N(alice.p), mvo()("owner", "alice.p")("symbol", symbol{CORE_SYM}));
    BOOST_REQUIRE_EQUAL(true, get_row_by_account(N(led.token), N(alice.p), N(accounts), symbol{CORE_SYM}.to_symbol_code()).empty());

    auto rlm = control->get_resource_limits_manager();
    auto legisram_ram_usage = rlm.get_account_ram_usage(N(led.ram));
    auto alice_ram_usage = rlm.get_account_ram_usage(N(alice.p));

    BOOST_REQUIRE_EQUAL(success(), sellram("alice.p", 2048));

    //make sure that ram was billed to alice, not to led.ram
    BOOST_REQUIRE_EQUAL(true, alice_ram_usage < rlm.get_account_ram_usage(N(alice.p)));
    BOOST_REQUIRE_EQUAL(legisram_ram_usage, rlm.get_account_ram_usage(N(led.ram)));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(ram_gift, legis_system_tester)
try
{
    active_and_vote_producers();

    auto rlm = control->get_resource_limits_manager();
    int64_t ram_bytes_orig, net_weight, cpu_weight;
    rlm.get_account_limits(N(alice.p), ram_bytes_orig, net_weight, cpu_weight);

    /*
    * It seems impossible to write this test, because buyrambytes action doesn't give you exact amount of bytes requested
    *
   //check that it's possible to create account bying required_bytes(2724) + userres table(112) + userres row(160) - ram_gift_bytes(1400)
   create_account_with_resources( N(abcdefghklmn), N(alice.p), 2724 + 112 + 160 - 1400 );
   //check that one byte less is not enough
   BOOST_REQUIRE_THROW( create_account_with_resources( N(abcdefghklmn), N(alice.p), 2724 + 112 + 160 - 1400 - 1 ),
                        ram_usage_exceeded );
   */

    //check that stake/unstake keeps the gift
    transfer("led", "alice.p", core_sym::from_string("1000.0000"), "led");
    BOOST_REQUIRE_EQUAL(success(), stake("led", "alice.p", core_sym::from_string("200.0000"), core_sym::from_string("100.0000")));
    int64_t ram_bytes_after_stake;
    rlm.get_account_limits(N(alice.p), ram_bytes_after_stake, net_weight, cpu_weight);
    BOOST_REQUIRE_EQUAL(ram_bytes_orig, ram_bytes_after_stake);

    BOOST_REQUIRE_EQUAL(success(), unstake("led", "alice.p", core_sym::from_string("20.0000"), core_sym::from_string("10.0000")));
    int64_t ram_bytes_after_unstake;
    rlm.get_account_limits(N(alice.p), ram_bytes_after_unstake, net_weight, cpu_weight);
    BOOST_REQUIRE_EQUAL(ram_bytes_orig, ram_bytes_after_unstake);

    uint64_t ram_gift = 1400;

    int64_t ram_bytes;
    BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("1000.0000")));
    rlm.get_account_limits(N(alice.p), ram_bytes, net_weight, cpu_weight);
    auto userres = get_total_stake(N(alice.p));
    BOOST_REQUIRE_EQUAL(userres["ram_bytes"].as_uint64() + ram_gift, ram_bytes);

    BOOST_REQUIRE_EQUAL(success(), sellram("alice.p", 1024));
    rlm.get_account_limits(N(alice.p), ram_bytes, net_weight, cpu_weight);
    userres = get_total_stake(N(alice.p));
    BOOST_REQUIRE_EQUAL(userres["ram_bytes"].as_uint64() + ram_gift, ram_bytes);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setabi_bios, legis_system_tester)
try
{
    abi_serializer abi_ser(fc::json::from_string((const char *)contracts::system_abi().data()).template as<abi_def>(), abi_serializer_max_time);
    set_code(config::system_account_name, contracts::bios_wasm());
    set_abi(config::system_account_name, contracts::bios_abi().data());
    // create_account(N(led.token));
    // set_abi(N(led.token), contracts::token_abi().data());
    // {
    //     auto res = get_row_by_account(config::system_account_name, config::system_account_name, N(abihash), N(led.token));
    //     _abi_hash abi_hash;
    //     auto abi_hash_var = abi_ser.binary_to_variant("abi_hash", res, abi_serializer_max_time);
    //     abi_serializer::from_variant(abi_hash_var, abi_hash, get_resolver(), abi_serializer_max_time);
    //     auto abi = fc::raw::pack(fc::json::from_string((const char *)contracts::token_abi().data()).template as<abi_def>());
    //     auto result = fc::sha256::hash((const char *)abi.data(), abi.size());

    //     BOOST_REQUIRE(abi_hash.hash == result);
    // }

    // set_abi(N(led.token), contracts::system_abi().data());
    // {
    //     auto res = get_row_by_account(config::system_account_name, config::system_account_name, N(abihash), N(led.token));
    //     _abi_hash abi_hash;
    //     auto abi_hash_var = abi_ser.binary_to_variant("abi_hash", res, abi_serializer_max_time);
    //     abi_serializer::from_variant(abi_hash_var, abi_hash, get_resolver(), abi_serializer_max_time);
    //     auto abi = fc::raw::pack(fc::json::from_string((const char *)contracts::system_abi().data()).template as<abi_def>());
    //     auto result = fc::sha256::hash((const char *)abi.data(), abi.size());

    //     BOOST_REQUIRE(abi_hash.hash == result);
    // }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setabi, legis_system_tester)
try
{
    set_abi(N(led.token), contracts::token_abi().data());
    {
        auto res = get_row_by_account(config::system_account_name, config::system_account_name, N(abihash), N(led.token));
        _abi_hash abi_hash;
        auto abi_hash_var = abi_ser.binary_to_variant("abi_hash", res, abi_serializer_max_time);
        abi_serializer::from_variant(abi_hash_var, abi_hash, get_resolver(), abi_serializer_max_time);
        auto abi = fc::raw::pack(fc::json::from_string((const char *)contracts::token_abi().data()).template as<abi_def>());
        auto result = fc::sha256::hash((const char *)abi.data(), abi.size());

        BOOST_REQUIRE(abi_hash.hash == result);
    }

    set_abi(N(led.token), contracts::system_abi().data());
    {
        auto res = get_row_by_account(config::system_account_name, config::system_account_name, N(abihash), N(led.token));
        _abi_hash abi_hash;
        auto abi_hash_var = abi_ser.binary_to_variant("abi_hash", res, abi_serializer_max_time);
        abi_serializer::from_variant(abi_hash_var, abi_hash, get_resolver(), abi_serializer_max_time);
        auto abi = fc::raw::pack(fc::json::from_string((const char *)contracts::system_abi().data()).template as<abi_def>());
        auto result = fc::sha256::hash((const char *)abi.data(), abi.size());

        BOOST_REQUIRE(abi_hash.hash == result);
    }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setparams, legis_system_tester)
try
{
    //install multisig contract
    abi_serializer msig_abi_ser = initialize_multisig();
    auto producer_names = active_and_vote_producers();

    //helper function
    auto push_action_msig = [&](const account_name &signer, const action_name &name, const variant_object &data, bool auth = true) -> action_result {
        string action_type_name = msig_abi_ser.get_action_type(name);

        action act;
        act.account = N(led.msig);
        act.name = name;
        act.data = msig_abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

        return base_tester::push_action(std::move(act), auth ? uint64_t(signer) : signer == N(bob.p) ? N(alice.p) : N(bob.p));
    };

    // test begins
    vector<permission_level> prod_perms;
    for (auto &x : producer_names)
    {
        prod_perms.push_back({name(x), config::active_name});
    }

    eosio::chain::chain_config params;
    params = control->get_global_properties().configuration;
    //change some values
    params.max_block_net_usage += 10;
    params.max_transaction_lifetime += 1;

    transaction trx;
    {
        variant pretty_trx = fc::mutable_variant_object()("expiration", "2023-01-01T00:30")("ref_block_num", 2)("ref_block_prefix", 3)("net_usage_words", 0)("max_cpu_usage_ms", 0)("delay_sec", 0)("actions", fc::variants({fc::mutable_variant_object()("account", name(config::system_account_name))("name", "setparams")("authorization", vector<permission_level>{{config::system_account_name, config::active_name}})("data", fc::mutable_variant_object()("params", params))}));
        abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
    }

    BOOST_REQUIRE_EQUAL(success(), push_action_msig(N(alice.p), N(propose), mvo()("proposer", "alice.p")("proposal_name", "setparams1")("trx", trx)("requested", prod_perms)));

    // get 16 approvals
    for (size_t i = 0; i < 15; ++i)
    {
        BOOST_REQUIRE_EQUAL(success(), push_action_msig(name(producer_names[i]), N(approve), mvo()("proposer", "alice.p")("proposal_name", "setparams1")("level", permission_level{name(producer_names[i]), config::active_name})));
    }

    transaction_trace_ptr trace;
    control->applied_transaction.connect(
        [&](std::tuple<const transaction_trace_ptr &, const signed_transaction &> p) {
            const auto &t = std::get<0>(p);
            if (t->scheduled)
            {
                trace = t;
            }
        });

    BOOST_REQUIRE_EQUAL(success(), push_action_msig(N(alice.p), N(exec), mvo()("proposer", "alice.p")("proposal_name", "setparams1")("executer", "alice.p")));

    BOOST_REQUIRE(bool(trace));
    BOOST_REQUIRE_EQUAL(1, trace->action_traces.size());
    BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

    produce_blocks(250);

    // make sure that changed parameters were applied
    auto active_params = control->get_global_properties().configuration;
    BOOST_REQUIRE_EQUAL(params.max_block_net_usage, active_params.max_block_net_usage);
    BOOST_REQUIRE_EQUAL(params.max_transaction_lifetime, active_params.max_transaction_lifetime);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(producers_upgrade_system_contract, legis_system_tester)
try
{
    //install multisig contract
    abi_serializer msig_abi_ser = initialize_multisig();
    auto producer_names = active_and_vote_producers();

    //helper function
    auto push_action_msig = [&](const account_name &signer, const action_name &name, const variant_object &data, bool auth = true) -> action_result {
        string action_type_name = msig_abi_ser.get_action_type(name);

        action act;
        act.account = N(led.msig);
        act.name = name;
        act.data = msig_abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

        return base_tester::push_action(std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice.p) : N(bob111111111));
    };
    // test begins
    vector<permission_level> prod_perms;
    for (auto &x : producer_names)
    {
        prod_perms.push_back({name(x), config::active_name});
    }

    transaction trx;
    {
        //prepare system contract with diffelease hash (contract differs in one byte)
        auto code = contracts::system_wasm();
        string msg = "interior votes must be unique and sorted";
        auto it = std::search(code.begin(), code.end(), msg.begin(), msg.end());
        BOOST_REQUIRE(it != code.end());
        std::copy(msg.begin(), msg.end(), it);

        variant pretty_trx = fc::mutable_variant_object()("expiration", "2021-12-31T00:30")("ref_block_num", 2)("ref_block_prefix", 3)("net_usage_words", 0)("max_cpu_usage_ms", 0)("delay_sec", 0)("actions", fc::variants({fc::mutable_variant_object()("account", name(config::system_account_name))("name", "setcode")("authorization", vector<permission_level>{{config::system_account_name, config::active_name}})("data", fc::mutable_variant_object()("account", name(config::system_account_name))("vmtype", 0)("vmversion", "0")("code", bytes(code.begin(), code.end())))}));
        abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
    }

    BOOST_REQUIRE_EQUAL(success(), push_action_msig(N(alice.p), N(propose), mvo()("proposer", "alice.p")("proposal_name", "upgrade1")("trx", trx)("requested", prod_perms)));

    // get 15 approvals
    for (size_t i = 0; i < 14; ++i)
    {
        BOOST_REQUIRE_EQUAL(success(), push_action_msig(name(producer_names[i]), N(approve), mvo()("proposer", "alice.p")("proposal_name", "upgrade1")("level", permission_level{name(producer_names[i]), config::active_name})));
    }

    //should fail
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("transaction authorization failed"),
                        push_action_msig(N(alice.p), N(exec), mvo()("proposer", "alice.p")("proposal_name", "upgrade1")("executer", "alice.p")));

    // one more approval
    BOOST_REQUIRE_EQUAL(success(), push_action_msig(name(producer_names[14]), N(approve), mvo()("proposer", "alice.p")("proposal_name", "upgrade1")("level", permission_level{name(producer_names[14]), config::active_name})));

    transaction_trace_ptr trace;
    control->applied_transaction.connect(
        [&](std::tuple<const transaction_trace_ptr &, const signed_transaction &> p) {
            const auto &t = std::get<0>(p);
            if (t->scheduled)
            {
                trace = t;
            }
        });

    BOOST_REQUIRE_EQUAL(success(), push_action_msig(N(alice.p), N(exec), mvo()("proposer", "alice.p")("proposal_name", "upgrade1")("executer", "alice.p")));

    // BOOST_REQUIRE( bool(trace) );
    // BOOST_REQUIRE_EQUAL( 1, trace->action_traces.size() );
    // BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace->receipt->status );

    // produce_blocks( 250 );
}
FC_LOG_AND_RETHROW()
BOOST_AUTO_TEST_SUITE_END()
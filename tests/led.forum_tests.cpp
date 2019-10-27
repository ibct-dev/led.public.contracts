#include <boost/test/unit_test.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>
#include "led.system_tester.hpp"

#include "Runtime/Runtime.h"

#include <fc/variant_object.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

class legis_forum_tester : public tester
{
public:
    legis_forum_tester()
    {
        produce_blocks(2);

        create_accounts({N(alice.p), N(bob.p), N(carol.p), N(led.forum), N(ibct)});
        produce_blocks(2);

        set_code(N(led.forum), contracts::forum_wasm());
        set_abi(N(led.forum), contracts::forum_abi().data());

        produce_blocks();

        const auto &accnt = control->db().get<account_object, by_name>(N(led.forum));
        abi_def abi;
        BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
        abi_ser.set_abi(abi, abi_serializer_max_time);
    }

    action_result push_action(const account_name &signer, const action_name &name, const variant_object &data)
    {
        string action_type_name = abi_ser.get_action_type(name);

        action act;
        act.account = N(led.forum);
        act.name = name;
        act.data = abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

        return base_tester::push_action(std::move(act), uint64_t(signer));
    }
    fc::variant get_proposal(account_name code, account_name proposal)
    {
        vector<char> data = get_row_by_account(code, code, N(proposal), proposal);
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("proposal_row", data, abi_serializer_max_time);
    }

    fc::variant get_vote(uint16_t id)
    {
        vector<char> data = get_row_by_account(N(led.forum), N(led.forum), N(vote), id);
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("vote_row", data, abi_serializer_max_time);
    }

    action_result propose(const account_name &proposer,
                          const account_name &proposal_name,
                          const string &title,
                          const string &proposal_json,
                          const time_point_sec &expires_at)
    {
        return push_action(proposer, N(propose), mvo()("proposer", proposer)("proposal_name", proposal_name)("title", title)("proposal_json", proposal_json)("expires_at", expires_at));
    }

    action_result expire(const account_name proposer, const account_name proposal_name)
    {
        return push_action(proposer, N(expire), mvo()("proposal_name", proposal_name));
    }

    action_result vote(const account_name voter,
                       const account_name proposal_name,
                       uint8_t vote,
                       const string &vote_json)
    {
        return push_action(voter, N(vote), mvo()("voter", voter)("proposal_name", proposal_name)("vote", vote)("vote_json", vote_json));
    }

    action_result unvote(const account_name voter, const account_name proposal_name)
    {
        return push_action(voter, N(unvote), mvo()("voter", voter)("proposal_name", proposal_name));
    }

    action_result clnproposal(const account_name proposer, const account_name proposal_name)
    {
        return push_action(proposer, N(clnproposal), mvo()("proposal_name", proposal_name));
    }

    action_result pasproposal(const account_name proposer, const account_name proposal_name)
    {
        return push_action(proposer, N(pasproposal), mvo()("proposal_name", proposal_name));
    }

    constexpr static uint32_t SIX_MONTHS_IN_SECONDS = (uint32_t)(6 * (365.25 / 12) * 24 * 60 * 60);

    constexpr static uint32_t FREEZE_PERIOD_IN_SECONDS = 3 * 24 * 60 * 60;

    abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(legis_forum_tests)

BOOST_FIXTURE_TEST_CASE(createproposal_clnproposal, legis_forum_tester)
try
{
    auto proposal = N(update);
    std::string title("update");
    std::string proposal_json("{'content':'update','type':'t1'}");
    auto expire_at = time_point_sec(control->head_block_time()) + SIX_MONTHS_IN_SECONDS;

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("propose is only available for KYC accounts"), propose(N(ibct), proposal, title, proposal_json, expire_at));
    BOOST_REQUIRE_EQUAL(success(), propose(N(alice.p), proposal, title, proposal_json, expire_at));
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), proposal, 0, "{}"));

    BOOST_REQUIRE_EQUAL("update", get_proposal(N(led.forum), proposal)["proposal_name"].get_string());
    BOOST_REQUIRE_EQUAL(1, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL("bob.p", get_vote(0)["voter"].get_string());

    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), proposal, 1, "{}"));

    BOOST_REQUIRE_EQUAL(0, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL(1, get_proposal(N(led.forum), proposal)["count_agree"].as_uint64());

    BOOST_REQUIRE_EQUAL("bob.p", get_vote(0)["voter"].get_string());

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("proposal must exist or be expired for to running clproposal."),
                        clnproposal(N(alice.p), proposal));

    BOOST_REQUIRE_EQUAL(success(), expire(N(alice.p), proposal));

    BOOST_REQUIRE_EQUAL(error("missing authority of ibct"), clnproposal(N(alice.p), proposal));
    BOOST_REQUIRE_EQUAL(success(), clnproposal(N(ibct), proposal));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(vote_unvote_passproposal, legis_forum_tester)
try
{
    std::vector<account_name> voters_name;
    {
        voters_name.reserve('z' - 'a' + 1);
        const std::string prefix("kyc");
        const std::string suffix(".p");
        for (char c = 'a'; c < 'a' + 21; ++c)
        {
            voters_name.emplace_back(prefix + std::string(1, c) + suffix);
        }
        create_accounts(voters_name);
    }

    auto proposal = N(update);
    std::string title("update");
    std::string proposal_json("{'content':'update','type':'t1'}");
    auto expire_at = time_point_sec(control->head_block_time()) + SIX_MONTHS_IN_SECONDS;

    BOOST_REQUIRE_EQUAL(success(), propose(N(alice.p), proposal, title, proposal_json, expire_at));
    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), proposal, 0, "{}"));
    BOOST_REQUIRE_EQUAL(wasm_assert_msg("vote is only available for KYC accounts"), vote(N(ibct), proposal, 0, "{}"));

    BOOST_REQUIRE_EQUAL("update", get_proposal(N(led.forum), proposal)["proposal_name"].get_string());
    BOOST_REQUIRE_EQUAL(1, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL("bob.p", get_vote(0)["voter"].get_string());

    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), proposal, 0, "{}"));

    BOOST_REQUIRE_EQUAL(1, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL(0, get_proposal(N(led.forum), proposal)["count_agree"].as_uint64());

    BOOST_REQUIRE_EQUAL("bob.p", get_vote(0)["voter"].get_string());

    BOOST_REQUIRE_EQUAL(wasm_assert_msg("proposal must exist or be expired for to running clproposal."),
                        clnproposal(N(alice.p), proposal));

    int8_t i = 0;
    for (const auto &p : voters_name)
    {
        BOOST_REQUIRE_EQUAL(success(), vote(p, proposal, i % 2, "{}"));
        i++;
    }

    BOOST_REQUIRE_EQUAL(12, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL(10, get_proposal(N(led.forum), proposal)["count_agree"].as_uint64());

    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), proposal, 1, "{}"));

    i = 1;
    for (const auto &p : voters_name)
    {
        BOOST_REQUIRE_EQUAL(success(), vote(p, proposal, i % 2, "{}"));
        i++;
    }

    BOOST_REQUIRE_EQUAL(10, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL(12, get_proposal(N(led.forum), proposal)["count_agree"].as_uint64());

    for (const auto &p : voters_name)
    {
        BOOST_REQUIRE_EQUAL(success(), vote(p, proposal, 1, "{}"));
        i++;
    }

    for (const auto &p : voters_name)
    {
        BOOST_REQUIRE_EQUAL(success(), vote(p, proposal, 1, "{}"));
    }

    BOOST_REQUIRE_EQUAL(0, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL(22, get_proposal(N(led.forum), proposal)["count_agree"].as_uint64());

    for (const auto &p : voters_name)
    {
        BOOST_REQUIRE_EQUAL(success(), vote(p, proposal, 0, "{}"));
    }

    BOOST_REQUIRE_EQUAL(success(), vote(N(bob.p), proposal, 0, "{}"));

    BOOST_REQUIRE_EQUAL(22, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL(0, get_proposal(N(led.forum), proposal)["count_agree"].as_uint64());

    BOOST_REQUIRE_EQUAL(success(), unvote(N(bob.p), proposal));

    BOOST_REQUIRE_EQUAL(21, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL(0, get_proposal(N(led.forum), proposal)["count_agree"].as_uint64());

    BOOST_REQUIRE_EQUAL(success(), expire(N(alice.p), proposal));

    for (const auto &p : voters_name)
    {
        BOOST_REQUIRE_EQUAL(success(), unvote(p, proposal));
    }

    BOOST_REQUIRE_EQUAL(21, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL(0, get_proposal(N(led.forum), proposal)["count_agree"].as_uint64());

    BOOST_REQUIRE_EQUAL(0, get_proposal(N(led.forum), proposal)["pass"].as_uint64());
    BOOST_REQUIRE_EQUAL(success(), pasproposal(N(ibct), proposal));
    BOOST_REQUIRE_EQUAL(1, get_proposal(N(led.forum), proposal)["pass"].as_uint64());

    BOOST_REQUIRE_EQUAL(success(), clnproposal(N(ibct), proposal));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(clnproposal_test, legis_forum_tester)
try
{
    std::vector<account_name> voters_name;
    {
        voters_name.reserve('z' - 'a' + 1);
        const std::string prefix("kyc");
        const std::string suffix(".p");
        for (char c = 'a'; c < 'a' + 21; ++c)
        {
            voters_name.emplace_back(prefix + std::string(1, c) + suffix);
        }
        create_accounts(voters_name);
    }

    auto proposal = N(update);
    std::string title("update");
    std::string proposal_json("{'content':'update','type':'t1'}");
    auto expire_at = time_point_sec(control->head_block_time()) + SIX_MONTHS_IN_SECONDS;

    BOOST_REQUIRE_EQUAL(success(), propose(N(alice.p), proposal, title, proposal_json, expire_at));

    int8_t i = 0;
    for (const auto &p : voters_name)
    {
        BOOST_REQUIRE_EQUAL(success(), vote(p, proposal, i % 2, "{}"));
        i++;
    }

    BOOST_REQUIRE_EQUAL(11, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL(10, get_proposal(N(led.forum), proposal)["count_agree"].as_uint64());

    i = 0;
    for (const auto &p : voters_name)
    {
        BOOST_REQUIRE_EQUAL(p.to_string(), get_vote(i)["voter"].get_string());
        i++;
    }

    proposal = N(update1);
    BOOST_REQUIRE_EQUAL(success(), propose(N(alice.p), proposal, title, proposal_json, expire_at));

    BOOST_REQUIRE_EQUAL("update1", get_proposal(N(led.forum), proposal)["proposal_name"].get_string());

    i = 0;
    for (const auto &p : voters_name)
    {
        BOOST_REQUIRE_EQUAL(success(), vote(p, proposal, i % 2, "{}"));
        i++;
    }

    BOOST_REQUIRE_EQUAL(11, get_proposal(N(led.forum), proposal)["count_disagree"].as_uint64());
    BOOST_REQUIRE_EQUAL(10, get_proposal(N(led.forum), proposal)["count_agree"].as_uint64());

    for (const auto &p : voters_name)
    {
        BOOST_REQUIRE_EQUAL(p.to_string(), get_vote(i)["voter"].get_string());
        i++;
    }

    BOOST_REQUIRE_EQUAL(success(), expire(N(alice.p), proposal));

    BOOST_REQUIRE_EQUAL(success(), clnproposal(N(ibct), proposal));

    // check another voters
    i = 0;
    for (const auto &p : voters_name)
    {
        BOOST_REQUIRE_EQUAL(p.to_string(), get_vote(i)["voter"].get_string());
        i++;
    }
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
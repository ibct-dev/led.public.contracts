#pragma once

#include <algorithm>
#include <string>

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

namespace eosio
{

using eosio::check;
using eosio::const_mem_fun;
using eosio::current_time_point;
using eosio::datastream;
using eosio::indexed_by;
using eosio::name;
using eosio::time_point_sec;
using std::function;
using std::string;

class[[eosio::contract("led.forum")]] forum : public contract
{
public:
    using contract::contract;

    [[eosio::action]] void propose(
        const name proposer,
        const name proposal_name,
        const string &title,
        const string &proposal_json,
        const time_point_sec &expires_at);

    [[eosio::action]] void expire(const name proposal_name);

    [[eosio::action]] void vote(
        const name voter,
        const name proposal_name,
        uint8_t vote,
        const string &vote_json);

    [[eosio::action]] void unvote(const name voter, const name proposal_name);

    [[eosio::action]] void clnproposal(const name proposal_name);

    [[eosio::action]] void pasproposal(const name proposal_name);

private:
    // 6 months in seconds (Computatio: 6 months * average days per month * 24 hours * 60 minutes * 60 seconds)
    constexpr static uint32_t SIX_MONTHS_IN_SECONDS = (uint32_t)(6 * (365.25 / 12) * 24 * 60 * 60);

    static inline time_point_sec current_time_point_sec()
    {
        return time_point_sec(current_time_point());
    }

    static uint128_t compute_by_proposal_key(const name proposal_name, const name voter)
    {
        return ((uint128_t)proposal_name.value) << 64 | voter.value;
    }

    static uint128_t compute_by_voter_key(const name proposal_name, const name voter)
    {
        return ((uint128_t)voter.value) << 64 | proposal_name.value;
    }

    struct [[eosio::table]] proposal_row
    {
        name proposal_name;
        name proposer;
        string title;
        string proposal_json;
        time_point_sec created_at;
        time_point_sec expires_at;
        int64_t count_agree = 0;
        int64_t count_disagree = 0;
        bool pass = false;

        auto primary_key() const { return proposal_name.value; }
        uint64_t by_proposer() const { return proposer.value; }
        bool is_expired() const { return current_time_point_sec() >= expires_at; }
    };
    typedef eosio::multi_index<
        "proposal"_n, proposal_row,
        indexed_by<"byproposer"_n, const_mem_fun<proposal_row, uint64_t, &proposal_row::by_proposer>>>
        proposals;

    struct [[eosio::table]] vote_row
    {
        uint64_t id;
        name proposal_name;
        name voter;
        uint8_t vote;
        string vote_json;
        time_point_sec updated_at;

        auto primary_key() const { return id; }
        uint128_t by_proposal() const { return forum::compute_by_proposal_key(proposal_name, voter); }
        uint128_t by_voter() const { return forum::compute_by_voter_key(proposal_name, voter); }
    };
    typedef eosio::multi_index<
        "vote"_n, vote_row,
        indexed_by<"byproposal"_n, const_mem_fun<vote_row, uint128_t, &vote_row::by_proposal>>,
        indexed_by<"byvoter"_n, const_mem_fun<vote_row, uint128_t, &vote_row::by_voter>>>
        votes;

    void update_vote(
        votes & vote_table,
        const name proposal_name,
        const name voter,
        uint8_t vote,
        const function<void(vote_row &)> updater);

    // Do not use directly, use the VALIDATE_JSON macro instead!
    void validate_json(
        const string &payload,
        size_t max_size,
        const char *not_object_message,
        const char *over_size_message);

    bool isKYC(const name &owner);
};

} // namespace eosio

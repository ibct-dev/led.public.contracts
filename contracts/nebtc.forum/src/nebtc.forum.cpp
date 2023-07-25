#include "nebtc.forum/nebtc.forum.hpp"

#define VALIDATE_JSON(Variable, MAX_SIZE)                   \
    eosio::forum::validate_json(                            \
        Variable,                                           \
        MAX_SIZE,                                           \
        #Variable " must be a JSON object (if specified).", \
        #Variable " should be shorter than " #MAX_SIZE " bytes.")

namespace eosio
{

void forum::propose(
    const name proposer,
    const name proposal_name,
    const string &title,
    const string &proposal_json,
    const time_point_sec &expires_at)
{
    require_auth(proposer);

    check(isKYC(proposer), "propose is only available for KYC accounts");

    check(proposal_name.length() > 2, "proposal name should be at least 3 characters long.");
    check(title.size() < 1024, "title should be less than 1024 characters long.");
    VALIDATE_JSON(proposal_json, 32768);

    check(expires_at > current_time_point_sec(), "expires_at must be a value in the future.");

    // Not a perfect assertion since we are not doing real date computation, but good enough for our use case
    time_point_sec max_expires_at = current_time_point_sec() + SIX_MONTHS_IN_SECONDS;
    check(expires_at <= max_expires_at, "expires_at must be within 6 months from now.");

    proposals proposal_table(_self, _self.value);
    check(proposal_table.find(proposal_name.value) == proposal_table.end(), "proposal with same name already exists.");

    proposal_table.emplace(proposer, [&](auto &row) {
        row.proposal_name = proposal_name;
        row.proposer = proposer;
        row.title = title;
        row.proposal_json = proposal_json;
        row.created_at = current_time_point_sec();
        row.expires_at = expires_at;
        row.pass = false;
    });
}

void forum::expire(const name proposal_name)
{
    proposals proposal_table(_self, _self.value);
    auto itr = proposal_table.find(proposal_name.value);
    check(itr != proposal_table.end(), "proposal not found.");
    check(!itr->is_expired(), "proposal is already expired.");

    auto proposer = itr->proposer;
    require_auth(proposer);

    proposal_table.modify(itr, proposer, [&](auto &row) {
        row.expires_at = current_time_point_sec();
    });
}

void forum::vote(
    const name voter,
    const name proposal_name,
    uint8_t vote,
    const string &vote_json)
{
    require_auth(voter);

    check(isKYC(voter), "vote is only available for KYC accounts");
    check(vote < 2, "vote must be 0 or 1");
    proposals proposal_table(_self, _self.value);
    auto &row = proposal_table.get(proposal_name.value, "proposal_name does not exist.");

    check(!row.is_expired(), "cannot vote on an expired proposal.");

    VALIDATE_JSON(vote_json, 8192);

    votes vote_table(_self, _self.value);
    update_vote(vote_table, proposal_name, voter, vote, [&](auto &row) {
        row.vote = vote;
        row.vote_json = vote_json;
    });
}

void forum::unvote(const name voter, const name proposal_name)
{
    require_auth(voter);

    proposals proposal_table(_self, _self.value);
    auto &row = proposal_table.get(proposal_name.value, "proposal_name does not exist.");

    votes vote_table(_self, _self.value);

    auto index = vote_table.template get_index<"byproposal"_n>();
    auto vote_key = compute_by_proposal_key(proposal_name, voter);

    auto itr = index.find(vote_key);
    check(itr != index.end(), "no vote exists for this proposal_name/voter pair.");

    if (!row.is_expired())
    {
        auto itr1 = proposal_table.find(proposal_name.value);
        if (itr->vote)
        {
            proposal_table.modify(itr1, same_payer, [&](auto &row) {
                row.count_agree--;
            });
        }
        else
        {
            proposal_table.modify(itr1, same_payer, [&](auto &row) {
                row.count_disagree--;
            });
        }
    }

    vote_table.erase(*itr);
}

/**
         * This method does **not** require any authorization, here is the reasoning for that.
         *
         * This method allows anyone to clean a proposal if the proposal is either expired or does
         * not exist anymore. This exact case can only happen either by itself (the proposal has reached
         * its expiration time) or by the a proposer action (`expire`). In either case, 3 days must elapse before calling `clnproposal`.
         *
         * In all cases it's ok to let anyone clean the votes since there is no more "use"
         * for the proposal nor the votes.
         */
void forum::clnproposal(const name proposal_name)
{
    proposals proposal_table(_self, _self.value);

    auto itr = proposal_table.find(proposal_name.value);
    check(itr == proposal_table.end() || itr->is_expired(), "proposal must exist or be expired for to running clproposal.");

    require_auth("ibct"_n);

    votes vote_table(_self, _self.value);
    auto index = vote_table.template get_index<"byproposal"_n>();

    auto vote_key_lower_bound = compute_by_proposal_key(proposal_name, name(0x0000000000000000));
    auto vote_key_upper_bound = compute_by_proposal_key(proposal_name, name(0xFFFFFFFFFFFFFFFF));

    auto lower_itr = index.lower_bound(vote_key_lower_bound);
    auto upper_itr = index.upper_bound(vote_key_upper_bound);

    while (lower_itr != upper_itr)
    {
        lower_itr = index.erase(lower_itr);
    }

    // Let's delete the actual proposal if we deleted all votes and the proposal still exists
    if (lower_itr == upper_itr && itr != proposal_table.end())
    {
        proposal_table.erase(itr);
    }
}

void forum::update_vote(
    votes &vote_table,
    const name proposal_name,
    const name voter,
    uint8_t vote,
    const function<void(vote_row &)> updater)
{
    auto index = vote_table.template get_index<"byproposal"_n>();
    proposals proposal_table(_self, _self.value);
    auto vote_key = compute_by_proposal_key(proposal_name, voter);

    auto itr = index.find(vote_key);
    auto itr1 = proposal_table.find(proposal_name.value);
    if (itr == index.end())
    {
        vote_table.emplace(voter, [&](auto &row) {
            row.id = vote_table.available_primary_key();
            row.proposal_name = proposal_name;
            row.voter = voter;
            row.updated_at = current_time_point_sec();
            updater(row);
        });
        if (vote)
        {
            proposal_table.modify(itr1, same_payer, [&](auto &row) {
                row.count_agree++;
            });
        }
        else
        {
            proposal_table.modify(itr1, same_payer, [&](auto &row) {
                row.count_disagree++;
            });
        }
    }
    else
    {
        if (vote != itr->vote)
        {
            if (vote)
            {
                proposal_table.modify(itr1, same_payer, [&](auto &row) {
                    row.count_disagree--;
                    row.count_agree++;
                });
            }
            else
            {
                proposal_table.modify(itr1, same_payer, [&](auto &row) {
                    row.count_disagree++;
                    row.count_agree--;
                });
            }
        }
        index.modify(itr, voter, [&](auto &row) {
            row.updated_at = current_time_point_sec();
            updater(row);
        });
    }
}

void forum::pasproposal(const name proposal_name)
{
    require_auth("ibct"_n);

    proposals proposal_table(_self, _self.value);
    auto itr = proposal_table.find(proposal_name.value);
    check(itr != proposal_table.end(), "proposal must exist");

    proposal_table.modify(itr, same_payer, [&](auto &row) {
        row.pass = true;
    });
}

// Do not use directly, use the VALIDATE_JSON macro instead!
void forum::validate_json(
    const string &payload,
    size_t max_size,
    const char *not_object_message,
    const char *over_size_message)
{
    if (payload.size() <= 0)
        return;

    check(payload[0] == '{', not_object_message);
    check(payload.size() < max_size, over_size_message);
}

bool forum::isKYC(const name &owner)
{
    const auto suffix = owner.suffix();
    return ("c"_n == suffix || "p"_n == suffix);
}
} // namespace eosio

EOSIO_DISPATCH(eosio::forum, (propose)(expire)(vote)(unvote)(clnproposal)(pasproposal))

#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <string>
#include <vector>

namespace eosiosystem {
    class system_contract;
}

namespace eosio {
    using std::string;
    using std::vector;
    typedef uint64_t id_type;

    class[[eosio::contract("led.token")]] token : public contract {
    public:
        using contract::contract;

        ACTION create( const name& issuer, const asset& maximum_supply );

        ACTION changeissuer( const name& to, const symbol& target );

        ACTION issue( const name& to, const asset& quantity, const string& memo );

        ACTION burn( const name& owner, const asset& quantity, const string& memo );

        ACTION burnfrom( const name& burner, const name& owner, const asset& quantity, const string& memo );

        ACTION transfer( const name& from, const name& to, const asset& quantity, const string& memo );

        ACTION approve( const name& owner, const name& spender, const asset& quantity );

        ACTION transferfrom( const name& spender, const name& from, const name& to, const asset& quantity, const string& memo );

        ACTION incallowance( const name& owner, const asset& quantity );

        ACTION decallowance( const name& owner, const asset& quantity );

        ACTION open( const name& owner, const symbol& symbol, const name& ram_payer );

        ACTION close( const name& owner, const symbol& symbol );

        ACTION createnft( const name& issuer, const string& sym );  // nft

        ACTION issuenft( const name& to, const asset& quantity, const vector<std::pair<uint64_t, name>>& token_infos, const string& memo );

        ACTION burnnft( const name& owner, const asset& quantity, const vector<uint64_t>& token_ids, const string& memo );

        ACTION burnnftfrom( const name& burner, const string& sym, id_type token_id, const string& memo );

        ACTION send( const name& from, const name& to, const string& sym, id_type token_id, const string& memo );

        ACTION approvenft( const name& owner, const name& spender, const string& sym, id_type token_id );

        ACTION sendfrom( const name& spender, const name& to, const string& sym, id_type token_id, const string& memo );

        ACTION auctiontoken( const name& auctioneer, const string& sym, id_type token_id, const asset& min_price, uint32_t sec );

        ACTION bidtoken( const name& bidder, const string& sym, id_type token_id, const asset& bid );

        ACTION claimtoken( const name& requester, const string& sym, id_type token_id );

        ACTION freeze( const name& account );

        ACTION unfreeze( const name& account );

        // dummy action
        ACTION bidresult( const asset& bid_key_currency ){};

        using bidresult_action = action_wrapper<"bidresult"_n, &token::bidresult>;

        static asset get_supply( const name& token_contract_account, const symbol_code& sym_code ) {
            stats statstable(token_contract_account, sym_code.raw());
            const auto& st = statstable.get(sym_code.raw());
            return st.supply;
        }

        static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code ) {
            accounts accountstable(token_contract_account, owner.value);
            const auto& ac = accountstable.get(sym_code.raw());
            return ac.balance;
        }

        using create_action = eosio::action_wrapper<"create"_n, &token::create>;
        using createnft_action = eosio::action_wrapper<"createnft"_n, &token::createnft>;
        using issue_action = eosio::action_wrapper<"issue"_n, &token::issue>;
        using burn_action = eosio::action_wrapper<"burn"_n, &token::burn>;
        using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
        using open_action = eosio::action_wrapper<"open"_n, &token::open>;
        using close_action = eosio::action_wrapper<"close"_n, &token::close>;
        using freeze_action = eosio::action_wrapper<"freeze"_n, &token::freeze>;
        using unfreeze_action = eosio::action_wrapper<"unfreeze"_n, &token::unfreeze>;

    private:
        TABLE frozen {
            name freezer;

            uint64_t primary_key() const { return freezer.value; }
        };

        TABLE account {
            asset balance;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

        TABLE allowance {
            asset balance;
            name spender;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

        TABLE currency_stats {
            asset supply;
            asset max_supply;
            name issuer;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
            uint64_t get_issuer() const { return issuer.value; }
        };

        TABLE nft {
            id_type token_id;  // Unique 64 bit identifier,
            name owner;        // token owner
            name tokenName;    // token name
            name spender;      // token spender

            id_type primary_key() const { return token_id; }
            uint64_t get_owner() const { return owner.value; }
        };

        TABLE nft_bid {
            id_type token_id;
            name high_bidder;
            asset high_bid;
            time_point_sec deadline;

            id_type primary_key() const { return token_id; }
        };
        typedef eosio::multi_index<"frozens"_n, frozen> frozens;

        typedef eosio::multi_index<"accounts"_n, account> accounts;

        typedef eosio::multi_index<"allowances"_n, allowance> allowances;

        typedef eosio::multi_index<"stat"_n, currency_stats,
                                indexed_by<"byissuer"_n, const_mem_fun<currency_stats, uint64_t, &currency_stats::get_issuer>>>
            stats;

        typedef eosio::multi_index<"nft"_n, nft,
                                indexed_by<"byowner"_n, const_mem_fun<nft, uint64_t, &nft::get_owner>>>
            nfts;

        typedef eosio::multi_index<"nftbids"_n, nft_bid> nft_bids;

        void sub_balance( const name& owner, const asset& value );
        void add_balance( const name& owner, const asset& value, const name& ram_payer );
        void check_nft( const asset& quantity );
        void check_frozen( const name& account );
    };

}  /// namespace eosio

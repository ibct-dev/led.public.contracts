#include "led.token/led.token.hpp"

namespace eosio {
    ACTION token::create( const name& issuer, const asset& maximum_supply ) {
        if (has_auth( "ibct"_n )) {
            require_auth( "ibct"_n );
        }
        else {
            require_auth(get_self());
        }

        check(is_account(issuer), "issuer account does not exist");
        check_frozen( issuer );

        asset mx_sup = maximum_supply;

        auto sym = mx_sup.symbol;
        check(sym.is_valid(), "invalid symbol name");
        check(mx_sup.is_valid(), "invalid supply");
        check(mx_sup.amount != asset::max_amount, "token must not be nft");
        if (!mx_sup.amount) mx_sup.amount = asset::max_amount - 1;
        check(mx_sup.amount > 0, "max-supply must be positive");

        stats statstable(get_self(), sym.code().raw());
        auto existing_st = statstable.find(sym.code().raw());
        check(existing_st == statstable.end(), "token with symbol already exists");

        statstable.emplace(get_self(), [&](auto& s) {
            s.supply.symbol = mx_sup.symbol;
            s.max_supply = mx_sup;
            s.issuer = issuer;
        });
    }

    ACTION token::changeissuer( const name& to, const symbol& target ) {
        check(is_account(to), "to account does not exist");

        check_frozen( to );

        auto target_token_supply = eosio::token::get_supply(get_self(), target.code() );
        check( target_token_supply.symbol == target, "specified target symbol does not exist (precision mismatch)" );

        auto sym = target_token_supply.symbol;
        check(sym.is_valid(), "invalid symbol name");

        stats statstable(get_self(), sym.code().raw());
        auto existing_st = statstable.find(sym.code().raw());
        check(existing_st != statstable.end(), "token with symbol does not exist");
        const auto& st = *existing_st;

        require_auth(st.issuer);

        auto payer = has_auth(to) ? to : same_payer;

        statstable.modify(st, payer, [&](auto& s) {
            s.issuer = to;
        });
    }

    ACTION token::issue( const name& to, const asset& quantity, const string& memo ) {
        auto sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        check_nft(quantity);
        check_frozen( to );

        stats statstable(get_self(), sym.code().raw());
        auto existing_st = statstable.find(sym.code().raw());
        check(existing_st != statstable.end(), "token with symbol does not exist, create token before issue");
        const auto& st = *existing_st;

        check( to == st.issuer, "tokens can only be issued to issuer account" );

        require_auth(st.issuer);

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must issue positive quantity");
        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
        check(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

        statstable.modify(st, same_payer, [&](auto& s) {
            s.supply += quantity;
        });

        add_balance(st.issuer, quantity, st.issuer);
    }

    ACTION token::burn( const name& owner, const asset& quantity, const string& memo ) {
        require_auth(owner);

        auto sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");

        check_nft(quantity);
        check_frozen( owner );

        check(memo.size() <= 256, "memo has more than 256 bytes");

        stats statstable(get_self(), sym.code().raw());
        auto existing_st = statstable.find(sym.code().raw());
        check(existing_st != statstable.end(), "token with symbol does not exist");
        const auto& st = *existing_st;

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must burn positive quantity");
        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

        sub_balance(owner, quantity);

        statstable.modify(st, same_payer, [&](auto& s) {
            s.supply -= quantity;
        });

        allowances allws(get_self(), owner.value);
        auto existing_allw = allws.find(sym.code().raw());
        if (existing_allw != allws.end()) {
            accounts acnts(get_self(), owner.value);
            auto existing_ac = acnts.find(sym.code().raw());
            if (!existing_ac->balance.amount)
                allws.erase(existing_allw);
            else if (existing_allw->balance.amount > existing_ac->balance.amount) {
                allws.modify(existing_allw, same_payer, [&](auto& a) {
                    a.balance.amount = existing_ac->balance.amount;
                });
            }
        }
    }

    ACTION token::burnfrom( const name& burner, const name& owner, const asset& quantity, const string& memo ) {
        require_auth(burner);
        check(burner != owner, "cannot burnfrom self");
        check(is_account(owner), "owner account does not exist");

        require_recipient(owner);

        auto sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");

        check_nft(quantity);
        check_frozen( owner );

        check(memo.size() <= 256, "memo has more than 256 bytes");

        stats statstable(get_self(), sym.code().raw());
        auto existing_st = statstable.find(sym.code().raw());
        check(existing_st != statstable.end(), "token with symbol does not exist");
        const auto& st = *existing_st;

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must transfer positive quantity");
        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

        allowances allws(get_self(), owner.value);
        auto existing_allw = allws.require_find(sym.code().raw(), "no symbol in the allowance table");
        check(existing_allw->spender == burner, "you are not a spender");
        check(existing_allw->balance.amount >= quantity.amount, "burner does not have enough allowed amount");

        if (existing_allw->balance.amount == quantity.amount)
            allws.erase(existing_allw);
        else {
            allws.modify(existing_allw, same_payer, [&](auto& a) {
                a.balance -= quantity;
            });
        }

        sub_balance(owner, quantity);

        statstable.modify(st, same_payer, [&](auto& s) {
            s.supply -= quantity;
        });
    }

    ACTION token::transfer( const name& from, const name& to, const asset& quantity, const string& memo ) {
        check(from != to, "cannot transfer to self");
        require_auth(from);
        check(is_account(to), "to account does not exist");

        auto sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");

        check_nft(quantity);
        check_frozen( from );

        stats statstable(get_self(), sym.code().raw());
        const auto& st = statstable.get(sym.code().raw());

        require_recipient(from);
        require_recipient(to);

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must transfer positive quantity");
        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        auto payer = has_auth(to) ? to : from;

        sub_balance(from, quantity);
        add_balance(to, quantity, payer);

        allowances allws(get_self(), from.value);
        auto existing_allw = allws.find(sym.code().raw());
        if (existing_allw != allws.end()) {
            accounts acnts(get_self(), from.value);
            auto existing_ac = acnts.find(sym.code().raw());
            if (!existing_ac->balance.amount)
                allws.erase(existing_allw);
            else if (existing_allw->balance.amount > existing_ac->balance.amount) {
                allws.modify(existing_allw, same_payer, [&](auto& a) {
                    a.balance.amount = existing_ac->balance.amount;
                });
            }
        }
    }

    ACTION token::approve( const name& owner, const name& spender, const asset& quantity ) {
        require_auth(owner);

        auto sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");

        check_nft(quantity);
        check_frozen( owner );

        stats statstable(get_self(), sym.code().raw());
        const auto& st = statstable.get(sym.code().raw());

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must issue positive quantity");
        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

        accounts acnts(get_self(), owner.value);
        auto existing_ac = acnts.require_find(sym.code().raw(), "owner does not have token with symbol");

        check(existing_ac->balance.amount >= quantity.amount, "not enough balance");

        allowances allws(get_self(), owner.value);
        auto existing_allw = allws.find(sym.code().raw());
        if (existing_allw == allws.end()) {
            allws.emplace(owner, [&](auto& a) {
                a.balance = quantity;
                a.spender = spender;
            });
        } else {
            allws.modify(existing_allw, same_payer, [&](auto& a) {
                a.balance = quantity;
                a.spender = spender;
            });
        }
    }

    ACTION token::transferfrom( const name& spender, const name& from, const name& to, const asset& quantity, const string& memo ) {
        require_auth(spender);
        check(from != to, "cannot transfer to self");
        check(is_account(from), "from account does not exist");
        check(is_account(to), "to account does not exist");
        check(spender != from, "spender and from must be different");

        require_recipient(from);
        require_recipient(to);

        auto sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");

        check_nft(quantity);
        check_frozen( from );

        stats statstable(get_self(), sym.code().raw());
        const auto& st = statstable.get(sym.code().raw());

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must transfer positive quantity");
        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        allowances allws(get_self(), from.value);
        auto existing_allw = allws.require_find(sym.code().raw(), "no symbol in the allowance table");
        check(existing_allw->balance.amount >= quantity.amount, "spender does not have enough allowed amount");

        if (existing_allw->balance.amount == quantity.amount)
            allws.erase(existing_allw);
        else {
            allws.modify(existing_allw, same_payer, [&](auto& a) {
                a.balance -= quantity;
            });
        }

        auto payer = has_auth(to) ? to : spender;

        sub_balance(from, quantity);
        add_balance(to, quantity, payer);
    }

    ACTION token::incallowance( const name& owner, const asset& quantity ) {
        require_auth(owner);

        auto sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");

        check_nft(quantity);
        check_frozen( owner );

        accounts acnts(get_self(), owner.value);
        auto existing_ac = acnts.require_find(sym.code().raw(), "owner does not have token with symbol");

        allowances allws(get_self(), owner.value);
        auto existing_allw = allws.require_find(sym.code().raw(), "spender is not registed");

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must issue positive quantity");
        check(existing_ac->balance.amount >= existing_allw->balance.amount + quantity.amount, "owner does not have enough increase allow amount");

        allws.modify(existing_allw, same_payer, [&](auto& a) {
            a.balance += quantity;
        });
    }

    ACTION token::decallowance( const name& owner, const asset& quantity ) {
        require_auth(owner);

        auto sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");

        check_nft(quantity);
        check_frozen( owner );

        allowances allws(get_self(), owner.value);
        auto existing_allw = allws.require_find(sym.code().raw(), "spender is not registed");

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must issue positive quantity");
        check(existing_allw->balance >= quantity, "there is not enough balance");

        allws.modify(existing_allw, same_payer, [&](auto& a) {
            a.balance -= quantity;
        });
    }

    ACTION token::open( const name& owner, const symbol& symbol, const name& ram_payer ) {
        require_auth(ram_payer);

        check_frozen( owner );

        auto sym_code_raw = symbol.code().raw();

        stats statstable(get_self(), sym_code_raw);
        const auto& st = statstable.get(sym_code_raw, "symbol does not exist");
        check(st.supply.symbol == symbol, "symbol precision mismatch");

        accounts acnts(get_self(), owner.value);
        auto existing_ac = acnts.find(sym_code_raw);
        if (existing_ac == acnts.end()) {
            acnts.emplace(ram_payer, [&](auto& a) {
                a.balance = asset{0, symbol};
            });
        }
    }

    ACTION token::close( const name& owner, const symbol& symbol ) {
        require_auth(owner);
        check_frozen( owner );

        accounts acnts(get_self(), owner.value);
        auto existing_ac = acnts.find(symbol.code().raw());
        check(existing_ac != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect.");
        check(existing_ac->balance.amount == 0, "Cannot close because the balance is not zero.");
        acnts.erase(existing_ac);
    }

    ACTION token::createnft( const name& issuer, const string& sym ) {
        if (has_auth( "ibct"_n )) {
            require_auth( "ibct"_n );
        }
        else {
            require_auth(get_self());
        }

        // Check if issuer account exists
        check(is_account(issuer), "issuer account does not exist");

        check_frozen( issuer );

        // Valid symbol
        asset supply(0, symbol(symbol_code(sym.c_str()), 0));

        auto symbol = supply.symbol;
        check(symbol.is_valid(), "invalid symbol name");

        // Check if currency with symbol already exists
        stats statstable(get_self(), symbol.code().raw());
        auto existing_st = statstable.find(symbol.code().raw());
        check(existing_st == statstable.end(), "symbol already exists");

        // Create new currency
        statstable.emplace(get_self(), [&](auto& s) {
            s.supply = supply;
            s.max_supply = asset{asset::max_amount, symbol};
            s.issuer = issuer;
        });
    }

    ACTION token::issuenft( const name& to, const asset& quantity, const vector<std::pair<uint64_t, name>>& token_infos, const string& memo ) {
        check(is_account(to), "to account does not exist");

        check_frozen( to );

        auto sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");
        check(sym.precision() == 0, "quantity must be a whole number");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        stats statstable(get_self(), sym.code().raw());
        auto existing_st = statstable.find(sym.code().raw());
        check(existing_st != statstable.end(), "token with symbol does not exist, create token before issue");
        const auto& st = *existing_st;

        require_auth(st.issuer);
        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must issue positive quantity of NFT");
        check(sym == st.supply.symbol, "symbol precision mismatch");
        check(quantity.amount == token_infos.size(), "mismatch between issue amount and token info");

        statstable.modify(st, same_payer, [&](auto& s) {
            s.supply += quantity;
        });

        nfts nftstable(get_self(), sym.code().raw());

        auto payer = has_auth(to) ? to : st.issuer;

        for (auto const& tk : token_infos) {
            auto existing_nft = nftstable.find(tk.first);
            check(existing_nft == nftstable.end(), "token with symbol already exists");
            nftstable.emplace(payer, [&](auto& nft) {
                nft.token_id = tk.first;
                nft.owner = to;
                nft.tokenName = tk.second;
                nft.spender = to;
            });
        }
        add_balance(to, quantity, payer);
    }

    ACTION token::burnnft( const name& owner, const asset& quantity, const vector<uint64_t>& token_ids, const string& memo ) {
        require_auth(owner);

        check_frozen( owner );

        auto sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");
        check(sym.precision() == 0, "quantity must be a whole number");

        check(memo.size() <= 256, "memo has more than 256 bytes");

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must burn positive quantity");
        check(quantity.amount == token_ids.size(), "mismatch between burn amount and token info");

        stats statstable(get_self(), sym.code().raw());
        auto existing_st = statstable.find(sym.code().raw());
        check(existing_st != statstable.end(), "symbol does not exist at stats");
        const auto& st = *existing_st;

        nfts nftstable(get_self(), sym.code().raw());
        for (auto const& token_id : token_ids) {
            auto existing_nft = nftstable.find(token_id);
            check(existing_nft != nftstable.end(), "token with symbol does not exists");
            check(existing_nft->owner == owner, "not the owner of token");
            nftstable.erase(existing_nft);
        }

        sub_balance(owner, quantity);
        statstable.modify(st, same_payer, [&](auto& s) {
            s.supply -= quantity;
        });
    }

    ACTION token::burnnftfrom( const name& burner, const string& sym, id_type token_id, const string& memo ) {
        require_auth(burner);

        asset as(0, symbol(symbol_code(sym.c_str()), 0));
        auto symbol = as.symbol;
        check(symbol.is_valid(), "invalid symbol name");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        stats statstable(get_self(), symbol.code().raw());
        auto existing_st = statstable.find(symbol.code().raw());
        check(existing_st != statstable.end(), "symbol does not exist at stats");
        const auto& st = *existing_st;

        nfts nftstable(get_self(), symbol.code().raw());
        auto existing_nft = nftstable.find(token_id);
        check(existing_nft != nftstable.end(), "token with symbol does not exists");

        check_frozen( existing_nft->owner );

        require_recipient(existing_nft->owner);

        check(burner == existing_nft->spender, "burner is not token spender");

        asset unit(1, symbol);

        sub_balance(existing_nft->owner, unit);

        nftstable.erase(existing_nft);
        statstable.modify(st, same_payer, [&](auto& s) {
            s.supply -= unit;
        });
    }

    ACTION token::send( const name& from, const name& to, const string& sym, id_type token_id, const string& memo ) {
        check(from != to, "cannot transfer to self");
        require_auth(from);
        check(is_account(to), "to account does not exist");

        require_recipient(from);
        require_recipient(to);

        asset as(0, symbol(symbol_code(sym.c_str()), 0));
        auto symbol = as.symbol;

        check(symbol.is_valid(), "invalid symbol name");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        nfts nftstable(get_self(), symbol.code().raw());
        auto existing_nft = nftstable.find(token_id);
        check(existing_nft != nftstable.end(), "token with symbol does not exists");
        check(from == existing_nft->owner, "not the owner of token");
        check(existing_nft->spender != get_self(), "if spender is get_self(), it can not transfer");

        check_frozen( existing_nft->owner );

        auto payer = has_auth(to) ? to : from;

        nftstable.modify(existing_nft, payer, [&](auto& nft) {
            nft.owner = to;
            nft.spender = to;
        });

        asset unit(1, symbol);

        sub_balance(from, unit);
        add_balance(to, unit, payer);
    }

    ACTION token::approvenft( const name& owner, const name& spender, const string& sym, id_type token_id ) {
        require_auth(owner);
        check_frozen( owner );

        asset as(0, symbol(symbol_code(sym.c_str()), 0));
        auto symbol = as.symbol;

        check(symbol.is_valid(), "invalid symbol name");

        nfts nftstable(get_self(), symbol.code().raw());
        auto existing_nft = nftstable.find(token_id);
        check(existing_nft != nftstable.end(), "token with symbol does not exists");
        check(owner == existing_nft->owner, "not the owner of token");
        check(owner == get_self() || existing_nft->spender != get_self(), "if spender is get_self(), it can not be changed");

        nftstable.modify(existing_nft, same_payer, [&](auto& nft) {
            nft.spender = spender;
        });
    }

    ACTION token::sendfrom( const name& spender, const name& to, const string& sym, id_type token_id, const string& memo ) {
        require_auth(spender);

        check(is_account(to), "to account does not exist");

        asset as(0, symbol(symbol_code(sym.c_str()), 0));
        auto symbol = as.symbol;

        check(symbol.is_valid(), "invalid symbol name");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        nfts nftstable(get_self(), symbol.code().raw());
        auto existing_nft = nftstable.find(token_id);
        check(existing_nft != nftstable.end(), "token with symbol does not exists");
        check(spender == existing_nft->spender, "spender is not token spender");
        check(spender != existing_nft->owner, "spender and owner must be different");
        name owner = existing_nft->owner;

        check_frozen( owner );

        require_recipient(owner);
        require_recipient(to);

        auto payer = has_auth(to) ? to : spender;

        nftstable.modify(existing_nft, payer, [&](auto& nft) {
            nft.owner = to;
            nft.spender = to;
        });

        asset unit(1, symbol);

        sub_balance(owner, unit);
        add_balance(to, unit, payer);
    }

    ACTION token::auctiontoken( const name& auctioneer, const string& sym, id_type token_id, const asset& min_price, uint32_t sec ) {
        require_auth(auctioneer);

        check_frozen( auctioneer );

        require_recipient(auctioneer);
        require_recipient(get_self());

        asset as(0, symbol(symbol_code(sym.c_str()), 0));
        auto symbol = as.symbol;
        check(symbol.is_valid(), "invalid symbol name");

        check(sec > 0 && sec < 259200, "sec must be a positive integer and can not exceed three days");

        const time_point_sec deadline = time_point_sec(current_time_point().sec_since_epoch()) + sec;

        nfts nftstable(get_self(), symbol.code().raw());
        auto existing_nft = nftstable.find(token_id);
        check(existing_nft != nftstable.end(), "token with symbol does not exists");
        check(existing_nft->owner == auctioneer, "not the owner of token");

        nft_bids nftbidstable(get_self(), symbol.code().raw());
        auto existing_bid = nftbidstable.find(token_id);
        check(existing_bid == nftbidstable.end(), "token bid already exist");

        check(min_price.amount > 0, "minimum price must be a positive integer");

        stats statstable(get_self(), min_price.symbol.code().raw());
        auto existing_st = statstable.find(min_price.symbol.code().raw());
        check(existing_st != statstable.end(), "minimum price symbol does not exist at stats");

        nftbidstable.emplace(auctioneer, [&](auto& b) {
            b.token_id = token_id;
            b.high_bidder = auctioneer;
            b.high_bid = min_price;
            b.deadline = deadline;
        });

        nftstable.modify(existing_nft, same_payer, [&](auto& nft) {
            nft.spender = get_self();
        });
    }

    ACTION token::bidtoken( const name& bidder, const string& sym, id_type token_id, const asset& bid ) {
        require_auth(bidder);
        check_frozen( bidder );

        asset as(0, symbol(symbol_code(sym.c_str()), 0));
        auto symbol = as.symbol;
        check(symbol.is_valid(), "invalid symbol name");

        nfts nftstable(get_self(), symbol.code().raw());
        auto existing_nft = nftstable.find(token_id);
        check(existing_nft != nftstable.end(), "token with symbol does not exists");

        check(bidder != existing_nft->owner, "token owners can not bid");

        nft_bids nftbidstable(get_self(), symbol.code().raw());
        auto existing_bid = nftbidstable.find(token_id);
        check(existing_bid != nftbidstable.end(), "token auction is not exist");
        check(bid.symbol == existing_bid->high_bid.symbol, "bid symbol is not correct");
        check(bid.amount > existing_bid->high_bid.amount, "the bid amount is insufficient");

        const time_point_sec time_now = time_point_sec(current_time_point().sec_since_epoch());
        check(existing_bid->deadline > time_now, "the auction deadline has passed");

        if (existing_bid->high_bidder != existing_nft->owner) {
            // refund
            action(
                permission_level{get_self(), "active"_n},
                get_self(), "transfer"_n,
                std::make_tuple(get_self(), existing_bid->high_bidder, existing_bid->high_bid, std::string("refund bidding fee")))
                .send();
        }

        // new high bidder
        nftbidstable.modify(existing_bid, same_payer, [&](auto& b) {
            b.high_bidder = bidder;
            b.high_bid = bid;
        });

        sub_balance(bidder, bid);
        add_balance(get_self(), bid, get_self());

        bidresult_action bid_act(bidder, std::vector<eosio::permission_level>{});
        bid_act.send(bid);
    }

    ACTION token::claimtoken( const name& requester, const string& sym, id_type token_id ) {
        require_auth(requester);
        check_frozen( requester );

        asset as(0, symbol(symbol_code(sym.c_str()), 0));
        auto symbol = as.symbol;
        check(symbol.is_valid(), "invalid symbol name");

        nfts nftstable(get_self(), symbol.code().raw());
        auto existing_nft = nftstable.find(token_id);
        check(existing_nft != nftstable.end(), "token with symbol does not exists");

        nft_bids nftbidstable(get_self(), symbol.code().raw());
        auto existing_bid = nftbidstable.find(token_id);
        check(existing_bid != nftbidstable.end(), "token auction is not exist");

        const time_point_sec time_now = time_point_sec(current_time_point().sec_since_epoch());
        check(existing_bid->deadline <= time_now, "deadline not over");
        check(requester == existing_nft->owner || requester == existing_bid->high_bidder, "the requester is not authorized");

        if (existing_bid->high_bidder != existing_nft->owner) {
            // bidding fee payment
            action(
                permission_level{get_self(), "active"_n},
                get_self(), "transfer"_n,
                std::make_tuple(get_self(), existing_nft->owner, existing_bid->high_bid, std::string("receive auction sale money")))
                .send();

            // nft ownership change
            action(
                permission_level{get_self(), "active"_n},
                get_self(), "sendfrom"_n,
                std::make_tuple(get_self(), existing_bid->high_bidder, sym, token_id, std::string("receive auction tokens")))
                .send();
        } else {
            nftstable.modify(existing_nft, same_payer, [&](auto& nft) {
                nft.spender = existing_nft->owner;
            });
        }

        nftbidstable.erase(existing_bid);
    }

    void token::freeze( const name& account ) {
        if (has_auth( "ibct"_n )) {
            require_auth( "ibct"_n );
        }
        else {
            require_auth(get_self());
        }

        frozens frozentable( get_self(), get_self().value );
        auto fitr = frozentable.find( account.value );
        check( fitr == frozentable.end(), "account is already frozen" );

        frozentable.emplace( get_self(), [&](auto& f) {
            f.freezer = account;
        });
    }

    void token::unfreeze( const name& account ) {
        if (has_auth( "ibct"_n )) {
            require_auth( "ibct"_n );
        }
        else {
            require_auth(get_self());
        }

        frozens frozentable( get_self(), get_self().value );
        auto fitr = frozentable.find( account.value );
        check( fitr != frozentable.end(), "account is not frozen" );

        frozentable.erase( fitr );
    }

    void token::sub_balance( const name& owner, const asset& value ) {
        accounts from_acnts(get_self(), owner.value);

        const auto& from = from_acnts.get(value.symbol.code().raw(), "no balance object found");
        check(from.balance.amount >= value.amount, "overdrawn balance");

        name payer = !has_auth(owner) ? same_payer : owner;

        from_acnts.modify(from, payer, [&](auto& a) {
            a.balance -= value;
        });
    }

    void token::add_balance( const name& owner, const asset& value, const name& ram_payer ) {
        accounts to_acnts(get_self(), owner.value);
        auto to = to_acnts.find(value.symbol.code().raw());
        if (to == to_acnts.end()) {
            to_acnts.emplace(ram_payer, [&](auto& a) {
                a.balance = value;
            });
        } else {
            to_acnts.modify(to, same_payer, [&](auto& a) {
                a.balance += value;
            });
        }
    }

    void token::check_nft( const asset& quantity ) {
        auto sym = quantity.symbol;

        stats statstable(get_self(), sym.code().raw());
        auto existing_st = statstable.find(sym.code().raw());
        check(existing_st->max_supply.amount != asset::max_amount, "nft token symbol");
    }

    void token::check_frozen( const name& account ) {
        frozens frozenstable( get_self(), get_self().value );
        auto fitr = frozenstable.find( account.value );
        check( fitr == frozenstable.end(), "account is frozen" );
    }
}  // namespace eosio

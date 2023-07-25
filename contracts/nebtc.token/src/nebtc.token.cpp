#include "nebtc.token/nebtc.token.hpp"

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

    void token::freeze( const name& account ) {
        if (has_auth( "ibct"_n )) {
            require_auth( "ibct"_n );
        }
        else {
            require_auth(get_self());
        }

        require_recipient(account);

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

        require_recipient(account);

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

    void token::check_frozen( const name& account ) {
        frozens frozenstable( get_self(), get_self().value );
        auto fitr = frozenstable.find( account.value );
        check( fitr == frozenstable.end(), "account is frozen" );
    }
}  // namespace eosio

/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/resource_limits.hpp>
#include "contracts.hpp"
#include "test_symbol.hpp"

#include <fc/variant_object.hpp>
#include <fstream>

using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

#ifndef TESTER
#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif
#endif

namespace eosio_system
{

class legis_system_tester : public TESTER
{
public:
    void basic_setup()
    {
        produce_blocks(2);

        create_accounts({N(led.token), N(led.ram), N(led.ramfee), N(led.stake),
                         N(led.cpay), N(led.pback), N(led.bpay),
                         N(led.vpay), N(led.saving), N(led.names), N(ibct)});

        produce_blocks(100);
        set_code(N(led.token), contracts::token_wasm());
        set_abi(N(led.token), contracts::token_abi().data());
        {
            const auto &accnt = control->db().get<account_object, by_name>(N(led.token));
            abi_def abi;
            BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
            token_abi_ser.set_abi(abi, abi_serializer_max_time);
        }
    }

    void create_core_token(symbol core_symbol = symbol{CORE_SYM})
    {
        FC_ASSERT(core_symbol.precision() != 4, "create_core_token assumes precision of core token is 4");
        create_currency(N(led.token), config::system_account_name, asset(100000000000000, core_symbol));
        issue(config::system_account_name, asset(10000000000000, core_symbol));
        BOOST_REQUIRE_EQUAL(asset(10000000000000, core_symbol), get_balance("led", core_symbol));
    }

    void deploy_contract(bool call_init = true)
    {
        set_code(config::system_account_name, contracts::system_wasm());
        set_abi(config::system_account_name, contracts::system_abi().data());
        if (call_init)
        {
            base_tester::push_action(config::system_account_name, N(init),
                                     config::system_account_name, mutable_variant_object()("version", 0)("core", CORE_SYM_STR));
        }

        {
            const auto &accnt = control->db().get<account_object, by_name>(config::system_account_name);
            abi_def abi;
            BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
            abi_ser.set_abi(abi, abi_serializer_max_time);
        }
    }

    void remaining_setup()
    {
        produce_blocks();

        // Assumes previous setup steps were done with core token symbol set to CORE_SYM
        create_account_with_resources(N(alice.p), config::system_account_name, core_sym::from_string("1.0000"), false);
        create_account_with_resources(N(bob.p), config::system_account_name, core_sym::from_string("0.4500"), false);
        create_account_with_resources(N(carol.p), config::system_account_name, core_sym::from_string("1.0000"), false);

        BOOST_REQUIRE_EQUAL(core_sym::from_string("1000000000.0000"), get_balance("led") + get_balance("led.ramfee") + get_balance("led.stake") + get_balance("led.ram"));
    }

    enum class setup_level
    {
        none,
        minimal,
        core_token,
        deploy_contract,
        full
    };

    legis_system_tester(setup_level l = setup_level::full)
    {
        if (l == setup_level::none)
            return;

        basic_setup();
        if (l == setup_level::minimal)
            return;

        create_core_token();
        if (l == setup_level::core_token)
            return;

        deploy_contract();
        if (l == setup_level::deploy_contract)
            return;

        remaining_setup();
    }

    template <typename Lambda>
    legis_system_tester(Lambda setup)
    {
        setup(*this);

        basic_setup();
        create_core_token();
        deploy_contract();
        remaining_setup();
    }

    void create_accounts_with_resources(vector<account_name> accounts, account_name creator = config::system_account_name)
    {
        for (auto a : accounts)
        {
            create_account_with_resources(a, creator);
        }
    }

    transaction_trace_ptr create_account_with_resources(account_name a, account_name creator, uint32_t ram_bytes = 8000)
    {
        signed_transaction trx;
        set_transaction_headers(trx);

        authority owner_auth;
        owner_auth = authority(get_public_key(a, "owner"));

        trx.actions.emplace_back(vector<permission_level>{{creator, config::active_name}},
                                 newaccount{
                                     .creator = creator,
                                     .name = a,
                                     .owner = owner_auth,
                                     .active = authority(get_public_key(a, "active"))});

        trx.actions.emplace_back(get_action(config::system_account_name, N(buyrambytes), vector<permission_level>{{creator, config::active_name}},
                                            mvo()("payer", creator)("receiver", a)("bytes", ram_bytes)));
        trx.actions.emplace_back(get_action(config::system_account_name, N(delegatebw), vector<permission_level>{{creator, config::active_name}},
                                            mvo()("from", creator)("receiver", a)("stake_net_quantity", core_sym::from_string("10.0000"))("stake_cpu_quantity", core_sym::from_string("10.0000"))("transfer", 0)));

        set_transaction_headers(trx);
        trx.sign(get_private_key(creator, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    transaction_trace_ptr create_account_with_resources(account_name a, account_name creator, asset ramfunds, bool multisig,
                                                        asset net = core_sym::from_string("10.0000"), asset cpu = core_sym::from_string("10.0000"))
    {
        signed_transaction trx;
        set_transaction_headers(trx);

        authority owner_auth;
        if (multisig)
        {
            // multisig between account's owner key and creators active permission
            owner_auth = authority(2, {key_weight{get_public_key(a, "owner"), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});
        }
        else
        {
            owner_auth = authority(get_public_key(a, "owner"));
        }

        trx.actions.emplace_back(vector<permission_level>{{creator, config::active_name}},
                                 newaccount{
                                     .creator = creator,
                                     .name = a,
                                     .owner = owner_auth,
                                     .active = authority(get_public_key(a, "active"))});

        trx.actions.emplace_back(get_action(config::system_account_name, N(buyram), vector<permission_level>{{creator, config::active_name}},
                                            mvo()("payer", creator)("receiver", a)("quant", ramfunds)));

        trx.actions.emplace_back(get_action(config::system_account_name, N(delegatebw), vector<permission_level>{{creator, config::active_name}},
                                            mvo()("from", creator)("receiver", a)("stake_net_quantity", net)("stake_cpu_quantity", cpu)("transfer", 0)));

        set_transaction_headers(trx);
        trx.sign(get_private_key(creator, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    action_result buyram(const account_name &payer, account_name receiver, const asset &eosin)
    {
        return push_action(payer, N(buyram), mvo()("payer", payer)("receiver", receiver)("quant", eosin));
    }
    action_result buyrambytes(const account_name &payer, account_name receiver, uint32_t numbytes)
    {
        return push_action(payer, N(buyrambytes), mvo()("payer", payer)("receiver", receiver)("bytes", numbytes));
    }

    action_result sellram(const account_name &account, uint64_t numbytes)
    {
        return push_action(account, N(sellram), mvo()("account", account)("bytes", numbytes));
    }

    action_result push_action(const account_name &signer, const action_name &name, const variant_object &data, bool auth = true)
    {
        string action_type_name = abi_ser.get_action_type(name);

        action act;
        act.account = config::system_account_name;
        act.name = name;
        act.data = abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

        return base_tester::push_action(std::move(act), auth ? uint64_t(signer) : signer == N(bob.p) ? N(alice.p) : N(bob.p));
    }

    action_result stake(const account_name &from, const account_name &to, const asset &net, const asset &cpu)
    {
        return push_action(name(from), N(delegatebw), mvo()("from", from)("receiver", to)("stake_net_quantity", net)("stake_cpu_quantity", cpu)("transfer", 0));
    }

    action_result stake(const account_name &acnt, const asset &net, const asset &cpu)
    {
        return stake(acnt, acnt, net, cpu);
    }

    action_result stake_with_transfer(const account_name &from, const account_name &to, const asset &net, const asset &cpu)
    {
        return push_action(name(from), N(delegatebw), mvo()("from", from)("receiver", to)("stake_net_quantity", net)("stake_cpu_quantity", cpu)("transfer", true));
    }

    action_result stake_with_transfer(const account_name &acnt, const asset &net, const asset &cpu)
    {
        return stake_with_transfer(acnt, acnt, net, cpu);
    }

    action_result unstake(const account_name &from, const account_name &to, const asset &net, const asset &cpu)
    {
        return push_action(name(from), N(undelegatebw), mvo()("from", from)("receiver", to)("unstake_net_quantity", net)("unstake_cpu_quantity", cpu));
    }

    action_result unstake(const account_name &acnt, const asset &net, const asset &cpu)
    {
        return unstake(acnt, acnt, net, cpu);
    }

    static fc::variant_object producer_parameters_example(int n)
    {
        return mutable_variant_object()("max_block_net_usage", 10000000 + n)("target_block_net_usage_pct", 10 + n)("max_transaction_net_usage", 1000000 + n)("base_per_transaction_net_usage", 100 + n)("net_usage_leeway", 500 + n)("context_free_discount_net_usage_num", 1 + n)("context_free_discount_net_usage_den", 100 + n)("max_block_cpu_usage", 10000000 + n)("target_block_cpu_usage_pct", 10 + n)("max_transaction_cpu_usage", 1000000 + n)("min_transaction_cpu_usage", 100 + n)("max_transaction_lifetime", 3600 + n)("deferred_trx_expiration_window", 600 + n)("max_transaction_delay", 10 * 86400 + n)("max_inline_action_size", 4096 + n)("max_inline_action_depth", 4 + n)("max_authority_depth", 6 + n)("max_ram_size", (n % 10 + 1) * 1024 * 1024)("ram_reserve_ratio", 100 + n);
    }

    action_result activate()
    {
        action_result r = push_action(N(ibct), N(activate), mvo()("activator", "ibct"));
        return r;
    }

    action_result reginterior(const account_name &acnt, int params_fixture = 1)
    {
        action_result r = push_action(acnt, N(reginterior), mvo()("interior", acnt)("producer_key", get_public_key(acnt, "active"))("election_promise", "")("url", "")("location", 0)("logo_256", ""));
        BOOST_REQUIRE_EQUAL(success(), r);
        return r;
    }

    action_result regfrontier(const account_name &acnt, const asset &ratio, int params_fixture = 1)
    {
        auto sym_name = ratio.symbol_name();
        auto maximum = asset::from_string("1000000.000 " + sym_name);
        base_tester::push_action(N(led.token), N(create), N(ibct), mutable_variant_object()("issuer", acnt)("maximum_supply", maximum));
        issue(acnt, maximum, acnt);
        action_result r = push_action(acnt, N(regfrontier), mvo()("frontier", acnt)("producer_key", get_public_key(acnt, "active"))("transfer_ratio", ratio)("category", 0)("url", "")("location", 0)("logo_256", ""));
        BOOST_REQUIRE_EQUAL(success(), r);
        return r;
    }

    action_result re_regfrontier(const account_name &acnt, const asset &ratio, int params_fixture = 1)
    {
        action_result r = push_action(acnt, N(regfrontier), mvo()("frontier", acnt)("producer_key", get_public_key(acnt, "active"))("transfer_ratio", ratio)("category", 0)("url", "")("location", 0)("logo_256", ""));
        BOOST_REQUIRE_EQUAL(success(), r);
        return r;
    }

    action_result regproxy(const account_name &acnt)
    {
        action_result r = push_action(acnt, N(regproxy), mvo()("proxy", acnt)("slogan", "")("background", "")("url", "")("location", 0)("logo_256", ""));
        return r;
    }

    action_result buyservice(const account_name &from, const asset &amount, const account_name &to)
    {
        action_result r = push_action(from, N(buyservice), mvo()("buyer", from)("quantity", amount)("frontier", to));
        return r;
    }

    action_result vote(const account_name &voter, const std::vector<account_name> &interiors, const account_name &proxy = name(0))
    {
        return push_action(voter, N(voteproducer), mvo()("voter", voter)("proxy", proxy)("interiors", interiors));
    }

    fc::variant get_dbw_obj(const account_name &from, const account_name &receiver) const
    {
        vector<char> data = get_row_by_account(config::system_account_name, from, N(delband), receiver);
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("delegated_bandwidth", data, abi_serializer_max_time);
    }

    uint32_t last_block_time() const
    {
        return time_point_sec(control->head_block_time()).sec_since_epoch();
    }

    int64_t bancor_convert(int64_t S, int64_t R, int64_t T) { return double(R) * T / (double(S) + T); };

    int64_t get_net_limit(account_name a)
    {
        int64_t ram_bytes = 0, net = 0, cpu = 0;
        control->get_resource_limits_manager().get_account_limits(a, ram_bytes, net, cpu);
        return net;
    };

    int64_t get_cpu_limit(account_name a)
    {
        int64_t ram_bytes = 0, net = 0, cpu = 0;
        control->get_resource_limits_manager().get_account_limits(a, ram_bytes, net, cpu);
        return cpu;
    };

    asset get_balance(const account_name &act, symbol balance_symbol = symbol{CORE_SYM})
    {
        vector<char> data = get_row_by_account(N(led.token), act, N(accounts), balance_symbol.to_symbol_code().value);
        return data.empty() ? asset(0, balance_symbol) : token_abi_ser.binary_to_variant("account", data, abi_serializer_max_time)["balance"].as<asset>();
    }

    fc::variant get_total_stake(const account_name &act)
    {
        vector<char> data = get_row_by_account(config::system_account_name, act, N(userres), act);
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("user_resources", data, abi_serializer_max_time);
    }

    fc::variant get_voter_info(const account_name &act)
    {
        vector<char> data = get_row_by_account(config::system_account_name, config::system_account_name, N(voters), act);
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("voter_info", data, abi_serializer_max_time);
    }

    fc::variant get_producer_info(const account_name &act)
    {
        vector<char> data = get_row_by_account(config::system_account_name, config::system_account_name, N(producers), act);
        return abi_ser.binary_to_variant("producer_info", data, abi_serializer_max_time);
    }

    fc::variant get_interior_info(const account_name &act)
    {
        vector<char> data = get_row_by_account(config::system_account_name, config::system_account_name, N(interiors), act);
        return abi_ser.binary_to_variant("interior_info", data, abi_serializer_max_time);
    }

    fc::variant get_frontier_info(const account_name &act)
    {
        vector<char> data = get_row_by_account(config::system_account_name, config::system_account_name, N(frontiers), act);
        return abi_ser.binary_to_variant("frontier_info", data, abi_serializer_max_time);
    }

    void create_currency(name contract, name manager, asset maxsupply)
    {
        auto act = mutable_variant_object()("issuer", manager)("maximum_supply", maxsupply);

        base_tester::push_action(contract, N(create), contract, act);
    }

    void issue(name to, const asset &amount, name manager = config::system_account_name)
    {
        base_tester::push_action(N(led.token), N(issue), manager, mutable_variant_object()("to", to)("quantity", amount)("memo", ""));
    }
    void transfer(name from, name to, const asset &amount, name manager = config::system_account_name)
    {
        base_tester::push_action(N(led.token), N(transfer), manager, mutable_variant_object()("from", from)("to", to)("quantity", amount)("memo", ""));
    }

    double stake2votes(asset stake)
    {
        auto now = control->pending_block_time().time_since_epoch().count() / 1000000;
        return stake.get_amount() * pow(2, int64_t((now - (config::block_timestamp_epoch / 1000)) / (86400 * 7)) / double(52)); // 52 week periods (i.e. ~years)
    }

    double stake2votes(const string &s)
    {
        return stake2votes(core_sym::from_string(s));
    }

    fc::variant get_stats(const string &symbolname)
    {
        auto symb = eosio::chain::symbol::from_string(symbolname);
        auto symbol_code = symb.to_symbol_code().value;
        vector<char> data = get_row_by_account(N(led.token), symbol_code, N(stat), symbol_code);
        return data.empty() ? fc::variant() : token_abi_ser.binary_to_variant("currency_stats", data, abi_serializer_max_time);
    }

    asset get_token_supply()
    {
        return get_stats("4," CORE_SYM_NAME)["supply"].as<asset>();
    }

    uint64_t microseconds_since_epoch_of_iso_string(const fc::variant &v)
    {
        return static_cast<uint64_t>(time_point::from_iso_string(v.as_string()).time_since_epoch().count());
    }

    fc::variant get_global_state()
    {
        vector<char> data = get_row_by_account(config::system_account_name, config::system_account_name, N(global), N(global));
        if (data.empty())
            std::cout << "\nData is empty\n"
                      << std::endl;
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("legis_global_state", data, abi_serializer_max_time);
    }

    fc::variant get_global_state2()
    {
        vector<char> data = get_row_by_account(config::system_account_name, config::system_account_name, N(global2), N(global2));
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("legis_global_state2", data, abi_serializer_max_time);
    }

    fc::variant get_refund_request(name account)
    {
        vector<char> data = get_row_by_account(config::system_account_name, account, N(refunds), account);
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant("refund_request", data, abi_serializer_max_time);
    }

    void active_and_pass_time()
    {
        activate();
        produce_block(fc::seconds(52 * 7 * 24 * 3600 / 2));
        produce_blocks();
        produce_block(fc::seconds(52 * 7 * 24 * 3600 / 2));
        produce_blocks();
        produce_block(fc::seconds(52 * 7 * 24 * 3600 / 2));
        produce_blocks();
    }

    vector<name> active_and_vote_producers()
    {
        activate();
        produce_block(fc::seconds(52 * 7 * 24 * 3600 / 2));
        produce_blocks();
        produce_block(fc::seconds(52 * 7 * 24 * 3600 / 2));
        produce_blocks();
        produce_block(fc::seconds(52 * 7 * 24 * 3600 / 2));
        produce_blocks();

        transfer("led", "alice.p", core_sym::from_string("150000000.0000"), "led");
        BOOST_REQUIRE_EQUAL(success(), stake("alice.p", "alice.p", core_sym::from_string("3000000.0000"), core_sym::from_string("3000000.0000")));
        transfer("led", "bob.p", core_sym::from_string("150000000.0000"), "led");
        BOOST_REQUIRE_EQUAL(success(), stake("bob.p", "bob.p", core_sym::from_string("3000000.0000"), core_sym::from_string("3000000.0000")));
        transfer("led", "carol.p", core_sym::from_string("150000000.0000"), "led");
        BOOST_REQUIRE_EQUAL(success(), stake("carol.p", "carol.p", core_sym::from_string("3000000.0000"), core_sym::from_string("3000000.0000")));

        // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
        std::vector<account_name> producer_names;
        {
            producer_names.reserve('z' - 'a' + 1);
            const std::string root("defproducer");
            for (char c = 'a'; c < 'a' + 21; ++c)
            {
                producer_names.emplace_back(root + std::string(1, c));
            }
            setup_producer_accounts(producer_names);
            for (const auto &p : producer_names)
            {
                BOOST_REQUIRE_EQUAL(success(), reginterior(p));
            }
        }
        produce_blocks(250);

        auto trace_auth = TESTER::push_action(config::system_account_name, updateauth::get_name(), config::system_account_name, mvo()("account", name(config::system_account_name).to_string())("permission", name(config::active_name).to_string())("parent", name(config::owner_name).to_string())("auth", authority(1, {key_weight{get_public_key(config::system_account_name, "active"), 1}}, {permission_level_weight{{config::system_account_name, config::eosio_code_name}, 1}, permission_level_weight{{config::producers_account_name, config::active_name}, 1}})));
        BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace_auth->receipt->status);

        //vote for producers
        {
            transfer(config::system_account_name, "alice.p", core_sym::from_string("100000000.0000"), config::system_account_name);
            BOOST_REQUIRE_EQUAL(success(), buyram("alice.p", "alice.p", core_sym::from_string("300000.0000")));
            BOOST_REQUIRE_EQUAL(success(), push_action(N(alice.p), N(voteproducer), mvo()("voter", "alice.p")("proxy", name(0).to_string())("interiors", vector<account_name>(producer_names.begin(), producer_names.begin() + 10))));
        }
        {
            transfer(config::system_account_name, "bob.p", core_sym::from_string("100000000.0000"), config::system_account_name);
            BOOST_REQUIRE_EQUAL(success(), buyram("bob.p", "bob.p", core_sym::from_string("300000.0000")));
            BOOST_REQUIRE_EQUAL(success(), push_action(N(bob.p), N(voteproducer), mvo()("voter", "bob.p")("proxy", name(0).to_string())("interiors", vector<account_name>(producer_names.begin() + 10, producer_names.begin() + 20))));
        }
        {
            transfer(config::system_account_name, "carol.p", core_sym::from_string("100000000.0000"), config::system_account_name);
            BOOST_REQUIRE_EQUAL(success(), buyram("carol.p", "carol.p", core_sym::from_string("300000.0000")));
            BOOST_REQUIRE_EQUAL(success(), push_action(N(carol.p), N(voteproducer), mvo()("voter", "carol.p")("proxy", name(0).to_string())("interiors", vector<account_name>(producer_names.begin() + 20, producer_names.begin() + 21))));
        }
        produce_blocks(250);

        auto producer_keys = control->head_block_state()->active_schedule.producers;
        BOOST_REQUIRE_EQUAL(21, producer_keys.size());
        BOOST_REQUIRE_EQUAL(name("defproducera"), producer_keys[0].producer_name);

        return producer_names;
    }

    abi_serializer initialize_multisig()
    {
        abi_serializer msig_abi_ser;
        {
            create_account_with_resources(N(led.msig), config::system_account_name);
            BOOST_REQUIRE_EQUAL(success(), buyram("led", "led.msig", core_sym::from_string("5000.0000")));
            produce_block();

            auto trace = base_tester::push_action(config::system_account_name, N(setpriv),
                                                  config::system_account_name, mutable_variant_object()("account", "led.msig")("is_priv", 1));

            set_code(N(led.msig), contracts::msig_wasm());
            set_abi(N(led.msig), contracts::msig_abi().data());

            produce_blocks();
            const auto &accnt = control->db().get<account_object, by_name>(N(led.msig));
            abi_def msig_abi;
            BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, msig_abi), true);
            msig_abi_ser.set_abi(msig_abi, abi_serializer_max_time);
        }
        return msig_abi_ser;
    }

    transaction_trace_ptr setup_producer_accounts(const std::vector<account_name> &accounts,
                                                  asset ram = core_sym::from_string("1.0000"),
                                                  asset cpu = core_sym::from_string("80.0000"),
                                                  asset net = core_sym::from_string("80.0000"))
    {
        account_name creator(config::system_account_name);
        signed_transaction trx;
        set_transaction_headers(trx);

        for (const auto &a : accounts)
        {
            authority owner_auth(get_public_key(a, "owner"));
            trx.actions.emplace_back(vector<permission_level>{{creator, config::active_name}},
                                     newaccount{
                                         .creator = creator,
                                         .name = a,
                                         .owner = owner_auth,
                                         .active = authority(get_public_key(a, "active"))});

            trx.actions.emplace_back(get_action(config::system_account_name, N(buyram), vector<permission_level>{{creator, config::active_name}},
                                                mvo()("payer", creator)("receiver", a)("quant", ram)));

            trx.actions.emplace_back(get_action(config::system_account_name, N(delegatebw), vector<permission_level>{{creator, config::active_name}},
                                                mvo()("from", creator)("receiver", a)("stake_net_quantity", net)("stake_cpu_quantity", cpu)("transfer", 0)));
        }

        set_transaction_headers(trx);
        trx.sign(get_private_key(creator, "active"), control->get_chain_id());
        return push_transaction(trx);
    }

    abi_serializer abi_ser;
    abi_serializer token_abi_ser;
};

inline fc::mutable_variant_object voter(account_name acct)
{
    return mutable_variant_object()("owner", acct)("proxy", name(0).to_string())("interiors", variants())("staked", int64_t(0))
        //("last_vote_weight", double(0))
        ("proxied_vote_weight", double(0))("is_proxy", 0);
}

inline fc::mutable_variant_object voter(account_name acct, const asset &vote_stake)
{
    return voter(acct)("staked", vote_stake.get_amount());
}

inline fc::mutable_variant_object voter(account_name acct, int64_t vote_stake)
{
    return voter(acct)("staked", vote_stake);
}

inline fc::mutable_variant_object proxy(account_name acct)
{
    return voter(acct)("is_proxy", 1);
}

inline uint64_t M(const string &eos_str)
{
    return core_sym::from_string(eos_str).get_amount();
}

} // namespace eosio_system
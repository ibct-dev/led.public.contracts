#pragma once

#include <eosio/action.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/fixed_bytes.hpp>
#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>

namespace eosiobios {
   using eosio::action_wrapper;
   using eosio::check;
   using eosio::checksum256;
   using eosio::ignore;
   using eosio::name;
   using eosio::permission_level;
   using eosio::public_key;

   struct permission_level_weight {
      permission_level permission;
      uint16_t         weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(permission_level_weight, (permission)(weight))
   };

   struct key_weight {
      eosio::public_key key;
      uint16_t          weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(key_weight, (key)(weight))
   };

   struct wait_weight {
      uint32_t wait_sec;
      uint16_t weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(wait_weight, (wait_sec)(weight))
   };

   struct authority {
      uint32_t                             threshold = 0;
      std::vector<key_weight>              keys;
      std::vector<permission_level_weight> accounts;
      std::vector<wait_weight>             waits;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(authority, (threshold)(keys)(accounts)(waits))
   };

   struct block_header {
      uint32_t                                timestamp;
      name                                    producer;
      uint16_t                                confirmed = 0;
      checksum256                             previous;
      checksum256                             transaction_mroot;
      checksum256                             action_mroot;
      uint32_t                                schedule_version = 0;
      std::optional<eosio::producer_schedule> new_producers;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(block_header,
                       (timestamp)(producer)(confirmed)(previous)(transaction_mroot)(
                          action_mroot)(schedule_version)(new_producers))
   };

   class [[eosio::contract("led.bios")]] bios : public eosio::contract {
    public:
      using contract::contract;
      [[eosio::action]] void newaccount(name creator, name name, ignore<authority> owner,
                                        ignore<authority> active) {}

      [[eosio::action]] void updateauth(ignore<name> account, ignore<name> permission,
                                        ignore<name> parent, ignore<authority> auth) {}

      [[eosio::action]] void deleteauth(ignore<name> account, ignore<name> permission) {}

      [[eosio::action]] void linkauth(ignore<name> account, ignore<name> code,
                                      ignore<name> type, ignore<name> requirement) {}

      [[eosio::action]] void unlinkauth(ignore<name> account, ignore<name> code,
                                        ignore<name> type) {}

      [[eosio::action]] void canceldelay(ignore<permission_level> canceling_auth,
                                         ignore<checksum256>      trx_id) {}

      [[eosio::action]] void setcode(name account, uint8_t vmtype, uint8_t vmversion,
                                     const std::vector<char> &code) {}

      [[eosio::action]] void setabi(name account, const std::vector<char> &abi);

      [[eosio::action]] void onerror(ignore<uint128_t>         sender_id,
                                     ignore<std::vector<char>> sent_trx);

      [[eosio::action]] void setpriv(name account, uint8_t is_priv);

      [[eosio::action]] void setalimits(name account, int64_t ram_bytes,
                                        int64_t net_weight, int64_t cpu_weight);

      [[eosio::action]] void setprods(std::vector<eosio::producer_key> schedule);

      [[eosio::action]] void setparams(const eosio::blockchain_parameters &params);

      [[eosio::action]] void reqauth(name from);

      [[eosio::action]] void activate(const eosio::checksum256 &feature_digest);

      [[eosio::action]] void reqactivated(const eosio::checksum256 &feature_digest);

      struct [[eosio::table]] abi_hash {
         name        owner;
         checksum256 hash;
         uint64_t    primary_key() const { return owner.value; }

         EOSLIB_SERIALIZE(abi_hash, (owner)(hash))
      };

      typedef eosio::multi_index<"abihash"_n, abi_hash> abi_hash_table;

      using newaccount_action = action_wrapper<"newaccount"_n, &bios::newaccount>;
      using updateauth_action = action_wrapper<"updateauth"_n, &bios::updateauth>;
      using deleteauth_action = action_wrapper<"deleteauth"_n, &bios::deleteauth>;
      using linkauth_action   = action_wrapper<"linkauth"_n, &bios::linkauth>;
      using unlinkauth_action = action_wrapper<"unlinkauth"_n, &bios::unlinkauth>;
      using canceldelay_action = action_wrapper<"canceldelay"_n, &bios::canceldelay>;
      using setcode_action    = action_wrapper<"setcode"_n, &bios::setcode>;
      using setabi_action     = action_wrapper<"setabi"_n, &bios::setabi>;
      using setpriv_action    = action_wrapper<"setpriv"_n, &bios::setpriv>;
      using setalimits_action = action_wrapper<"setalimits"_n, &bios::setalimits>;
      using setprods_action   = action_wrapper<"setprods"_n, &bios::setprods>;
      using setparams_action = action_wrapper<"setparams"_n, &bios::setparams>;
      using reqauth_action   = action_wrapper<"reqauth"_n, &bios::reqauth>;
      using activate_action  = action_wrapper<"activate"_n, &bios::activate>;
      using reqactivated_action = action_wrapper<"reqactivated"_n, &bios::reqactivated>;
   };

}   // namespace eosiobios

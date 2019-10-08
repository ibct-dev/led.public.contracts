#pragma once

#include <eosio/eosio.hpp>
#include <eosio/ignore.hpp>
#include <eosio/transaction.hpp>

namespace eosio {

   class [[eosio::contract("led.wrap")]] wrap : public contract {
      public:
         using contract::contract;

         [[eosio::action]]
         void exec( ignore<name> executer, ignore<transaction> trx );

         using exec_action = eosio::action_wrapper<"exec"_n, &wrap::exec>;
   };

} /// namespace eosio

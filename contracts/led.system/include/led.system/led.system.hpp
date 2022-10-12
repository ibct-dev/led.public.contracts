#pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <led.system/exchange_state.hpp>
#include <led.system/native.hpp>

#include <deque>
#include <optional>
#include <string>
#include <type_traits>
#include <algorithm>

namespace eosiosystem {

   using eosio::asset;
   using eosio::block_timestamp;
   using eosio::check;
   using eosio::const_mem_fun;
   using eosio::datastream;
   using eosio::indexed_by;
   using eosio::name;
   using eosio::same_payer;
   using eosio::symbol;
   using eosio::symbol_code;
   using eosio::time_point;
   using eosio::time_point_sec;
   using eosio::unsigned_int;
   using eosio::current_time_point;

   template<typename E, typename F>
   static inline auto has_field( F flags, E field )
   -> std::enable_if_t< std::is_integral_v<F> && std::is_unsigned_v<F> &&
                        std::is_enum_v<E> && std::is_same_v< F, std::underlying_type_t<E> >, bool>
   {
      return ( (flags & static_cast<F>(field)) != 0 );
   }

   template<typename E, typename F>
   static inline auto set_field( F flags, E field, bool value = true )
   -> std::enable_if_t< std::is_integral_v<F> && std::is_unsigned_v<F> &&
                        std::is_enum_v<E> && std::is_same_v< F, std::underlying_type_t<E> >, F >
   {
      if( value )
         return ( flags | static_cast<F>(field) );
      else
         return ( flags & ~static_cast<F>(field) );
   }

   static constexpr uint32_t seconds_per_year      = 52 * 7 * 24 * 3600;
   static constexpr uint32_t seconds_per_week      = 24 * 3600 * 7;
   static constexpr uint32_t seconds_per_day       = 24 * 3600;
   static constexpr int64_t  useconds_per_year     = int64_t(seconds_per_year) * 1000'000ll;
   static constexpr int64_t  useconds_per_week     = int64_t(seconds_per_week) * 1000'000ll;
   static constexpr int64_t  useconds_per_day      = int64_t(seconds_per_day) * 1000'000ll;
   static constexpr uint32_t blocks_per_year       = 2 * seconds_per_year;  // half seconds per year
   static constexpr uint32_t blocks_per_week       = 2 * seconds_per_week;  // half seconds per week
   static constexpr uint32_t blocks_per_day        = 2 * seconds_per_day;   // half seconds per day
   

   static constexpr int64_t  ram_gift_bytes        = 1400;
   static constexpr int64_t  min_daily_pay         = 100'0000;
   static constexpr double   continuous_rate       = 0.01980;              // 2% annual rate -> ln(1.02)

   static constexpr uint32_t refund_delay_sec      = 3 * seconds_per_day;

   template <typename T>
   int32_t remove_secondary_index( uint64_t code, uint64_t scope, uint64_t table ) {
      using namespace eosio::_multi_index_detail;

      uint64_t pk;

      auto min = secondary_key_traits<T>::lowest();
      auto itr = secondary_index_db_functions<T>::db_idx_lowerbound( code, scope, table, min, pk );

      // itr == -1, no secondary index
      // itr < -1, type mismatch
      if( itr <= -1 ) return itr;

      while( itr > -1 ) {
         auto next_itr = secondary_index_db_functions<T>::db_idx_next( itr, &pk );
         secondary_index_db_functions<T>::db_idx_remove( itr );
         itr = next_itr;
      }

      // secondary index is removed
      return 0;
   }

#define REMOVE_SECONDARY_INDEX( ITR, TYPE, CODE, SCOPE, TABLE ) \
   ITR = remove_secondary_index<TYPE>( CODE, SCOPE, TABLE ); \
   if ( ITR == -1 ) break; \
   else if ( ITR == 0 ) continue;

   struct [[eosio::table("global"), eosio::contract("led.system")]] legis_global_state : eosio::blockchain_parameters {
      uint64_t free_ram()const { return max_ram_size - total_ram_bytes_reserved; }

      uint64_t             max_ram_size = 32ll*1024 * 1024 * 1024; // <TEST> 64g -> 32g
      uint64_t             total_ram_bytes_reserved = 0;
      int64_t              total_ram_stake = 0;
      uint16_t             new_ram_per_block = 0;
      block_timestamp      last_ram_increase;

      block_timestamp      last_producer_size_update;
      uint32_t             maximum_producers = 3;
      uint32_t             half_year_cnt = 0;

      block_timestamp      last_producer_schedule_update;
      time_point           last_bucket_fill;

      int64_t              total_purchase_amount = 0;
      int64_t              total_stake_amount = 0;
      int64_t              total_vote_amount = 0;

      int64_t              perctb_bucket = 0;
      int64_t              pervote_bucket = 0;
      int64_t              perblock_bucket = 0;
      uint32_t             total_unpaid_blocks = 0; /// all blocks which have been produced but not paid

      bool                 activated = 0;

      uint16_t             last_producer_schedule_size = 0;
      double               total_frontier_service_weight = 0;
      double               total_interior_vote_weight = 0; /// the sum of all interiors votes

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE_DERIVED( legis_global_state, eosio::blockchain_parameters,
                                (max_ram_size)(total_ram_bytes_reserved)(total_ram_stake)(new_ram_per_block)(last_ram_increase)
                                (last_producer_size_update)(maximum_producers)(half_year_cnt)
                                (last_producer_schedule_update)(last_bucket_fill)
                                (total_purchase_amount)(total_stake_amount)(total_vote_amount)
                                (perctb_bucket)(pervote_bucket)(perblock_bucket)(total_unpaid_blocks)
                                (activated)
                                (last_producer_schedule_size)(total_frontier_service_weight)(total_interior_vote_weight))
   };

   struct [[eosio::table("global2"), eosio::contract("led.system")]] legis_global_state2 {
      legis_global_state2(){}
      uint8_t              revision = 0; ///< used to track version updates in the future.

      std::vector<name>    active_producers;
      std::vector<name>    proposed_producers;
      uint32_t             last_schedule_version = 0;
      block_timestamp      last_watch_producer_time;
      uint32_t             total_interval_produce_blocks = 0;

      block_timestamp      last_frontier_service_table_update;
      block_timestamp      last_frontier_buyer_table_clear_time;

      EOSLIB_SERIALIZE( legis_global_state2,
                        (revision)
                        (active_producers)(proposed_producers)(last_schedule_version)(last_watch_producer_time)(total_interval_produce_blocks)
                        (last_frontier_service_table_update)(last_frontier_buyer_table_clear_time) )
   };

   struct [[eosio::table, eosio::contract("led.system")]] producer_info2 {
      name                    owner;
      double                  votepay_share;  
      time_point              last_votepay_share_update;

      uint64_t primary_key()const { return owner.value;                             }

      EOSLIB_SERIALIZE( producer_info2, (owner)(votepay_share)(last_votepay_share_update))
   };

   struct [[eosio::table, eosio::contract("led.system")]] producer_info {
      name                    owner;
      bool                    producer_type;  // 0 : interior, 1 : frontier
      eosio::public_key       producer_key; /// a packed public key object
      bool                    is_active = true;
      bool                    is_punished = false;
      uint32_t                demerit = 0;            // <TEST>
      uint32_t                unpaid_blocks = 0;
      uint32_t                interval_produce_blocks = 0;
      time_point              last_claim_time;
      std::string             url;                 // the url of the block producer, normally the url of the block producer presentation website
      uint16_t                location = 0;        // is the country code as defined in the ISO 3166, https://en.wikipedia.org/wiki/List_of_ISO_3166_country_codes
      std::string             logo_256;

      uint64_t primary_key()const { return owner.value;                             }
      bool     active()const      { return is_active;                               }
      bool     punished() const   { return is_punished;                             }
      void     punish()           { is_active = false; is_punished = true;          }
      void     deactivate()       { producer_key = public_key(); is_active = false; }

      EOSLIB_SERIALIZE( producer_info, (owner)(producer_type)(producer_key)(is_active)(is_punished)(demerit)(unpaid_blocks)(interval_produce_blocks)(last_claim_time)(url)(location)(logo_256) )
   };

   struct [[eosio::table, eosio::contract("led.system")]] frontier_info {
      name                    owner;
      bool                    is_active;
      eosio::asset            transfer_ratio;
      double                  service_weights = 0;
      std::string             url;
      uint8_t                 category;
      std::string             logo_256;

      std::vector<name>       buyers;
      std::vector<double>     service_weight_window;
      std::vector<int64_t>    service_weight_window_date;
      uint32_t                service_window_state = 0;
      int32_t                 privIdx = 31;
      double                  decrease_service_weight = 0;

      uint64_t primary_key()const                  { return owner.value;                                                    }
      bool     active()const                       { return is_active;                                                      }
      double   by_services()const                  { return is_active ? -service_weights : service_weights;                 }
      bool     buyer_exists(const name buyer)const { return std::find(buyers.begin(), buyers.end(), buyer) != buyers.end(); }
      void     clear_buyers() {
         if( buyers.size() ) {
            for(auto itr = buyers.begin(); itr != buyers.end();) {
               itr = buyers.erase(itr);
            }
         }
      }
      void     set_service_weight(double weight)   {
         if(!service_weight_window.size()) service_weight_window.resize(30);
         if(!service_weight_window_date.size()) service_weight_window_date.resize(30);
         int64_t service_date = ((current_time_point().sec_since_epoch() - (block_timestamp::block_timestamp_epoch / 1000)) / (24 * 3600)); 
         int32_t idx = service_date % 30; // idx => 0 ~ 29
         service_weight_window_date[idx] = service_date;
         if(idx != privIdx){
            if(privIdx != 31) {
               int priv = privIdx;
               while(priv != idx){
                  service_window_state |= (1 << priv);
                  priv = (priv + 1) % 30;
               }
            }
         }
         if(!(service_window_state & (1 << idx))){ //PENDING state
            service_weight_window[idx] += weight;
            service_weights += weight;
         }
         else{                                  //CLOSED state
            decrease_service_weight += service_weight_window[idx];
            service_weights -= service_weight_window[idx];
            service_window_state &= ~(1 << idx);
            service_weight_window[idx] = weight;
            service_weights += service_weight_window[idx];
         }
         privIdx = idx;
      }
      EOSLIB_SERIALIZE( frontier_info, (owner)(is_active)(transfer_ratio)(service_weights)(url)(category)(logo_256)(buyers)(service_weight_window)(service_weight_window_date)(service_window_state)(privIdx)(decrease_service_weight) )
   };

   struct [[eosio::table, eosio::contract("led.system")]] interior_info {
      name            owner;
      bool            is_active;
      std::string     url;
      std::string     election_promise;
      std::string     logo_256;

      double          vote_weights = 0;

      uint64_t primary_key()const { return owner.value;                              }
      bool     active()const      { return is_active;                                }
      double   by_votes()const    { return is_active ? -vote_weights : vote_weights; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( interior_info, (owner)(is_active)(url)(election_promise)(logo_256)(vote_weights) )
   };

   struct [[eosio::table, eosio::contract("led.system")]] buyer_info {
      name        owner;                   /// the buyer
      int64_t     cumulative_payback = 0;

      uint64_t primary_key()const { return owner.value;        }
      uint64_t by_paybacks()const { return cumulative_payback; }
      EOSLIB_SERIALIZE( buyer_info, (owner)(cumulative_payback) )
   };

   struct [[eosio::table, eosio::contract("led.system")]] voter_info {
      name                        owner; /// the voter
      name                        proxy; /// the proxy set by the voter, if any
      std::vector<name>           interiors; /// the interiors approved by this voter if no proxy set
      int64_t                     staked = 0;

      /**
       *  Every time a vote is cast we must first "undo" the last vote weight, before casting the
       *  new vote weight.  Vote weight is calculated as:
       *
       *  stated.amount * 2 ^ ( weeks_since_launch/weeks_per_year)
       */
      double                      last_vote_weight = 0; /// the vote weight cast the last time the vote was updated
      int64_t                     last_vote = 0;
      int64_t                     last_stake = 0;

      /**
       * Total vote weight delegated to this voter.
       */
      double                      proxied_vote_weight= 0; /// the total vote weight delegated to this voter as a proxy
      bool                        is_proxy = 0; /// whether the voter is a proxy for others

      uint32_t                    flags1 = 0;
      uint32_t                    reserved2 = 0;
      eosio::asset                reserved3;

      uint64_t primary_key()const { return owner.value; }

      enum class flags1_fields : uint32_t {
         ram_managed = 1,
         net_managed = 2,
         cpu_managed = 4
      };

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( voter_info, (owner)(proxy)(interiors)(staked)(last_vote_weight)(last_vote)(last_stake)(proxied_vote_weight)(is_proxy)(flags1)(reserved2)(reserved3) )
   };

   struct [[eosio::table, eosio::contract("led.system")]] proxy_info {
      /**
         *  Set proxy info
         *
         *  @param proxy - the proxy account name
         *  @param slogan - a short description
         *  @param background - who is the proxy?
         *  @param url - optional. url to website
         *  @param location - optional. a location representing a proxy(ISO 3166-1 Numeric)
         *  @param logo_256 - optional. url to an image with the size of 256 x 256 px
      */
      name        owner;
      std::string slogan;
      std::string background;
      std::string url;
      uint16_t    location = 0;        // ISO 3166-1 Numeric
      std::string logo_256;

      uint64_t primary_key()const { return owner.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( proxy_info, (owner)(slogan)(background)(url)(location)(logo_256) )
   };

   struct [[eosio::table]] currency_stats {
      eosio::asset    supply;
      eosio::asset    max_supply;
      name            issuer;

      uint64_t primary_key()const { return supply.symbol.code().raw(); }
   };

   struct [[eosio::table]] account {
      eosio::asset    balance;

      uint64_t primary_key()const { return balance.symbol.code().raw(); }
   };

   struct [[eosio::table, eosio::contract("led.system")]] user_resources {
      name          owner;
      asset         net_weight;
      asset         cpu_weight;
      int64_t       ram_bytes = 0;

      bool is_empty()const { return net_weight.amount == 0 && cpu_weight.amount == 0 && ram_bytes == 0; }
      uint64_t primary_key()const { return owner.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( user_resources, (owner)(net_weight)(cpu_weight)(ram_bytes) )
   };

   /**
    *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
    */
   struct [[eosio::table, eosio::contract("led.system")]] delegated_bandwidth {
      name          from;
      name          to;
      asset         net_weight;
      asset         cpu_weight;

      bool is_empty()const { return net_weight.amount == 0 && cpu_weight.amount == 0; }
      uint64_t  primary_key()const { return to.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( delegated_bandwidth, (from)(to)(net_weight)(cpu_weight) )

   };

   struct [[eosio::table, eosio::contract("led.system")]] refund_request {
      name            owner;
      time_point_sec  request_time;
      eosio::asset    net_amount;
      eosio::asset    cpu_amount;

      bool is_empty()const { return net_amount.amount == 0 && cpu_amount.amount == 0; }
      uint64_t  primary_key()const { return owner.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( refund_request, (owner)(request_time)(net_amount)(cpu_amount) )
   };

   typedef eosio::singleton< "global"_n, legis_global_state >   global_state_singleton;
   typedef eosio::singleton< "global2"_n, legis_global_state2 > global_state2_singleton;

   typedef eosio::multi_index< "producers"_n, producer_info > producers_table;
   typedef eosio::multi_index< "frontiers"_n, frontier_info,
                              indexed_by<"servweights"_n, const_mem_fun<frontier_info, double, &frontier_info::by_services>  >
                             > frontiers_table;
   typedef eosio::multi_index< "interiors"_n, interior_info,
                              indexed_by<"voteweights"_n, const_mem_fun<interior_info, double, &interior_info::by_votes>  >
                             > interiors_table;

   typedef eosio::multi_index< "buyers"_n, buyer_info,
                              indexed_by<"paybacks"_n, const_mem_fun<buyer_info, uint64_t, &buyer_info::by_paybacks>  >
                             > buyers_table;

   typedef eosio::multi_index< "voters"_n, voter_info >  voters_table;

   typedef eosio::multi_index< "proxies"_n, proxy_info >  proxies_table;


   // led.token table
   typedef eosio::multi_index< "stat"_n, currency_stats > stats;

   typedef eosio::multi_index< "accounts"_n, account > accounts;

   /**
    *  These tables are designed to be constructed in the scope of the relevant user, this
    *  facilitates simpler API for per-user queries
    */
   typedef eosio::multi_index< "userres"_n, user_resources >      user_resources_table;
   typedef eosio::multi_index< "delband"_n, delegated_bandwidth > del_bandwidth_table;
   typedef eosio::multi_index< "refunds"_n, refund_request >      refunds_table;

   class [[eosio::contract("led.system")]] system_contract : public native {

      private:
         global_state_singleton  _global;
         global_state2_singleton _global2;

         legis_global_state      _gstate;
         legis_global_state2     _gstate2;

         producers_table         _producers;
         frontiers_table         _frontiers;
         interiors_table         _interiors;
         
         buyers_table            _buyers;
         voters_table            _voters;
         proxies_table           _proxies;

         rammarket               _rammarket;

      public:
         static constexpr eosio::name active_permission{"active"_n};
         static constexpr eosio::name token_account{"led.token"_n};
         static constexpr eosio::name ram_account{"led.ram"_n};
         static constexpr eosio::name ramfee_account{"led.ramfee"_n};
         static constexpr eosio::name stake_account{"led.stake"_n};
         static constexpr eosio::name bpay_account{"led.bpay"_n};
         static constexpr eosio::name cpay_account{"led.cpay"_n};
         static constexpr eosio::name vpay_account{"led.vpay"_n};
         static constexpr eosio::name saving_account{"led.saving"_n};
         static constexpr eosio::name losa_account{"losa"_n};
         static constexpr eosio::name null_account{"led.null"_n};
         static constexpr symbol ramcore_symbol = symbol(symbol_code("RAMCORE"), 4);
         static constexpr symbol ram_symbol     = symbol(symbol_code("RAM"), 0);
         
         system_contract( name s, name code, datastream<const char*> ds );
         ~system_contract();

         static symbol get_core_symbol( name system_account = "led"_n ) {
            rammarket rm(system_account, system_account.value);
            const static auto sym = get_core_symbol( rm );
            return sym;
         }

         // Actions:
         [[eosio::action]]
         void init( unsigned_int version, const symbol& core );
         [[eosio::action]]
         void onblock( ignore<block_header> header );
         [[eosio::action]]
         void activate( const name& activator );
         [[eosio::action]]
         void deactivate( const name& activator );

         [[eosio::action]]
         void setalimits( const name& account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight );

         [[eosio::action]]
         void setacctram( const name& account, const std::optional<int64_t>& ram_bytes );

         [[eosio::action]]
         void setacctnet( const name& account, const std::optional<int64_t>& net_weight );

         [[eosio::action]]
         void setacctcpu( const name& account, const std::optional<int64_t>& cpu_weight );

         // functions defined in delegate_bandwidth.cpp

         /**
          *  Stakes SYS from the balance of 'from' for the benfit of 'receiver'.
          *  If transfer == true, then 'receiver' can unstake to their account
          *  Else 'from' can unstake at any time.
          */
         [[eosio::action]]
         void delegatebw( const name& from, const name& receiver,
                          const asset& stake_net_quantity, const asset& stake_cpu_quantity, bool transfer );

         /**
          *  Decreases the total tokens delegated by from to receiver and/or
          *  frees the memory associated with the delegation if there is nothing
          *  left to delegate.
          *
          *  This will cause an immediate reduction in net/cpu bandwidth of the
          *  receiver.
          *
          *  A transaction is scheduled to send the tokens back to 'from' after
          *  the staking period has passed. If existing transaction is scheduled, it
          *  will be canceled and a new transaction issued that has the combined
          *  undelegated amount.
          *
          *  The 'from' account loses voting power as a result of this call and
          *  all producer tallies are updated.
          */
         [[eosio::action]]
         void undelegatebw( const name& from, const name& receiver,
                            const asset& unstake_net_quantity, const asset& unstake_cpu_quantity );


         /**
          * Increases receiver's ram quota based upon current price and quantity of
          * tokens provided. An inline transfer from receiver to system contract of
          * tokens will be executed.
          */
         [[eosio::action]]
         void buyram( const name& payer, const name& receiver, const asset& quant );
         [[eosio::action]]
         void buyrambytes( const name& payer, const name& receiver, uint32_t bytes );

         /**
          *  Reduces quota my bytes and then performs an inline transfer of tokens
          *  to receiver based upon the average purchase price of the original quota.
          */
         [[eosio::action]]
         void sellram( const name& account, int64_t bytes );

         /**
          *  This action is called after the delegation-period to claim all pending
          *  unstaked tokens belonging to owner
          */
         [[eosio::action]]
         void refund( const name& owner );

         // functions defined in producer.cpp
         [[eosio::action]]
         void punishprod( const name& producer );

         [[eosio::action]]
         void punishoff( const name& producer );

         [[eosio::action]]
         void regfrontier( const name& frontier, const public_key& producer_key, const asset& transfer_ratio, uint8_t category, const std::string& url, uint16_t location, const std::string& logo_256 );

         [[eosio::action]]
         void reginterior( const name& interior, const public_key& producer_key, const std::string& election_promise, const std::string& url, uint16_t location, const std::string& logo_256 );

         [[eosio::action]]
         void unregprod( const name& producer );

         [[eosio::action]]
         void unregproduce( const name& producer );

         [[eosio::action]]
         void changeratio( const name& frontier, const asset& transfer_ratio );

         [[eosio::action]]
         void setram( uint64_t max_ram_size );
         [[eosio::action]]
         void setramrate( uint16_t bytes_per_block );

         [[eosio::action]]
         void voteproducer( const name& voter, const name& proxy, const std::vector<name>& interiors );

         [[eosio::action]]
         void buyservice( const name& buyer, const asset& quantity, const name& frontier );

         [[eosio::action]]
         void buyledservice( const name& buyer, const asset& quantity, const name& frontier );
         
         [[eosio::action]]
         void buyledsvcdom( const name& buyer, const asset& quantity, const name& frontier, const name& provider );

         [[eosio::action]]
         void regproxy( const name& proxy, const std::string& slogan, const std::string& background, const std::string& url, uint16_t location, const std::string& logo_256 );

         [[eosio::action]]
         void unregproxy( const name& proxy );

         [[eosio::action]]
         void setparams( const eosio::blockchain_parameters& params );

         // functions defined in producer_pay.cpp
         [[eosio::action]]
         void claimrewards( const name& owner );

         [[eosio::action]]
         void setpriv( const name& account, uint8_t is_priv );

         [[eosio::action]]
         void rmvproducer( const name& producer );

         [[eosio::action]]
         void updtrevision( uint8_t revision );

         // Dummy action for deeplink
         [[eosio::action]]
         void testaction() { }

         using init_action = eosio::action_wrapper<"init"_n, &system_contract::init>;
         using activate_action = eosio::action_wrapper<"activate"_n, &system_contract::activate>;
         using deactivate_action = eosio::action_wrapper<"deactivate"_n, &system_contract::deactivate>;
         using setacctram_action = eosio::action_wrapper<"setacctram"_n, &system_contract::setacctram>;
         using setacctnet_action = eosio::action_wrapper<"setacctnet"_n, &system_contract::setacctnet>;
         using setacctcpu_action = eosio::action_wrapper<"setacctcpu"_n, &system_contract::setacctcpu>;
         using delegatebw_action = eosio::action_wrapper<"delegatebw"_n, &system_contract::delegatebw>;
         using undelegatebw_action = eosio::action_wrapper<"undelegatebw"_n, &system_contract::undelegatebw>;
         using buyram_action = eosio::action_wrapper<"buyram"_n, &system_contract::buyram>;
         using buyrambytes_action = eosio::action_wrapper<"buyrambytes"_n, &system_contract::buyrambytes>;
         using sellram_action = eosio::action_wrapper<"sellram"_n, &system_contract::sellram>;
         using refund_action = eosio::action_wrapper<"refund"_n, &system_contract::refund>;

         using punishprod_action = eosio::action_wrapper<"punishprod"_n, &system_contract::punishprod>;
         using punishoff_action = eosio::action_wrapper<"punishoff"_n, &system_contract::punishoff>;
         using regfrontier_action = eosio::action_wrapper<"regfrontier"_n, &system_contract::regfrontier>;
         using reginterior_action = eosio::action_wrapper<"reginterior"_n, &system_contract::reginterior>;
         using changeratio_action = eosio::action_wrapper<"changeratio"_n, &system_contract::changeratio>;
         using unregprod_action = eosio::action_wrapper<"unregprod"_n, &system_contract::unregprod>;
         using unregproduce_action = eosio::action_wrapper<"unregproduce"_n, &system_contract::unregproduce>;
         using buyservice_action = eosio::action_wrapper<"buyservice"_n, &system_contract::buyservice>;
         using buyledservice_action = eosio::action_wrapper<"buyledservice"_n, &system_contract::buyledservice>;
         using buyledsvcdom_action = eosio::action_wrapper<"buyledsvcdom"_n, &system_contract::buyledsvcdom>;
         using voteproducer_action = eosio::action_wrapper<"voteproducer"_n, &system_contract::voteproducer>;
         using regproxy_action = eosio::action_wrapper<"regproxy"_n, &system_contract::regproxy>;
         using unregproxy_action = eosio::action_wrapper<"unregproxy"_n, &system_contract::unregproxy>;

         using setram_action = eosio::action_wrapper<"setram"_n, &system_contract::setram>;
         using setramrate_action = eosio::action_wrapper<"setramrate"_n, &system_contract::setramrate>;
         
         using claimrewards_action = eosio::action_wrapper<"claimrewards"_n, &system_contract::claimrewards>;
         using rmvproducer_action = eosio::action_wrapper<"rmvproducer"_n, &system_contract::rmvproducer>;
         using updtrevision_action = eosio::action_wrapper<"updtrevision"_n, &system_contract::updtrevision>;

         using setpriv_action = eosio::action_wrapper<"setpriv"_n, &system_contract::setpriv>;
         using setalimits_action = eosio::action_wrapper<"setalimits"_n, &system_contract::setalimits>;
         using setparams_action = eosio::action_wrapper<"setparams"_n, &system_contract::setparams>;

         using testaction_action = eosio::action_wrapper<"testaction"_n, &system_contract::testaction>;

      private:
         // Implementation details:

         static symbol get_core_symbol( const rammarket& rm ) {
            auto itr = rm.find(ramcore_symbol.raw());
            check(itr != rm.end(), "system contract must first be initialized");
            return itr->quote.balance.symbol;
         }

         //defined in led.system.cpp
         static legis_global_state get_default_parameters();
         symbol core_symbol()const;
         void update_ram_supply();
         bool isCompany( const name& owner );
         bool isPerson( const name& owner );
         void punish( const name& owner );

         // defined in delegate_bandwidth.cpp
         void changebw( name from, const name& receiver,
                        const asset& stake_net_quantity, const asset& stake_cpu_quantity, bool transfer );
         void update_voting_power( const name& voter, const asset& total_update );

         // defined in producer_pay.cpp
         void watch_block_producer( const block_timestamp& timestamp );

         // defined in producer.cpp
         void update_elected_producers( const block_timestamp& timestamp );
         void update_votes( const name& voter_name, const name& proxy, const std::vector<name>& interiors, bool voting );
         void propagate_weight_change( const voter_info& voter );
         
         template <auto system_contract::*...Ptrs>
         class registration {
            public:
               template <auto system_contract::*P, auto system_contract::*...Ps>
               struct for_each {
                  template <typename... Args>
                  static constexpr void call( system_contract* this_contract, Args&&... args )
                  {
                     std::invoke( P, this_contract, args... );
                     for_each<Ps...>::call( this_contract, std::forward<Args>(args)... );
                  }
               };
               template <auto system_contract::*P>
               struct for_each<P> {
                  template <typename... Args>
                  static constexpr void call( system_contract* this_contract, Args&&... args )
                  {
                     std::invoke( P, this_contract, std::forward<Args>(args)... );
                  }
               };

               template <typename... Args>
               constexpr void operator() ( Args&&... args )
               {
                  for_each<Ptrs...>::call( this_contract, std::forward<Args>(args)... );
               }

               system_contract* this_contract;
         };
   };

} /// eosiosystem

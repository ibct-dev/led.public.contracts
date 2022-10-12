#include <eosio/crypto.hpp>
#include <eosio/datastream.hpp>
#include <eosio/eosio.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/privileged.hpp>
#include <eosio/serialize.hpp>
#include <eosio/singleton.hpp>

#include <led.system/led.system.hpp>
#include <led.token/led.token.hpp>

#include <algorithm>
#include <cmath>

namespace eosiosystem {

   using eosio::const_mem_fun;
   using eosio::current_time_point;
   using eosio::indexed_by;
   using eosio::microseconds;
   using eosio::singleton;
   using eosio::token;

   void system_contract::punishprod( const name& producer ) {
      require_auth( "ibct"_n );

      auto prod = _producers.find( producer.value );
      check( prod != _producers.end(), "target producer is not exist" );
      check( !prod->punished(), "producer is already punished" );
      
      _producers.modify( prod, same_payer, [&]( producer_info& info ) {
         info.punish();
      });

      if(prod->producer_type == 1) {
         auto fitr = _frontiers.find( producer.value );
         _frontiers.modify( fitr, same_payer, [&]( frontier_info& info ) {
            info.is_active = false;
         });
      } else {
         auto iitr = _interiors.find( producer.value );
         _interiors.modify( iitr, same_payer, [&]( interior_info& info ) {
            info.is_active = false;
         });
      }
   }

   void system_contract::punishoff( const name& producer ) {
      require_auth( "ibct"_n );

      auto prod = _producers.find( producer.value );
      check( prod != _producers.end(), "producer does not exist" );

      check( prod->punished(), "producer does not punished");

      _producers.modify( prod, producer, [&]( producer_info& info ){
         info.is_punished = false;
         info.is_active = true;
      });

      if(prod->producer_type == 1) {
         auto fitr = _frontiers.find( producer.value );
         _frontiers.modify( fitr, same_payer, [&]( frontier_info& info ) {
            info.is_active = true;
         });
      } else {
         auto iitr = _interiors.find( producer.value );
         _interiors.modify( iitr, same_payer, [&]( interior_info& info ) {
            info.is_active = true;
         });
      }
   }

   void system_contract::regfrontier( const name& frontier, const public_key& producer_key, const asset& transfer_ratio, uint8_t category, const std::string& url, uint16_t location, const std::string& logo_256 ) {
      check( transfer_ratio.amount > 0, "transfer ratio must be positive integer" );
      check( url.size() < 512, "url too long" );
      check( producer_key != eosio::public_key(), "public key should not be the default value" );
      
      check( isCompany(frontier), "frontier registration is only available for company accounts" );
      
      require_auth( frontier );

      auto prod = _producers.find( frontier.value );
      auto fitr = _frontiers.find( frontier.value );
      const auto ct = current_time_point();

      auto sym = transfer_ratio.symbol.code();
      stats statstable( token_account, sym.raw() );
      const auto& st = statstable.get( sym.raw(), "token contract does not have a dapp token symbol" );
      
      accounts fromAccount( token_account, frontier.value );
      const auto& ac = fromAccount.get( sym.raw(), "frontier does not have a dapp token symbol" );
      check( transfer_ratio.amount <= ac.balance.amount, "There is not enough DAPP tokens");

      check( st.issuer.value == frontier.value, "frontier is not the issuer of the DAPP token" );
      
      if ( prod != _producers.end() ) {
         check( !prod->punished(), "suggest punishoff first and get cleared for punishment" );
         check( _interiors.find( frontier.value ) == _interiors.end(), "frontier is already exist in interiors" );
         _producers.modify( prod, frontier, [&]( producer_info& info ){
            info.producer_key          = producer_key;
            info.is_active             = true;
            if ( info.last_claim_time == time_point() )
               info.last_claim_time = ct;
            info.url                   = url;
            info.location              = location;
            info.logo_256              = logo_256;
         });
         _frontiers.modify( fitr, frontier, [&]( frontier_info& info ){
            info.is_active       = true;
            info.url             = url;
            info.category        = category;
            info.logo_256        = logo_256;
            info.transfer_ratio  = asset(transfer_ratio);
         });
      } else {
         _producers.emplace( frontier, [&]( producer_info& info ){
            info.owner           = frontier;
            info.producer_type   = 1;
            info.producer_key    = producer_key;
            info.is_active       = true;
            info.is_punished     = false;
            info.demerit         = 0;
            info.last_claim_time = ct;
            info.url             = url;
            info.location        = location;
            info.logo_256        = logo_256;
         });
         _frontiers.emplace( frontier, [&]( frontier_info& info ){
            info.owner           = frontier;
            info.is_active       = true;
            info.service_weights = 0;
            info.url             = url;
            info.category        = category;
            info.logo_256        = logo_256;
            info.transfer_ratio  = asset(transfer_ratio);
         });
      }
   }

   void system_contract::reginterior( const name& interior, const public_key& producer_key, const std::string& election_promise, const std::string& url, uint16_t location, const std::string& logo_256 ) {
      check( election_promise.length() <= 1024, "election promise too long" );
      check( url.size() < 512, "url too long" );
      check( producer_key != eosio::public_key(), "public key should not be the default value" );
      
      require_auth( interior );

      auto prod = _producers.find( interior.value );
      auto iitr = _interiors.find( interior.value );
      const auto ct = current_time_point();
      
      if ( prod != _producers.end() ) {
         check( !prod->punished(), "suggest punishoff first and get cleared for punishment" );
         check( _frontiers.find( interior.value ) == _frontiers.end(), "interior is already exist in frontiers" );
         _producers.modify( prod, interior, [&]( producer_info& info ){
            info.producer_key          = producer_key;
            info.is_active             = true;
            if ( info.last_claim_time == time_point() )
               info.last_claim_time = ct;
            info.url                   = url;
            info.location              = location;
            info.logo_256              = logo_256;
         });
         _interiors.modify( iitr, interior, [&]( interior_info& info ){
            info.is_active             = true;
            info.url                   = url;
            info.election_promise      = election_promise;
            info.logo_256              = logo_256;
         });
      } else {
         _producers.emplace( interior, [&]( producer_info& info ){
            info.owner           = interior;
            info.producer_type   = 0;
            info.producer_key    = producer_key;
            info.is_active       = true;
            info.is_punished     = false;
            info.demerit         = 0;
            info.last_claim_time = ct;
            info.url             = url;
            info.location        = location;
            info.logo_256        = logo_256;
         });
         _interiors.emplace( interior, [&]( interior_info& info ){
            info.owner            = interior;
            info.is_active        = true;
            info.vote_weights     = 0;
            info.url              = url;
            info.election_promise = election_promise;
            info.logo_256         = logo_256;
         });
      }
   }

   void system_contract::changeratio( const name& frontier, const asset& transfer_ratio ) {
      check( transfer_ratio.amount > 0, "transfer ratio must be positive integer" );
      require_auth( frontier );

      auto fitr = _frontiers.find( frontier.value );
      check( fitr != _frontiers.end(), "frontier does not exist" );

      auto sym = transfer_ratio.symbol.code();
      stats statstable( token_account, sym.raw() );
      const auto& st = statstable.get( sym.raw(), "token contract does not have a dapp token symbol" );
      
      accounts fromAccount( token_account, frontier.value );
      const auto& ac = fromAccount.get( sym.raw(), "frontier does not have a dapp token symbol" );
      check( transfer_ratio.amount <= ac.balance.amount, "There is not enough DAPP tokens");
      
      _frontiers.modify( fitr, frontier, [&]( frontier_info& info ){
         info.transfer_ratio = asset(transfer_ratio);
      });
   }

   void system_contract::unregprod( const name& producer ) {
      require_auth( producer );

      const auto& prod = _producers.get( producer.value, "producer not found" );

      _producers.modify( prod, same_payer, [&]( producer_info& info ){
         info.deactivate();
      });

      if(prod.producer_type == 1) {
         auto fitr = _frontiers.find( producer.value );
         _frontiers.modify( fitr, same_payer, [&]( frontier_info& info ) {
            info.is_active = false;
         });
      } else {
         auto iitr = _interiors.find( producer.value );
         _interiors.modify( iitr, same_payer, [&]( interior_info& info ) {
            info.is_active = false;
         });
      }
   }

   void system_contract::unregproduce( const name& producer ) {
      require_auth( producer );

      auto itr = _producers.find( producer.value );
      if ( itr == _producers.end() ) {
         printf("Producer does not exist in table, nothing to delete.");
         //return;
      } 
      _producers.erase( itr );
      _producers2.erase( itr );
      
      const auto& prod = _producers.get( producer.value, "producer not found" );

      if(prod.producer_type == 1) {
         auto fitr = _frontiers.find( producer.value );
         _frontiers.erase(fitr);
      } else {
         auto iitr = _interiors.find( producer.value );
         _interiors.erase(iitr);
      }
   }


   void system_contract::update_elected_producers( const block_timestamp& block_time ) {
      bool isclear = false;
      _gstate.last_producer_schedule_update = block_time;

      if ((block_time.slot - _gstate2.last_frontier_service_table_update.slot) > blocks_per_day){
         _gstate2.last_frontier_service_table_update = block_time;
         _gstate.total_purchase_amount = 0;
         for(auto& p : _frontiers){
            _frontiers.modify( p, same_payer, [&]( frontier_info& info ){
               info.set_service_weight(0);
               if(info.decrease_service_weight > 0){
                  _gstate.total_frontier_service_weight -= info.decrease_service_weight;
                  info.decrease_service_weight = 0;
               }
               if ((block_time.slot - _gstate2.last_frontier_buyer_table_clear_time.slot) > blocks_per_week * 4){
                  isclear = true;
                  info.clear_buyers();
               }
            });
         }
         if(isclear){
            _gstate2.last_frontier_buyer_table_clear_time = block_time;
         }
      }
      
      if (((block_time.slot - _gstate.last_producer_size_update.slot) > (blocks_per_year / 2)) && _gstate.maximum_producers != 21){
         _gstate.last_producer_size_update = block_time;
         _gstate.maximum_producers += 6;
         _gstate.half_year_cnt += 1;
      }

      auto fidx = _frontiers.get_index<"servweights"_n>();
      auto iidx = _interiors.get_index<"voteweights"_n>();

      std::vector< std::pair<eosio::producer_key,uint16_t> > top_producers;
      top_producers.reserve(_gstate.maximum_producers);

      for ( auto it = fidx.cbegin(); it != fidx.cend() && top_producers.size() < (_gstate.maximum_producers * 2 / 3) && 0 < it->service_weights && it->active(); ++it ) {
         auto& prod = _producers.get( it->owner.value, "frontier not found" );
         top_producers.emplace_back( std::pair<eosio::producer_key,uint16_t>({{prod.owner, prod.producer_key}, prod.location}) );
      }

      for ( auto it = iidx.cbegin(); it != iidx.cend() && top_producers.size() < _gstate.maximum_producers && 0 < it->vote_weights && it->active(); ++it ) {
         auto& prod = _producers.get( it->owner.value, "interior not found" );
         top_producers.emplace_back( std::pair<eosio::producer_key,uint16_t>({{prod.owner, prod.producer_key}, prod.location}) );
      }

      if ( top_producers.size() == 0 || top_producers.size() < _gstate.last_producer_schedule_size ) {
         return;
      }

      /// sort by producer name
      std::sort( top_producers.begin(), top_producers.end() );

      std::vector<eosio::producer_key> producers;

      producers.reserve(top_producers.size());
      _gstate2.proposed_producers.clear();
      for( const auto& item : top_producers ) {
         producers.push_back(item.first);
         _gstate2.proposed_producers.push_back( item.first.producer_name );
      }

      if ( set_proposed_producers( producers ) >= 0 ) {
         _gstate.last_producer_schedule_size = static_cast<decltype(_gstate.last_producer_schedule_size)>(top_producers.size());
      }
   }

   std::string symbol_to_string(const asset val) {
      uint64_t v = val.symbol.code().raw();
      v >>= 8;
      std::string result;
      while (v > 0) {
               char c = static_cast<char>(v & 0xFF);
               result += c;
               v >>= 8;
      }
      return result;
   }

   std::string int64_to_string( const int64_t value ) {
      std::string result;
      result.reserve( 20 ); // max. 20 digits possible
      int64_t q = value;
      do {
         result += "0123456789"[ q % 10 ];
         q /= 10;
      } while ( q );
      std::reverse( result.begin(), result.end() );
      return result;
   }

   double stake2vote( int64_t staked ) {
      /// TODO subtract 2080 brings the large numbers closer to this decade
      double weight = int64_t( (current_time_point().sec_since_epoch() - (block_timestamp::block_timestamp_epoch / 1000)) / (seconds_per_day * 7) )  / double( 52 );
      return double(staked) * std::pow( 2, weight );
   }

   void system_contract::buyservice( const name& buyer, const asset& quantity, const name& frontier ) {
      require_auth( buyer );
      require_recipient( buyer );
      require_recipient( frontier );

      check( buyer != frontier, "cannot buy from self" );
      check( quantity.is_valid(), "invalid quantity" );
      check( quantity.symbol.code() == core_symbol().code(), "this token is not system token" );
      check( quantity.amount > 0, "must positive quantity" );
      check( quantity.amount < 100000001, "must quantity below 10000 LED" );

      auto from_voter = _voters.find( buyer.value );
      if (from_voter == _voters.end()) {
          from_voter = _voters.emplace(buyer, [&](auto& v) {
              v.owner = buyer;
              v.staked = 0;
              v.last_stake = 0;
          });
      }
      // check( from_voter != _voters.end(), "user must stake before they can
      // buy" ); /// staking creates voter object

      accounts fromBuyer(token_account, buyer.value);
      const auto& bn = fromBuyer.get( core_symbol().code().raw(), "buyer does not have a core symbol" );
      check( quantity.amount <= bn.balance.amount, "There is not enough system tokens to buy" );

      auto pitr = _producers.find( frontier.value );
      auto fitr = _frontiers.find( frontier.value );
      check( pitr != _producers.end() && fitr != _frontiers.end(), "target frontier is not exist" );
      check( pitr->active(), "frontier is not currently registered" );
      
      // Frontier가 Dapp Token을 지급할 수 있는지
      auto sym = fitr->transfer_ratio.symbol.code();
      accounts fromAccount(token_account, frontier.value);
      const auto& ac = fromAccount.get(sym.raw(), "frontier does not have a dapp token symbol");
      auto payment_token = asset(static_cast<int64_t>((double)fitr->transfer_ratio.amount * quantity.amount) / 10000, fitr->transfer_ratio.symbol);
      check( payment_token.amount <= ac.balance.amount, "There is not enough DAPP tokens to transfer");

      // frontier에게 service fee 송금
      {
         token::transfer_action buyer_transfer_act{token_account, {buyer, active_permission}};
         buyer_transfer_act.send(buyer, frontier, quantity, "service fee");
      }

      // Dapp Token 지급
      {
         token::transfer_action frontier_transfer_act{token_account, {frontier, active_permission}};
         frontier_transfer_act.send(frontier, buyer, payment_token, "transfer dapp token");
      }

      //KYC Logic
      if ( isPerson( buyer ) ){
         // Buy Service에 해당하는 service weight를 증가
         double service_weight = stake2vote(int64_t(quantity.amount));
         _frontiers.modify( fitr, same_payer, [&]( auto& f ) {
            f.set_service_weight(service_weight);
            _gstate.total_frontier_service_weight += service_weight;
         });

         // Payback
         if(!fitr->buyer_exists( buyer )){
            auto payback_token = asset(static_cast<int64_t>(quantity.amount * 0.05), quantity.symbol);
            {
               token::transfer_action frontier_payback_transfer_act{token_account, {frontier, active_permission}};
               frontier_payback_transfer_act.send(frontier, buyer, payback_token, "frontier payback");
            }
         }
         
         auto buyer_itr = _buyers.find( buyer.value );
         if( buyer_itr == _buyers.end() ) {
            _buyers.emplace( buyer, [&]( auto& b ) {
               b.owner              = buyer;
               b.cumulative_payback = quantity.amount * 0.05;
            });
         } else {
            _buyers.modify( buyer_itr, same_payer, [&]( auto& b ) {
               b.cumulative_payback += quantity.amount * 0.05;
            });
         }
         
         if(!fitr->buyer_exists( buyer )){
            _frontiers.modify( fitr, frontier, [&]( frontier_info& info ){
               info.buyers.push_back( buyer );
            });
         }
      }
      _gstate.total_purchase_amount += quantity.amount;
   }

   void system_contract::buyledservice( const name& buyer, const asset& quantity, const name& frontier ) {
      require_auth( buyer );
      require_recipient( buyer );
      require_recipient( frontier );

      check( buyer != frontier, "cannot buy from self" );
      check( quantity.is_valid(), "invalid quantity" );
      check( quantity.symbol.code() == core_symbol().code(), "this token is not system token" );
      check( quantity.amount > 0, "must positive quantity" );

      auto from_voter = _voters.find(buyer.value);
      if (from_voter == _voters.end()) {
          from_voter = _voters.emplace(buyer, [&](auto& v) {
              v.owner = buyer;
              v.staked = 0;
              v.last_stake = 0;
          });
      }
      // check( from_voter != _voters.end(), "user must stake before they can
      // buy" ); /// staking creates voter object

      accounts fromBuyer(token_account, buyer.value);
      const auto& bn = fromBuyer.get( core_symbol().code().raw(), "buyer does not have a core symbol" );
      check( quantity.amount <= bn.balance.amount, "There is not enough system tokens to buy" );

      auto pitr = _producers.find( frontier.value );
      auto fitr = _frontiers.find( frontier.value );
      check( pitr != _producers.end() && fitr != _frontiers.end(), "target frontier is not exist" );
      check( pitr->active(), "frontier is not currently registered" );
      
      // frontier에게 service fee 송금
      {
         token::transfer_action buyer_transfer_act{token_account, {buyer, active_permission}};
         buyer_transfer_act.send(buyer, frontier, quantity, "service fee");
      }

      //KYC Logic
      if ( isPerson( buyer ) ){
         // Buy Service에 해당하는 service weight를 증가
         double service_weight = stake2vote(int64_t(quantity.amount));
         _frontiers.modify( fitr, same_payer, [&]( auto& f ) {
            f.set_service_weight(service_weight);
            _gstate.total_frontier_service_weight += service_weight;
         });
         
         auto buyer_itr = _buyers.find( buyer.value );
         if( buyer_itr == _buyers.end() ) {
            _buyers.emplace( buyer, [&]( auto& b ) {
               b.owner              = buyer;
            });
         }
         
         if(!fitr->buyer_exists( buyer )){
            _frontiers.modify( fitr, frontier, [&]( frontier_info& info ){
               info.buyers.push_back( buyer );
            });
         }
      }
      _gstate.total_purchase_amount += quantity.amount;
   }

   void system_contract::buyledsvcdom( const name& buyer, const asset& quantity, const name& frontier, const name& provider ) {
      require_auth( buyer );
      require_recipient( buyer );
      require_recipient( frontier );
      require_recipient( provider );

      check( buyer != provider, "cannot buy from self" );
      check( provider != frontier, "cannot same provider and frontier" );
      check( name(provider.suffix().to_string() + ".c") == frontier, "provider must be sub-account of frontier");
      check( quantity.is_valid(), "invalid quantity" );
      check( quantity.symbol.code() == core_symbol().code(), "this token is not system token" );
      check( quantity.amount > 0, "must positive quantity" );

      auto from_voter = _voters.find( buyer.value );
      if (from_voter == _voters.end()) {
          from_voter = _voters.emplace(buyer, [&](auto& v) {
              v.owner = buyer;
              v.staked = 0;
              v.last_stake = 0;
          });
      }
      // check( from_voter != _voters.end(), "user must stake before they can buy" ); /// staking creates voter object

      accounts fromBuyer(token_account, buyer.value);
      const auto& bn = fromBuyer.get( core_symbol().code().raw(), "buyer does not have a core symbol" );
      check( quantity.amount <= bn.balance.amount, "There is not enough system tokens to buy" );

      auto pitr = _producers.find( frontier.value );
      auto fitr = _frontiers.find( frontier.value );
      check( pitr != _producers.end() && fitr != _frontiers.end(), "target frontier is not exist" );
      check( pitr->active(), "frontier is not currently registered" );
      
      // 실 서비스 제공자인 provider에게 service fee 송금
      {
         token::transfer_action buyer_transfer_act{token_account, {buyer, active_permission}};
         buyer_transfer_act.send(buyer, provider, quantity, "service fee");
      }

      //KYC Logic
      if ( isPerson( buyer ) ){
         // Buy Service에 해당하는 service weight를 frontier의 것으로 증가
         double service_weight = stake2vote(int64_t(quantity.amount));
         _frontiers.modify( fitr, same_payer, [&]( auto& f ) {
            f.set_service_weight(service_weight);
            _gstate.total_frontier_service_weight += service_weight;
         });
         
         auto buyer_itr = _buyers.find( buyer.value );
         if( buyer_itr == _buyers.end() ) {
            _buyers.emplace( buyer, [&]( auto& b ) {
               b.owner              = buyer;
            });
         }
         
         if(!fitr->buyer_exists( buyer )){
            _frontiers.modify( fitr, frontier, [&]( frontier_info& info ){
               info.buyers.push_back( buyer );
            });
         }
      }
      _gstate.total_purchase_amount += quantity.amount;
   }

   void system_contract::voteproducer( const name& voter, const name& proxy, const std::vector<name>& interiors ) {
      require_auth( voter );
      update_votes( voter, proxy, interiors, true );
   }

   void system_contract::update_votes( const name& voter_name, const name& proxy, const std::vector<name>& interiors, bool voting ) {
      //validate input
      if ( proxy ) {
         check( interiors.size() == 0, "cannot vote for interiors and proxy at same time" );
         check( voter_name != proxy, "cannot proxy to self" );
      } else {
         check( interiors.size() <= (_gstate.maximum_producers / 2), "attempt to vote for too many producers" );
         for( size_t i = 1; i < interiors.size(); ++i ) {
            check( interiors[i-1] < interiors[i], "interior votes must be unique and sorted" );
         }
      }

      auto voter = _voters.find( voter_name.value );
      check( voter != _voters.end(), "user must stake before they can vote" ); /// staking creates voter object
      check( !proxy || !voter->is_proxy, "account registered as a proxy is not allowed to use a proxy" );

      auto new_vote_weight = stake2vote( voter->staked );
      int64_t new_staked = voter->staked;
      
      if (!interiors.size() && !proxy) {
         _gstate.total_vote_amount -= voter->last_vote;
         new_staked = 0;
      } else {
         _gstate.total_vote_amount -= voter->last_vote;
         _gstate.total_vote_amount += new_staked;
      }

      if( voter->is_proxy ) {
         new_vote_weight += voter->proxied_vote_weight;
      }

      std::map<name, std::pair<double, bool /*new*/> > producer_deltas;
      if ( voter->last_vote_weight > 0 ) {
         if( voter->proxy ) {
            auto old_proxy = _voters.find( voter->proxy.value );
            check( old_proxy != _voters.end(), "old proxy not found" ); //data corruption
            _voters.modify( old_proxy, same_payer, [&]( auto& vp ) {
                  vp.proxied_vote_weight -= voter->last_vote_weight;
               });
            propagate_weight_change( *old_proxy );
         } else {
            for( const auto& p : voter->interiors ) {
               auto& d = producer_deltas[p];
               d.first -= voter->last_vote_weight;
               d.second = false;
            }
         }
      }

      if( proxy ) {
         auto new_proxy = _voters.find( proxy.value );
         check( new_proxy != _voters.end(), "invalid proxy specified" ); //if ( !voting ) { data corruption } else { wrong vote }
         check( !voting || new_proxy->is_proxy, "proxy not found" );
         if ( new_vote_weight >= 0 ) {
            _voters.modify( new_proxy, same_payer, [&]( auto& vp ) {
                  vp.proxied_vote_weight += new_vote_weight;
               });
            propagate_weight_change( *new_proxy );
         }
      } else {
         if( new_vote_weight >= 0 ) {
            for( const auto& p : interiors ) {
               auto& d = producer_deltas[p];
               d.first += new_vote_weight;
               d.second = true;
            }
         }
      }

      for( const auto& pd : producer_deltas ) {
         auto pitr = _producers.find( pd.first.value );
         auto iitr = _interiors.find( pd.first.value );
         if( pitr != _producers.end() ) {
            if( voting && !pitr->active() && pd.second.second /* from new set */ ) {
               check( false, ( "interior " + pitr->owner.to_string() + " is not currently registered" ).data() );
            }
            double init_vote_weights = iitr->vote_weights;
            _interiors.modify( iitr, same_payer, [&]( auto& i ) {
               i.vote_weights += pd.second.first;
               if ( i.vote_weights < 0 ) { // floating point arithmetics can give small negative numbers
                  i.vote_weights = 0;
               }
               _gstate.total_interior_vote_weight += pd.second.first;
               //check( p.vote_weights >= 0, "something bad happened" );
            });
         } else {
            if( pd.second.second ) {
               check( false, ( "interior " + pd.first.to_string() + " is not registered" ).data() );
            }
         }
      }

      _voters.modify( voter, same_payer, [&]( auto& av ) {
         av.last_vote_weight = new_vote_weight;
         av.last_vote        = new_staked;
         av.interiors        = interiors;
         av.proxy            = proxy;
      });
   }

   void system_contract::regproxy( const name& proxy, const std::string& slogan, const std::string& background, const std::string& url, uint16_t location, const std::string& logo_256 ) {
      // Validate input
      check( isPerson(proxy), "proxy registration is only available for kyc personal accounts" );

      check(slogan.length() <= 64, "slogan too long");

      check(background.length() <= 1024, "background too long");

      check(url.length() <= 256, "url too long");
      if (!url.empty()) {
         check(url.substr(0, 4) == "http", "url should begin with http");
      }

      check(logo_256.length() <= 256, "logo_256 too long");
      if (!logo_256.empty()) {
            check(logo_256.substr(0, 4) == "http", "logo_256 should begin with http");
      }

      // Require auth from the proxy account
      require_auth( proxy );

      // Check if exists
      auto pitr = _voters.require_find( proxy.value, "you must stake first" );
      auto current = _proxies.find( proxy.value );

      // Update
      if ( current != _proxies.end() ) {
            _proxies.modify( current, same_payer, [&]( auto& i ) {
               i.owner      = proxy;
               i.slogan     = slogan;
               i.background = background;
               i.url        = url;
               i.location   = location;
               i.logo_256   = logo_256;
            });
      // Insert
      } else {
            _proxies.emplace( proxy, [&]( auto& i ) {
               i.owner      = proxy;
               i.slogan     = slogan;
               i.background = background;
               i.url        = url;
               i.location   = location;
               i.logo_256   = logo_256;
            });
      }

      _voters.modify( pitr, same_payer, [&]( auto& p ) {
         p.is_proxy = 1;
      });
      propagate_weight_change( *pitr );
   }

   void system_contract::unregproxy( const name& proxy ) {
      require_auth( proxy );
      auto pitr = _proxies.require_find( proxy.value, "proxy does not exist" );
      auto vitr = _voters.require_find( proxy.value, "proxy does not exist" );
      _voters.modify( vitr, same_payer, [&]( auto& p ) {
         p.is_proxy = 0;
      });
      propagate_weight_change( *vitr );
      _proxies.erase( pitr );
   }

   void system_contract::propagate_weight_change( const voter_info& voter ) {
      check( !voter.proxy || !voter.is_proxy, "account registered as a proxy is not allowed to use a proxy" );
      double new_weight = stake2vote( voter.staked );
      if ( voter.is_proxy ) {
         new_weight += voter.proxied_vote_weight;
      }

      /// don't propagate small changes (1 ~= epsilon)
      if ( fabs( new_weight - voter.last_vote_weight ) > 1 )  {
         if ( voter.proxy ) {
            auto& proxy = _voters.get( voter.proxy.value, "proxy not found" ); //data corruption
            _voters.modify( proxy, same_payer, [&]( auto& p ) {
                  p.proxied_vote_weight += new_weight - voter.last_vote_weight;
               }
            );
            propagate_weight_change( proxy );
         } else {
            auto delta = new_weight - voter.last_vote_weight;
            for ( auto acnt : voter.interiors ) {
               auto& iitr = _interiors.get( acnt.value, "interior not found" ); //data corruption
               _interiors.modify( iitr, same_payer, [&]( auto& i ) {
                     i.vote_weights += delta;
                     _gstate.total_interior_vote_weight += delta;
               });
            }
         }
      }
      _voters.modify( voter, same_payer, [&]( auto& v ) {
            v.last_vote_weight = new_weight;
         }
      );
   }
   
} /// namespace eosiosystem
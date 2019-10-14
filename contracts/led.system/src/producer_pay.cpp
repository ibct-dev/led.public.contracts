#include <eosio/producer_schedule.hpp>

#include <led.system/led.system.hpp>
#include <led.token/led.token.hpp>

namespace eosiosystem {

   using eosio::current_time_point;
   using eosio::microseconds;
   using eosio::token;

   void system_contract::watch_block_producer( const block_timestamp& timestamp ) {
      uint32_t total_interval_block_threshold = (timestamp.slot - _gstate2.last_watch_producer_time.slot) * 0.90;
      uint32_t prod_interval_block_threshold = total_interval_block_threshold / _gstate.maximum_producers;

      if( _gstate2.total_interval_produce_blocks < total_interval_block_threshold ) {
         for ( auto acnt : _gstate2.active_producers ) {
            auto& prod = _producers.get( acnt.value, "producer not found" );  //data corruption
            if( prod.interval_produce_blocks < prod_interval_block_threshold ) {
               _producers.modify(prod, same_payer, [&](auto& p) {
                  if( !p.is_punished ) punish(acnt);
                  p.demerit++;
               });
            }
         }
      } 
      for ( auto acnt : _gstate2.active_producers ) {
         auto& prod = _producers.get( acnt.value, "producer not found" );  //data corruption
         _producers.modify(prod, same_payer, [&](auto& p) {
            p.interval_produce_blocks = 0;
         });
      }
      
      _gstate2.last_watch_producer_time      = timestamp;
      _gstate2.total_interval_produce_blocks = 0;
   }

   void system_contract::onblock( ignore<block_header> ) {
      using namespace eosio;

      require_auth(get_self());

      block_timestamp                           timestamp;
      name                                      producer;
      uint16_t                                  confirmed;
      checksum256                               previous;
      checksum256                               transaction_mroot;
      checksum256                               action_mroot;
      uint32_t                                  schedule_version;

      _ds >> timestamp >> producer >> confirmed >> previous >> transaction_mroot >> action_mroot >> schedule_version;

      /** until activated no new rewards are paid */
      if( !_gstate.activated )
         return;

      if( _gstate.last_bucket_fill == time_point() )  /// start the presses
         _gstate.last_bucket_fill = current_time_point();

      /**
       * At startup the initial producer may not be one that is registered / elected
       * and therefore there may be no producer object for them.
       */
      auto prod = _producers.find( producer.value );
      if ( prod != _producers.end() ) {
         _gstate.total_unpaid_blocks++;
         _producers.modify( prod, same_payer, [&](auto& p ) {
               p.unpaid_blocks++;
               p.interval_produce_blocks++;
               _gstate2.total_interval_produce_blocks++;
         });
      }

      if( schedule_version > _gstate2.last_schedule_version ) {
         watch_block_producer( timestamp );
         _gstate2.active_producers = _gstate2.proposed_producers;
         _gstate2.last_schedule_version = schedule_version;
      }

      // timer is expired
      if( (timestamp.slot - _gstate2.last_watch_producer_time.slot) > 7200 ) {    // <TEST> 7200 => 120
         watch_block_producer( timestamp );
      }

      /// only update block producers once every minute, block_timestamp is in half seconds
      if( timestamp.slot - _gstate.last_producer_schedule_update.slot > 120 ) {
         update_elected_producers( timestamp );
      }
   }

   using namespace eosio;
   void system_contract::claimrewards( const name& owner ) {
      require_auth( owner );

      const auto& prod = _producers.get( owner.value );
      check( prod.active(), "producer does not have an active key" );

      check( _gstate.activated,
                    "cannot claim rewards until the chain is activated" );

      const auto ct = current_time_point();

      check( ct - prod.last_claim_time > microseconds(useconds_per_day), "already claimed rewards within past day" );

      const asset token_supply   = token::get_supply(token_account, core_symbol().code() );
      const auto usecs_since_last_fill = (ct - _gstate.last_bucket_fill).count();

      if( usecs_since_last_fill > 0 && _gstate.last_bucket_fill > time_point() ) {
         auto new_tokens = static_cast<int64_t>( (continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill)) / double(useconds_per_year) );

         auto to_producers       = (new_tokens / 4) * _gstate.half_year_cnt;
         auto to_rexpool         = (new_tokens / 4);
         auto to_saving          = new_tokens - (to_producers + to_rexpool);
         
         auto to_per_block_pay   = to_producers / 5;
         auto to_per_vote_pay    = to_producers / 5;
         auto to_per_ctb_pay     = to_producers - (to_per_block_pay + to_per_vote_pay);
         
         {
            token::issue_action issue_act{token_account, {{get_self(), active_permission}}};
            issue_act.send(get_self(), asset(new_tokens, core_symbol()), "issue tokens for producer pay and rexpools");
         }
         {
            token::transfer_action transfer_act{token_account, {{get_self(), active_permission}}};
            transfer_act.send(get_self(), rexpay_account, asset(to_rexpool, core_symbol()), "fund rex pool");
            transfer_act.send(get_self(), saving_account, asset(to_saving, core_symbol()), "fund saving pool");
            transfer_act.send(get_self(), bpay_account, asset(to_per_block_pay, core_symbol()), "fund per-block bucket");
            transfer_act.send(get_self(), cpay_account, asset(to_per_ctb_pay, core_symbol()), "fund per-ctb bucket");
            transfer_act.send(get_self(), vpay_account, asset(to_per_vote_pay, core_symbol()), "fund per-vote bucket");
         }
         channel_to_rex(rexpay_account, asset(to_rexpool, core_symbol()));

         _gstate.perblock_bucket += to_per_block_pay;
         _gstate.perctb_bucket   += to_per_ctb_pay;
         _gstate.pervote_bucket  += to_per_vote_pay;
         
         _gstate.last_bucket_fill = ct;
      }

      auto iitr = _interiors.find( owner.value );
      auto fitr = _frontiers.find( owner.value );

      int64_t producer_per_block_pay = 0;
      if( _gstate.total_unpaid_blocks > 0 ) {
         producer_per_block_pay = (_gstate.perblock_bucket * prod.unpaid_blocks) / _gstate.total_unpaid_blocks;
      }

      int64_t interior_per_vote_pay = 0;
      int64_t frontier_per_ctb_pay  = 0;

      if( iitr != _interiors.end() ) {
         if( _gstate.total_interior_vote_weight > 0 ) {
            interior_per_vote_pay = int64_t((_gstate.pervote_bucket * iitr->vote_weights) / _gstate.total_interior_vote_weight);
         }
         if( interior_per_vote_pay < min_daily_pay ) {
            interior_per_vote_pay = 0;
         }
      } else if ( fitr != _frontiers.end() ) {
         if( _gstate.total_frontier_service_weight > 0 ) {
            frontier_per_ctb_pay = int64_t((_gstate.perctb_bucket * fitr->service_weights) / _gstate.total_frontier_service_weight);
         }
         if( frontier_per_ctb_pay < min_daily_pay ) {
            frontier_per_ctb_pay = 0;
         }
      }
      
      _gstate.pervote_bucket      -= interior_per_vote_pay;
      _gstate.perctb_bucket       -= frontier_per_ctb_pay;
      _gstate.perblock_bucket     -= producer_per_block_pay;
      _gstate.total_unpaid_blocks -= prod.unpaid_blocks;

      _producers.modify( prod, same_payer, [&](auto& p) {
         p.last_claim_time = ct;
         p.unpaid_blocks   = 0;
      });

      if ( producer_per_block_pay > 0 ) {
         token::transfer_action transfer_act{ token_account, { {bpay_account, active_permission}, {owner, active_permission} } };
         transfer_act.send( bpay_account, owner, asset(producer_per_block_pay, core_symbol()), "producer block pay" );
      }
      if ( interior_per_vote_pay > 0 ) {
         token::transfer_action transfer_act{ token_account, { {vpay_account, active_permission}, {owner, active_permission} } };
         transfer_act.send( vpay_account, owner, asset(interior_per_vote_pay, core_symbol()), "producer vote pay" );
      }
      if ( frontier_per_ctb_pay > 0 ) {
         token::transfer_action transfer_act{ token_account, { {cpay_account, active_permission}, {owner, active_permission} } };
         transfer_act.send( cpay_account, owner, asset(frontier_per_ctb_pay, core_symbol()), "producer contribution pay" );
      }
   }

} //namespace eosiosystem

# led.contracts

## Version : 1.7.0

IBCT's Ledgis is an EOSIO-based block chain aimed at ensuring fair value for all users who provide or use quality services. To achieve this goal, IBCT has developed a consensus algorithm called DPoSS and has new policies such as buyservice. This repository contains examples of smart contracts which would be deployed to Ledgis mainnet. They are provided for reference purposes and can be changed anytime to enhance their features or fix errors.

   * [led.system](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.system)
   * [led.token](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.token)
   * [led.bios](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.bios)
   * [led.msig](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.msig)
   * [led.wrap](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.wrap)
   * [led.forum](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.forum)

Dependencies:
* [led v1.8.x](https://github.com/ibct-dev/LEDGIS/releases/tag/v1.8.4)
* [eosio.cdt v1.6.x](https://github.com/EOSIO/eosio.cdt/releases/tag/v1.6.3)

To build the contracts and the unit tests:
* First, ensure that your __legis__ is compiled to the core symbol for the Ledgis blockchain that intend to deploy to.
* Second, make sure that you have ```sudo make install```ed __legis__.
* Then just run the ```build.sh``` in the top directory to build all the contracts and the unit tests for these contracts.

After build (Automated tests are not supported yet):
* The unit tests executable is placed in the _build/tests_ and is named __unit_test__.
* The contracts are built into a _bin/\<contract name\>_ folder in their respective directories.
* Finally, simply use __cleos__ to _set contract_ by pointing to the previously mentioned directory.
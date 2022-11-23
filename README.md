<p align="center">
   <img width=40% src="https://user-images.githubusercontent.com/25478540/200444314-e2180ca2-caa8-4a34-86ea-7b205045cd63.png">
</p>

<h1 align="center">IBCT: LEDGIS MainNet Contracts</h1>

<div align="center">

[![Version](https://img.shields.io/badge/version-1.7.0-6fc5fb.svg)](https://github.com/ibct-dev/led.public.contracts/tree/v1.7.x/)
[![Ledgis](https://img.shields.io/badge/led-1.8.4-010e20.svg)](https://github.com/ibct-dev/LEDGIS/tree/v1.8.4/)
[![eosiocdt](https://img.shields.io/badge/eosiocdt-1.6.3-2ca045.svg)](https://github.com/EOSIO/eosio.cdt/tree/v1.6.3/)
[![Language](https://img.shields.io/badge/language-C++-d80089.svg)](http://www.cplusplus.com/)

</div>

## Overview

**IBCT**'s **[LEDGIS](https://github.com/ibct-dev/LEDGIS)** is an EOSIO-based blockchain aimed at _ensuring fair value for all users who provide or use quality services_. To achieve this goal, IBCT has developed a consensus algorithm called _DPoSS_ and has new policies such as _buyservice_. This repository contains examples of smart contracts which would be deployed to LEDGIS mainnet. They are provided for reference purposes and can be changed anytime to enhance their features or fix errors.

## Contracts

-   [led.system](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.system)
-   [led.token](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.token)
-   [led.bios](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.bios)
-   [led.msig](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.msig)
-   [led.wrap](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.wrap)
-   [led.forum](https://github.com/ibct-dev/led.public.contracts/tree/master/contracts/led.forum)

## Dependencies

-   [eosio.cdt v1.6.x](https://github.com/EOSIO/eosio.cdt/releases/tag/v1.6.3)
-   [LEDGIS v1.8.x](https://github.com/ibct-dev/LEDGIS/releases/tag/v1.8.4)

## Build

To build the contracts and unit-tests:

-   First, ensure that your **LEDGIS** is compiled to the core symbol for the LEDGIS blockchain that intend to deploy to.
-   Second, make sure that you have **LEDGIS** made by `sudo make install`.
-   Then, just run the `build.sh` in the top directory to build all the contracts.
-   The build requires the exact location of `nodeos` and `eosio.cdt`.
-   If you want to build also unit-tests for the contracts, run the build script with `-t` option.
-   Note that the tests require LEDGIS version dependency as `1.8.x`.

After build (Automated tests are not supported yet):

-   The unit-tests 'executable' would be placed in the _`build/tests/`_ with name **"unit_test"**.
-   The contracts are built into a _`bin/<contract-name>`_ folder in their respective directories.
-   Finally, simply use **"cleos"** to _`set contract`_ by pointing to the previously mentioned directory.

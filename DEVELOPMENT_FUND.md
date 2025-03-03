# JunkCoin Development Fund

JunkCoin includes a development fund that allocates a portion of each block reward to support ongoing development and maintenance of the JunkCoin ecosystem.

## Overview

- **Allocation**: 20% of each block reward is allocated to the development fund
- **Active Height Range**: Blocks from height 350,000 to 1,400,000 on mainnet
- **Distribution**: The development fund output is sent to a designated address that changes at specific height intervals

## Technical Implementation

The development fund is implemented directly in the JunkCoin Core codebase:

1. When a block is mined, the coinbase transaction includes two outputs:
   - First output: 80% of the block reward (plus all transaction fees) goes to the miner
   - Second output: 20% of the block reward goes to the development fund address

2. The development fund address is determined based on the current block height, rotating through a predefined list of addresses at specific intervals.

## For Miners

### Internal Mining

If you're mining using JunkCoin Core's internal miner, the development fund output is automatically included in blocks you mine. No additional configuration is required.

### External Mining

When mining with external software (like minerd, cgminer, etc.), the JunkCoin Core node will automatically add the development fund output to blocks you submit. You don't need to configure your mining software to include the development fund output.

The node will:
1. Receive the block template from your mining software
2. Add the development fund output to the coinbase transaction
3. Adjust the miner reward accordingly
4. Recalculate the merkle root
5. Process the block normally

## Verification

You can verify that the development fund is working correctly by:

1. Examining the coinbase transaction of any block within the development fund height range
2. Confirming that it has at least two outputs
3. Verifying that one output sends 20% of the block reward to the correct development fund address

## Development Fund Addresses

The development fund uses the following addresses:

### Mainnet
- `7hMacNfGt1UEGshvWkRamRxZKUKeJhJ7J6`

### Testnet
- `3PM5bKKhggNFzYjLnLsbUF7XHNSuA4bSVY`
- `39Ak9GuMHfpWL3VoTm9NigyELzPC5toiE4`
- `35tfGskRDxU3tWU3n5n2uvCqeHmGDKorVN`

### Regtest
- `3PSpnu5Fdt34u2EEdHZjfaogcVCMC72h24`

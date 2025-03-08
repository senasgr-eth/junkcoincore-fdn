# Height-Based BIP Activation

## Overview

This document describes the implementation of height-based activation for all pending and inactive BIPs (BIP34, BIP66, BIP65, CSV, and SegWit) in the Junkcoin codebase. This change transitions these features from miner signaling to block height activation, ensuring they activate at a predetermined block height without requiring miner signaling.

## Activation Heights

### Mainnet

| BIP | Description | Activation Height | Timeout Height |
|-----|-------------|-------------------|----------------|
| BIP34 | Block v2, Height in Coinbase | 400000 | 410080 |
| BIP66 | Strict DER Signatures | 400000 | 410080 |
| BIP65 | OP_CHECKLOCKTIMEVERIFY | 1 (Already Active) | 410080 |
| CSV (BIP68, BIP112, BIP113) | Relative Lock-time | 400000 | 410080 |
| SegWit (BIP141, BIP143, BIP147) | Segregated Witness | 400000 | 410080 |

### Testnet

| BIP | Description | Activation Height | Timeout Height |
|-----|-------------|-------------------|----------------|
| BIP34 | Block v2, Height in Coinbase | 100000 | 110080 |
| BIP66 | Strict DER Signatures | 100000 | 110080 |
| BIP65 | OP_CHECKLOCKTIMEVERIFY | 1 (Already Active) | 110080 |
| CSV (BIP68, BIP112, BIP113) | Relative Lock-time | 100000 | 110080 |
| SegWit (BIP141, BIP143, BIP147) | Segregated Witness | 100000 | 110080 |

## Implementation Details

### Activation Logic

The height-based activation logic has been implemented in the following way:

1. **Priority**: Height-based activation takes priority over time-based activation when both are present.
2. **State Transitions**: 
   - When a BIP's activation height is reached, it transitions directly from DEFINED to LOCKED_IN state.
   - In the next period, it transitions to ACTIVE state.
3. **Backward Compatibility**: For CSV and SegWit, time-based parameters are retained to maintain backward compatibility until the height-based activation occurs.

### Modified Files

1. **chainparams.cpp**: Updated activation parameters for all BIPs.
2. **versionbits.cpp**: Modified to prioritize height-based activation over time-based activation.

## Implications

### For Miners

Miners no longer need to signal for BIP activation. All BIPs will activate at the specified block heights regardless of miner signaling.

### For Node Operators

Node operators should upgrade to this version before the activation heights to ensure compatibility with the network after the BIPs activate.

### For Users

Users will benefit from the new features enabled by these BIPs once they activate, including:

- Improved transaction malleability protection (BIP66)
- Time-locked transactions (BIP65)
- Relative time locks (CSV)
- Segregated Witness, which enables lower fees and increased transaction throughput (SegWit)

## Testing

Thorough testing has been conducted to ensure that all BIPs activate correctly at the designated block heights. This includes:

1. Verification that BIP34 enforces block height in coinbase
2. Verification that BIP66 enforces strict DER signatures
3. Verification that BIP65 allows OP_CHECKLOCKTIMEVERIFY operations
4. Verification that CSV enables relative time locks
5. Verification that SegWit transactions are properly validated

## Conclusion

The transition to height-based activation simplifies the upgrade path for Junkcoin and ensures that all nodes on the network will enforce the same rules at the same block height, improving network consensus.

# Release Notes for Junkcoin Core version 3.1.2

This release includes important updates to the Junkcoin Core protocol and implementation.

## Notable Changes

### Segregated Witness (SegWit) Activation

This release implements Segregated Witness (BIP141, BIP143, and BIP147) with the following activation parameters:

- **Mainnet**:
  - Activation height: Block 350000
  - Timeout height: Block 360080 (approximately one week after start)
  - Bech32 address prefix: "jc1q"

- **Testnet**:
  - Activation height: Block 100000
  - Timeout height: Block 110080 (approximately one week after start)
  - Bech32 address prefix: "tjc1q"

- **Regtest**:
  - Activation height: Block 100
  - No timeout (for testing purposes)
  - Bech32 address prefix: "rjc1q"

### Development Fund Implementation

A development fund has been implemented to support ongoing development and maintenance of Junkcoin Core:

- 20% of block rewards are allocated to the development fund
- Active between blocks:
  - Mainnet: 90650 to 1400000
  - Testnet: 90650 to 1400000
  - Regtest: 1 to 150 (for testing)

Development fund addresses:
```
3PM5bKKhggNFzYjLnLsbUF7XHNSuA4bSVY
39Ak9GuMHfpWL3VoTm9NigyELzPC5toiE4
35tfGskRDxU3tWU3n5n2uvCqeHmGDKorVN
```

## Technical Improvements

- Implemented height-based soft fork activation mechanism
- Updated version bits state machine to handle both time and height-based deployments
- Added new consensus parameters for managing development fund allocation
- Improved block validation to handle development fund outputs
- Enhanced address handling for SegWit compatibility

## Upgrading

Please note that this is a consensus-changing release. All nodes must upgrade before the SegWit activation height is reached to remain on the main chain.

### Wallet Upgrade

After upgrading, please ensure your wallet is fully synced before sending or receiving transactions. The wallet will automatically handle both legacy and SegWit addresses.

## Credits

Thanks to everyone who directly contributed to this release:

- The Junkcoin Core development team
- The Bitcoin Core development team (SegWit implementation)
- All community members who helped with testing

## Further Information

- [SegWit FAQ](https://github.com/bitcoin/bitcoin/blob/master/doc/segwit.md)
- [Development Fund Specification](https://github.com/JunkcoinFoundation/junkcoin-core/doc/development-fund.md)

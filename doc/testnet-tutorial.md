# Junkcoin Testnet Tutorial

## Resetting Testnet

If you need to restart the testnet from block 1 while retaining the genesis block (block 0), follow these steps:

1. Stop the Junkcoin daemon:
```bash
sudo systemctl stop junkcoind
```

2. Clear the testnet blockchain data (this preserves your wallet.dat):
```bash
sudo rm -rf ~/.junkcoin/testnet3/blocks/* ~/.junkcoin/testnet3/chainstate/*
```

3. Start the Junkcoin daemon in testnet mode:
```bash
sudo junkcoind -testnet -daemon
```

## Testnet Configuration

The testnet is configured with the following parameters (from chainparams.cpp):

- Block time target: 60 seconds
- Initial difficulty (nBits): 0x1e0ffff0
- Minimum difficulty blocks allowed: Yes
- Solo mining allowed: Yes (no peers required)
- Non-standard transactions allowed: Yes
- Default port: 19771

## Mining on Testnet

Since the testnet has no DNS seeds configured, you'll need to either:
1. Add peers manually, or
2. Generate blocks through mining (recommended for testing)

To start mining on testnet:

1. Get a new testnet address:
```bash
junkcoin-cli -testnet getnewaddress
```

2. Generate blocks to your address:
```bash
junkcoin-cli -testnet generatetoaddress 1 your_testnet_address
```

## Important Notes

- The testnet uses the same genesis block as mainnet (block 0)
- Testnet data is stored in ~/.junkcoin/testnet3/
- Wallet.dat is preserved during blockchain reset
- Mining difficulty is lower on testnet
- No DNS seeds are configured, so initial connections may be limited

## Checking Testnet Status

Check current block height:
```bash
junkcoin-cli -testnet getblockcount
```

Check network status:
```bash
junkcoin-cli -testnet getnetworkinfo
```

Add a node manually:
```bash
junkcoin-cli -testnet addnode "ip:port" "add"
```

## Troubleshooting

If the testnet is stuck:
1. Check debug.log for errors: `tail -f ~/.junkcoin/testnet3/debug.log`
2. Verify network connections: `junkcoin-cli -testnet getpeerinfo`
3. Consider restarting with the steps above
4. If mining doesn't start, check CPU usage and system resources

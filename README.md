# Junkcoin Core Wallet

The **Junkcoin Core Wallet** repository hosts the source code for the core wallet of Junkcoin, a historic blockchain launched on **May 3, 2013**, and revived in **November 2024**. Designed as a fork of Litecoin, Junkcoin combines the reliability of Litecoin with exciting features like **random bonus blocks**, making mining a unique and engaging experience.

This repository serves as the foundation for the development, maintenance, and continuous improvement of the Junkcoin Core Wallet, ensuring stability, security, and performance.

## Branches
This repository follows a clear branching strategy:
- **`main`**: Stable releases. This is the default branch, hosting thoroughly tested and production-ready code.
- **`dev`**: Active development and feature work. All new contributions and updates are first merged here before reaching `main`.

## Mining Rules
Junkcoin operates on a **Proof-of-Work (PoW)** consensus mechanism. Its current mining parameters include:

- **Total supply**: Approximately 54 million JKC.
- **Block time**: 1 minute.
- **Difficulty adjustment**: Once per day.
- **Mining duration**: 12 years, ending with the total coin emission.
- **Reward schedule**:
  - **Day 1**: 500 coins per block.
  - **Day 2**: 200 coins per block.
  - **Day 3 & 4**: 100 coins per block.
  - **From Day 5 onwards**: 50 coins per block, halving every 2 years (or 518,400 blocks).

### **Lottery Blocks (Random Bonus Blocks)**
In addition to the regular mining rewards, Junkcoin features **random bonus blocks** to incentivize miners and add excitement to the mining process. These include:
- **1% chance**: A block will yield **triple** the normal reward (e.g., 150 coins per block during the first 2 years).
- **0.01% chance (1 in 10,000)**: A block will yield **1,000 coins**, regardless of the mining phase.

This innovative feature brings an element of unpredictability and fun to the mining process, setting Junkcoin apart from other PoW cryptocurrencies.

## Ports

### **Default Ports**
- **Connection**: 9771
- **JSON-RPC**: 9772

### **Testnet Ports**
- **Connection**: 19771
- **JSON-RPC**: 19772

## Official Junkcoin Website and Community Forum
For more information, please visit the official Junkcoin website:
[https://junk-coin.com](https://junk-coin.com)

## License
This project is licensed under the **MIT License**, ensuring it remains open-source and community-driven.

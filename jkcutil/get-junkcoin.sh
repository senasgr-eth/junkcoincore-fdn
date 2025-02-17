#!/bin/bash

sudo apt -y update
sudo apt-get install -y libc6-dev g++-multilib python p7zip-full pwgen jq curl
cd ~

if [ -f junkcoin.zip ]
then
    rm junkcoin.zip
fi
wget -O junkcoin.zip `curl -s 'https://api.github.com/repos/junkcoin-fdn/junkcoin-core/releases/latest' | jq -r '.assets[].browser_download_url' | egrep "junkcoin.+x64.zip"`
7z x -y junkcoin.zip
chmod -R a+x ~/junkcoin-pkg
rm junkcoin.zip

cd ~/junkcoin-pkg
./fetch-params.sh

if ! [[ -d ~/.junkcoin ]]
then
    mkdir -p ~/.junkcoin
fi

if ! [[ -f ~/.junkcoin/junkcoin.conf ]]
then
    echo "rpcuser=rpc`pwgen 15 1`" > ~/.junkcoin/junkcoin.conf
    echo "rpcpassword=rpc`pwgen 15 1`" >> ~/.junkcoin/junkcoin.conf
fi

./junkcoind

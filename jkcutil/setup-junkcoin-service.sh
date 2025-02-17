#!/bin/bash

if [ $# -eq 0 ]
then
    echo "Junkcoin systemd unit setup."
    echo -e "Run:\n$0 user\nor install for current user\n$0 $USER"
    exit 1
fi

if id "$1" >/dev/null 2>&1
then
    echo "Installing BitcoinZ service for $1 user..."
else
    echo -e "User $1 does not exist.\nTo add user run the following command:\nsudo adduser --disabled-password --gecos '' $1"
    exit 1
fi

cat > /tmp/config_setup.sh << EOF
#!/bin/bash
if ! [[ -d ~/.junkcoin ]]
then
    mkdir -p ~/.junkcoin
fi

if ! [[ -f ~/.junkcoin/junkcoin.conf ]]
then
    echo "rpcuser=rpc`pwgen 15 1`" > ~/.junkcoin/junkcoin.conf
    echo "rpcpassword=rpc`pwgen 15 1`" >> ~/.junkcoin/junkcoin.conf
fi
EOF
chmod +x /tmp/config_setup.sh
sudo -H -u $1 /tmp/config_setup.sh
sudo -H -u $1 ~/junkcoin-pkg/fetch-params.sh


cat > /etc/systemd/system/junkcoin.service << EOF
[Unit]
Description=junkcoin

[Service]
ExecStart=`cd ~; pwd`/junkcoin-pkg/junkcoind
User=$1
Restart=always


[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable junkcoin
systemctl start junkcoin

systemctl status junkcoin

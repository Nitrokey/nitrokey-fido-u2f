#!/bin/bash
echo
echo "Install Nitrokey FIDO U2F Udev rules file on Ubuntu-like OS."
echo
echo "*** Copy rules file and restart udev service"
set -x

sudo cp ./70-u2f.rules /etc/udev/rules.d/ -v --backup
sudo udevadm control --reload-rules && sudo udevadm trigger

set +x
echo "*** Finished"
echo

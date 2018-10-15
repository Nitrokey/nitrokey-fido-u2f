sudo cp ./70-u2f.rules /etc/udev/rules.d/ -v
sudo udevadm control --reload-rules && udevadm trigger

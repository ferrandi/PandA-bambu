#!/bin/bash
vagrant box update
vagrant plugin install vagrant-vbguest
vagrant up
vagrant ssh-config > config.txt
scp -F config.txt default:/home/vagrant/panda-0.9.7-dev/build/bambu-x86_64.AppImage .
vagrant halt

#!/bin/bash
vagrant box update
vagrant plugin install vagrant-vbguest
vagrant up
vagrant ssh-config > config.txt
scp -F config.txt default:/home/vagrant/PandA-bambu/build/bambu-x86_64.AppImage .
vagrant halt

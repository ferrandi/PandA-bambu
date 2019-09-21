#!/bin/bash
vagrant box update
vagrant plugin install vagrant-vbguest
vagrant up
vagrant halt

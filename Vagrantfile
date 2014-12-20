# -*- mode: ruby -*-
# vi: set ft=ruby :

require 'socket'

Vagrant.configure("2") do |config|

# Determine the base box to use by checking the hostname.
# Add your hostname to the list to opt-out of nubots-14.04.
# # 'precise32' was used prior to 2014-04-30.
# config.vm.box = "precise32"
# config.vm.box_url = "http://files.vagrantup.com/precise32.box"

# The NUbots changed to 'trusty32' on 2014-04-30.
config.vm.box = "trusty32"
config.vm.box_url = "https://cloud-images.ubuntu.com/vagrant/trusty/current/trusty-server-cloudimg-i386-vagrant-disk1.box"


  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  # config.vm.network :public_network

  # Enable provisioning with Puppet stand alone.  Puppet manifests
  # are contained in a directory path relative to this Vagrantfile.
  config.vm.provision :puppet do |puppet|
    puppet.manifests_path = "puppet/manifests"
    puppet.module_path = "puppet/modules"
    puppet.manifest_file  = "nodes.pp"
  end

  # config.vm.provider "virtualbox" do |v|
  #   v.gui = true
  # end

  # Define the NUbots development VM, and make it the primary VM
  # (meaning that a plain `vagrant up` will only create this machine)
  config.vm.define "nubotsvm", primary: true do |nubots|
    nubots.vm.hostname = "nubotsvm.nubots.net"

    nubots.vm.network :private_network, ip: "192.168.33.77"

    nubots.vm.network :forwarded_port, guest: 12000, host: 12000
    nubots.vm.network :forwarded_port, guest: 12001, host: 12001

    # Add hostname here if running NUbugger on the VM
    if ['Ne', 'jordan-XPS13', 'taylor-ubuntu'].include?(Socket.gethostname) # NUbugger Port
      nubots.vm.network :forwarded_port, guest: 9090, host: 9090
    end

    #if ['s24'].include?(Socket.gethostname) # NUbugger Port
    #  nubots.vm.network :public_network, bridge: "WiFi"
    #end

    # Syntax: "path/on/host", "/path/on/guest"
    # nubots.vm.synced_folder ".", "/home/vagrant/nubots/NUbots"

    # Note: Use NFS for more predictable shared folder support.
    #   The guest must have 'apt-get install nfs-common'
    nubots.vm.synced_folder ".", "/home/vagrant/nubots/NUbots"

    # Share NUbugger repository with the VM if it has been placed in the same
    # directory as the NUbots repository
    if File.directory?("../NUbugger")
      nubots.vm.synced_folder "../NUbugger", "/home/vagrant/nubots/NUbugger"
    end
  end
end
#!/bin/bash

<<LICENSE

Copyright (C) 2020  Gioacchino Mazzurco <gio@eigenlab.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

LICENSE

<<README
Generate a clean DHT bootstrap node list.
Feeds on previously known nodes from standard input and a few well known
mainline DHT bootstrap nodes. Prints only nodes that appears to be active to a
quick nmap check. Make sure your internet connection is working well before
using this.

Example usage:
--------------------------------
cat bdboot.txt | bdboot_generate.sh | tee /tmp/bdboot_generated.txt
cat /tmp/bdboot_generated.txt | sort -u > bdboot.txt
--------------------------------
README

function check_dht_host()
{
	mHost="$1"
	mPort="$2"
	
	sudo nmap -oG - -sU -p $mPort $mHost | grep open | \
		awk '{print $2" "$5}' | awk -F/ '{print $1}'
}

cat | while read line; do
	hostIP="$(echo $line | awk '{print $1}')"
	hostPort="$(echo $line | awk '{print $2}')"
	check_dht_host $hostIP $hostPort
done

check_dht_host router.utorrent.com 6881
check_dht_host router.bittorrent.com 6881
check_dht_host dht.libtorrent.org 25401
check_dht_host dht.transmissionbt.com 6881


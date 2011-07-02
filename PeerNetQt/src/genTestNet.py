#!/usr/bin/python
# must be compatible with 2.6.1 (OSX10.6)

import random, os, stat


basePort = 20000;
ipaddr = '127.0.0.1'; 
nPeers = 30
nConnect = 20;
nGUIs = 3;
fBlockedDirect = 0.3;
fBlockedProxy = 0.1;

# for OSX.
#execpath = '../../PeerNetQt.app/Contents/MacOS/PeerNetQt';
# for Linux
execpath = '../../PeerNetQt';

# function to generate a folder for a peer.
def generatePeer(folder, peerId, restrict, directfriends, proxyfriends, relayfriends):
  return;


def generateNet(basefolder, npeers, nfriends, fracProxy, fracRelay, nguis):
  ids = [];
  folders = [];
  ports = [];
  blocks = [];
  relays = [];

  portRange = npeers * 100;
  blockrange = portRange * fracProxy;
  relayrange = portRange * fracRelay;
  if (relayrange > blockrange):
    relayrange = blockrange;

  for i in range(npeers):
    # generate an id.
    id = generatePeerId();
    folder = 'p%03d' % (i+1);
    port = basePort + (i+1) * 100;
    block = random.randint(basePort, basePort + portRange);
    if (1 == random.randint(0, 1)):
      relay = block + blockrange - relayrange; #relay is at the upper end
    else:
      relay = block; #relay is at the lower end.

    # add into a array;
    folders.append(folder);
    ids.append(id);
    ports.append(port);
    blocks.append(block);
    relays.append(relay);

  shuffledIdxs = range(npeers);
  # use the shuffle to choose the gui ones.
  random.shuffle(shuffledIdxs);
  guis = shuffledIdxs[0:nguis];
  if (os.path.exists(basefolder)):
    print 'Warning Path: %s already exists' % basefolder;
  else:
    os.mkdir(basefolder);

  # open up the master script.
  scriptfilename = '%s/runall.sh' % (basefolder);
  fd = open(scriptfilename, 'w');
  fd.write('#!/bin/sh\n\n');

  #
  analysisfilename = '%s/checkforerrs.sh' % (basefolder);
  fd2 = open(analysisfilename, 'w');
  fd2.write('#!/bin/sh\n\n');

  #
  warningfilename = '%s/checkforwarnings.sh' % (basefolder);
  fd3 = open(warningfilename, 'w');
  fd3.write('#!/bin/sh\n\n');

  #
  exprfilename = '%s/checkforexpr.sh' % (basefolder);
  fd4 = open(exprfilename, 'w');
  fd4.write('#!/bin/sh\n\n');
  fd4.write('if [$# -lt 2]\n');
  fd4.write('then\n');
  fd4.write('  echo \"script needs expression as argument\"\n');
  fd4.write('fi\n\n')

  #
  lsfilename = '%s/checklogs.sh' % (basefolder);
  fd5 = open(lsfilename, 'w');
  fd5.write('#!/bin/sh\n\n');

  for i in range(npeers):
    random.shuffle(shuffledIdxs);
    print 'Peer %d : %s' % (i+1, ids[i]);
    print '\tFolder: %s' % (folders[i]);
    print '\tProxy Range: %d-%d' % (blocks[i], blocks[i] + blockrange);
    print '\tRelay Range: %d-%d' % (relays[i], relays[i] + relayrange);
    print '\tFriends:'
    friends = [];
    for j in range(nfriends):
      fid = shuffledIdxs[j];
      if (fid != i):
        print '\t\tIdx: %d Port: %d Id: %s' % (fid, ports[fid], ids[fid])
        friends.append(ids[fid]);
      else:
        print '\t\tSkipping Self as Peer!'
  
    folder = '%s/%s' % (basefolder, folders[i]);
    if (os.path.exists(folder)):
      print 'Warning Path: %s already exists' % folder;
    else:
      os.mkdir(folder);

    # now generate the files.
    genbdboot(folder, ipaddr, ports);
    makePeerRunScript(folder, i in guis, ports[i], blocks[i], blockrange, relays[i], relayrange);
    makePeerFriendList(folder, friends);
    makePeerConfig(folder, ids[i]);

    fd.write('cd %s\n' % folders[i]);
    fd.write('./run.sh &\n');
    fd.write('cd ..\n\n');
  
    fd2.write('echo ------------ PEER FOLDER: %s\n' % folders[i]);
    fd2.write('cd %s\n' % folders[i]);
    fd2.write('grep -n ERROR pn.log\n');
    fd2.write('cd ..\n\n');

    fd3.write('echo ------------ PEER FOLDER: %s\n' % folders[i]);
    fd3.write('cd %s\n' % folders[i]);
    fd3.write('grep -n WARNING pn.log\n');
    fd3.write('cd ..\n\n');

    fd4.write('echo ------------ PEER FOLDER: %s\n' % folders[i]);
    fd4.write('cd %s\n' % folders[i]);
    fd4.write('grep -n -A 10 $1 pn.log\n');
    fd4.write('cd ..\n\n');


    fd5.write('echo ------------ PEER FOLDER: %s\n' % folders[i]);
    fd5.write('cd %s\n' % folders[i]);
    fd5.write('ls -l pn.log\n');
    fd5.write('cd ..\n\n');

  fd.close();
  fd2.close();
  fd3.close();
  fd4.close();
  fd5.close();

  os.chmod(scriptfilename, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IROTH); 
  os.chmod(analysisfilename, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IROTH); 
  os.chmod(warningfilename, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IROTH); 
  os.chmod(exprfilename, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IROTH); 
  os.chmod(lsfilename, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IROTH); 

  return;





def generatePeerId():
  length = 20;
  id = "";
  for i in range(length):
    r = random.randint(0, 255);
    id += "%02x" % (r);
  return id;

def genbdboot(folder, ipaddr, ports):
  filename = '%s/bdboot.txt' % (folder);
  fd = open(filename, 'w');
  for port in ports:
    fd.write(('%s %d\n' % (ipaddr, port)));
  
  return;

def makePeerConfig(folder, id):
  return;

def makePeerRunScript(folder, gui, port, proxyblock, proxyrange, relayblock, relayrange):
  filename = '%s/run.sh' % (folder);
  fd = open(filename, 'w');
  noguistr = '-n';
  if (gui):
    noguistr = '';

  fd.write('#/bin/sh\n\n');
  fd.write('EXEC=%s\n\n' % execpath);
  #fd.write(('$EXEC %s -l -p %d -r %d-%d -R %d-%d -c . > /dev/null 2>&1' % (noguistr, port, proxyblock, proxyblock+proxyrange, relayblock, relayblock+relayrange)));
  fd.write(('$EXEC %s -l -p %d -r %d-%d -R %d-%d -c . > pn.log 2>&1' % (noguistr, port, proxyblock, proxyblock+proxyrange, relayblock, relayblock+relayrange)));
  
  fd.close();
  os.chmod(filename, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IROTH); 
  return;

def makePeerFriendList(folder, friends):
  filename = '%s/peerlist.txt' % (folder);
  fd = open(filename, 'w');
  for friend in friends:
    fd.write('%s\n' % friend);
  
  return;

def makePeerConfig(folder, id):
  filename = '%s/peerconfig.txt' % (folder);
  fd = open(filename, 'w');
  fd.write('%s\n' % id);
  fd.close();
  return;



	
tmpId = generatePeerId()
print tmpId;

generateNet('testpeernet', nPeers, nConnect, fBlockedDirect, fBlockedProxy, nGUIs);

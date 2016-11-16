import os
import sys
import signal
from optparse import OptionParser
from threading import Thread, current_thread
import netifaces as ni
import socket
# import ipaddress
import time
import subprocess

'''
multicast cases:
	1. packet from tap interface
	2. config number = 1,2,5
unicast cases:
	1. packet from tap interface
	2. config number = 0,3,4,6,7
'''

multicast_config = ['1','2','5']
unicast_config = ['0','3','4','6','7']
triggers = []
device_map = {} # ipv6 -> wlan(ipv4)
riot_ip = ''
tap_ip = ''
wlan_ip = ''
blacklist = []

parser = OptionParser()

def pprint(arg):
    name = current_thread().getName()
    if name == 'Thread-1':
        print('\033[94m'+str(arg)+'\033[00m')
    elif name == 'Thread-2':
        print('\033[92m'+str(arg)+'\033[00m')
    else:
        print(str(arg))

def ip_validate(addr, ipv4):
    if ipv4:
        addr = addr.split('.')
        if len(addr) == 4:
            for value in addr:
                if not value.isdigit() or int(value) < 0 or int(value) > 255:
                    return False
        else:
            return False
    else:
        if addr == '::1' or addr == '::':
            return True
        addr = addr.split('%')[0].split(':')
        if len(addr) <= 8 and len(addr) >=3:
            for value in addr:
                try:
                    v = int(value, 16)
                    if v < 0 or v > 65535:
                        return False
                except ValueError:
                    if value == '':
                        continue
                    return False
        else:
            return False
    return True

def multicast(data, port=8809, ipv4=True):
    pprint("multicast():")
    pprint("data -> "+data)
    pprint("port -> "+str(port))
    pprint("is_ipv4 -> "+str(ipv4))
    global device_map
    global wlan_ip
    flag = False
    if ipv4:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.bind((wlan_ip, 0))
    else:
        # this part will never be used in our case
        pprint("[-]Anomally detected in multicast function!!!")
        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    sock.sendto(data, ('<broadcast>', port))
    sock.close()
    pprint("[+]sent")
    return True

def unicast(data, address, ipv4):
    pprint("unicast():")
    pprint("data -> "+data)
    pprint("address -> "+str(address))
    pprint("is_ipv4 ->"+str(ipv4))
    global parser
    options, args = parser.parse_args()
    command = 'echo'
    ip = address[0]
    if ipv4:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        if not ip_validate(ip, True):
            pprint("[-]Invalid ip")
            return False
    else:
        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        if not ip_validate(ip, False):
            pprint("[-]Invalid ip")
            return False
    sock.sendto(data, address)
    pprint("[+]sent")
    sock.close()
    return True

def createTap(iface):
    pprint("createTap():")
    pprint("iface -> "+iface)
    os.system('sudo ip tuntap add '+iface+' mode tap user ${USER}')
    os.system('sudo ip link set '+iface+' up')

def startUDPserver(port, ipv4):
    pprint("\033[00m")
    pprint("startUDPserver():")
    pprint("port -> "+str(port))
    pprint("is_ipv4 -> "+str(ipv4))
    if ipv4:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        server_address = ('0.0.0.0', int(port))
    else:
        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        server_address = ("::", int(port))
    sock.bind(server_address)
    return sock

def listen(sock, lis):
    pprint("listen():")
    pprint("sock -> "+str(sock))
    pprint("lis -> "+lis)
    global multicast
    global unicast
    global triggers
    global device_map
    global riot_ip
    global tap_ip
    global wlan_ip
    while(True):
        data, address = sock.recvfrom(4096)
        pprint("while():")
        pprint("[+]Received {} from {}".format(data, address))
        if data and len(data.split(' '))>=3:
            values = data.split(' ')
            config = values[0]
            ip = values[-2]
            if config in multicast_config and lis=='local' and ip == "::":
                pprint("[+]multicast local")
                riot_ip = address
                pprint("[+]riot_ip-> "+str(riot_ip))
                multicast(data)
            elif config in unicast_config and lis=='local':
                pprint("[+]unicast local")
                try:
                    unicast(data, (device_map[ip], 8809), True)
                except Exception, e:
                    pprint(e)
            elif lis=='remote':
                # check if it is a broadcast packet from my own ip
                if config in multicast_config and address[0] == wlan_ip:
                    pprint("[+]Packet discarded")
                    continue
                # end
                pprint("[+]remote")
                remote_ip = values[-1]
                pprint("[+]Source of Data: "+str(remote_ip))
                if ip_validate(remote_ip, True):
                    if remote_ip not in triggers:
                        triggers.append(address[0])
                elif ip_validate(remote_ip, False):
                    if remote_ip not in device_map.keys():
                        device_map[remote_ip] = address[0]
                else:
                    pprint("[-]Invalid ip")
                    continue
                pprint("[+]riot_ip-> "+str(riot_ip))
                unicast(data, riot_ip, False)
            else:
                pass
            pprint("[+]Device map:")
            pprint(device_map)
            pprint("[+]Triggers:")
            pprint(triggers)
        else:
            pprint("[-]Invalid data format!!!")

if __name__ == "__main__":
    global parser
    parser.add_option("-t", "--tap", dest="tap", help="Specify the name of the tap interface to create")
    parser.add_option("-w", "--wireless", dest="wlan", help="Specify the name of the wireless interface")
    parser.add_option("-p", "--path", dest="path", help="Specify the path of riot application")
    parser.add_option("-4", action="store_true", dest="ipv4", help="Use for ipv4 addressing", default=False) 
    (options, args) = parser.parse_args()

    pprint('[+]Creating tap interface '+options.tap)
    createTap(options.tap)

    pprint('[+]Assigning ip to tap interface')
    if os.getenv('RIOT_APP'):
        command = 'bash -c \"cd '+os.getenv('RIOT_APP')+' && sudo make term\"'
    else:
        command = 'bash -c \"cd '+options.path+' && sudo make term\"'
    p = subprocess.call("timeout 5 xterm -e "+command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    pprint('\n\nConnect to wifi and press Enter!!!\n\n')
    raw_input()

    global tap_ip
    global riot_ip
    global wlan_ip
    interfaces = ni.interfaces()
    if options.tap in interfaces and options.wlan in interfaces:
        try:
            tap_ip = ni.ifaddresses(options.tap)[10][0]['addr'].split('%')[0]
            wlan_ip = ni.ifaddresses(options.wlan)[2][0]['addr']
            # riot_ip = str(ipaddress.IPv6Address(unicode(tap_ip))+1)
        except Exception, e:
            pprint(e)
            pprint("[+]IP addresses are not properly alloted to interfaces")
            sys.exit(0)
    else:
        pprint('[+]Error Occurred: one or two interfaces is not present')
        sys.exit(0)
    pprint("tap_ip->"+tap_ip)
    pprint("wlan_ip->"+wlan_ip)

    # used for local packet from tap
    pprint("[+]Starting UDP listening server on port 8808 (Listening on tap interface)")
    t_local = Thread(target=listen, args=(startUDPserver(8808, ipv4=options.ipv4), 'local'))
    t_local.daemon = True
    t_local.start()

    # used for remote packet from wifi
    pprint("[+]Starting UDP listening server on port 8809 (Listening on wireless interface)")
    t_remote = Thread(target=listen, args=(startUDPserver(8809, ipv4=True), 'remote'))
    t_remote.daemon = True
    t_remote.start()

    pprint("\n\nStart riot instance!!!\n\n")

    # keeping the main thread running
    while True:
        pass

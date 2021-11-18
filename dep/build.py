import os
import time
def do_os(cmd):
	b = os.system(cmd)
	if b != 0:
		exit(1)
if os.path.isfile('done'):
	print('done')
	exit()
if not os.path.exists('include'):
	os.mkdir('include')
if not os.path.exists('lib'):
	os.mkdir('lib')
print ('download micro-ecc')

do_os("wget https://gitee.com/nicehero/micro-ecc/attach_files/848500/download/micro-ecc-1.0.tar.gz")
do_os("tar xvf micro-ecc-1.0.tar.gz")
do_os("mv micro-ecc-1.0 include/micro-ecc")
do_os("rm -rf micro-ecc-1.0.tar.gz")

print ('build micro-ecc')

if os.name == "nt":
	do_os("gcc -c include/micro-ecc/uECC.c")
	do_os("ar -r lib/libuECC.a uECC.o")
else:
	#do_os("gcc -c include/micro-ecc/uECC.c")
	#do_os("ar -rcs lib/libuECC.a uECC.o")
	do_os("gcc -shared -fPIC -o lib/libuECC.so include/micro-ecc/uECC.c")
do_os("rm -rf uECC.o")

print ('end micro-ecc')
time.sleep(2)
print ('download asio')
do_os("wget https://gitee.com/nicehero/asio/attach_files/879378/download/asio-asio-1-19-2.tar.gz")
do_os("tar xvf asio-asio-1-19-2.tar.gz")
do_os("mv asio-asio-1-19-2/asio/include/asio include/")
do_os("mv asio-asio-1-19-2/asio/include/asio.hpp include/asio/")
do_os("rm -rf asio-asio-1-19-2")
do_os("rm -rf asio-asio-1-19-2.tar.gz")
time.sleep(2)
print ('download tiny_sha3')
do_os("git clone https://gitee.com/nicehero/tiny_sha3")
do_os("mv tiny_sha3 include/")
if os.name == "nt":
	do_os("gcc -c include/tiny_sha3/sha3.c")
	do_os("ar -r lib/libsha3.a sha3.o")
else:
	#do_os("gcc -c include/tiny_sha3/sha3.c")
	#do_os("ar -rcs lib/libsha3.a sha3.o")
	do_os("gcc -shared -fPIC -o lib/libsha3.so include/tiny_sha3/sha3.c")
do_os("rm -rf sha3.o")

print ('download kcp')
do_os("wget https://gitee.com/nicehero/kcp/attach_files/848501/download/kcp-1.7.tar.gz")
do_os("tar xvf kcp-1.7.tar.gz")
do_os("mv kcp-1.7 include/kcp")
do_os("rm -rf kcp-1.7.tar.gz")
print ('build kcp')
if os.name == "nt":
	body = open("include/kcp/ikcp.c","rb").read().decode()
	body = body.replace("vsprintf(buffer","vsnprintf(buffer,1024")
	open("include/kcp/ikcp.c","wb").write(body.encode())
	do_os("gcc -c include/kcp/ikcp.c")
	do_os("ar -r lib/libikcp.a ikcp.o")
else:
	do_os("gcc -shared -fPIC -o lib/libikcp.so include/kcp/ikcp.c")
do_os("rm -rf ikcp.o")

do_os("echo 0.1 > done")
print ('done')
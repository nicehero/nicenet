import os
import time
import shutil

GIT_SSL = "-c http.sslVerify=false"

def do_os(cmd):
    print(f"  -> {cmd}")
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

print('download micro-ecc')
do_os(f"git {GIT_SSL} clone --depth 1 --branch v1.0 https://gitee.com/nicehero/micro-ecc.git include/micro-ecc")
shutil.rmtree('include/micro-ecc/.git', ignore_errors=True)

print('build micro-ecc')
if os.name == "nt":
    do_os("gcc -c include/micro-ecc/uECC.c")
    do_os("ar -r lib/libuECC.a uECC.o")
else:
    do_os("gcc -shared -fPIC -o lib/libuECC.so include/micro-ecc/uECC.c")
do_os("rm -rf uECC.o")

print('end micro-ecc')
time.sleep(2)

print('download asio')
do_os(f"git {GIT_SSL} clone --depth 1 --branch asio-1-19-2 https://gitee.com/nicehero/asio.git _asio_tmp")
do_os("mv _asio_tmp/asio/include/asio include/")
do_os("mv _asio_tmp/asio/include/asio.hpp include/asio/")
shutil.rmtree('_asio_tmp', ignore_errors=True)
time.sleep(2)

print('download tiny_sha3')
do_os(f"git {GIT_SSL} clone --depth 1 https://gitee.com/nicehero/tiny_sha3.git include/tiny_sha3")
shutil.rmtree('include/tiny_sha3/.git', ignore_errors=True)
if os.name == "nt":
    do_os("gcc -c include/tiny_sha3/sha3.c")
    do_os("ar -r lib/libsha3.a sha3.o")
else:
    do_os("gcc -shared -fPIC -o lib/libsha3.so include/tiny_sha3/sha3.c")
do_os("rm -rf sha3.o")

print('download kcp')
do_os(f"git {GIT_SSL} clone --depth 1 --branch 1.7 https://gitee.com/nicehero/kcp.git include/kcp")
shutil.rmtree('include/kcp/.git', ignore_errors=True)
print('build kcp')
if os.name == "nt":
    body = open("include/kcp/ikcp.c","rb").read().decode()
    body = body.replace("vsprintf(buffer","vsnprintf(buffer,1024")
    open("include/kcp/ikcp.c","wb").write(body.encode())
    do_os("gcc -c include/kcp/ikcp.c")
    do_os("ar -r lib/libikcp.a ikcp.o")
else:
    do_os("gcc -shared -fPIC -o lib/libikcp.so include/kcp/ikcp.c")
do_os("rm -rf ikcp.o")

print('download jsoncons')
do_os(f"git {GIT_SSL} clone --depth 1 --branch v0.168.2 https://gitee.com/nicehero/jsoncons.git _jsoncons_tmp")
do_os("mv _jsoncons_tmp/include/* include/")
shutil.rmtree('_jsoncons_tmp', ignore_errors=True)

with open('done', 'w') as f:
    f.write('0.1')
print('done')

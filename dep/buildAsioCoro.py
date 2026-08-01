import os
import shutil

GIT_SSL = "-c http.sslVerify=false"

def do_os(cmd):
    print(f"  -> {cmd}")
    b = os.system(cmd)
    if b != 0:
        exit(1)

if not os.path.exists('include'):
    os.mkdir('include')
if not os.path.exists('lib'):
    os.mkdir('lib')

print('download asio')
do_os("rm -rf include/asio")

do_os(f"git {GIT_SSL} clone --depth 1 --branch asio-1-19-2 https://gitee.com/nicehero/asio.git _asio_tmp")
do_os("mv _asio_tmp/asio/include/asio include/")
do_os("mv _asio_tmp/asio/include/asio.hpp include/asio/")
shutil.rmtree('_asio_tmp', ignore_errors=True)

print('done')

import os
def do_os(cmd):
	b = os.system(cmd)
	if b != 0:
		exit(1)
if not os.path.exists('include'):
	os.mkdir('include')
if not os.path.exists('lib'):
	os.mkdir('lib')
print ('download asio')
do_os("rm -rf include/asio")

do_os("wget https://gitee.com/nicehero/asio/attach_files/879378/download/asio-asio-1-19-2.tar.gz")
do_os("tar xvf asio-asio-1-19-2.tar.gz")
do_os("mv asio-asio-1-19-2/asio/include/asio include/")
do_os("mv asio-asio-1-19-2/asio/include/asio.hpp include/asio/")
do_os("rm -rf asio-asio-1-19-2")
do_os("rm -rf asio-asio-1-19-2.tar.gz")

print ('done')
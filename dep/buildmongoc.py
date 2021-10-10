import os
def do_os(cmd):
	b = os.system(cmd)
	if b != 0:
		exit(1)
print ('download mongo-c') #mongo-c-driver-1.14.0.tar.gz
do_os("wget https://gitee.com/nicehero/mongo-c-driver/attach_files/848515/download/mongo-c-driver-1.14.0.tar.gz")
print ('build mongoc')
do_os('tar xvf mongo-c-driver-1.14.0.tar.gz')
do_os('mkdir build_mongoc')
os.chdir('./build_mongoc')
if os.name == "nt":
	p = os.getcwd()
	do_os('cmake -G "MinGW Makefiles" "-DCMAKE_INSTALL_PREFIX=%s\\..\\mongo-c-driver" "-DCMAKE_PREFIX_PATH=%s\\..\\mongo-c-driver" ..\\mongo-c-driver-1.14.0'%(p,p))
	do_os('make install"')
	os.chdir('../')
	do_os('mv mongo-c-driver/include/libmongoc-1.0/mongoc include/')
	do_os('mv mongo-c-driver/include/libbson-1.0/bson include/')
	do_os('mv mongo-c-driver/lib/* lib/')
	do_os('mv mongo-c-driver/bin/* lib/')
	do_os('rm -rf mongo-c-driver')
	do_os('rm -rf build_mongoc')
else:
	p = os.getcwd()
	do_os('cmake "-DCMAKE_INSTALL_PREFIX=%s/../mongo-c-driver" "-DCMAKE_PREFIX_PATH=%s/../mongo-c-driver" "-DCMAKE_BUILD_TYPE=Release" ../mongo-c-driver-1.14.0'%(p,p))
	do_os('make install')
	os.chdir('../')
	do_os('mv mongo-c-driver/include/libmongoc-1.0/mongoc include/')
	do_os('mv mongo-c-driver/include/libbson-1.0/bson include/')
	try:
		do_os('mv mongo-c-driver/lib/* lib/')
	except:
		pass
	try:
		do_os('mv mongo-c-driver/lib64/* lib/')
	except:
		pass
	do_os('mv mongo-c-driver/bin/* lib/')
	do_os('rm -rf mongo-c-driver')
	do_os('rm -rf build_mongoc')
do_os('rm -rf mongo-c-driver-1.14.0')
do_os('rm -rf mongo-c-driver-1.14.0.tar.gz')

import os
import sys
import shutil
import zipfile
import platform

def do_os(cmd):
    print(f"  -> {cmd}")
    b = os.system(cmd)
    if b != 0:
        exit(1)

if not os.path.exists('include'):
    os.mkdir('include')
if not os.path.exists('lib'):
    os.mkdir('lib')

# Skip if already installed
if os.path.isfile('lib/libmysql.a') and os.path.isdir('include/mysql'):
    print('mysql already installed')
    exit(0)

SYSTEM = platform.system()

# ============================================================================
# Linux / macOS — use system package manager
# ============================================================================
if SYSTEM == "Linux":
    print("installing MySQL client library via package manager...")
    # Try apt-get first, then yum
    if os.system("which apt-get > /dev/null 2>&1") == 0:
        do_os("apt-get install -y libmysqlclient-dev")
    elif os.system("which yum > /dev/null 2>&1") == 0:
        do_os("yum install -y mysql-devel")
    elif os.system("which dnf > /dev/null 2>&1") == 0:
        do_os("dnf install -y mysql-devel")
    else:
        print("ERROR: cannot install mysql client. Install libmysqlclient-dev manually.")
        exit(1)

    # Find installed lib and copy to our dep/
    import glob
    for lib in ["/usr/lib/x86_64-linux-gnu/libmysqlclient.so",
                "/usr/lib64/libmysqlclient.so",
                "/usr/lib/libmysqlclient.so"]:
        if os.path.isfile(lib):
            shutil.copy2(lib, "lib/")
            break
    # Headers are usually in /usr/include/mysql/
    if os.path.isdir("/usr/include/mysql"):
        if os.path.isdir("include/mysql"):
            shutil.rmtree("include/mysql")
        shutil.copytree("/usr/include/mysql", "include/mysql")
    print('mysql install done (Linux)')
    exit(0)

if SYSTEM == "Darwin":
    print("installing MySQL client library via Homebrew...")
    do_os("brew install mysql-client")
    # brew installs to /usr/local/opt/mysql-client
    prefix = "/usr/local/opt/mysql-client"
    if os.path.isdir(prefix + "/include/mysql"):
        if os.path.isdir("include/mysql"):
            shutil.rmtree("include/mysql")
        shutil.copytree(prefix + "/include/mysql", "include/mysql")
    for lib in ["libmysqlclient.dylib", "libmysqlclient.a"]:
        path = os.path.join(prefix, "lib", lib)
        if os.path.isfile(path):
            shutil.copy2(path, "lib/")
    print('mysql install done (macOS)')
    exit(0)

# ============================================================================
# Windows — download MySQL Connector/C (client lib only, ~8MB)
# ============================================================================

# Connector/C 6.1.11 — compatible with MySQL 5.5 / 5.6 / 5.7
MYSQL_ZIP  = "mysql-connector-c-6.1.11-winx64.zip"
MYSQL_DIR  = "mysql-connector-c-6.1.11-winx64"

MYSQL_URLS = [
    "https://downloads.mysql.com/archives/get/p/19/file/" + MYSQL_ZIP,
    "https://cdn.mysql.com/archives/mysql-connector-c-6.1/" + MYSQL_ZIP,
]

# Download
if not os.path.isfile(MYSQL_ZIP):
    downloaded = False
    for url in MYSQL_URLS:
        print(f"trying: {url}")
        ret = os.system(f'curl -sL -o "{MYSQL_ZIP}" "{url}"')
        if ret == 0 and os.path.isfile(MYSQL_ZIP) and os.path.getsize(MYSQL_ZIP) > 1000000:
            downloaded = True
            break
        # Clean up failed attempt
        if os.path.isfile(MYSQL_ZIP):
            os.remove(MYSQL_ZIP)
    if not downloaded:
        print("ERROR: Could not download MySQL. Place mysql-5.6.49-winx64.zip in dep/ manually.")
        print("  Expected ZIP structure: mysql-5.6.49-winx64/include/  and  .../lib/libmysql.dll")
        exit(1)

# Extract
print('extracting...')
if not os.path.isdir(MYSQL_DIR):
    with zipfile.ZipFile(MYSQL_ZIP, 'r') as zf:
        # Only extract include/ and lib/ to save time
        members = [m for m in zf.namelist()
                   if m.startswith(MYSQL_DIR + '/include/')
                   or m.startswith(MYSQL_DIR + '/lib/libmysql')]
        zf.extractall('.', members)
    if not os.path.isdir(MYSQL_DIR):
        for name in os.listdir('.'):
            if name.startswith('mysql-') and os.path.isdir(name):
                MYSQL_DIR = name
                break

# Copy headers
print('installing headers...')
src_include = os.path.join(MYSQL_DIR, 'include')
dst_include = 'include/mysql'
if os.path.isdir(dst_include):
    shutil.rmtree(dst_include)
shutil.copytree(src_include, dst_include)

# Copy DLL
print('installing libmysql.dll...')
src_dll = os.path.join(MYSQL_DIR, 'lib', 'libmysql.dll')
if os.path.isfile(src_dll):
    shutil.copy2(src_dll, 'lib/libmysql.dll')
else:
    print("ERROR: libmysql.dll not found in extracted archive")
    exit(1)

# Generate MinGW import library (libmysql.a from libmysql.dll)
print('generating libmysql.a (MinGW import library)...')
os.chdir('lib')
do_os('gendef libmysql.dll')
do_os('dlltool -d libmysql.def -l libmysql.a -D libmysql.dll')
if os.path.isfile('libmysql.def'):
    os.remove('libmysql.def')
os.chdir('..')

# Copy DLL to project root for runtime
print('copying libmysql.dll to project root...')
root_dll = os.path.join('..', 'libmysql.dll')
shutil.copy2('lib/libmysql.dll', root_dll)

# Cleanup
print('cleaning up...')
shutil.rmtree(MYSQL_DIR, ignore_errors=True)
if os.path.isfile(MYSQL_ZIP):
    os.remove(MYSQL_ZIP)

print('mysql install done (Windows)')

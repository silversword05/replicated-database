1. sudo chown -R mpatel48 /data
2. service memcached start / service memcached stop / service memcached restart

# Installation Things
1. sudo apt-get update
2. sudo apt-get install cmake
3. sudo apt-get install bison
4. sudo apt-get install flex
5. sudo apt-get install memcached
6. Install libmemcached
    git clone https://github.com/awesomized/libmemcached.git
    mkdir build-libmemcached
    cd $_
    cmake ../libmemcached
    make -j10
    sudo make install
7. Example libmemcached
    git clone https://github.com/smerrill/libmemcached-example.git
    gcc -o example memcached.c -lmemcached
    ./example
8. Additional install for testing
    sudo apt install libtbb-dev
    sudo apt install libevent-dev
    sudo apt install libsasl2-dev
9. Run Tests
    cd build-libmemcached
    cmake -DBUILD_TESTING=ON ../libmemcached
    make test
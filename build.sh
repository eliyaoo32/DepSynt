# Install packages
cd ./packages
sudo dpkg -i *.deb
# Create symbolic link of libboost_json.so
sudo ln -s /usr/lib/x86_64-linux-gnu/libboost_json.so.1.83.0 /usr/lib/x86_64-linux-gnu/libboost_json.so
cd ../

# Build ABC
cd ./libs/abc
make ABC_USE_NO_READLINE=1 ABC_USE_PIC=1 libabc.so
ln -s ./libabc.so /usr/local/lib/libabc.so
cd ../../

# Build Synthesis
cmake .
make synthesis
ln -s ./synthesis /usr/local/bin/depsynt
ln -s ./synthesis ./depsynt

# Install packages
cd ./packages
sudo dpkg -i *.deb
cd ../

# Build ABC
cd ./libs/abc
make ABC_USE_NO_READLINE=1 ABC_USE_PIC=1 libabc.so
ln -s ./libabc.so /usr/local/lib/libabc.so
cd ../../

# Build Synthesis
cmake .
make synthesis
ln -s ./synthesis /usr/local/bin/synthesis

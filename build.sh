# Install BOOST
cd ./packages/boost
sudo dpkg -i *.deb
cd ../../

# Install nlohmann/json
cd ./packages/nlohmann-json
sudo dpkg -i *.deb
cd ../../

# Install Spot
cd ./packages/spot
sudo dpkg -i *.deb
cd ../../

# Install ABC
cd ./libs/abc
make ABC_USE_NO_READLINE=1 ABC_USE_PIC=1 libabc.so
ln -s ./libabc.so /usr/local/lib/libabc.so
cd ../../

# Build Synthesis
cmake .
make synthesis
ln -s ./synthesis /usr/local/bin/depsynt
ln -s ./synthesis ./depsynt

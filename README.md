# blesecurity 

This small example library contains a c program which asserts whether bluetooth security can be set on a l2cap socket. It is essentially just a test program.

# building

* Install node-gyp.

```
sudo npm install node-gyp -g
```

* Rebuild the project.

```
node-gyp rebuild
```

# usage

* Locate the tag you wanna try and communicate too.

```
hcitool lescan
```

* Connect and try and upgrade security using it's ble address.

```
sudo ./build/Release/assert-security X:X:X:X:X:X random
```

# disclaimer

I am not a c programmer.

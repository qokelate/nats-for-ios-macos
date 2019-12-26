# libNATS for iOS/macos
----------------

### building

#### macos:

```
mkdir build
cd build
cmake ../nats.c; make -j4
```

#### iOS:

```
mkdir build
cd build
cmake -DIOS=1 ../nats.c; make -j4
```
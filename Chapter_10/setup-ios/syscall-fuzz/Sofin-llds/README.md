
# For Darwin - 10.11:

```
sudo mv /usr/bin/ld /usr/bin/ld.orig
sudo install this/repo/Darwin/3.9/ld /usr/bin/ld

# No additional actions required.. it usually just works

```


# For FreeBSD 11.0:

```
install this/repo/FreeBSD/3.9/ld.lld /usr/bin/ld.lld

# Pass -fuse-ld=lld to your compiler f.i. via CFLAGS/CXXFLAGS

```

# os161-shell
Support for multiple running processes on OS/161 and for accessing files. 
Operating System module final project at PoliTO for the academic year 2021/2022.

Students:
- Bancila, Vlad George
- Bert, Diego
- Combetto, Paolo

## How to get started

Assuming that you have installed the toolchain and the MIPS simulator (if not, follow the instructions here http://www.os161.org/resources/setup.html or use the Linux VM provided by PoliTO) you should have an `os161` folder somewhere on your computer.

At this point, go inside the `os161` folder and clone this repository:

```
git clone https://github.com/drvladbancila/os161-shell
```
Now move inside the repository and run the following command making sure to change the `--ostree` parameter so that it points to a directory named `root` inside the `os161` directory

```
cd os161-shell
./configure --ostree=$HOME/os161/root
```

Now it is time to build the userland. From the `os161-shell` directory run:
```
bmake
bmake install
```
Files will be copied to `os161/root`.

To configure the kernel, always from the `os161-shell` directory:
```
mkdir -p kern/compile/DUMBVM
cd kern/conf
./config DUMBVM
cd ../compile/DUMBVM
bmake depend
bmake
bmake install
```
This should copy the built kernel inside the `os161/root` directory (you should see a `kernel` file).

Now, copy the example config file for sys161 with the following command (from `os161` directory):
```
cp tools/share/examples/sys161/sys161.conf/sample root/sys161.conf`
```
and then move into `root` and create the disk files
```
cd root
disk161 create LHD0.img 5M
disk161 create LHD1.img 5M
```

Once you arrived here, to execute the MIPS simulator and run the OS161 kernel on it, run (from `root`):
```
sys161 kernel
```

## Testing new features

Everytime you edit the kernel because you add new functionalities to it, you need to recompile it. From `os161`:

```
mv os161-shell/kern/compile/DUMBVM
bmake depend
bmake
bmake install
```

And then run from the `root` directory:
```
sys161 kernel
```

## git workflow

- Everytime you need to add something, create a branch for the feature with `git checkout -b myfeature` or `git branch myfeature && git checkout myfeature`.
- Push on a remote branch called as the local branch to have backup `git push origin myfeature`
- When you want to be synchronized with `main`, fetch latest commits and rebase on top of it `git rebase origin main`
- To push on main `git push origin main`. Note that pushes on main are disabled by default. A push on main will create a pull request and will need to be approved from github.com


# cuckarood29 / cuckatoo31
A submission to CPU Speedup Bounty Challenge.

## How to build and run

    cd cuckarood29_c
    make
    t4/cuckarood29 -n 15

You will most likely see the error message "mmap failed" because your computer does not have Huge pages setup. See the next section for some info on huge pages.  
Cuckarood29 needs 4 pages of 1GiB memory; cuckatoo31 needs 11 pages of 1GiB memory.

To run the benchmark (default is 21 graphs for cuckarood29 and 42 graphs for cuckatoo31)

    cd cuckarood29_asm
    make
    t4/benchmark -n 0 -r 21

    cd cuckatoo31_asm
    make
    t4/benchmark -n 58 -r 42

## Huge pages
I can only tell what I did to my Ubuntu and Linux Mint setup to enable huge pages. You may have to do some research on how to configure your OS.

Edit /etc/default/grub with the following line.  
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash `hugepagesz=1G hugepages=4 hugepagesz=2M hugepages=8`"

The boot parameter will enable both 1GiB pages and 2MiB pages, reserving 1GiB\*4 and 2MiB\*8. If you want to test cuckatoo31, substitute 4 pages with 11 pages, but make sure you have at least 16GiB system memory or you might cause your computer to swap to disk heavily.

In the shell,  
`sudo update-grub`

Reboot your computer. You can read the following two files to see the number of pages reserved. To read the file, use `cat`. To write the file, use `echo sudo tee`.  
/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages  
/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

After testing my memory hungry demo, you may want to release the memory from huge page pool to allow other applications to use.  
`echo 0 | sudo tee /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages`  
Remember that the huge page reservation will take effect again when you reboot unless you undo the changes in /etc/default/grub and run `sudo update-grub`.

## Timing results
```
Core i7-7700K stock turbo 4.4GHz
Dual Channel DDR4-2400 CL17
Idle 20W
               nosol  withsol effective        power
cd29-tromp    2513ms   2988ms 0.394gps         100-114W
cd29-xen-c    1633ms   2090ms 0.604gps  1.53x  101-118W
cd29-xen-asm  1591ms   1965ms 0.622gps  1.58x  96-118W
c31-tromp    11070ms  12272ms 0.0901gps        99-127W
c31-xen-c     8487ms  10455ms 0.1172gps 1.30x  96-145W
c31-xen-asm   8140ms   9721ms 0.1223gps 1.36x  96-145W

Core i7-7700K stock turbo 4.4GHz
Dual channel DDR4-2133 CL15
Idle 20W
               nosol  withsol effective        power
cd29-tromp    2575ms   3050ms 0.385gps         100-112W
cd29-xen-c    1676ms   2132ms 0.589gps  1.53x  100-122W
cd29-xen-asm  1618ms   2020ms 0.611gps  1.59x  101-122W
c31-tromp    11419ms  12920ms 0.0873gps        96-125W
c31-xen-c     8724ms  10729ms 0.1140gps 1.31x  96-140W
c31-xen-asm   8374ms   9953ms 0.1189gps 1.36x  96-145W

Core i7-7700K limited to 3.4GHz (echo 75 | sudo tee /sys/devices/system/cpu/intel_pstate/max_perf_pct)
Dual channel DDR4-2133 CL15
Idle 20W
               nosol  withsol effective        power
cd29-tromp    3014ms   3629ms 0.329gps         53-57W
cd29-xen-c    1926ms   2514ms 0.512gps  1.56x  55-60W
cd29-xen-asm  1828ms   2335ms 0.540gps  1.64x  55-60W
c31-tromp    13605ms  15512ms 0.0733gps        51-56W
c31-xen-c     9679ms  12093ms 0.1027gps 1.40x  55-66W
c31-xen-asm   9127ms  11100ms 0.1090gps 1.49x  55-68W
```
* Power consumption data is not particularly accurate. It is done by looking at a measuring device that updates the reading once a second. The point is to show that running a CPU at turbo frequency is not energy efficient. The most efficient operating point is less than 3.4GHz and we have to make a tradeoff between speed and efficiency.

## Ideas implemented
1. Use huge page and single stage bucketing. This has the most effect on speed compared to other ideas below.
2. Get parity bit involved in bucketing. Parity bit is the direction bit in cuckarood and off-by-one bit in cuckatoo. When reading out buckets, a pair of buckets are processed together in each loop iteration.  
This enables only one of the following optimization:  
    1. Half as much bitmap initialization, the pair of buckets share the same bitmap.
        * Zero entire bitmap
        * Read bucket 0 to mark bitmap
        * Read bucket 1, if bitmap is marked, edge survives.
        * Read bucket 1, clear the particular bit to 0 in the bitmap to indicate presence in bucket 1. (The logic is inverted: bitmap=0 means presence)
        * Read bucket 0, if bitmap is 0, edge survives. If bitmap=1, it was marked in the first pass and there is no corresponding edge in bucket 1 to match it.
    2. Instead of reading the bucket data 2 times, we can read only 1.5 times.
        * Zero entire bitmap0
        * Read bucket 0 to mark bitmap0
        * Zero entire bitmap1 (This is separate memory from bitmap0)
        * Read bucket 1, test bitmap0 to determine edge survival, mark bitmap1.
        * Read bucket 0, test bitmap1 to determine edge survival.
3. At a suitable round, make pairs of edges so that the graph becomes single partition and we will search for 21-cycle. The number of nodes to be processed by cycle finder is halved. One of the renaming rounds (round14) does not need unrename information.
4. Renaming, I can think of two methods to do node renaming (compression): "first seen in bucket, first get assigned" and "small old node id gets small new node id." I do not know which method is superior. I could be making things unnecessarily complicated here. The "small old node id gets small new node id" method is done by scanning the bitmap after counting, and then reusing the bitmap as an array of uint32 which holds the new node id. Unrenaming is pretty complicated and involves guess-and-check.
5. DFS cycle finder using the same concept of links[] and adjlist[]. But populating links[] and adjlist[] is done separately using multithreaded routine. Walking the graph is single threaded and uses explicit stack instead of recursive calls. Combined with the edge pairing trick which halves the number of nodes, the cycle finder runs much faster, nevertheless it is only a small fraction of the overall run time of PoW.
6. [cuckarood only] Complicated loop unrolling to interleave between siphash computation and bucketing.
7. [cuckarood only] A trit-based trimmer is introduced after pairing edges. The feature of this kind of trimming is no bitmap initialization in each round. I can only get this idea to work on a single partition directed graph. There is only a small speedup to the overall PoW because this happens at a late stage.

## Known issues
"cuckatoo31 -n 32" has density error at round 8 and has to abort on that graph. This type of occurrence is deemed rare so the impact to solution finding is minimal.

## My BTC address
Please send reward to bc1q3rj25wj4n2gm0rsr8e4fjtumfw3laq42tn3rwz

# VirtualBox Kernel Address Leak POC

This is in response to a ticket filed by Robert C. Williams (wcrobert)
Here is the ticket: https://www.virtualbox.org/ticket/15167

Below is the ticket before we started work on our POC. Reproduced here for the
sake of completeness.

### Kernel Address Info Leak

##### Description

I reported this via secalert_us@… and was told to resubmit here:

vbox kernel module seems to printk kernel addresses that get picked up by
syslog. This information could be used by someone who has gained uid/gid syslog
adm (On Ubuntu) to successfully chain an attack to kernel data structures (thus
defeating ASLR). Information from /proc/modules is sanitized for non-root
users.

The requested fix is to stop printing out kernel addresses.

Host $ lsb_release -a No LSB modules are available. Distributor ID: Ubuntu
Description: Ubuntu 14.04.4 LTS Release: 14.04 Codename: trusty

$uname -a Linux wcrobert-MOBL1 3.19.0-18-generic #18~14.04.1-Ubuntu SMP Wed May
20 09:38:33 UTC 2015 x86_64 x86_64 x86_64 GNU/Linux

VBox Version: Version 5.0.14 r105127

What I found in syslog:

```log
Feb 11 11:27:57 wcrobert-MOBL1 kernel: [    5.881847] vboxdrv: Found 4 processor cores
Feb 11 11:27:57 wcrobert-MOBL1 kernel: [    5.901307] vboxdrv: TSC mode is Invariant, tentative frequency 2593993759 Hz
Feb 11 11:27:57 wcrobert-MOBL1 kernel: [    5.901310] vboxdrv: Successfully loaded version 5.0.14 (interface 0x00240000)
Feb 11 11:27:57 wcrobert-MOBL1 kernel: [    6.112417] vboxpci: IOMMU not found (not registered)
Feb 11 12:16:23 wcrobert-MOBL1 kernel: [ 2913.482380] vboxdrv: ffffffffc0000020 VMMR0.r0
Feb 11 12:16:23 wcrobert-MOBL1 kernel: [ 2913.571393] vboxdrv: ffffffffc00fa020 VBoxDDR0.r0
Feb 11 12:16:23 wcrobert-MOBL1 kernel: [ 2913.572892] vboxdrv: ffffffffc0119020 VBoxDD2R0.r0
Feb 11 12:16:23 wcrobert-MOBL1 kernel: [ 2913.606759] vboxdrv: ffffffffc011d020 VBoxEhciR0.r0
```

##### Response from frank:

- Status changed from new to closed
- Resolution set to wontfix

Sorry, this is NOT a security problem and not a problem at all. Having these
addresses is not a problem because special permissions are required to make use
of them.

## Why POC?

This is a security issue because the leaking of kernel addresses allows an
attacker to defeat KASLR.

KASLR (Kernel Address Space Layout Randomisation) makes it so that if you find
an exploitable vulnerability in anything running in ring 0 (for example a
driver) you are unable to use standard techniques such as ROP to gain code
execution because you don't know where any of the gadgets you need to use are.

When something running in ring 0 leaks addresses, which is what VirtualBox is
doing, the attacker now knows where the gadgets they need are located and thus
can execute whatever code they want at the highest level of privilege.

We believed it was necessary to create a POC for this because by leaking
kernel addresses VirtualBox allows attackers to gain the highest level of
execution privilege on a system.

## What does your POC do?

Our POC uses the addresses that VirtualBox leaks to `dmesg` to construct a ROP
chain which makes /etc/shadow world writeable. This then allows an attacker
with the lowest level of permissions to change the root password on the system
thus owning it.

## How does your POC do that?

Our POC grabs the leaked addresses from `dmesg` and constructs a ROP chain that
points to gadgets in `VMMR0.r0`. We created a extremely basic kernel module which
experiences a stack buffer overflow. The module init calls the vulnerable
function which `memcpy`'s into a buffer on the stack. The ROP chain is
prepended with the address of a `retq` which creates a ret slide into the
functional part of our payload.

The exploit now builds a string in the `.bss` section of the `VMMR0.r0` driver.
The string, `/etc/shadow`, is then passed to the `chmod` function and `syscall`
is invoked.

> Following the `syscall` instruction are NULL bytes which rip is set to and
> `dmesg` reports that the module hit an invalid instruction. This is okay
> because we still changed the permissions on the file so we've succeeded in
> escalating our privileges.

## Running For Yourself

You need to have python3.5 and python3-pwntools installed (you must be on the
`rop_load_add_register_to_stack` branch in pdxjohnny's fork or wait for it to
be merged into master). You also need unalignedrop installed.

```log
sudo -E pip3 install -r requirements.txt
```

You need to start a virtual machine and make sure that the driver VMMR0.r0
has loaded by looking in dmesg. After you start dmesg there will be a line
near the bottom of the output that says vboxdrv: <leaked address> VMMR0.r0.
This will definetly happen on VirtualBox version 5.1.0 and 5.1.2.

```log
$ ls -lAf /etc/shadow
-rw------- 1 root root 806 Aug 29 17:12 /etc/shadow
$ rm Makefile
$ ln -s Makefile.vbox3 Makefile
$ make clean all
$ ./poc.py kernel VMMR0.r0 payload 42
$ ls -lAf /etc/shadow
-rw-rw-rw- 1 root root 806 Aug 29 17:12 /etc/shadow
```

## Impact

This affects VirtualBox version 5.1.0 and beyond so far was we know. And all
earlier 32-bit versions.

## Recommendations

We **strongly** recommend that VirtualBox be patched so that it does not log the
addresses of its modules.

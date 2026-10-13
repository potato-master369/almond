Driver readme for disk/part/ext2
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Hello! It's me, potato-master369 again.
I had some fun with the ascii art.

Was playing DDLC, and while I was thinking
of the Metaverse ES weird ass thin client,
I remembered I was writing an operating system.
DDLC is an amazing game btw.

So basically ext2 is the loonix
extended filesystem 2. Basically ancient
version of ext4.

---
You might want to skip ahead for the actual docs.
---

The actual docs
------------------------------------

ext2 relies on a few things

- superblock (at block 2 (0-indexed); first 1024B
  is for bootloader code that we dont use)

  See include/def.h for the superblock def.

- root inode (**ALWAYS** inode 2). Stores pointers
  to all the files in the / folder.

- What is an inode? Basically it's a thing that
  stores data. It can be a directory or a file. They
  are structured different.

- To resolve a path, you walk from (for the example of
  /home/ebayan/.config/sway/config)

  / (root inode) -> home (dir entry) -> ...
  -> sway (dir entry) -> config (file inode)

  This is a simplified explanation but close enough.

- Then, to read the file, you go,
  // This is very sketchy I didnt check the names.
  BLOCK0... BLOCK11 (12 x 4KB = 48KB)
  // assume one block is 4kb. Standard.
  Then after that, we go to IND1.
  In IND1 we go from IND1_BLOCK0 through IND1_BLOCK1023
  (1024x4KB = 4MB)
  After that we go to IND2.
  In IND2 we go from IND2_IND1_0 through IND2_IND1_1023
  That's another (1024x1024x4KB=4GB). That's about
  where we stop.

* REGISTRATION PROCESS *
===                  ===

As for now,

mount -> set fsid -> create a new private reference and
store in resv -> resolve inode_n and store ->

* MOUNTING PROCESS *
===              ===

TODO; fill later

More yapping
---------------------------------------

This was really a pain in the ass to write. As the first fs
driver, I had to figure out how I really want to structure
my file access system, and I think I did a decent (?) job.

Remember, there is no true happiness to be found in the
Literature Club, and there is also no happiness in writing
kernel drivers.

LEGAL (plz dont sue me)
----------------------------------------
/*
 * Copyright (c) 1997 Manuel Bouyer.
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)fs.h	8.10 (Berkeley) 10/27/94
 *  Modified for ext2fs by Manuel Bouyer.
 */

THIS STUFF IS UNDER BSD LICENSE NOT THE ENTIRE SOURCE TREEEEEEEEEE
Changes remain under MIT.

// SAVE ME
// IM BEING FORCED BY MYSELF TO WRITE THESE
// READMES THAT ARENT EVEN USEFUL

EOF





























































































// Why are you still here?

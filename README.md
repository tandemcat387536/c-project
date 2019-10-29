# Directories and files tree

**AUTHOR** Matej Kovar

Program, which calculates size of each directory and file in *PATH*, which is given in input.
Directories and files are written in specific format -> tree.

There are several OPTIONS how to write size of directories and files.
This is how arguments should look -> **"./dt [OPTIONS] PATH"**, where *PATH* is path to directory (or file)
 **OPTIONS** :
 * *"-a"* : calculates size with actual size of file instead of real size allocated on disk (allocated blocks 512)
 * *"-s"* : sorts files or directories in directory based on size, not alphabetical
 * *"-p"* : writes percentage usage of disk instead of size
 * *"-d NUMBER"* : writes only directories and files in maximum depth of NUMBER

You can combine these options (where it makes sense) -> for example _"-p -s -d 3 PATH"_

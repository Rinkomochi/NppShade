# Third-Party Notices

NppShade's own code, including code derived from `chcg/nppcrypt`, is licensed
under the GNU General Public License, version 2 or (at your option) any later
version. See `LICENSE`.

The following third-party components are included in the source tree; the
components used by the plugin or its auxiliary command-line build retain their
own license terms, separate from the license for NppShade's own code.

| Component | License and required notice |
| --- | --- |
| Crypto++ | Boost Software License 1.0 for the library compilation, with file-specific notices and exceptions. Full text: `doc/cryptopp/License.txt`. |
| CLI11 1.5.4 | 3-Clause BSD. Full text: `doc/cli11/LICENSE`. |
| scrypt | Two-clause BSD-style license. Full text: `doc/scrypt/COPYRIGHT`. |
| TinyXML-2 | zlib license. Full text: `doc/tinyxml2/readme.md` and the source file headers. |
| Keccak reference code | CC0 1.0 for the sponge implementation. `src/keccak/brg_endian.h` has a separate permissive license reproduced below. |
| bcrypt / crypt_blowfish | Public domain, with a permissive fallback license stated in `src/bcrypt/crypt_blowfish.cpp`. |
| Notepad++ plugin interface headers | GNU GPL version 2 or later, with an additional notice in `src/npp/PluginInterface.h`. |

## Keccak `brg_endian.h` notice

Copyright (c) 1998-2008, Brian Gladman, Worcester, UK. All rights reserved.

The redistribution and use of this software (with or without changes) is
allowed without the payment of fees or royalties provided that:

1. Source code distributions include the above copyright notice, this list of
   conditions and the following disclaimer.
2. Binary distributions include the above copyright notice, this list of
   conditions and the following disclaimer in their documentation.
3. The name of the copyright holder is not used to endorse products built
   using this software without specific written permission.

This software is provided 'as is' with no explicit or implied warranties in
respect of its properties, including, but not limited to, correctness and/or
fitness for purpose.

## Keccak reference implementation

The Keccak sponge function was designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. The implementation authors waived
copyright and related or neighboring rights to the source code under CC0 1.0:
<https://creativecommons.org/publicdomain/zero/1.0/>.

## bcrypt / crypt_blowfish

The code comes from John the Ripper, with reentrant and `crypt(3)` interfaces
added. It was written by Solar Designer in 1998-2014. No copyright is claimed
and the software is placed in the public domain. If that dedication is
ineffective, Copyright (c) 1998-2014 Solar Designer permits redistribution and
use in source and binary forms, with or without modification. There is no
express or implied warranty. Credit is appreciated; modifications are
requested, but not required, to remain in the public domain. This license does
not restrict placing the code or modifications under another license.

The implementation is compatible with OpenBSD's bcrypt.c for the `$2b$`
prefix, originally by Niels Provos, and uses some of his ideas. The password
hashing algorithm was designed by David Mazieres. Source: `src/bcrypt/crypt_blowfish.cpp`.

## Notepad++ plugin interface

Copyright (C) 2003 Don HO. The headers are distributed under the GNU General
Public License, version 2 or (at your option) any later version. The source
header says that, for purposes of that license, Notepad++ considers an
application a derivative work if it integrates Notepad++ source code, includes
or aggregates Notepad++ in a proprietary executable installer, or links to a
library or executes a program that does either. The complete notice is in
`src/npp/PluginInterface.h`.

<table><tr><td bgcolor="#64151d"><strong><font color="#ffffff">Disclaimer</font></strong><br><font color="#ffffff">NppShade is at an early stage. Use it at your own risk. Keep your passwords safe and retain the original files and reliable backups. A lost password cannot be recovered.</font></td></tr></table>

# NppShade for Notepad++

[中文](README_zh-CN.md) · [English](README.md) · [日本語](README_ja-JP.md) · [한국어](README_ko-KR.md)

NppShade encrypts and decrypts text, calculates hashes, generates random data, and converts Base16, Base32 and Base64. The interface supports Simplified Chinese, English, Japanese and Korean. It follows the Windows display language by default. Choose another language in **Preferences > Interface language**; dialogs update immediately, while menu labels update after restarting Notepad++.

NppShade is maintained by [Rinkomochi](https://github.com/Rinkomochi/Nppcrypt-CN). Parts of the source are derived from [chcg/nppcrypt](https://github.com/chcg/nppcrypt); their original license notices are retained.

## License

NppShade's own code is licensed under the GNU General Public License, version 2 or (at your option) any later version. See [LICENSE](LICENSE). Third-party components retain their own licenses; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Release ZIPs include these notices.

## Install or update

1. Check whether Notepad++ is x86 or x64 and download the matching ZIP from GitHub Releases.
2. Save open files and exit Notepad++.
3. Extract the ZIP to `plugins/NppShade/` in the Notepad++ installation directory.
4. Restart Notepad++ and open **Plugins > NppShade**. Back up the existing DLL and preferences before replacing them.

If the menu does not appear, check the architecture, installation path and restart. Windows may require administrator access to write under Program Files. The preferences file is `nppshade.xml` in the Notepad++ plugin configuration directory. If only the former `nppcrypt.xml` exists, NppShade reads it and saves migrated settings to `nppshade.xml`.

When a version tag is released on GitHub, the workflow builds x86 and x64 DLLs and attaches a ZIP for each architecture.

## Encrypt a whole file

Practice with a disposable text file first.

1. Open **Plugins > NppShade > Preferences**, enable whole-file encryption, and enter `nppshade;enc;npp` in the extension field. `nppshade` alone is also valid. Choose **OK**.
2. Create a UTF-8 file with a short test note.
3. With no text selected, choose **Encrypt**. Select a destination, enter a password, then enter it again to confirm. Keep the password; it cannot be recovered.
4. The editor continues to show editable plaintext, while the saved file contains ciphertext. Saving `note.txt` this way usually creates `note.txt.nppshade` and leaves the original plaintext file intact. Other extensions, such as `txt` or `ini`, can be added if needed.
5. Close and reopen the encrypted file. Enter the password to restore the text. A failed attempt leaves the password dialog open so you can try again or cancel.

**Ask before encrypting on save** controls whether saving asks before changing the encryption settings. Turn it off to save directly with the remembered settings. Closing a file clears the password used for its saves; reopening asks for it again.

If decryption is cancelled, the editor still contains ciphertext. Do not edit or save it as if it were plaintext. Close and reopen the file, or select the complete encrypted block and try again.

### Extension rules

Separate extensions with English semicolons, for example `nppshade;enc;npp`. Surrounding spaces and a leading dot are removed, matching is case-insensitive, and duplicates are discarded. Do not enter `*.enc`, a path, or a multi-part suffix such as `tar.gz`.

- Any listed suffix enables whole-file handling.
- The first suffix is used for new encrypted files when no registered suffix was entered.
- An existing `.enc` file is not renamed just because `nppshade` is first.
- If a save-as name has no registered suffix, the first suffix is appended.
- A suffix alone does not encrypt a file. Plain text named `.enc` has no encrypted header and will not prompt for a password when opened.

## Encrypt a selection

Select the text and choose **Encrypt**. Only the selection is replaced by a ciphertext block; surrounding text remains plaintext. To decrypt it, select the complete block, including its `<nppshade ...>` header, then choose **Decrypt** and enter the password.

With no selection, **Encrypt** uses the whole-file workflow and requires extension handling to be enabled. **Decrypt** requires a selection. A whole encrypted file with a valid header can also be opened and handled automatically.

## Passwords and advanced settings

The first encryption asks you to enter the password twice. Passwords are case-sensitive and may include spaces, punctuation or non-English characters. The help question mark opens help; it does not reveal or recover the password. Use **Show** to reveal the current entry temporarily.

When opening a file with a valid header, only the password field is shown because the header contains the encryption parameters. Manual decryption also offers **Password encoding** for legacy passwords entered as Hex, Base32 or Base64. Data without a recognized header requires the full settings.

For ordinary use, keep the defaults. Advanced settings include:

| Setting | Purpose | Suggested choice |
| --- | --- | --- |
| Cipher, key length and mode | Select how the content is encrypted | AES/Rijndael, 256-bit, GCM |
| Password encoding | Interpret input as UTF-8 or encoded key bytes | `utf8` for normal passwords |
| Show | Temporarily reveal the password being entered | Use only when the screen is private |
| Authenticate IV/salt | Include these values in authentication where supported | Leave enabled |
| Output encoding | Represent ciphertext as bytes, Hex, Base32 or Base64 | Base64 for text files |
| Salt | Vary the derived key between encryptions | Use the random value stored in the header |
| scrypt, bcrypt or PBKDF2 | Derive a key and increase the cost of guessing | Keep scrypt; higher costs take more time and memory |
| IV | Provide initial data for encryption | Generate randomly; avoid zero or reused custom values |
| HMAC | Authenticate the header and ciphertext with a separate key | Optional; GCM already authenticates by default |

## Preset keys

Preset keys are used for HMAC and keyed hash algorithms; they are not the encryption password. The built-in preset is named `nppshade`. To add one, enter a name, choose **Random**, then **Add**. A key is stored as Base64 encoding of 16 bytes, normally 24 characters. Up to 20 presets are supported, including the built-in preset which cannot be deleted.

Custom preset keys are stored in the preferences file. Base64 is an encoding, not encryption. The built-in key is public and should only be used to detect accidental changes. Create your own key for private authentication and keep a backup of the configuration. Encrypted files store the preset index, so keep the same key at the same index when moving files to another computer. Removing an earlier preset can change later indices.

## Hash

Select text and open **Hash**. **Insert** replaces the selection with the digest; **Copy** leaves the text unchanged and copies the result. Use **Copy** to keep the original text. SHA-256 is suitable for general checks; MD5 is not suitable for resisting deliberate forgery. A hash cannot recover the original text or encrypt it.

Digest length sets the number of output bytes, and encoding controls how they are displayed. Keyed algorithms require the same input, algorithm and key to produce the same result.

## Random data

Choose the byte count, character restriction and encoding. **Insert** replaces the selection or inserts at the caret; **Copy** leaves the document unchanged. Unrestricted binary data may contain invisible characters and is not convenient as a typed password. Hex, Base32 and Base64 output uses more characters than the original byte count.

## Encoding conversion

Select text and choose **Encoding conversion**. Set **Source** to its current format and **Target** to the desired format. For example, `hello` encoded as Base64 is `aGVsbG8=`. **Convert** replaces the selection; **Copy** keeps it. Line breaks, line width and uppercase affect formatting only. Encoding is reversible and does not provide secrecy. The encryption header and conversion dialog use different default Base32 alphabets; do not interchange them.

## Display and common questions

Dialogs adapt to the display DPI and available work area. Older Notepad++ versions may limit behavior when moving between monitors with different scaling.

- **The password was rejected.** Check the password and file completeness. GCM, CCM and EAX detect password or data mismatches; some older unauthenticated modes cannot reliably detect a wrong password.
- **The editor shows plaintext but the file contains ciphertext.** Text is edited in memory and encrypted when saved. Reopen the file and enter the password to show it again.
- **Can a forgotten password be recovered?** No. The help button, preset keys and changing the suffix do not bypass encryption.
- **Are backups encrypted too?** Notepad++ session snapshots, automatic backups and copies made by other applications are outside the plugin's control. The original plaintext file may also remain on disk.
- **Where are settings stored?** Normally under `%APPDATA%\Notepad++\plugins\config\nppshade.xml`; portable installations use `plugins/config/`. Exit Notepad++ and back up the file before editing it.
- **Why is an authentication key requested?** The file uses a custom HMAC key. It may differ from the password used to encrypt the text.

## Build from source

Install Visual Studio 2022 with the C++ desktop tools and Windows SDK. In a Developer PowerShell, run:

```powershell
msbuild projects\msvc2017\nppshade.sln /m /t:nppshade /p:Configuration=Release /p:Platform=x64
```

The x64 DLL is written to `bin/nppshade/x64/Release/NppShade.dll`. New encrypted content uses an `<nppshade>` header; existing `<nppcrypt>` headers remain readable. The source also includes the command-line project; consult its `--help` output for command-line options. Desktop preset keys are not directly available to that tool.

Report issues at the [project page](https://github.com/Rinkomochi/Nppcrypt-CN). Include the Notepad++ and plugin versions and reproduction steps; do not attach real passwords or private files.

# Security Policy

This documentation was last updated February 2022. The latest version can
always be found at: 
https://github.com/atheme/atheme/blob/master/SECURITY.md

## Supported Versions

Currently, the latest minor release will receive security and bugfix
updates as new point releases. This is currently the 7.2 release series,
with 7.2.12 being the currently supported stable release.

For the lifetime of the upcoming 7.3 release series, we are planning to keep
supporting the 7.2 series with security updates as needed.

Older releases are not officially supported. We may provide updates for
severe security vulnerabilities affecting them on a best-effort basis,
however we cannot recommend relying on this and strongly urge users to
update to an officially supported release instead.

If you are using Atheme as distributed by a third party (such as an OS
package repository), they may have backported security fixes to an earlier
release. In such a case, please consult your distributor's packaging
policy for details.

## Reporting a Vulnerability

Our security contacts are, in alphabetical order:

| GitHub username | Libera Chat account name |
| --------------- | ------------------------ |
| @aaronmdjones   | `amdj`                   |
| @ilbelkyr       | `ilbelkyr`               |

If circumstances permit, you can contact us via IRC by sending us a private
message on the Libera Chat IRC network. Please double-check you are actually
talking to the right people; we are generally opped in the `#atheme` channel.

Otherwise, you may prefer to contact us via email at `security@atheme.org`
instead. If you use PGP, please encrypt your mail for *all* of these keys:

- [`6645CCE551CB5AF25B5636B96E52BD84AF14021F`][pgp-ilbelkyr]
- [`97D58E607188C8C986481CB76A2F898000519052`][pgp-amdj]

We will look into the issue as quickly as possible. To avoid us having to
request further clarification, please try to include all relevant details,
especially whether the vulnerability has already been disclosed to third
parties.

## Vulnerability Disclosure

We aim to ensure the security of Atheme installations and their users. To
this end, we follow a coordinated disclosure model under which we provide
administrators of major Atheme installations with advance notice of security
vulnerabilities and their corresponding fixes before sharing the details
with the general public. Where possible, we will also publish mitigation
instructions before releasing the details. Outside extraordinary circumstances,
we will release full details at most two weeks after learning about the
vulnerability.

Security-related announcements will be found in our Git repository's
`NEWS.md` file, as well as in the release notes for applicable releases.

[pgp-ilbelkyr]: https://keys.openpgp.org/vks/v1/by-fingerprint/6645CCE551CB5AF25B5636B96E52BD84AF14021F
[pgp-amdj]: https://keys.openpgp.org/vks/v1/by-fingerprint/97D58E607188C8C986481CB76A2F898000519052

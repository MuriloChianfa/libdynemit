# Security Policy

## Supported Versions

The following versions are currently being supported with security updates:

| Version | Supported |
| ------- | --------- |
| 1.x.x   |     Yes   |

## Reporting a Vulnerability

The libdynemit team takes security vulnerabilities seriously. We appreciate your efforts to responsibly disclose your findings and will make every effort to acknowledge your contributions.

### How to Report a Security Vulnerability

**Please DO NOT report security vulnerabilities through public GitHub issues.**

Instead, please report them via email to:

**murilo.chianfa@outlook.com**

### What to Include in Your Report

To help us better understand the nature and scope of the security issue, please include as much of the following information as possible:

- **Type of vulnerability** (e.g., buffer overflow, integer overflow, use-after-free, memory alignment issue, etc.)
- **Full paths of source file(s)** related to the vulnerability
- **Location of the affected source code** (tag/branch/commit or direct URL)
- **Step-by-step instructions** to reproduce the issue
- **Proof-of-concept or exploit code** (if available)
- **Impact of the vulnerability**, including how an attacker might exploit it
- **Affected versions** of libdynemit
- **Compiler version and flags** used during testing
- **CPU architecture** where the vulnerability was discovered
- **Any special configuration** required to reproduce the issue

### Preferred Language

Please use **English** for all communications.

## Response Timeline

- **Initial Response**: We aim to acknowledge receipt of your vulnerability report within **48 hours**.
- **Status Updates**: We will send you regular updates about our progress, at least every **7 days**.
- **Disclosure Timeline**: We aim to patch critical vulnerabilities within **90 days** of the initial report.

## What to Expect

### After You Submit a Report

1. **Acknowledgment**: We will confirm receipt of your report within 48 hours
2. **Assessment**: We will assess the vulnerability and determine its severity
3. **Communication**: We will keep you informed of our progress
4. **Fix Development**: We will develop a patch for the vulnerability
5. **Testing**: We will test the fix thoroughly across multiple compilers and CPU architectures
6. **Disclosure**: We will coordinate with you on the disclosure timeline

### Severity Assessment

We use the following criteria to assess vulnerability severity:

- **Critical**: Remote code execution, arbitrary memory corruption, or data exfiltration
- **High**: Buffer overflows in SIMD code, integer overflows leading to memory corruption, ifunc resolver vulnerabilities
- **Medium**: Memory alignment issues, limited memory leaks, or issues affecting specific compiler versions
- **Low**: Minor issues with minimal security impact

## Security Update Process

When we release a security fix:

1. **Private Patch**: We first create a private patch
2. **Notification**: We notify you and request validation of the fix
3. **Release**: We release the patch in a new version
4. **Advisory**: We publish a security advisory with details
5. **Credit**: We credit you in the advisory (unless you prefer to remain anonymous)

## Security Considerations for libdynemit

### Library Security Model

- libdynemit is a **static library** that is linked into user applications
- The library uses **ifunc resolvers** which execute at program load time
- SIMD operations process user-supplied data and must handle edge cases safely
- The library requires proper **memory alignment** for optimal performance and correctness

### Attack Surface

- **Buffer Overflows**: SIMD loops must correctly handle array bounds
- **Integer Overflows**: Size calculations must not overflow
- **Memory Alignment**: Misaligned accesses can cause crashes or incorrect results
- **ifunc Resolver Safety**: Resolvers must be thread-safe and dlopen-safe
- **NULL Pointer Dereferences**: Input validation is critical

### Known Safe Practices

The library follows these security practices:

1. **Thread-safe CPU detection**: Uses `detect_simd_level_ts()` in ifunc resolvers
2. **Bounds checking**: Loop implementations check array bounds
3. **No dynamic memory allocation**: The library is allocation-free
4. **Const-correctness**: Input pointers are marked const where appropriate
5. **Compiler warnings**: Built with `-Wall -Wextra -Wconversion -Werror` to catch potential issues

### Hardening Recommendations

For applications using libdynemit:

1. Ensure input arrays are properly sized before passing to library functions
2. Verify memory alignment requirements (typically 32-byte or 64-byte aligned)
3. Build with security flags: `-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2`
4. Use sanitizers during testing: AddressSanitizer, UndefinedBehaviorSanitizer
5. Test on multiple CPU architectures to catch portability issues

## Security Advisories

Published security advisories can be found at:

- GitHub Security Advisories: https://github.com/MuriloChianfa/libdynemit/security/advisories
- Release Notes: https://github.com/MuriloChianfa/libdynemit/releases

## Bug Bounty Program

We do not currently have a bug bounty program, but we deeply appreciate security research and will publicly acknowledge your contributions (with your permission).

## Hall of Fame

We recognize security researchers who have helped improve libdynemit's security:

<!-- Security researchers will be listed here -->

_No security vulnerabilities have been reported yet._

## Questions?

If you have questions about this security policy, please email us at murilo.chianfa@outlook.com.

---

**Thank you for helping keep libdynemit and its users safe!**

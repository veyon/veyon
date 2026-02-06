# Security Notes for Veyon VNC Server

This document describes known security considerations in the Veyon VNC server implementation.

## Fixed Security Issues (2026-02-06)

### 1. Buffer Overflow in VncServerProtocol::start() - FIXED
**Severity**: HIGH  
**Location**: `core/src/VncServerProtocol.cpp:63`  
**Issue**: Used unsafe `sprintf()` function which could lead to buffer overflows.  
**Fix**: Replaced with `snprintf()` with proper buffer size checking.  
**Status**: ✅ Fixed

### 2. Missing Input Validation - FIXED
**Severity**: MEDIUM  
**Locations**: 
- `core/src/VncServerProtocol.cpp:227` (username)
- `server/src/ServerAuthenticationManager.cpp:228` (token)
- `server/src/ServerAuthenticationManager.cpp:133` (signature)

**Issue**: No length validation on authentication-related inputs could lead to DoS attacks via memory exhaustion.  
**Fix**: Added maximum length validation:
- Username: 256 characters
- Token: 4KB
- Signature: 8KB

**Status**: ✅ Fixed

## Known Security Considerations

### 3. DES Encryption Usage
**Severity**: HIGH  
**Location**: `core/src/d3des.h`, `core/src/VncClientProtocol.cpp:60`  
**Issue**: The code uses DES (Data Encryption Standard) which is cryptographically broken:
- 56-bit effective key length (easily brute-forced)
- Known cryptanalytic attacks
- Weak key derivation (password directly used as key, null-padded)

**Legacy Context**: DES usage appears to be for compatibility with standard VNC protocol implementations.

**Mitigation Status**: ⚠️ Documented only - requires major protocol refactoring to fix  
**Recommended**: 
- Use AES-256 or ChaCha20 for new implementations
- Consider implementing VNC security type 19 (Veyon Auth with modern crypto)
- Ensure DES is only used for backward compatibility with strict access controls

### 4. No Authentication Rate Limiting
**Severity**: MEDIUM  
**Location**: `server/src/ServerAuthenticationManager.cpp` (all auth methods)  
**Issue**: All three authentication methods (KeyFile, Logon, Token) lack attempt rate limiting:
- No delay after failed attempts
- No account lockout mechanism
- No IP-based throttling

**Impact**: Brute force attacks possible on all authentication methods.

**Mitigation Status**: ⚠️ Documented only - requires session/state management infrastructure  
**Recommended**: 
- Implement exponential backoff after failed attempts
- Add connection rate limiting by IP address
- Consider temporary lockout after N failed attempts

### 5. Large Message Size Limits
**Severity**: LOW-MEDIUM  
**Location**: `core/src/VariantArrayMessage.h:58`  
**Configuration**: `MaxMessageSize = 32MB`  
**Issue**: The 32MB message size limit, while reasonable for legitimate use, could still be exploited for memory exhaustion attacks if many connections are established simultaneously.

**Mitigation Status**: ⚠️ Accepted risk - may be legitimate requirement for screen sharing  
**Recommended**:
- Monitor memory usage per connection
- Implement connection limits
- Consider dynamic message size limits based on authentication state

### 6. Static Assertions for Buffer Safety
**Status**: ✅ Good practice observed  
**Location**: `core/src/VncClientProtocol.cpp:406-407`  
The code uses `static_assert` to validate buffer sizes at compile time:
```cpp
static_assert( sizeof(m_pixelFormat) >= sz_rfbPixelFormat, "m_pixelFormat has wrong size" );
static_assert( sizeof(m_pixelFormat) >= sizeof(message.format), "m_pixelFormat too small" );
```
This is a good security practice that prevents buffer overflows.

## General Security Recommendations

1. **Keep Dependencies Updated**: Ensure Qt, OpenSSL, and other dependencies are kept up-to-date with security patches.

2. **Network Security**: Always use Veyon over trusted networks or through a VPN. The VNC protocol itself has inherent security limitations.

3. **Access Controls**: Use strong authentication methods (key-based auth preferred over password-based).

4. **Logging and Monitoring**: Enable comprehensive logging to detect potential security incidents.

5. **Regular Security Audits**: Perform regular code audits and use static analysis tools (CodeQL, Coverity, etc.).

## Reporting Security Issues

If you discover a security vulnerability in Veyon, please report it responsibly by emailing security@veyon.io. Do not disclose security issues publicly until a fix is available.

## References

- [VNC Security Types](https://www.rfc-editor.org/rfc/rfc6143.html#section-7.1)
- [NIST Deprecated Algorithms](https://www.nist.gov/cryptography)
- [CWE-120: Buffer Overflow](https://cwe.mitre.org/data/definitions/120.html)
- [CWE-327: Use of Broken Cryptographic Algorithm](https://cwe.mitre.org/data/definitions/327.html)

# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| latest on `master` | Yes |
| tagged releases (`v1.x`) | Yes |
| older commits | No |

## Reporting a Vulnerability

If you discover a security vulnerability in CallJvm, please report it responsibly.

**Do NOT open a public GitHub issue for security vulnerabilities.**

Instead, email the maintainer directly:

- **Contact**: Open a private security advisory via [GitHub Security Advisories](https://github.com/XWHQSJ/CallJvm/security/advisories/new)

Please include:

1. Description of the vulnerability
2. Steps to reproduce
3. Affected components (e.g., frame parser, socket handler, JNI bridge)
4. Potential impact assessment
5. Suggested fix (if any)

## Response Timeline

- **Acknowledgment**: Within 48 hours
- **Initial assessment**: Within 1 week
- **Fix or mitigation**: Depends on severity, typically within 2 weeks for critical issues

## Scope

The following areas are in scope for security reports:

- Buffer overflows in the frame parser (`frame.h/cpp`)
- Memory safety issues in socket handling
- JNI lifecycle bugs leading to use-after-free or double-free
- Thread safety violations in the thread pool
- Denial of service via malformed input

The following are out of scope:

- Vulnerabilities in the JVM itself (report to the JDK vendor)
- Issues requiring local root access on the host machine
- Social engineering attacks

## Security Design

CallJvm uses several defensive measures:

- **Length-prefixed framing** with a 16 MiB maximum to prevent unbounded reads
- **RAII guards** (`JvmGuard`, `JniEnvGuard`) for exception-safe resource cleanup
- **JNI exception checking** after every JNI call via `jni_check()`
- **Signal handling** for graceful shutdown
- **Sanitizer CI** (ASan, UBSan, TSan) to catch memory and concurrency bugs
- **libFuzzer target** for the frame parser to detect malformed input handling
